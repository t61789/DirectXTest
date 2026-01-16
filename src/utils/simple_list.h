#pragma once
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace dt
{
    template <typename T>
    struct SimpleList
    {
        static_assert(std::is_trivial_v<T>);

        SimpleList() = default;
        explicit SimpleList(uint32_t capacity, uint32_t alignment = 16);
        ~SimpleList();
        SimpleList(const SimpleList& other);
        SimpleList(SimpleList&& other) noexcept;
        SimpleList& operator=(const SimpleList& other);
        SimpleList& operator=(SimpleList&& other) noexcept;
        
        T& operator[](size_t index) { return m_data[index]; }
        const T& operator[](size_t index) const { return m_data[index]; }

        T* Data() const { return m_data; }
        uint32_t Size() const { return m_back - m_data; }
        uint32_t Capacity() const { return m_capacity; }
        bool Empty() const { return m_back <= m_data; }
        
        template <bool Check = true>
        void Add(const T& element);
        bool Pop(T& element);
        
        void Resize(uint32_t newSize);
        void Reserve(uint32_t newCapacity);
        void Clear();

    private:
        T* m_data = nullptr;
        T* m_back = nullptr;
        uint32_t m_capacity = 0;
        std::align_val_t m_alignment = static_cast<std::align_val_t>(16);

        void CopyFrom(const SimpleList& other);
        void StealFrom(SimpleList& other);
        
        T* Alloc(uint32_t count);
        void Release(T* ptr);
    };

    template <typename T>
    SimpleList<T>::SimpleList(const uint32_t capacity, const uint32_t alignment)
    {
        assert(alignment >= 16 && (alignment & (alignment - 1)) == 0);
        
        m_alignment = static_cast<std::align_val_t>(alignment);
        m_data = Alloc(capacity);
        m_capacity = capacity;
        m_back = m_data;
    }

    template <typename T>
    SimpleList<T>::~SimpleList()
    {
        if (m_data)
        {
            Release(m_data);
            m_data = nullptr;
        }
        m_capacity = 0;
        m_back = nullptr;
    }

    template <typename T>
    SimpleList<T>::SimpleList(const SimpleList& other)
    {
        CopyFrom(other);
    }

    template <typename T>
    SimpleList<T>::SimpleList(SimpleList&& other) noexcept
    {
        StealFrom(other);
    }

    template <typename T>
    SimpleList<T>& SimpleList<T>::operator=(const SimpleList& other)
    {
        if (this == &other)
        {
            return *this;
        }

        CopyFrom(other);

        return *this;
    }

    template <typename T>
    SimpleList<T>& SimpleList<T>::operator=(SimpleList&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        StealFrom(other);
        
        return *this;
    }

    template <typename T>
    template <bool Check>
    void SimpleList<T>::Add(const T& element)
    {
        if constexpr (Check)
        {
            auto newSize = Size() + 1;
            if (newSize >= m_capacity)
            {
                Reserve(std::max(m_capacity * 2, newSize));
            }
        }

        *m_back = element;
        ++m_back;
    }

    template <typename T>
    bool SimpleList<T>::Pop(T& element)
    {
        if (m_back <= m_data)
        {
            return false;
        }

        --m_back;
        element = *m_back;

        return true;
    }

    template <typename T>
    void SimpleList<T>::Resize(const uint32_t newSize)
    {
        Reserve(newSize);

        m_back = m_data + newSize;
    }

    template <typename T>
    void SimpleList<T>::Reserve(const uint32_t newCapacity)
    {
        if (newCapacity <= m_capacity)
        {
            return;
        }

        auto newData = Alloc(newCapacity);
        auto preSize = Size();
        if (m_data)
        {
            if (preSize > 0)
            {
                memcpy(newData, m_data, preSize * sizeof(T));
            }
            Release(m_data);
        }
        m_data = newData;
        m_back = m_data + preSize;
        m_capacity = newCapacity;
    }

    template <typename T>
    void SimpleList<T>::Clear()
    {
        m_back = m_data;
    }

    template <typename T>
    void SimpleList<T>::CopyFrom(const SimpleList& other)
    {
        if (m_data)
        {
            Release(m_data);
        }
        
        auto otherSize = other.Size();
        if (other.m_capacity != 0)
        {
            m_data = Alloc(other.m_capacity);
            if (otherSize > 0)
            {
                memcpy(m_data, other.m_data, otherSize * sizeof(T));
            }
        }
        m_capacity = other.m_capacity;
        m_back = m_data + otherSize;
    }

    template <typename T>
    void SimpleList<T>::StealFrom(SimpleList& other)
    {
        assert(m_alignment == other.m_alignment);
        
        m_data = other.m_data;
        m_back = other.m_back;
        m_capacity = other.m_capacity;

        other.m_data = nullptr;
        other.m_back = nullptr;
        other.m_capacity = 0;
    }

    template <typename T>
    T* SimpleList<T>::Alloc(const uint32_t count)
    {
        auto ptr = operator new(count * sizeof(T), m_alignment);
        return static_cast<T*>(ptr);
    }

    template <typename T>
    void SimpleList<T>::Release(T* ptr)
    {
        operator delete(ptr, m_alignment);
    }
}