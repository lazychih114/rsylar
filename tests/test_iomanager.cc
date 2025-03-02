#include "sylar/sylar.h"
//#include "sylar/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();



void test_fiber() {
    int sock = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    SYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8089);
    inet_pton(AF_INET, "10.211.55.2", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ, [sock](){
            char buffer[1024];  // 定义缓冲区存储读取的数据
            int n = read(sock, buffer, sizeof(buffer) - 1);  // 从 socket 读取数据
            if(n > 0) {
                buffer[n] = '\0';  // 添加字符串结束符
                SYLAR_LOG_INFO(g_logger) << "Received: " << buffer;  // 打印读取的内容
            } else if(n == 0) {
                SYLAR_LOG_INFO(g_logger) << "Connection closed by peer";  // 对端关闭连接
                close(sock);  // 关闭 socket
            } else {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    SYLAR_LOG_INFO(g_logger) << "No data available now";  // 暂时无数据
                } else {
                    SYLAR_LOG_ERROR(g_logger) << "Read error: " << strerror(errno);  // 读取出错
                    close(sock);  // 关闭 socket
                }
            }
            // SYLAR_LOG_INFO(g_logger) << "read callback:" << " ";
            // sylar::IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::WRITE);
            close(sock);
        });
        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE, [](){
            SYLAR_LOG_INFO(g_logger) << "write callback";
            
            //close(sock);
            // sylar::IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::READ);
            // close(sock);
        });
    } else {
        SYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
    SYLAR_LOG_INFO(g_logger) << "the iom is over";
}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    sylar::IOManager iom(2, false,"test");
    for(int i=0;i<1000;i++)
    {
        iom.schedule(&test_fiber);
    }
    //iom.schedule(&test_fiber);
    
    iom.stop();
}

// sylar::Timer::ptr s_timer;
// void test_timer() {
//     sylar::IOManager iom(2);
//     s_timer = iom.addTimer(1000, [](){
//         static int i = 0;
//         SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
//         if(++i == 3) {
//             s_timer->reset(2000, true);
//             //s_timer->cancel();
//         }
//     }, true);
// }

int main(int argc, char** argv) {
    sylar::Thread::SetName("main");
    test1();
    //test_timer();
    return 0;
}
