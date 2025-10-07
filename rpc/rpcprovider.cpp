#include "rpcprovider.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include "rpcheader.pb.h"
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

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
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
  //获取可用ip
  char *ipC;
  char hname[128];
  struct hostent *hent;
  gethostname(hname, sizeof(hname));
  hent = gethostbyname(hname);
  for (int i = 0; hent->h_addr_list[i]; i++) {
    ipC = inet_ntoa(*(struct in_addr *)(hent->h_addr_list[i]));  // IP地址
  }
  std::string ip = std::string(ipC);
  //    // 获取端口
  //    if(getReleasePort(port)) //在port的基础上获取一个可用的port，不知道为何没有效果
  //    {
  //        std::cout << "可用的端口号为：" << port << std::endl;
  //    }
  //    else
  //    {
  //        std::cout << "获取可用端口号失败！" << std::endl;
  //    }
  //写入文件 "test.conf"
  std::string node = "node" + std::to_string(nodeIndex);
  std::ofstream outfile;
  outfile.open("test.conf", std::ios::app);  //打开文件并追加写入
  if (!outfile.is_open()) {
    std::cout << "打开文件失败！" << std::endl;
    exit(EXIT_FAILURE);
  }
  outfile << node + "ip=" + ip << std::endl;
  outfile << node + "port=" + std::to_string(port) << std::endl;
  outfile.close();

  // 创建并绑定 sylar TcpServer
  sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress(ip + ":" + std::to_string(port));
  // create a small TcpServer subclass that dispatches accepted clients to RpcProvider
  class RpcTcpServer : public sylar::TcpServer {
  public:
    RpcTcpServer(sylar::IOManager* w, sylar::IOManager* io, sylar::IOManager* accept)
        : sylar::TcpServer(w, io, accept) {}
    // forward the client to owner when accepted
    void handleClient(sylar::Socket::ptr client) override {
      if(client) {
        // read loop: try to read whole messages (the protocol expects a varint header + body)
        while(client->isValid()) {
          // read some data into buffer (simple approach: read into string until socket closes)
          std::string buf;
          buf.resize(4096);
          int n = client->recv(&buf[0], buf.size());
          if(n > 0) {
            buf.resize(n);
            // if owner is set, dispatch
            if(owner) owner->OnMessage(client, buf);
          } else if(n == 0) {
            break;
          } else {
            break;
          }
        }
      }
    }
    // attach owner pointer
    class RpcProvider* owner = nullptr;
  };
  auto rpc_server = std::make_shared<RpcTcpServer>(m_eventLoop, m_eventLoop, m_eventLoop);
  m_server = rpc_server;
  if(!addr || !m_server->bind(addr)) {
    std::cout << "RpcProvider bind fail: " << addr->toString() << std::endl;
    return;
  }
  // set owner so handleClient can call back into this RpcProvider
  rpc_server->owner = this;

  // TcpServer will call handleClient for each accepted client; we will subclass
  // RpcProvider to override handleClient behavior by scheduling a lambda that
  // reads from the socket and dispatches messages. For simplicity we reuse
  // existing handleClient mechanism by scheduling our own reader below.
  /*
  bind的作用：
  如果不使用std::bind将回调函数和TcpConnection对象绑定起来，那么在回调函数中就无法直接访问和修改TcpConnection对象的状态。因为回调函数是作为一个独立的函数被调用的，它没有当前对象的上下文信息（即this指针），也就无法直接访问当前对象的状态。
  如果要在回调函数中访问和修改TcpConnection对象的状态，需要通过参数的形式将当前对象的指针传递进去，并且保证回调函数在当前对象的上下文环境中被调用。这种方式比较复杂，容易出错，也不便于代码的编写和维护。因此，使用std::bind将回调函数和TcpConnection对象绑定起来，可以更加方便、直观地访问和修改对象的状态，同时也可以避免一些常见的错误。
  */
  std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

  // start server (this will spawn accept workers that call handleClient)
  m_server->start();
  // we rely on the application's IOManager event loop; block here similarly
  // m_eventLoop.loop();
  /*
  这段代码是在启动网络服务和事件循环，其中server是一个TcpServer对象，m_eventLoop是一个EventLoop对象。

首先调用server.start()函数启动网络服务。在Muduo库中，TcpServer类封装了底层网络操作，包括TCP连接的建立和关闭、接收客户端数据、发送数据给客户端等等。通过调用TcpServer对象的start函数，可以启动底层网络服务并监听客户端连接的到来。

接下来调用m_eventLoop.loop()函数启动事件循环。在Muduo库中，EventLoop类封装了事件循环的核心逻辑，包括定时器、IO事件、信号等等。通过调用EventLoop对象的loop函数，可以启动事件循环，等待事件的到来并处理事件。

在这段代码中，首先启动网络服务，然后进入事件循环阶段，等待并处理各种事件。网络服务和事件循环是两个相对独立的模块，它们的启动顺序和调用方式都是确定的。启动网络服务通常是在事件循环之前，因为网络服务是事件循环的基础。启动事件循环则是整个应用程序的核心，所有的事件都在事件循环中被处理。
  */
}

// 新的socket连接回调
void RpcProvider::OnConnection(sylar::Socket::ptr client) {
  // placeholder: using sylar Socket, no-op for now
  if(!client || !client->isValid()) {
    return;
  }
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
// 已建立连接用户的读写事件回调 如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
// 这里来的肯定是一个远程调用请求
// 因此本函数需要：解析请求，根据服务名，方法名，参数，来调用service的来callmethod来调用本地的业务
// OnMessage is now considered to handle raw chunks; we buffer per-connection and parse complete messages
void RpcProvider::OnMessage(sylar::Socket::ptr client, const std::string &recv_buf) {
  if(!client) return;
  // We'll maintain a per-connection buffer in a map keyed by socket pointer address
  static std::unordered_map<void*, std::string> conn_buffers;
  void* key = static_cast<void*>(client.get());
  std::string &buffer = conn_buffers[key];
  buffer.append(recv_buf);

  // parse loop: read varint header_size (varint may be 1..5 bytes), then header_str, then args
  while(true) {
    // Use a CodedInputStream on the current buffer to attempt to read a varint
    google::protobuf::io::ArrayInputStream ais(buffer.data(), (int)buffer.size());
    google::protobuf::io::CodedInputStream cis(&ais);
    // Try to read varint32 header_size but do not advance buffer until we confirm full message
    google::protobuf::io::CodedInputStream::Limit limit = cis.PushLimit(buffer.size());
    uint32_t header_size = 0;
    // store current pointer offset by using Internal/ReadVarint32Unsafe isn't public, so use ReadVarint32
    if(!cis.ReadVarint32(&header_size)) {
      // not enough bytes for varint, wait for more data
      break;
    }
    // compute varint length: re-create single Varint writer to measure length
    // Instead, advance a temporary stream to the position after varint
    // We can get how many bytes were read by comparing pointer positions through internal API is not exposed
    // So workaround: read varint again via manual decode from buffer
    // Manual varint decode to find varint byte length
    size_t varint_len = 0;
    uint32_t tmp = 0;
    for(size_t i = 0; i < std::min<size_t>(5, buffer.size()); ++i) {
      uint8_t byte = (uint8_t)buffer[i];
      tmp |= (uint32_t)(byte & 0x7F) << (7 * i);
      ++varint_len;
      if((byte & 0x80) == 0) break;
    }
    if(varint_len == 0 || varint_len > 5) {
      // incomplete varint
      break;
    }
    // Now we have header_size and varint_len. Total needed = varint_len + header_size + args_size
    if(buffer.size() < varint_len + header_size) {
      // header body not yet complete
      break;
    }
    // read header_str
    std::string rpc_header_str = buffer.substr(varint_len, header_size);
    // parse RpcHeader to obtain args_size
    RPC::RpcHeader rpcHeader;
    if(!rpcHeader.ParseFromString(rpc_header_str)) {
      std::cout << "rpc_header parse error" << std::endl;
      // drop connection buffer to avoid infinite loop
      buffer.clear();
      break;
    }
    uint32_t args_size = rpcHeader.args_size();
    size_t total_len = varint_len + header_size + args_size;
    if(buffer.size() < total_len) {
      // wait for full args
      break;
    }
    std::string args_str = buffer.substr(varint_len + header_size, args_size);
    // handle single complete request
    HandleRpcRequest(client, rpc_header_str, args_str);
    // erase processed bytes
    buffer.erase(0, total_len);
    // continue loop to see if more complete messages are available
  }
}

// 把解析出来的 header_str 和 args_str 做最终处理并调用service
void RpcProvider::HandleRpcRequest(sylar::Socket::ptr client, const std::string &rpc_header_str, const std::string &args_str) {
  RPC::RpcHeader rpcHeader;
  if(!rpcHeader.ParseFromString(rpc_header_str)) {
    std::cout << "rpc_header_str parse error" << std::endl;
    return;
  }
  std::string service_name = rpcHeader.service_name();
  std::string method_name = rpcHeader.method_name();
  auto it = m_serviceMap.find(service_name);
  if (it == m_serviceMap.end()) {
    std::cout << "service:" << service_name << " is not exist!" << std::endl;
    return;
  }
  auto mit = it->second.m_methodMap.find(method_name);
  if (mit == it->second.m_methodMap.end()) {
    std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
    return;
  }
  google::protobuf::Service *service = it->second.m_service;
  const google::protobuf::MethodDescriptor *method = mit->second;

  google::protobuf::Message *request = service->GetRequestPrototype(method).New();
  if (!request->ParseFromString(args_str)) {
    std::cout << "request parse error, content:" << args_str << std::endl;
    delete request;
    return;
  }
  google::protobuf::Message *response = service->GetResponsePrototype(method).New();

  google::protobuf::Closure *done =
    google::protobuf::NewCallback<RpcProvider, sylar::Socket::ptr, google::protobuf::Message *>(
      this, &RpcProvider::SendRpcResponse, client, response);

  service->CallMethod(method, nullptr, request, response, done);
  // request object is owned by service call (it may have been used). We can delete request now if needed
  delete request;
}

// Closure的回调操作，用于序列化rpc的响应和网络发送,发送响应回去
void RpcProvider::SendRpcResponse(sylar::Socket::ptr client, google::protobuf::Message *response) {
  std::string response_str;
  if (response->SerializeToString(&response_str))  // response进行序列化
  {
    // 通过 sylar Socket 发送响应
    client->send(response_str.c_str(), response_str.size());
  } else {
    std::cout << "serialize response_str error!" << std::endl;
  }
  //    conn->shutdown(); // 模拟http的短链接服务，由rpcprovider主动断开连接  //改为长连接，不主动断开
}
RpcProvider::RpcProvider(sylar::IOManager* io) {
  m_eventLoop = io;
}
RpcProvider::~RpcProvider() {
  std::cout << "call RpcProvider::~RpcProvider()" << std::endl;
  if(m_server) {
    std::cout << "[func - RpcProvider::~RpcProvider()]: server info: " << m_server->toString() << std::endl;
  }
}
