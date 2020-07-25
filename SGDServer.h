#ifndef __PSLITE_SGD_SERVER_H__
#define __PSLITE_SGD_SERVER_H__

#include <map>
#include "ps/ps.h"
#include <Eigen/Eigen>
#include <functional>

template<class Val>
class SGDServer
{
public:
    using Matrix = Eigen::Matrix<Val, Eigen::Dynamic, Eigen::Dynamic>;

    SGDServer(int app_id, double lr = 0.01):
        _lr(lr)
    {
        auto server = new ps::KVServer<Val>(app_id);
        using namespace std::placeholders;
        server->set_request_handle(std::bind(&SGDServer::req_handler, this, _1, _2, _3));

        auto onExit = [server](){delete server;};
        ps::RegisterExitCallback(onExit);
    }

private:
    void req_handler(ps::KVMeta const& meta, ps::KVPairs<Val> const& data, ps::KVServer<Val>* server)
    {
        ps::KVPairs<Val> res;
        if (meta.pull){
            std::vector<Val> vals;
            auto pos = _store.cend();
            for (auto i = 0; i < data.keys.size(); ++i){
                auto k = data.keys[i];
                if ((pos = _store.find(k)) == _store.cend()){
                    continue;
                }
                
                auto p = _store[k].data();
                auto n = _store[k].rows();
                res.keys.push_back(k);
                res.lens.push_back(n);
                std::copy(p, p+n, std::back_inserter(vals));
            }

            res.vals.CopyFrom(vals.cbegin(), vals.cend());
        }else{
            auto offset = 0;
            for (auto i = 0; i < data.keys.size(); ++i){
                auto k = data.keys[i];
                auto n = data.lens[i];

                auto pos = _store.cend();
                if ((pos = _store.find(k)) == _store.cend()){
                    Matrix v = Matrix::Random(n, 1);
                    _store[k] = v;
                }

                Matrix v = Matrix::Zero(n, 1);
                Eigen::Index j = 0;
                for (auto pos = offset; pos < offset + n && j < n; ++pos, ++j){
                    v(j, 0) = data.vals[pos];
                }

                _store[k] = _store[k] + _lr*v; //update
                offset += n;
            }
        }

        server->Response(meta, res);
    }

    double _lr;    
    std::map<ps::Key, Matrix> _store;
};

#endif
