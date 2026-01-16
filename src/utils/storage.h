#pragma once
#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    template <typename Backend>
    class Storage
    {
    public:
        size_t GetSizeB() const { return m_sizeB; }

        size_t Alloc(size_t sizeB);
        void GetBlock(size_t key, size_t& offsetB, size_t& sizeB);
        
        void Write(size_t key, const void* data);
        void Read(size_t key, void* data);
        void Reserve(size_t capacityB);

    private:
        Storage() = default;
        
        size_t DerivedGetCapacity() { return static_cast<Backend*>(this)->GetCapacity(); }
        void DerivedSetCapacity(size_t capacityB) { return static_cast<Backend*>(this)->SetCapacity(capacityB); }
        void DerivedWrite(size_t offsetB, size_t sizeB, const void* data) { static_cast<Backend*>(this)->Write(offsetB, sizeB, data); }
        void DerivedRead(size_t offsetB, size_t sizeB, void* data) { static_cast<Backend*>(this)->Read(offsetB, sizeB, data); }
        
        struct Block
        {
            size_t key;
            size_t offsetB;
            size_t sizeB;
        };
        
        uint32_t m_sizeB = 0;

        umap<size_t, uint32_t> m_keyMapper;
        vec<Block> m_elements;
        
        friend Backend;
    };

    template <typename T>
    size_t Storage<T>::Alloc(const size_t sizeB)
    {
        assert(DerivedGetCapacity() > 0);
        
        auto newSize = m_sizeB + sizeB;
        Reserve(newSize);

        Block obj;
        obj.key = Utils::GetRandomSizeT();
        obj.offsetB = m_sizeB;
        obj.sizeB = sizeB;

        assert(m_keyMapper.find(obj.key) == m_keyMapper.end());

        m_keyMapper[obj.key] = static_cast<uint32_t>(m_elements.size());
        m_elements.push_back(obj);

        m_sizeB = newSize;

        return obj.key;
    }

    template <typename T>
    void Storage<T>::Write(const size_t key, const void* data)
    {
        auto it = m_keyMapper.find(key);
        if (it == m_keyMapper.end())
        {
            THROW_ERRORF("Can't find element with index %d", key)
        }
        
        auto& element = m_elements[it->second];
        DerivedWrite(element.offsetB, element.sizeB, data);
    }

    template <typename T>
    void Storage<T>::Read(const size_t key, void* data)
    {
        auto it = m_keyMapper.find(key);
        if (it == m_keyMapper.end())
        {
            THROW_ERRORF("Can't find element with index %d", key)
        }

        auto& element = m_elements[it->second];
        DerivedRead(element.offsetB, element.sizeB, data);
    }

    template <typename T>
    void Storage<T>::Reserve(const size_t capacityB)
    {
        if (capacityB > DerivedGetCapacity())
        {
            auto newCapacityB = DerivedGetCapacity();
            while (capacityB > newCapacityB)
            {
                newCapacityB <<= 1;
            }

            DerivedSetCapacity(newCapacityB);
        }
    }

    template <typename T>
    void Storage<T>::GetBlock(const size_t key, size_t& offsetB, size_t& sizeB)
    {
        auto it = m_keyMapper.find(key);
        if (it == m_keyMapper.end())
        {
            THROW_ERRORF("Can't find element with index %d", key)
        }

        auto& element = m_elements[it->second];
        offsetB = element.offsetB;
        sizeB = element.sizeB;
    }
}
