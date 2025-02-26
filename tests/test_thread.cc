#include "sylar/sylar.h"
#include <unistd.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
//sylar::RWMutex s_mutex;
sylar::Mutex s_mutex;

void fun1() {
    // SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
    //                          << " this.name: " << sylar::Thread::GetThis()->getName()
    //                          << " id: " << sylar::GetThreadId()
    //                          << " this.id: " << sylar::Thread::GetThis()->getId();

    // for(int i = 0; i < 100000; ++i) {
    //     //sylar::RWMutex::WriteLock lock(s_mutex);
    //     sylar::Mutex::Lock lock(s_mutex);
    //     ++count;
    // }
    sylar::Mutex::Lock lock(s_mutex);
    ++count;
}

void fun2() {
    while(true) {
        SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << sylar::Thread::GetName();
        break;
    }
    int i=10000;
    while(i)
    {
        fun1();
        i--;
    }
}

void fun3() {
    while(true) {
        SYLAR_LOG_INFO(g_logger) << "========================================";
        break;
    }
    int i=100000000;
    while(i)
    {
        fun1();
        i--;
    }
}

int main(int argc, char** argv) {
    
    YAML::Node root = YAML::LoadFile("/root/rsylar/bin/conf/log.yml");
    //sylar::Config::LogAllConfigVar();
    //std::cout << "------------------------------" << std::endl;
    sylar::Config::LoadFromYaml(root);
    g_logger = SYLAR_LOG_ROOT();
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    //sylar::Config::LogAllConfigVar();
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 10; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(&fun2, "name_" + std::to_string(i * 2)));
        //sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        // sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_1" + std::to_string(i * 2)));

        // thrs.push_back(thr2);
    }

    //sleep(10);

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}
