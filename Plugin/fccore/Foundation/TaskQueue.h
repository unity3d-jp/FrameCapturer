#pragma once

#include <vector>
#include <deque>
#include <memory>
#include <algorithm>
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
    using ResourcePtrs = std::vector<ResourcePtr>;
    using Lock = std::unique_lock<std::mutex>;

    class ResourceHolder
    {
    public:
        ResourceHolder() {}
        ResourceHolder(SharedResources *owner, ResourcePtr res)
            : m_owner(owner), m_resource(res)
        {}
        ~ResourceHolder()
        {
            if (m_owner && m_resource) {
                if (m_resource.use_count() == 1) {
                    m_owner->release(m_resource);
                }
            }
        }

        operator bool() const { return m_resource; }
        Resource& operator*() { return *m_resource; }
        const Resource& operator*() const { return *m_resource; }
        Resource* operator->() { return m_resource.get(); }
        const Resource* operator->() const { return m_resource.get(); }

    private:
        mutable SharedResources *m_owner = nullptr;
        ResourcePtr m_resource;
    };

    template<typename ...Params>
    void emplace(Params&&... params)
    {
        add(new Resource(std::forward<Params>(params)...));
    }

    void add(Resource *v)
    {
        Lock l(m_mutex);
        m_resources.emplace_back(v);
        m_max_resources = std::max<size_t>(m_max_resources, m_resources.size());
    }

    ResourceHolder acquire()
    {
        return acquire(std::chrono::hours(24));
    }

    template<class Rep, class Period>
    ResourceHolder acquire(std::chrono::duration<Rep, Period> wait_time)
    {
        ResourceHolder ret;
        if (m_max_resources == 0) { return ret; }
        {
            Lock l(m_mutex);
            if (m_resources.empty()) {
                m_condition.wait_for(l, wait_time);
            }
            if (!m_resources.empty()) {
                ret = { this, m_resources.back() };
                m_resources.pop_back();
            }
        }
        return ret;
    }

private:
    void release(ResourcePtr v)
    {
        {
            Lock l(m_mutex);
            m_resources.push_back(v);
        }
        m_condition.notify_one();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    ResourcePtrs m_resources;
    size_t m_max_resources = 0;
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
