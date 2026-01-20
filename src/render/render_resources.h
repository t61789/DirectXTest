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

        bool hasOddNegativeScale;
        XMFLOAT4X4 localToWorld;
        XMFLOAT4X4 worldToLocal;
    };
    
    struct RenderResources
    {
        RenderResources() = default;

        // Render Resources
        XMUINT2 screenSize = { 0, 0 };
        sp<RenderTexture> gBufferRt1 = nullptr;
        sp<RenderTarget> gBufferRenderTarget = nullptr;
        sp<ViewProjInfo> mainCameraVp = nullptr;
        sp<Cbuffer> mainCameraViewCbuffer = nullptr;
        vecsp<RenderObject> renderObjects;
        sp<RenderTexture> litResultRt = nullptr;
        sp<RenderTexture> shadowmapRt = nullptr;
        XMFLOAT3 mainLightDir = { 1.0f, 1.0f, 1.0f };

        // Render States
        sp<ViewProjInfo> curVp = nullptr;
        sp<RenderTarget> curRenderTarget = nullptr;
        vecpair<sp<DxResource>, D3D12_RESOURCE_STATES> transitions;
    };
}
