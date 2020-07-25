#include "LRWorker.h"
#include "SGDServer.h"
#include "ps/ps.h"

int main(int argc, char* argv[])
{
    ps::Start(0);
    std::shared_ptr<SGDServer<double>> server;
    std::shared_ptr<LRWorker> worker;
    if (ps::IsServer()){
        server = std::make_shared<SGDServer<double>>(0, 0.1);
    }else{
        worker = std::make_shared<LRWorker>("./data.csv", 0, 0, 50, 10);
        worker->train();
    }

    ps::Finalize(0, true);
    return 0;
}
