#pragma once

#include "common/string_handle.h"
#include "common/utils.h"

namespace dt
{
    class Cbuffer;
    class IResource;

    class GameResource : public Singleton<GameResource>
    {
    public:
        uint64_t GetFrameCount() const { return m_frameCount; }
        double GetTime() const { return m_time; }
        double GetDeltaTime() const { return m_deltaTime; }
        void GetScreenSize(uint32_t& width, uint32_t& height) const { width = m_screenWidth; height = m_screenHeight; }
        
        void RegisterResource(cr<StringHandle> path, crsp<IResource> resource);
        void UnregisterResource(cr<StringHandle> path);
        sp<IResource> GetResource(cr<StringHandle> path);
        template <typename T>
        sp<T> GetResource(cr<StringHandle> path);

    private:
        uint64_t m_frameCount;
        double m_time;
        double m_deltaTime;

        uint32_t m_screenWidth;
        uint32_t m_screenHeight;

        umap<string_hash, wp<IResource>> m_resources;
        vecsp<Cbuffer> m_predefinedCbuffers = vecsp<Cbuffer>(PREDEFINED_CBUFFER.size());

        friend class Game;
        friend class Cbuffer;
        friend class Shader;
    };

    template <typename T>
    sp<T> GameResource::GetResource(cr<StringHandle> path)
    {
        auto result = GetResource(path);
        if (!result)
        {
            return nullptr;
        }

        auto result0 = std::dynamic_pointer_cast<T>(result);
        if (!result0)
        {
            THROW_ERRORF("Resource type miss match: %s", path.CStr());
        }

        return result0;
    }

    static GameResource* GR() { return GameResource::Ins(); }
}
