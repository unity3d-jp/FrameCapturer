#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>


template<class T>
class SharedResources
{
public:
    using Resource = T;
    using ResourcePtr = std::shared_ptr<T>;

    void push(Resource *v)
    {
        unlock(ResourcePtr(v));
    }

    void unlock(ResourcePtr v)
    {
        {
            std::unique_lock<std::mutex> l(m_mutex);
            m_resources.push_back(v);
        }
        m_condition.notify_one();
    }

    ResourcePtr lock()
    {
        ResourcePtr ret;
        {
            std::unique_lock<std::mutex> l(m_mutex);
            if (m_resources.empty()) {
                m_condition.wait(l);
            }
            if (!m_resources.empty()) {
                ret = m_resources.back();
                m_resources.pop_back();
            }
        }
        return ret;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::vector<ResourcePtr> m_resources;
};


class TaskQueue
{
public:
    using Task = std::function<void()>;
    using Tasks = std::deque<Task>;
    using Lock = std::unique_lock<std::mutex>;

    TaskQueue();
    ~TaskQueue();
    void wait();
    bool feed();
    void run(const Task& v);

private:
    void process();

    std::thread             m_thread;
    std::mutex              m_mutex;
    std::condition_variable m_condition;
    std::atomic_bool        m_stop = { false };
    std::atomic_bool        m_running = { false };
    Tasks                   m_tasks;
};
