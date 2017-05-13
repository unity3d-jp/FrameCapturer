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
};

static fcAsyncDeleteManager g_async_delete_manager;
static bool g_async_delete = false;


fcAPI void fcEnableAsyncReleaseContext(bool v)
{
    g_async_delete = v;
}

fcAPI void fcWaitAsyncDelete()
{
    g_async_delete_manager.wait();
}


fcContextBase::~fcContextBase()
{
    if (m_on_delete) {
        m_on_delete(m_on_delete_param);
    }
}

void fcContextBase::release()
{
    if (g_async_delete) { g_async_delete_manager.add([this]() { delete this; }); }
    else { delete this; }
}

void fcContextBase::setOnDeleteCallback(void(*cb)(void*), void *param)
{
    m_on_delete = cb;
    m_on_delete_param = param;
}
