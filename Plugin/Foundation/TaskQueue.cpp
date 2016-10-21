#include "pch.h"
#include "TaskQueue.h"

void TaskQueue::start()
{
    stop();
    m_stop = false;

    m_thread = std::thread([this]() { process(); });
}

void TaskQueue::run(const Task& v)
{
    {
        Lock l(m_mutex);
        m_tasks.push_back(v);
    }
    m_condition.notify_one();
}

void TaskQueue::stop()
{
    if (m_thread.joinable()) {
        m_stop = true;
        m_condition.notify_one();
        m_thread.join();
    }
}

void TaskQueue::process()
{
    while (!m_stop)
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
