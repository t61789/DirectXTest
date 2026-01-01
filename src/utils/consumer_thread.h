#pragma once
#include <deque>
#include <functional>
#include <mutex>
#include <boost/lockfree/queue.hpp>
#include <tracy/Tracy.hpp>

namespace dt
{
    template <typename T>
    class ConsumerThread
    {
    public:
        template <typename ConsumeFunc>
        explicit ConsumerThread(ConsumeFunc&& f, const char* name = nullptr);
        ~ConsumerThread();
        ConsumerThread(const ConsumerThread& other) = delete;
        ConsumerThread(ConsumerThread&& other) noexcept = delete;
        ConsumerThread& operator=(const ConsumerThread& other) = delete;
        ConsumerThread& operator=(ConsumerThread&& other) noexcept = delete;

        bool IsStopped() const { return !m_thread.joinable(); }

        void Enqueue(const T& product);
        void Stop(bool immediate = false);
        void Wait();
        void Join();

    private:
        template <class ConsumeFunc>
        void Thread(ConsumeFunc&& consumeFunc);
        
        std::deque<T> m_productQueue;
        std::mutex m_mutex;
        std::condition_variable m_hasProductCond;
        std::condition_variable m_idleCond;
        std::thread m_thread;

        std::atomic<bool> m_idle = false;
        std::atomic<bool> m_stopFlag = false;
        std::atomic<bool> m_stopImmediate = false;
    };

    template <typename T>
    template <typename ConsumeFunc>
    ConsumerThread<T>::ConsumerThread(ConsumeFunc&& f, const char* name)
    {
        m_thread = std::thread([this, f=std::move(f), name]
        {
            if (name)
            {
                tracy::SetThreadName(name);
            }
            
            Thread(f);
        });
    }

    template <typename T>
    ConsumerThread<T>::~ConsumerThread()
    {
        Stop(true);

        Join();
    }

    template <typename T>
    void ConsumerThread<T>::Enqueue(const T& product)
    {
        ZoneScoped;

        assert(!m_stopFlag);

        std::lock_guard lock(m_mutex);
        m_productQueue.push_back(product);
        m_hasProductCond.notify_one();
    }

    template <typename T>
    void ConsumerThread<T>::Stop(const bool immediate)
    {
        std::lock_guard lock(m_mutex);
        m_stopFlag = true;
        m_stopImmediate = immediate;
        m_hasProductCond.notify_one();
    }

    template <typename T>
    void ConsumerThread<T>::Wait()
    {
        std::unique_lock lock(m_mutex);
        m_idleCond.wait(lock, [this]
        {
            return m_idle.load() && m_productQueue.empty();
        });
    }

    template <typename T>
    void ConsumerThread<T>::Join()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    template <typename T>
    template <typename ConsumeFunc>
    void ConsumerThread<T>::Thread(ConsumeFunc&& consumeFunc)
    {
        while (true)
        {
            T product;

            {
                std::unique_lock lock(m_mutex);
                if (m_stopImmediate.load())
                {
                    break;
                }
                
                if (m_productQueue.empty())
                {
                    if (m_stopFlag)
                    {
                        break;
                    }

                    m_idle.store(true);
                    m_idleCond.notify_all();

                    m_hasProductCond.wait(lock, [this]
                    {
                        return !m_productQueue.empty() || m_stopFlag;
                    });
                    m_idle.store(false);

                    continue;
                }
                
                product = std::move(m_productQueue.front());
                m_productQueue.pop_front();
            }

            consumeFunc(product);
        }

        m_productQueue.clear();

        m_idle.store(true);
        m_idleCond.notify_all();
    }
}
