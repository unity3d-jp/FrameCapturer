#ifndef fcThreadPool_h
#define fcThreadPool_h

#ifndef fcWithTBB

#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class fcWorkerThread;
class fcThreadPool;
class fcTaskGroup;


class fcThreadPool
{
friend class fcWorkerThread;
friend class fcTaskGroup;
public:
    static fcThreadPool& getInstance();
    void enqueue(const std::function<void()> &f);

private:
    fcThreadPool(size_t);
    ~fcThreadPool();

private:
    std::vector< std::thread > m_workers;
    std::deque< std::function<void()> > m_tasks;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    bool m_stop;
};



class fcTaskGroup
{
public:
    fcTaskGroup();
    ~fcTaskGroup();
    template<class F> void run(const F &f);
    void wait();

private:
    std::atomic_int m_active_tasks;
};

template<class F>
void fcTaskGroup::run(const F &f)
{
    ++m_active_tasks;
    fcThreadPool::getInstance().enqueue([this, f](){
        f();
        --m_active_tasks;
    });
}

#else // fcWithTBB

#include <tbb/tbb.h>
typedef tbb::task_group fcTaskGroup;

#endif // fcWithTBB

#endif // fcThreadPool_h
