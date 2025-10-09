#include "mprpcchannel.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <map>
#include "mprpccontroller.h"
#include "sylar/sylar.h"
#include "sylar/http/http_connection.h"
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar {
namespace rpc {
using namespace google;
/*
header_size + service_name method_name args_size + args
*/
// 所有通过stub代理对象调用的rpc方法，都会走到这里了，
// 统一通过rpcChannel来调用方法
// 统一做rpc方法调用的数据数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                              google::protobuf::Message* response, google::protobuf::Closure* done) {

  if (!connect()) {
    controller->SetFailed("connect to server failed!");
    SYLAR_LOG_ERROR(g_logger) << "connect to server failed!";
    return;
  }
  // 使用 HTTP 客户端发送 RPC 请求，body 保持原有的二进制格式：varint(header_len) + header + args
  const google::protobuf::ServiceDescriptor* sd = method->service();
  std::string service_name = sd->name();     // service_name
  std::string method_name = method->name();  // method_name

  // 获取参数的序列化字符串长度 args_size
  std::string req_str;

  if (request->SerializeToString(&req_str)) {
    // SYLAR_LOG_DEBUG(g_logger) << "req_str:" << req_str;
  } else {
    controller->SetFailed("serialize request error!");
    return;
  }
  // SYLAR_LOG_INFO(g_logger) << "req=" << request->DebugString();
  // 发送请求
  http::HttpRequest::ptr req(new http::HttpRequest());
  req->setMethod(http::HttpMethod::POST);
  req->setPath("/rpc/" + service_name + "/" + method_name);
  req->setHeader("Content-Type", "application/octet-stream");
  req->setClose(false);
  req->setBody(req_str);
  req->setHeader("Content-Length", std::to_string(req_str.size()));
  // SYLAR_LOG_INFO(g_logger) << "req=" << req->toString();
  m_httpconn->sendRequest(req);


  http::HttpResponse::ptr rsp = m_httpconn->recvResponse();
  if(!rsp) {
    controller->SetFailed("recv response error!");
    m_isConnected = false;
    SYLAR_LOG_ERROR(g_logger) << "recv response error!";
    return;
  }
  // SYLAR_LOG_INFO(g_logger) << "rsp=" << rsp->toString();
  response->ParseFromString(rsp->getBody());
  // SYLAR_LOG_INFO(g_logger) << "rsp=" << response->DebugString();
}


MprpcChannel::MprpcChannel(string ip, short port, bool connectNow) : m_ip(ip), m_port(port){
  // 使用tcp编程，完成rpc方法的远程调用，使用的是短连接，因此每次都要重新连接上去，待改成长连接。
  // 没有连接或者连接已经断开，那么就要重新连接呢,会一直不断地重试
  // 读取配置文件rpcserver的信息
  // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
  // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
  // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
  //  /UserServiceRpc/Login
  if (!connectNow) {
    return;
  }  //可以允许延迟连接
  connect();
  
}
bool MprpcChannel::connect() {
  if (m_isConnected&&m_httpconn->isConnected()) {
    return true;
  }
  sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress(m_ip + ":" + std::to_string(m_port));
  if(!addr) {
      SYLAR_LOG_INFO(g_logger) << "get addr error";
      return false;
  }

  sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
  bool rt = sock->connect(addr);
  if(!rt) {
      SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " failed";
      return false;
  }

  m_httpconn = std::make_shared<http::HttpConnection>(sock);
  m_isConnected = true;
  
  return true;
}

}
}