#pragma once

#include "common/const.h"

namespace dt
{
    class FastMemoryAllocator
    {
    public:
        FastMemoryAllocator()
        {
            m_memoryBlocks.resize(32);
            m_curBlockIndex = 10;
            m_curBlockSize = 0;
        }

        ~FastMemoryAllocator()
        {
            for (auto& block : m_memoryBlocks)
            {
                delete[] block;
            }
        }

        FastMemoryAllocator(const FastMemoryAllocator& other) = delete;
        FastMemoryAllocator(FastMemoryAllocator&& other) noexcept = delete;
        FastMemoryAllocator& operator=(const FastMemoryAllocator& other) = delete;
        FastMemoryAllocator& operator=(FastMemoryAllocator&& other) noexcept = delete;

        void* Allocate(const uint32_t size)
        {
            assert(size > 0);
            
            auto blockIndex = m_curBlockIndex;
            auto blockSize = m_curBlockSize;
            while (true)
            {
                if (blockIndex >= m_memoryBlocks.size())
                {
                    assert(false && "Out of memory");
                    return nullptr;
                }
                
                uint32_t blockCapacity = 1u << blockIndex;
                
                if (blockSize + size <= blockCapacity)
                {
                    if (!m_memoryBlocks[blockIndex])
                    {
                        m_memoryBlocks[blockIndex] = new uint8_t[blockCapacity];
                    }
                    
                    auto ptr = m_memoryBlocks[blockIndex] + blockSize;
                    m_curBlockIndex = blockIndex;
                    m_curBlockSize = blockSize + size;
                    return ptr;
                }

                blockIndex++;
                blockSize = 0;
            }
        }

        template <typename T, typename... Args>
        T* Allocate(Args&&... args)
        {
            static_assert(std::is_trivially_constructible_v<T>, "T must be trivially constructible");

            return new (Allocate(sizeof(T)))T(std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        sp<T> AllocateShared(Args&&... args)
        {
            auto ptr = new (Allocate(sizeof(T)))T(std::forward<Args>(args)...);
            return sp<T>(ptr, [](T* t)
            {
                t->~T();
            });
        }
        
    private:
        uint32_t m_curBlockIndex;
        uint32_t m_curBlockSize;
        vec<uint8_t*> m_memoryBlocks;
    };
}