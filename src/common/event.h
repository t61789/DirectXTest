#pragma once
#include <functional>
#include <vector>

namespace dt
{
    using EventHandler = unsigned long long;

    static EventHandler GenEventHandler()
    {
        static EventHandler handler = 0;
        return ++handler;
    }
    
    template<typename... Args>
    class Event
    {
    public:
        template<typename T>
        EventHandler Add(T* obj, void (T::*func)(Args...), EventHandler handler = 0)
        {
            auto cb = new std::function<void(Args...)>([obj, func](Args... a) {
                return (obj->*func)(a...);
            });
            
            return AddCallback(cb, handler);
        }
        
        EventHandler Add(void (*func)(Args...), EventHandler handler = 0)
        {
            auto cb = new std::function<void(Args...)>([func](Args... a) {
                return (*func)(a...);
            });
            
            return AddCallback(cb, handler);
        }
        
        EventHandler Add(const std::function<void(Args...)>& cb, EventHandler handler = 0)
        {
            auto cb_ptr = new std::function<void(Args...)>(cb);
            
            return AddCallback(cb_ptr, handler);
        }

        void Remove(EventHandler handler)
        {
            if (handler == 0)
            {
                return;
            }

            auto index = -1;
            for (auto& callback : m_callbacks)
            {
                index++;
                if (callback.first == handler)
                {
                    delete callback.second;
                    break;
                }
            }

            if (index == -1)
            {
                return;
            }

            m_callbacks.erase(m_callbacks.begin() + index);
        }

        void Invoke(Args... args)
        {
            for (auto& callback : m_callbacks)
            {
                (*callback.second)(std::forward<Args>(args)...);
            }
        }

    private:
        std::vector<std::pair<EventHandler, std::function<void(Args ...)>*>> m_callbacks;

        EventHandler AddCallback(std::function<void(Args...)>* cb, EventHandler handler)
        {
            if (handler == 0)
            {
                handler = GenEventHandler();
            }
            
            m_callbacks.push_back(std::pair(handler, cb));
            return handler;
        }
    };
}
