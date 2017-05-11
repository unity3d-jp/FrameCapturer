#include "pch.h"
#include "fcInternal.h"
#include "TaskGroup.h"

TaskGroup::TaskGroup()
{
}

TaskGroup::~TaskGroup()
{
    wait();
}

void TaskGroup::wait()
{
    for (auto& f : m_futures) {
        f.get();
    }
    m_futures.clear();
}
