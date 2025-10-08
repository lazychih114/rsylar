#include "rpcprovider.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include <google/protobuf/message.h>
// #include "sylar/util.h"
/*
service_name =>  service描述
                        =》 service* 记录服务对象
                        method_name  =>  method方法对象
json   protobuf
*/
// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
// 只是简单的把服务描述符和方法描述符全部保存在本地而已
// todo 待修改 要把本机开启的ip和端口写在文件里面
namespace sylar {
namespace rpc {
static Logger::ptr g_logger = SYLAR_LOG_NAME("system");
void RpcProvider::NotifyService(google::protobuf::Service *service) {
  ServiceInfo service_info;

  // 获取了服务对象的描述信息
  const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
  // 获取服务的名字
  std::string service_name = pserviceDesc->name();
  // 获取服务对象service的方法的数量
  int methodCnt = pserviceDesc->method_count();

  std::cout << "service_name:" << service_name << std::endl;

  for (int i = 0; i < methodCnt; ++i) {
    // 获取了服务对象指定下标的服务方法的描述（抽象描述） UserService   Login
    const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
    std::string method_name = pmethodDesc->name();
    service_info.m_methodMap.insert({method_name, pmethodDesc});
  }
  service_info.m_service = service;
  m_serviceMap.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run(int nodeIndex, short port) {
  // create an HTTP server and register a servlet for RPC，并支持长连接
  http::HttpServer::ptr server(new http::HttpServer(true
        , m_eventLoop, m_eventLoop, m_eventLoop));
  m_server = server;
  server->setName("sylar_rpc_server");

  // Register a servlet to handle RPC calls at path /rpc
  auto dispatch = server->getServletDispatch();
  dispatch->addGlobServlet("/rpc/*", [this](http::HttpRequest::ptr req
                , http::HttpResponse::ptr rsp
                , http::HttpSession::ptr session)->int32_t {
    // only accept POST
    if(req->getMethod() != http::HttpMethod::POST) {
      rsp->setStatus(http::HttpStatus::METHOD_NOT_ALLOWED);
      rsp->setBody("Only POST allowed\n");
      return 0;
    }
    std::string body = req->getBody();
    // Simple HTTP-RPC convention:
    // POST /rpc/<ServiceName>/<MethodName>
    // body = serialized request protobuf
    std::string path = req->getPath();
    const std::string prefix = "/rpc/";
    if(path.size() <= prefix.size() || path.substr(0, prefix.size()) != prefix) {
      rsp->setStatus(http::HttpStatus::BAD_REQUEST);
      rsp->setBody("invalid rpc path, expected /rpc/<Service>/<Method>");
      return 0;
    }
    std::string rest = path.substr(prefix.size());
    auto pos = rest.find('/');
    if(pos == std::string::npos) {
      rsp->setStatus(http::HttpStatus::BAD_REQUEST);
      rsp->setBody("invalid rpc path, missing method");
      return 0;
    }
    std::string service_name = rest.substr(0, pos);
    std::string method_name = rest.substr(pos + 1);
    std::string args_str = body; // entire body is the serialized request
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end()) {
      rsp->setStatus(http::HttpStatus::NOT_FOUND);
      rsp->setBody("service not found");
      return 0;
    }
    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end()) {
      rsp->setStatus(http::HttpStatus::NOT_FOUND);
      rsp->setBody("method not found");
      return 0;
    }
    
    // 解析请求，反序列化参数
    google::protobuf::Service *service = it->second.m_service;
    const google::protobuf::MethodDescriptor *method = mit->second;

    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str)) {
      delete request;
      rsp->setStatus(http::HttpStatus::BAD_REQUEST);
      rsp->setBody("request parse error");
      return 0;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 调用server本地方法
    service->CallMethod(method, nullptr, request, response, nullptr);

    // 序列化响应并返回
    std::string response_str;
    if (response->SerializeToString(&response_str)) {
      rsp->setStatus(http::HttpStatus::OK);
      rsp->setHeader("Content-Type", "application/octet-stream");
      rsp->setBody(response_str);
    } else {
      rsp->setStatus(http::HttpStatus::INTERNAL_SERVER_ERROR);
      rsp->setBody("serialize response error");
    }
    delete request;
    delete response;
    return 0;
  });

  // bind to address and start
  std::string bind_addr = "0.0.0.0:" + std::to_string(port);
  auto addr = sylar::Address::LookupAnyIPAddress(bind_addr);
  if(!addr) {
    SYLAR_LOG_ERROR(g_logger) << "bind address lookup failed: " << bind_addr;
    return;
  }
  if(!server->bind(addr)) {
    SYLAR_LOG_ERROR(g_logger) << "bind failed: " << bind_addr;
    return;
  }
  server->start();
}



/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
service_name method_name args    定义proto的message类型，进行数据头的序列化和反序列化
                                 service_name method_name args_size
16UserServiceLoginzhang san123456

header_size(4个字节) + header_str + args_str
10 "10"
10000 "1000000"
std::string   insert和copy方法
*/
// (Old TCP/raw OnMessage handling removed) RPC is handled over HTTP in Run()

// 把解析出来的 header_str 和 args_str 做最终处理并调用service

RpcProvider::RpcProvider(sylar::IOManager* io) {
  m_eventLoop = io;
}

RpcProvider::~RpcProvider() {
  SYLAR_LOG_INFO(g_logger) << "call RpcProvider::~RpcProvider()";
  if(m_server) {
    SYLAR_LOG_INFO(g_logger) << "[func - RpcProvider::~RpcProvider()]: server info: " << m_server->toString();
  }
}

}
}