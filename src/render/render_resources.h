#pragma once
#include <d3d12.h>

#include "render_context.h"
#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    class DxResource;
    class RenderTarget;
    class Cbuffer;
    class RenderComp;
    class Mesh;
    class Material;
    class Shader;

    struct RenderObject
    {
        sp<Shader> shader;
        sp<Material> material;
        sp<Mesh> mesh;
        sp<Cbuffer> perObjectCbuffer;
    };
    
    struct RenderResources
    {
        RenderResources() = default;

        // Render Resources
        XMUINT2 screenSize = { 0, 0 };
        sp<RenderTexture> gBufferRt1 = nullptr;
        sp<RenderTarget> gBufferRenderTarget = nullptr;
        sp<ViewProjInfo> mainCameraVp = nullptr;
        vecsp<RenderObject> renderObjects;
        sp<RenderTexture> litResultRt = nullptr;

        // Render States
        sp<ViewProjInfo> curVp = nullptr;
        sp<RenderTarget> curRenderTarget = nullptr;
        vecpair<sp<DxResource>, D3D12_RESOURCE_STATES> transitions;

        void SetVp(crsp<ViewProjInfo> vp);
    };
}
