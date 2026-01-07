#pragma once
#include "common/utils.h"
#include "game/game_resource.h"

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
        void Add(IRecyclable* garbage, uint32_t frameCount);
        void Flush();

    private:
        std::mutex m_mutex;
        vec<IRecyclable*> m_pending;
        umap<uint32_t, vec<IRecyclable*>> m_garbage;
    };

    inline void RecycleBin::Add(IRecyclable* garbage, const uint32_t frameCount)
    {
        std::lock_guard lock(m_mutex);
        m_garbage[frameCount].push_back(garbage);
    }

    inline void RecycleBin::Flush()
    {
        std::lock_guard lock(m_mutex);

        auto curFrame = GR() ? GR()->GetFrameCount() : ~0u;
        if (curFrame <= 1) // 在程序开始到Game::Update之间的资源是第0帧的，会在第1帧的Render中使用，而第1帧会清除第0帧的资源，所以跳过这次清理
        {
            return;
        }

        vec<decltype(m_garbage)::iterator> eraseGarbage;
        for (auto it = m_garbage.begin(); it != m_garbage.end(); ++it)
        {
            auto [frameCount, garbage] = *it;
            if (frameCount < curFrame)
            {
                for (auto ptr : garbage)
                {
                    delete ptr;
                }

                eraseGarbage.push_back(it);
            }
        }

        for (auto it : eraseGarbage)
        {
            m_garbage.erase(it);
        }
    }

    template<typename T, typename... Args>
    static std::shared_ptr<T> make_recyclable(Args&&... args)
    {
        assert(Utils::IsMainThread());
        
        T* rawPtr = new T(std::forward<Args>(args)...);

        auto curFrame = GR() ? GR()->GetFrameCount() : 0;
        return std::shared_ptr<T>(rawPtr, [curFrame](T* ptr)
        {
            RecycleBin::Ins()->Add(static_cast<IRecyclable*>(ptr), curFrame);
        });
    }
}
