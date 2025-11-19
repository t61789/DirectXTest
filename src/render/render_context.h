#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "common/utils.h"
#include "common/math.h"

namespace dt
{
    class RenderTargetPool;
    class RenderTarget;
    class RenderTexture;
    class Scene;
    class Object;
    class CameraComp;
    class RenderComp;
    class LightComp;

    struct ViewProjInfo
    {
        XMFLOAT4X4 vMatrix;
        XMFLOAT4X4 pMatrix;
        XMFLOAT4X4 vpMatrix;
        XMFLOAT4 viewCenter;
        std::optional<std::array<XMVECTOR, 6>> frustumPlanes = std::nullopt;
        std::optional<XMFLOAT4X4> ivpMatrix;

        void UpdateIVP();

        static sp<ViewProjInfo> Create(cr<XMMATRIX> vMatrix, cr<XMMATRIX> pMatrix, bool useIVP = false);
        static sp<ViewProjInfo> Create(cr<XMMATRIX> vMatrix, cr<XMMATRIX> pMatrix, cr<XMVECTOR> viewCenter, bool useIVP = false);
    };

    class RenderContext : public Singleton<RenderContext>
    {
    public:
        uint32_t screenWidth = 0;
        uint32_t screenHeight = 0;

        uint32_t mainLightShadowSize = 0;
        float mainLightShadowRange = 80;
        
        LightComp* mainLight = nullptr;

        vecwp<RenderTexture> gBufferTextures;
        wp<RenderTexture> depthBufferTex;
        wp<RenderTexture> shadingBufferTex;
        
        CameraComp* camera = nullptr;
        Scene* scene = nullptr;
        RenderTargetPool* renderTargetPool = nullptr;

        const vecwp<Object>* allSceneObjs;
        
        const vecwp<LightComp>* lights;
        const vecwp<CameraComp>* cameras;
        const vecwp<RenderComp>* allRenderObjs;
        vec<RenderComp*> visibleRenderObjs;

        sp<ViewProjInfo> mainVPInfo = nullptr;
        sp<ViewProjInfo> shadowVPInfo = nullptr;

        RenderContext() = default;
        ~RenderContext() = default;
        RenderContext(const RenderContext& other) = delete;
        RenderContext(RenderContext&& other) noexcept = delete;
        RenderContext& operator=(const RenderContext& other) = delete;
        RenderContext& operator=(RenderContext&& other) noexcept = delete;

        void PushViewProjMatrix(crsp<ViewProjInfo> viewProjInfo);
        void PopViewProjMatrix();
        UsingObject UsingViewProjMatrix(crsp<ViewProjInfo> viewProjInfo);
        crsp<ViewProjInfo> CurViewProjMatrix() const;

    private:
        std::unordered_map<std::string, RenderTexture*> m_rts;
        vecsp<ViewProjInfo> m_vpMatrixStack;

        static void SetViewProjMatrix(crsp<ViewProjInfo> viewProjInfo);
    };

    static RenderContext* RC()
    {
        return RenderContext::Ins();
    }
}

