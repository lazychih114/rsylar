#include "sylar/sylar.h"
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_scheduler() {
    // 测试用例管理用智能指针
    struct TestCase {
        typedef sylar::Mutex MutexType;
        MutexType m_mutex;
        typedef std::shared_ptr<TestCase> ptr;
        std::atomic<int> count{0};
        std::atomic<int> finish{0};
        sylar::Scheduler* scheduler;
    };

    // 1. 基本调度测试
    // {
    //     sylar::Scheduler sc(3, false);
    //     sc.start();

    //     auto tc = std::make_shared<TestCase>();
    //     tc->scheduler = sylar::Scheduler::GetThis();

    //     for(int i = 0; i < 1000; ++i) {
    //         sc.schedule([tc,i](){
    //             TestCase::MutexType::Lock lock(tc->m_mutex);
    //             ++tc->count;
    //             //sylar::Fiber::YieldToHold();  // 测试协程让出
    //             ++tc->finish;
    //             SYLAR_LOG_INFO(g_logger) << "fiber " << i << " is over";
    //             // 验证是否在调度器线程执行
    //             //SYLAR_ASSERT(sylar::Scheduler::GetThis() == tc->scheduler);
    //         });
    //     }

        
    //     sc.stop();
    //     sleep(3);
    //     SYLAR_ASSERT(tc->count == 1000);
    //     SYLAR_LOG_INFO(g_logger) << "----------------";
    // }

    // // 2. 跨调度器切换测试
    // {
    //     sylar::Scheduler sc1(2, true, "sc1");
    //     sylar::Scheduler sc2(2, true, "sc2");
        
    //     std::atomic<int> cross_count{0};
    //     std::vector<int> execute_threads;

    //     sc1.schedule([&](){
    //         // 切换到sc2调度器
    //         sylar::SchedulerSwitcher sw(&sc2);
    //         cross_count++;
            
    //         // 在sc2中调度任务
    //         sc2.schedule([&](){
    //             execute_threads.push_back(sylar::GetThreadId());
    //             cross_count++;
    //         });
    //     });

    //     // 等待跨调度器任务完成
    //     while(cross_count.load() < 2) {
    //         sylar::Fiber::YieldToHold();
    //     }

    //     SYLAR_ASSERT(execute_threads.size() == 1);
    //     SYLAR_ASSERT(execute_threads[0] != sylar::GetThreadId());

    //     sc1.stop();
    //     sc2.stop();
    // }

    // 3. 性能压力测试（50000个任务）
    {
        const int TASK_COUNT = 50000;
        sylar::Scheduler sc(8, false);
        sc.start();
        
        std::atomic<int> counter{0};
        //sylar::Timer timer;
        auto start_time = std::chrono::steady_clock::now(); 

        for(int i = 0; i < TASK_COUNT; ++i) {
            sc.schedule([&counter](){
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }

        // while(counter.load() < TASK_COUNT) {
        //     sylar::Fiber::YieldToHold();
        // }

        //double elapsed = timer.elapsed();
        sc.stop();
        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        SYLAR_LOG_INFO(g_logger) << "result is " << counter;
        SYLAR_LOG_INFO(g_logger) << "Processed " << TASK_COUNT 
                                      << " tasks in " << elapsed << " seconds ("
                                      << (TASK_COUNT / elapsed) << " tasks/s)";
        
    }

    // // 4. 异常处理测试
    // {
    //     sylar::Scheduler sc(2, false);
    //     sc.start();
        
    //     bool exception_handled = false;
        
    //     // 抛出异常的协程
    //     sc.schedule([&](){
    //         try {
    //             throw std::runtime_error("test exception");
    //         } catch (...) {
    //             exception_handled = true;
    //         }
    //     });

    //     // 正常任务
    //     std::atomic<int> normal_count{0};
    //     for(int i = 0; i < 5; ++i) {
    //         sc.schedule([&](){
    //             normal_count++;
    //         });
    //     }

    //     // 等待所有任务完成
    //     while(normal_count.load() < 5 || !exception_handled) {
    //         sylar::Fiber::YieldToHold();
    //     }

    //     SYLAR_ASSERT(exception_handled);
    //     SYLAR_ASSERT(normal_count == 5);
    //     sc.stop();
    // }

    // 5. 嵌套调度测试
    // {
    //     sylar::Scheduler main_sc(4, true, "main");
    //     sylar::Scheduler sub_sc(2, false, "sub");
        
    //     std::atomic<int> nested_count{0};
    //     const int NESTED_LEVEL = 5;

    //     auto schedule_nested = [&](int level) {
    //         if(level >= NESTED_LEVEL) return;
            
    //         sub_sc.schedule([=, &nested_count](){
    //             nested_count++;
                
    //             // 递归调度
    //             if(level < NESTED_LEVEL-1) {
    //                 main_sc.schedule(std::bind(schedule_nested, level+1));
    //             }
    //         });
    //     };

    //     main_sc.schedule(std::bind(schedule_nested, 0));
        
    //     // 等待嵌套任务完成
    //     while(nested_count.load() < NESTED_LEVEL) {
    //         sylar::Fiber::YieldToHold();
    //     }

    //     SYLAR_ASSERT(nested_count == NESTED_LEVEL);
    //     main_sc.stop();
    //     sub_sc.stop();
    // }
}

int main() {
    sylar::Thread::SetName("main");
    test_scheduler();
    return 0;
}