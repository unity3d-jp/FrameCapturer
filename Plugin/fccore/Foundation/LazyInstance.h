#pragma once

#include <memory>

template<class T>
class LazyInstance
{
public:
    T& get()
    {
        if (!m_instance) {
            m_instance.reset(new T());
        }
        return *m_instance;
    }


private:
    std::unique_ptr<T> m_instance;
};
