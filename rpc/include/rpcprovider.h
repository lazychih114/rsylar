#pragma once
#include <google/protobuf/descriptor.h>
#include "sylar/tcp_server.h"
#include "sylar/socket.h"
#include "sylar/iomanager.h"
#include "sylar/sylar.h"
#include <functional>
#include <string>
#include <unordered_map>
#include "google/protobuf/service.h"

// 框架提供的专门发布rpc服务的网络对象类
// todo:现在rpc客户端变成了 长连接，因此rpc服务器这边最好提供一个定时器，用以断开很久没有请求的连接。
// todo：为了配合这个，那么rpc客户端那边每次发送之前也需要真正的
class RpcProvider {
public:
  // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
  void NotifyService(google::protobuf::Service *service);

  // 启动rpc服务节点，开始提供rpc远程网络调用服务
  void Run(int nodeIndex, short port);

private:
  // sylar event loop + tcp server
  sylar::IOManager* m_eventLoop;
  // sylar::IOManager m_eventLoop = sylar::IOManager::GetThis();
  std::shared_ptr<sylar::TcpServer> m_server;
  // 是否支持长连接
  bool m_isKeepalive;

  // service服务类型信息
  struct ServiceInfo {
    google::protobuf::Service *m_service;                                                     // 保存服务对象
    std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap;  // 保存服务方法
  };
  // 存储注册成功的服务对象和其服务方法的所有信息
  std::unordered_map<std::string, ServiceInfo> m_serviceMap;

  // 新的socket连接回调 (使用 sylar::Socket::ptr 表示连接)
  void OnConnection(sylar::Socket::ptr client);
  // 已建立连接用户的读写事件回调 (接收整包数据后调用)
  void OnMessage(sylar::Socket::ptr client, const std::string &data);
  // 处理已解析的 rpc header 和 args（header 已经是反序列化前的字符串）
  void HandleRpcRequest(sylar::Socket::ptr client, const std::string &rpc_header_str, const std::string &args_str);
  // Closure的回调操作，用于序列化rpc的响应并发送
  void SendRpcResponse(sylar::Socket::ptr client, google::protobuf::Message *);

public:
  RpcProvider(sylar::IOManager* io = sylar::IOManager::GetThis());
  ~RpcProvider();
};

