#pragma once
#include "common/utils.h"

namespace dt
{
    class IRecyclable
    {
    public:
        IRecyclable() = default;
        virtual ~IRecyclable() = default;
        IRecyclable(const IRecyclable& other) = delete;
        IRecyclable(IRecyclable&& other) noexcept = delete;
        IRecyclable& operator=(const IRecyclable& other) = delete;
        IRecyclable& operator=(IRecyclable&& other) noexcept = delete;
    };
    
    class RecycleBin : public Singleton<RecycleBin>
    {
    public:
        void Add(IRecyclable* garbage);
        void Flush();

    private:
        std::mutex m_mutex;
        vec<IRecyclable*> m_pending;
    };

    inline void RecycleBin::Add(IRecyclable* garbage)
    {
        std::lock_guard lock(m_mutex);
        m_pending.push_back(garbage);
    }

    inline void RecycleBin::Flush()
    {
        std::lock_guard lock(m_mutex);
        for (auto ptr : m_pending)
        {
            delete ptr;
        }
        m_pending.clear();
    }

    template<typename T, typename... Args>
    static std::shared_ptr<T> make_recyclable(Args&&... args)
    {
        T* rawPtr = new T(std::forward<Args>(args)...);
    
        return std::shared_ptr<T>(rawPtr, [](T* ptr)
        {
            RecycleBin::Ins()->Add(static_cast<IRecyclable*>(ptr));
        });
    }
}
