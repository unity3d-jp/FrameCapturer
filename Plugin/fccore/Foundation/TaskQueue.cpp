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
    if (!m_running) {
        m_thread = std::thread([this]() { process(); });
        m_running = true;
    }

    {
        Lock l(m_mutex);
        m_tasks.push_back(v);
    }
    m_condition.notify_one();
}

void TaskQueue::wait()
{
    if (m_running) {
        m_stop = true;
        m_condition.notify_one();
        m_thread.join();
        m_running = false;
        m_stop = false;
    }
}

bool TaskQueue::feed()
{
    Task task;
    {
        Lock lock(m_mutex);
        if (!m_tasks.empty()) {
            task = m_tasks.front();
            m_tasks.pop_front();
        }
    }

    if (task) {
        task();
        return true;
    }
    else {
        return false;
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
