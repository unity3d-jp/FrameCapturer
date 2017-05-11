#pragma once
#include <vector>
#include <deque>
#include <future>
#include <atomic>

class TaskGroup
{
public:
    TaskGroup();
    ~TaskGroup();

    int getMaxTasks() const { return m_max_tasks; }
    void setMaxTasks(int v) { m_max_tasks = v; }

    void wait();

    template<class Body>
    void run(const Body &body)
    {
        while (m_futures.size() >= m_max_tasks) {
            m_futures.front().get();
            m_futures.pop_front();
        }

        m_futures.push_back(std::async(std::launch::async, body));
    }

private:
    std::deque<std::future<void>> m_futures;
    int m_max_tasks = 8;
};
