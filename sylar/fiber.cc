#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>
namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

//static thread_local uint64_t t_fiberid = 0;
static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        //return malloc(size);
        void* ptr = malloc(size);
        SYLAR_LOG_DEBUG(g_logger) << "Alloc stack memory: " << ptr << ", size: " << size;
        return ptr;
    }

    static void Dealloc(void* vp, size_t size) {
        SYLAR_LOG_DEBUG(g_logger) << "Dealloc stack memory: " << vp << ", size: " << size;

        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    m_state = EXEC;

    coctx_init(&m_ctx); 
    ++s_fiber_count;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :use_caller(use_caller)
    ,m_id(++s_fiber_id)
    ,m_cb(cb) {
    SetThis(this);
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    
    if(coctx_init(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.ss_size = m_stacksize;
    m_ctx.ss_sp = (char*)m_stack;
    // m_ctx.uc_link = nullptr;
    // m_ctx.uc_stack.ss_sp = m_stack;
    // m_ctx.uc_stack.ss_size = m_stacksize;

    // coctx_make(&m_ctx, 
    //     (use_caller ? &Fiber::CallerMainFunc : &Fiber::MainFunc), 
    //     this, nullptr);

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
    if(!use_caller) {
        //makecontext(&m_ctx, &Fiber::MainFunc, 0);
        //typedef void* (*coctx_pfn_t)( void* s, void* s2 );
        coctx_make(&m_ctx,this->run, nullptr, nullptr);
    } else {
        //makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        coctx_make(&m_ctx,this->run, nullptr, nullptr);
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

Fiber::~Fiber() {
    SetThis(this);
    --s_fiber_count;
    if(m_ctx.ss_sp) {
        //SYLAR_LOG_DEBUG(g_logger) << "---------------"<< m_state << "---" << m_id;
        SYLAR_ASSERT(m_state == TERM
                || m_state == EXCEPT
                || m_state == INIT);
        StackAllocator::Dealloc(m_ctx.ss_sp, m_ctx.ss_size);
    } else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << " total=" << s_fiber_count;
}

//重置协程函数，并重置状态
//INIT，TERM, EXCEPT
void Fiber::reset(std::function<void()> cb) {
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM
            || m_state == EXCEPT
            || m_state == INIT);
    m_cb = cb;

    if(!use_caller) {
        //makecontext(&m_ctx, &Fiber::MainFunc, 0);
        //typedef void* (*coctx_pfn_t)( void* s, void* s2 );
        coctx_make(&m_ctx,this->run, nullptr, nullptr);
    } else {
        //makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        coctx_make(&m_ctx,this->run, nullptr, nullptr);
    }

    m_state = INIT;
}


//切换到当前协程执行
void Fiber::resume() {
    SetThis(this);

    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(use_caller)
        coctx_swap(&Scheduler::GetMainFiber()->m_ctx, &m_ctx);
    else
        coctx_swap(&t_threadFiber->m_ctx, &m_ctx);
    // if(coctx_swap(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
    //     SYLAR_ASSERT2(false, "swapcontext");
    // }
}

//切换到后台执行
void Fiber::yield() {
    // auto cur = GetThis().get();
    //SYLAR_ASSERT(m_state == EXEC);
    // if(m_state != HOLD&&m_state != TERM) SYLAR_LOG_ERROR(g_logger) << m_state;
    // if(m_state == EXEC) m_state = HOLD;
    if(use_caller){
        SetThis(Scheduler::GetMainFiber());
        coctx_swap(&m_ctx, &Scheduler::GetMainFiber()->m_ctx);
    }
    else {
        SetThis(t_threadFiber.get());
        if(m_state!=TERM) m_state = READY;
        coctx_swap(&m_ctx, &t_threadFiber->m_ctx);
    }
}

void Fiber::PrintFiberInfo()
{
    SYLAR_LOG_DEBUG(g_logger) << "当前运行的协程id为 " << t_fiber->m_id;
    SYLAR_LOG_DEBUG(g_logger) << "当前运行的协程返回的协程为 " << t_threadFiber->m_id;
}


//设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

//返回当前协程
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    t_fiber = main_fiber.get();
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}


//总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}
void* Fiber::run(void* a,void* b) {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->yield();
    
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

}

}
