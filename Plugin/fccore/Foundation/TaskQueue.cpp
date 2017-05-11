#include "pch.h"
#include "TaskQueue.h"

TaskQueue::TaskQueue()
{
}

TaskQueue::~TaskQueue()
{
    wait();
}

void TaskQueue::run(const Task& v)
{
    if (!m_thread.joinable()) {
        m_thread = std::thread([this]() { process(); });
    }

    {
        Lock l(m_mutex);
        m_tasks.push_back(v);
    }
    m_condition.notify_one();
}

void TaskQueue::wait()
{
    if (m_thread.joinable()) {
        m_stop = true;
        m_condition.notify_one();
        m_thread.join();
    }
}

void TaskQueue::process()
{
    while (!m_stop || !m_tasks.empty())
    {
        Task task;
        {
            Lock lock(m_mutex);
            while (!m_stop && m_tasks.empty()) {
                m_condition.wait(lock);
            }
            if (m_stop && m_tasks.empty()) { return; }

            task = m_tasks.front();
            m_tasks.pop_front();
        }
        task();
    }
}
