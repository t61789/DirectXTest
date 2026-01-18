#pragma once

#include "common/utils.h"
#include "game/game_resource.h"

namespace dt
{
    template<typename K, typename V>
    class TimeOutBuffer
    {
    public:
        explicit TimeOutBuffer(V defaultVal) : m_defaultVal(defaultVal)
        {
            
        }
        
        V GetOrAdd(const K& key)
        {
            auto curFrame = GR()->GetFrameCount();
            
            auto it = m_map.find(key);
            if (it != m_map.end())
            {
                it->second.usedTime = curFrame;
                
                ClearTimeOut();
                return it->second.value;
            }
            
            Elem value;
            value.value = m_defaultVal;
            value.usedTime = curFrame;
            m_map[key] = value;

            ClearTimeOut();
            return value.value;
        }
        
        void Set(K key, V value)
        {
            auto curFrame = GR()->GetFrameCount();
            
            Elem elem;
            elem.value = value;
            elem.usedTime = curFrame;
            m_map[key] = elem;
            
            ClearTimeOut();
        }
        
    private:
        class Elem
        {
        public:
            V value;
            int usedTime = 0;
        };

        V m_defaultVal;
        int m_prevClearFrame = 0;
        std::unordered_map<K, Elem> m_map;

        void ClearTimeOut()
        {
            auto curFrame = GR()->GetFrameCount();
            if (curFrame - m_prevClearFrame < 10)
            {
                return;
            }
            m_prevClearFrame = curFrame;

            for (auto it = m_map.begin(); it != m_map.end();)
            {
                if (curFrame - it->second.usedTime > 10)
                {
                    it = m_map.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    };
}
