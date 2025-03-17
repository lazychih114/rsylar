#include "sylar/tcp_server.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

namespace sylar{

class TServer : public TcpServer
{
public:
    TServer(sylar::IOManager* worker = sylar::IOManager::GetThis()
              ,sylar::IOManager* io_woker = sylar::IOManager::GetThis()
              ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis())
    :TcpServer(worker,io_woker,accept_worker)
    {

    }
    void handleClient(Socket::ptr client) override {
        SYLAR_LOG_INFO(g_logger) << "handleClient: " << *client;\
        std::string buffs;
        buffs.resize(4096);
        int rt= client->recv(&buffs[0], buffs.size());
        buffs.resize(rt);
        SYLAR_LOG_INFO(g_logger) << buffs;    
    }
};
}


void run() {
    
    auto addr = sylar::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = sylar::UnixAddress::ptr(new sylar::UnixAddress("/tmp/unix_addr"));
    std::vector<sylar::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    sylar::TcpServer::ptr tcp_server(new sylar::TServer());
    std::vector<sylar::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    sylar::IOManager ioworker(2);
    //sylar::IOManager acptworker(2);
    ioworker.schedule(&run);
    return 0;
}
