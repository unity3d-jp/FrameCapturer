#include "pch.h"
#include "fcThreadPool.h"

#ifndef fcWithTBB

class fcWorkerThread
{
public:
    void operator()();
};


void fcWorkerThread::operator()()
{
    fcThreadPool &pool = fcThreadPool::getInstance();
    std::function<void()> task;
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(pool.m_queue_mutex);
            while (!pool.m_stop && pool.m_tasks.empty()) {
                pool.m_condition.wait(lock);
            }
            if (pool.m_stop) { return; }

            task = pool.m_tasks.front();
            pool.m_tasks.pop_front();
        }
        task();
    }
}

fcThreadPool::fcThreadPool(size_t threads)
    : m_stop(false)
{
    for (size_t i = 0; i < threads; ++i) {
        m_workers.push_back(std::thread(fcWorkerThread()));
    }
}

fcThreadPool::~fcThreadPool()
{
    m_stop = true;
    m_condition.notify_all();

    for (auto& worker : m_workers) {
        worker.join();
    }
}

fcThreadPool& fcThreadPool::getInstance()
{
    static fcThreadPool s_instance(std::thread::hardware_concurrency());
    return s_instance;
}

void fcThreadPool::enqueue(const std::function<void()> &f)
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_tasks.push_back(std::function<void()>(f));
    }
    m_condition.notify_one();
}



fcTaskGroup::fcTaskGroup()
{
    m_active_tasks = 0;
}

fcTaskGroup::~fcTaskGroup()
{
}

void fcTaskGroup::wait()
{
    fcThreadPool &pool = fcThreadPool::getInstance();
    while (m_active_tasks > 0)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(pool.m_queue_mutex);
            if (!pool.m_tasks.empty()) {
                task = pool.m_tasks.front();
                pool.m_tasks.pop_front();
            }
        }
        if (task) { task(); }
    }
}

#endif // fcWithTBB
