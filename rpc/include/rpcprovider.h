#pragma once
#include <google/protobuf/descriptor.h>
#include "sylar/sylar.h"
#include <functional>
#include <string>
#include <unordered_map>
#include "google/protobuf/service.h"

// 框架提供的专门发布rpc服务的网络对象类
// todo:现在rpc客户端变成了 长连接，因此rpc服务器这边最好提供一个定时器，用以断开很久没有请求的连接。
// todo：为了配合这个，那么rpc客户端那边每次发送之前也需要真正的
namespace sylar {
namespace rpc{

class RpcProvider{
public:
  typedef std::shared_ptr<RpcProvider> ptr;
  // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
  void NotifyService(google::protobuf::Service *service);

  // 启动rpc服务节点，开始提供rpc远程网络调用服务
  void Run(int nodeIndex, short port);

private:
  // sylar event loop + tcp server
  sylar::IOManager* m_eventLoop;
  // sylar::IOManager m_eventLoop = sylar::IOManager::GetThis();
  std::shared_ptr<sylar::http::HttpServer> m_server;

  // service服务类型信息
  struct ServiceInfo {
    google::protobuf::Service *m_service;                                                     // 保存服务对象
    std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap;  // 保存服务方法
  };
  // 存储注册成功的服务对象和其服务方法的所有信息
  std::unordered_map<std::string, ServiceInfo> m_serviceMap;

  // (Raw TCP/raw-socket handling removed) RPC is handled over HTTP in Run().

public:
  RpcProvider(sylar::IOManager* io = sylar::IOManager::GetThis());
  ~RpcProvider();
};




}
}

