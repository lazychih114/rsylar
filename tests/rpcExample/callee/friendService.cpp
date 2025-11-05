#include <mprpcchannel.h>
#include <iostream>
#include <string>
#include "friend.pb.h"

#include <vector>
#include "rpcprovider.h"
sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class FriendService : public fixbug::FiendServiceRpc {
 public:
  std::vector<std::string> GetFriendsList(uint32_t userid) {
    SYLAR_LOG_INFO(g_logger) << "local do GetFriendsList service! userid:" << userid;
    std::vector<std::string> vec;
    vec.push_back("gao yang");
    vec.push_back("liu hong");
    vec.push_back("wang shuo");
    return vec;
  }

  // 重写基类方法
  void GetFriendsList(::google::protobuf::RpcController *controller, const ::fixbug::GetFriendsListRequest *request,
                      ::fixbug::GetFriendsListResponse *response, ::google::protobuf::Closure *done) {
    uint32_t userid = request->userid();
    std::vector<std::string> friendsList = GetFriendsList(userid);
    response->mutable_result()->set_errcode(0);
    response->mutable_result()->set_errmsg("");
    for (std::string &name : friendsList) {
      std::string *p = response->add_friends();
      *p = name;
    }
  }
};

void runnode(int nodeid, int port){
  
  std::string ip = "127.0.0.1";
  auto stub = new fixbug::FiendServiceRpc_Stub(new sylar::rpc::MprpcChannel(ip, port, false));
  static std::shared_ptr<sylar::rpc::RpcProvider> g_provider;
  auto provider = std::make_shared<sylar::rpc::RpcProvider>();
  g_provider = provider;
  // sylar::rpc::RpcProvider provider;
  // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
  provider->NotifyService(new FriendService());
  provider->Run(nodeid,port); 
}


int main(int argc, char **argv) {
  
  g_logger->setLevel(sylar::LogLevel::INFO);
  sylar::IOManager iom(2,true,"iom");

  iom.schedule(std::bind(runnode, 1, 7788));
  return 0;
}
