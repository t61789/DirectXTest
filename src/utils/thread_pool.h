#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "common/const.h"

namespace dt
{
    class ThreadPool
    {
        using Task = std::function<void()>;

        struct CompareByPriority
        {
            bool operator()(const std::pair<int32_t, Task>& a, const std::pair<int32_t, Task>& b) const
            {
                return a.first < b.first;
            }
        };
        
    public:
        explicit ThreadPool(uint32_t numThreads);
        ~ThreadPool();
        ThreadPool(const ThreadPool& other) = delete;
        ThreadPool(ThreadPool&& other) noexcept = delete;
        ThreadPool& operator=(const ThreadPool& other) = delete;
        ThreadPool& operator=(ThreadPool&& other) noexcept = delete;

        template <typename F>
        void Run(F&& task, int32_t priority = 0);

    private:
        vec<std::thread> m_threads;
        std::priority_queue<std::pair<int32_t, Task>, std::vector<std::pair<int32_t, Task>>, CompareByPriority> m_tasks;
        std::mutex m_taskMutex;
        std::condition_variable m_taskCond;
        bool m_shutdown = false;

        void Worker();
    };

    template <typename F>
    void ThreadPool::Run(F&& task, int32_t priority)
    {
        std::lock_guard lock(m_taskMutex);
        m_tasks.emplace(priority, std::forward<F>(task));
        m_taskCond.notify_all();
    }
}
