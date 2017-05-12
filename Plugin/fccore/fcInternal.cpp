#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"

class fcAsyncDeleteManager
{
public:
    ~fcAsyncDeleteManager()
    {
        wait();
    }

    void wait()
    {
        while (m_task_queue.feed()) {}
        m_task_queue.wait();
    }

    void add(std::function<void()> v)
    {
        m_task_queue.run(v);
    }

private:
    TaskQueue m_task_queue;
} g_asyncReleaseManager;

fcAPI void fcWaitAsyncDelete()
{
    g_asyncReleaseManager.wait();
}


fcContextBase::~fcContextBase()
{
    if (m_on_delete) {
        m_on_delete(m_on_delete_param);
    }
}

void fcContextBase::release(bool async)
{
    if (async) { g_asyncReleaseManager.add([this]() { delete this; }); }
    else { delete this; }
}

void fcContextBase::setOnDeleteCallback(void(*cb)(void*), void *param)
{
    m_on_delete = cb;
    m_on_delete_param = param;
}
