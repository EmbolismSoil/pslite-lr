#ifndef __PSLITE_LRWORKER_H__
#define __PSLITE_LRWORKER_H__
#include <Eigen/Eigen>
#include "ps/ps.h"
#include <memory>
#include <vector>
#include <assert.h>
#include <atomic>
#include <fstream>
#include <iostream>

template<typename M>
static void load_csv (const std::string & path, M& m) {
    std::ifstream indata;
    indata.open(path);
    std::string line;
    std::vector<double> values;
    uint rows = 0;
    while (std::getline(indata, line)) {
        std::stringstream lineStream(line);
        std::string cell;
        while (std::getline(lineStream, cell, ',')) {
            values.push_back(std::stod(cell));
        }
        ++rows;
    }
    m = Eigen::Map<const Eigen::Matrix<typename M::Scalar, M::RowsAtCompileTime, M::ColsAtCompileTime, Eigen::RowMajor>>(values.data(), rows, values.size()/rows);
}

class LRWorker
{
public:
    LRWorker(std::string const& path, int app_id, int custome_id, int epoch = 10, int batch=50):
        _epoch(epoch),
        _batch(batch)
    {
        assert(epoch > 0);

        load_csv(path, _data);
        assert(_data.rows() != 0);

        _nfeatures = _data.cols() - 1;
        assert(_nfeatures > 0);

        _ps = std::make_shared<ps::KVWorker<double>>(app_id, custome_id);
    }

    int train()
    {
        std::vector<ps::Key> keys = {0, 1};
        std::vector<double> vals(_nfeatures + 1, 0);
        std::vector<int> lens = {static_cast<int>(_nfeatures), 1};
        
        _ps->Wait(_ps->Push(keys, vals, lens));

        vals.clear();
        lens.clear();
        _ps->Wait(_ps->Pull(keys, &vals, &lens));
        assert(vals.size() == _nfeatures + 1);
        
        Eigen::MatrixXd w = Eigen::Map<Eigen::MatrixXd>(vals.data(), _nfeatures, 1);
        double b = *vals.rbegin();

        for (int i = 0; i < _epoch; ++i){
            Eigen::MatrixXd X;
            Eigen::MatrixXd y;
            while(iterBatch(X, y) >= 0){
                Eigen::MatrixXd g_w;
                double g_b; 
                Eigen::MatrixXd ypre;

                predict(X, w, b, ypre);
                _g(X, y, ypre, g_w, g_b);

                //PUSH GRADIENT
                vals.clear();
                std::vector<double> buf;

                auto p = g_w.data();
                std::copy(p, p + _nfeatures, std::back_inserter(buf));
                buf.push_back(g_b);

                //_ps->Wait(_ps->PushPull(keys, buf, &vals, &lens)); //PUSH&PULL
                _ps->Wait(_ps->Push(keys, buf, lens));
                _ps->Wait(_ps->Pull(keys, &vals, &lens));

                //update parameters
                w = Eigen::Map<Eigen::MatrixXd>(vals.data(), _nfeatures, 1);
                b = *vals.rbegin();
            }
        }

        printAcc(w, b);
    }

    int predict(Eigen::MatrixXd const& x, Eigen::MatrixXd const& w, double const b,  Eigen::MatrixXd& y)
    {
        Eigen::ArrayXd h = (-1.0*x*w).array() - b;
        y = (1/(1 + Eigen::exp(h))).matrix();
        return 0;
    }

private:
    void printAcc(Eigen::MatrixXd const& w, double const b)
    {
        auto subset = _data;
        Eigen::MatrixXd y = subset.block(0, 0, subset.rows(), 1);
        Eigen::MatrixXd x = subset.block(0, 1, subset.rows(), subset.cols() - 1);

        Eigen::MatrixXd ypre;
        predict(x, w, b, ypre);

        std::cout << "y: \n" << y.transpose() << std::endl;
        std::cout << "ypre: \n" << ypre.transpose() << std::endl;
    }

    void _g(Eigen::MatrixXd const& X, Eigen::MatrixXd const& y, Eigen::MatrixXd const& ypre, Eigen::MatrixXd& g_w, double& g_b) 
    {
        auto n = X.rows();
        Eigen::ArrayXd y_arr = y.array();
        Eigen::ArrayXd ypre_arr = ypre.array();

        Eigen::ArrayXd err = y_arr - ypre_arr ;
        Eigen::MatrixXd m = (err/static_cast<double>(n)).matrix();
        g_w = m.transpose() * X;
        g_b = err.mean();
    }   

    int iterBatch(Eigen::MatrixXd& x, Eigen::MatrixXd& y)
    {
        static Eigen::Index i = 0;
        auto start = std::min(i, _data.rows());
        auto end = std::min(start + _batch, _data.rows());

        if (start == end){
            i = 0;
            return -1;
        }

        i += 1;
        Eigen::MatrixXd subset = _data.block(start, 0, end - start, _data.cols());
        y = subset.block(0, 0, subset.rows(), 1);
        x = subset.block(0, 1, subset.rows(), subset.cols() - 1);
        return 0;
    }

    Eigen::MatrixXd _data;        

    int _epoch;
    int _batch;
    Eigen::Index _nfeatures;
    std::shared_ptr<ps::KVWorker<double>> _ps;
};
#endif
