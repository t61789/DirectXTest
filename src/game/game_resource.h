#pragma once

#include "common/string_handle.h"
#include "common/utils.h"

namespace dt
{
    class Mesh;
    class Material;
    class Image;
    class Scene;
    class Cbuffer;
    class IResource;

    class GameResource : public Singleton<GameResource>
    {
    public:
        GameResource();
        ~GameResource();
        GameResource(const GameResource& other) = delete;
        GameResource(GameResource&& other) noexcept = delete;
        GameResource& operator=(const GameResource& other) = delete;
        GameResource& operator=(GameResource&& other) noexcept = delete;

        uint64_t GetFrameCount() const { return m_frameCount; }
        double GetTime() const { return m_time; }
        double GetDeltaTime() const { return m_deltaTime; }
        void GetScreenSize(uint32_t& width, uint32_t& height) const { width = m_screenWidth; height = m_screenHeight; }
        sp<Cbuffer> GetPredefinedCbuffer(cr<StringHandle> name) const;
        
        void RegisterResource(cr<StringHandle> path, crsp<IResource> resource);
        void UnregisterResource(cr<StringHandle> path);
        sp<IResource> GetResource(cr<StringHandle> path);
        template <typename T>
        sp<T> GetResource(cr<StringHandle> path);

        Scene* mainScene = nullptr;

        sp<Material> testMat = nullptr;
        sp<Material> blitMat = nullptr;
        sp<Mesh> quadMesh = nullptr;
        sp<Mesh> sphereMesh = nullptr;

        sp<Image> errorTex = nullptr;
        sp<Image> skyboxTex = nullptr;

    private:
        uint64_t m_frameCount = 0;
        double m_time = 0;
        double m_deltaTime = 0;

        uint32_t m_screenWidth = 0;
        uint32_t m_screenHeight = 0;

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

    static sp<Cbuffer> GetGlobalCbuffer() { return GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER); }

    static sp<Cbuffer> GetPerViewCbuffer() { return GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER); }

    static sp<Cbuffer> GetPerObjectCbuffer() { return GR()->GetPredefinedCbuffer(PER_OBJECT_CBUFFER); }
}
