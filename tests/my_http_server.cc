#include "sylar/http/http_server.h"
#include "sylar/log.h"
#define XX(...) #__VA_ARGS__

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
sylar::IOManager::ptr worker;
void run() {
    g_logger->setLevel(sylar::LogLevel::INFO);
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    //sylar::http::HttpServer::ptr http_server(new sylar::http::HttpServer(true, worker.get(),worker.get()));
    sylar::http::HttpServer::ptr http_server(new sylar::http::HttpServer(true));
    bool ssl = true;
    
    while(!http_server->bind(addr, ssl)) {
        SYLAR_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    if(ssl) {
        http_server->loadCertificates("/home/reversedog/projects/sylar/rsylar/bin/ssl/localhost.crt", "/home/reversedog/projects/sylar/rsylar/bin/ssl/localhost.key");
    }

    
    auto sd = http_server->getServletDispatch();
    sd->addServlet("/sylar/xx", [](sylar::http::HttpRequest::ptr req
                ,sylar::http::HttpResponse::ptr rsp
                ,sylar::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/sylar/*", [](sylar::http::HttpRequest::ptr req
                ,sylar::http::HttpResponse::ptr rsp
                ,sylar::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/sylarx/*", [](sylar::http::HttpRequest::ptr req
                ,sylar::http::HttpResponse::ptr rsp
                ,sylar::http::HttpSession::ptr session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });
    http_server->start();
    SYLAR_LOG_INFO(g_logger) << "等待停止";
}

int main(int argc, char** argv) {
    // sylar::Thread::SetName("main");
    sylar::IOManager iom(2,true,"iom");
    //sylar::IOManager iom(1);
    //worker.reset(new sylar::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
