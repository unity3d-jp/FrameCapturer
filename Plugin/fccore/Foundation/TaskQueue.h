#pragma once

#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>


template<class T>
class ResourceQueue
{
public:
    void push(T v)
    {
        std::unique_lock<std::mutex> l(m_mutex);
        m_resources.push_back(v);
    }

    T pop()
    {
        T ret;
        for (;;) {
            {
                std::unique_lock<std::mutex> l(m_mutex);
                if (!m_resources.empty()) {
                    ret = m_resources.back();
                    m_resources.pop_back();
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return ret;
    }

private:
    std::mutex m_mutex;
    std::deque<T> m_resources;
};


class TaskQueue
{
public:
    using Task = std::function<void()>;
    using Tasks = std::deque<Task>;
    using Lock = std::unique_lock<std::mutex>;

    void start();
    void run(const Task& v);
    void stop();

private:
    void process();

    std::thread             m_thread;
    std::mutex              m_mutex;
    std::condition_variable m_condition;
    Tasks                   m_tasks;
    bool                    m_stop = false;
};
