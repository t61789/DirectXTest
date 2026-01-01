#pragma once
#include <cstdint>

#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    template <typename T>
    class SingleElemPool
    {
    public:
        struct Handle
        {
            Handle() = default;
            ~Handle();
            Handle(const Handle& other) = delete;
            Handle(Handle&& other) noexcept = delete;
            Handle& operator=(const Handle& other) = delete;
            Handle& operator=(Handle&& other) noexcept = delete;
            
            uint32_t GetIndex() const { return m_index; }
            
            T data;

        private:
            uint32_t m_index;
            SingleElemPool* m_pool;

            friend class SingleElemPool;
        };

        explicit SingleElemPool(uint32_t capacity);
        ~SingleElemPool();
        SingleElemPool(const SingleElemPool& other) = delete;
        SingleElemPool(SingleElemPool&& other) noexcept = delete;
        SingleElemPool& operator=(const SingleElemPool& other) = delete;
        SingleElemPool& operator=(SingleElemPool&& other) noexcept = delete;

        sp<Handle> Alloc(bool append = false);

        template <typename F>
        sp<Handle> Find(F&& predicate);

    private:
        struct Elem
        {
            wp<Handle> handle;
            uint32_t index;
        };
        
        void Release(const Handle* handle);
        
        vec<Elem> m_data;
        uint32_t m_capacity;
        uint32_t m_count = 0;
        uint32_t m_size = 0;
        uint32_t m_firstFreeIndex = 0;
    };

    template <typename T>
    SingleElemPool<T>::Handle::~Handle()
    {
        m_pool->Release(this);
    }

    template <typename T>
    SingleElemPool<T>::SingleElemPool(const uint32_t capacity)
    {
        assert(capacity > 0);
        
        m_data = vec<Elem>(capacity);
        for (uint32_t i = 0; i < capacity; ++i)
        {
            m_data[i].index = i;
        }
        
        m_capacity = capacity;
    }

    template <typename T>
    SingleElemPool<T>::~SingleElemPool()
    {
        assert(m_count == 0);
    }

    template <typename T>
    sp<typename SingleElemPool<T>::Handle> SingleElemPool<T>::Alloc(const bool append)
    {
        auto searchStart = append ? m_size : m_firstFreeIndex;
        
        for (uint32_t i = searchStart; i < m_capacity; ++i)
        {
            if (!m_data[i].handle.expired())
            {
                continue;
            }

            auto handle = msp<Handle>();
            handle->m_index = i;
            handle->m_pool = this;

            m_data[i].handle = handle;
            m_firstFreeIndex = i + 1;
            ++m_count;
            m_size = (std::max)(i + 1, m_size);
            
            return handle;
        }

        THROW_ERROR("Out of space")
    }

    template <typename T>
    void SingleElemPool<T>::Release(const Handle* handle)
    {
        auto index = handle->m_index;
        assert(index < m_capacity);

        auto& elem = m_data[index];

        m_firstFreeIndex = (std::min)(m_firstFreeIndex, index);
        --m_count;

        while (m_size > 0 && m_data[m_size - 1].handle.expired())
        {
            --m_size;
        }
    }

    template <typename T>
    template <typename Predicate>
    sp<typename SingleElemPool<T>::Handle> SingleElemPool<T>::Find(Predicate&& predicate)
    {
        for (uint32_t i = 0; i < m_size; ++i)
        {
            auto& handle = m_data[i].handle;
            if (!handle.expired() && predicate(handle.lock()))
            {
                return handle.lock();
            }
        }

        return nullptr;
    }
}
