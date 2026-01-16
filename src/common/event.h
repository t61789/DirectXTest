#pragma once
#include <functional>
#include <vector>

#include "common/utils.h"

namespace dt
{
    using EventHandler = size_t;

    static EventHandler CreateEventHandler()
    {
        static size_t globalEventId = 0;
        ++globalEventId;
        return globalEventId;
    }
    
    template<typename... Args>
    class Event
    {
    public:
        template<typename T>
        EventHandler Add(T* obj, void (T::*func)(Args...), EventHandler handler = 0);
        EventHandler Add(std::function<void(Args...)> func, EventHandler handler = 0);
        
        void Remove(EventHandler handler);

        void Invoke(Args... args);

    private:
        std::vector<std::pair<size_t, std::function<void(Args ...)>>> m_callbacks;

        EventHandler AddCallback(std::function<void(Args...)> f, EventHandler handler);
    };

    template <typename ... Args>
    template <typename T>
    EventHandler Event<Args...>::Add(T* obj, void(T::* func)(Args...), const EventHandler handler)
    {
        auto cb = [obj, func](Args... a) {
            return (obj->*func)(a...);
        };
            
        return AddCallback(std::move(cb), handler);
    }

    template <typename ... Args>
    EventHandler Event<Args...>::Add(std::function<void(Args...)> func, const EventHandler handler)
    {
        return AddCallback(std::move(func), handler);
    }

    template <typename ... Args>
    void Event<Args...>::Remove(EventHandler handler)
    {
        if (handler == 0)
        {
            return;
        }

        remove_if(m_callbacks, [handler](const std::pair<size_t, std::function<void(Args...)>>& pair)
        {
            return pair.first == handler;
        });
    }

    template <typename ... Args>
    void Event<Args...>::Invoke(Args... args)
    {
        if (m_callbacks.size() == 1)
        {
            m_callbacks[0].second(std::forward<Args>(args)...);
            return;
        }
        
        for (auto& callback : m_callbacks)
        {
            callback.second(args...);
        }
    }

    template <typename ... Args>
    EventHandler Event<Args...>::AddCallback(std::function<void(Args...)> f, EventHandler handler)
    {
        if (!handler)
        {
            handler = CreateEventHandler();
        }
            
        m_callbacks.push_back(std::pair(handler, std::move(f)));
        return handler;
    }
}
