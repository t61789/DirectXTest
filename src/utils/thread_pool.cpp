#include "thread_pool.h"

#include "consumer_thread.h"

namespace dt
{
    ThreadPool::ThreadPool(const uint32_t numThreads)
    {
        for (uint32_t i = 0; i < numThreads; ++i)
        {
            m_threads.emplace_back(&ThreadPool::Worker, this);
        }
    }

    ThreadPool::~ThreadPool()
    {
        {
            std::lock_guard lock(m_taskMutex);
            m_shutdown = true;
            m_taskCond.notify_all();
        }

        for (auto& thread : m_threads)
        {
            thread.join();
        }
    }

    void ThreadPool::Worker()
    {
        tracy::SetThreadName("ThreadPool Worker");
        
        while (true)
        {
            Task task;
            
            {
                std::unique_lock lock(m_taskMutex);
                m_taskCond.wait(lock, [this]
                {
                    return m_shutdown || !m_tasks.empty();
                });
                
                if (m_shutdown)
                {
                    return;
                }
                
                task = m_tasks.top().second;
                m_tasks.pop();
            }

            task();
        }
    }
}
