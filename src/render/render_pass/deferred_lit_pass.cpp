#include "deferred_lit_pass.h"

#include "window.h"
#include "common/render_texture.h"
#include "game/game_resource.h"
#include "objects/scene.h"
#include "render/cbuffer.h"
#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/render_thread.h"

namespace dt
{
    DeferredLitPass::DeferredLitPass()
    {
        m_litMaterial = Material::LoadFromFile("materials/deferred_lit.mtl");

        RenderTextureDesc rtDesc;
        rtDesc.dxDesc.type = TextureType::TEXTURE_2D;
        rtDesc.dxDesc.format = TextureFormat::RGBA;
        rtDesc.dxDesc.wrapMode = TextureWrapMode::CLAMP;
        rtDesc.dxDesc.filterMode = TextureFilterMode::BILINEAR;
        rtDesc.dxDesc.width = Window::Ins()->GetWidth();
        rtDesc.dxDesc.height = Window::Ins()->GetHeight();
        rtDesc.dxDesc.channelCount = 4;
        rtDesc.dxDesc.hasMipmap = false;
        rtDesc.dxDesc.name = L"Lit Result RT";
        rtDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_litResultRt = msp<RenderTexture>(rtDesc);
        m_litResultRenderTarget = RenderTarget::Create(m_litResultRt, nullptr);
        
        RenderTextureDesc gBufferColorDesc;
        gBufferColorDesc.dxDesc.type = TextureType::TEXTURE_2D;
        gBufferColorDesc.dxDesc.format = TextureFormat::RGBA;
        gBufferColorDesc.dxDesc.wrapMode = TextureWrapMode::CLAMP;
        gBufferColorDesc.dxDesc.filterMode = TextureFilterMode::NEAREST;
        gBufferColorDesc.dxDesc.width = Window::Ins()->GetWidth();
        gBufferColorDesc.dxDesc.height = Window::Ins()->GetHeight();
        gBufferColorDesc.dxDesc.channelCount = 4;
        gBufferColorDesc.dxDesc.hasMipmap = false;
        gBufferColorDesc.dxDesc.name = L"GBuffer Color0 RT";
        gBufferColorDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_gBufferRt0 = msp<RenderTexture>(gBufferColorDesc);
        gBufferColorDesc.dxDesc.name = L"GBuffer Color1 RT";
        gBufferColorDesc.dxDesc.format = TextureFormat::RGBA16;
        m_gBufferRt1 = msp<RenderTexture>(gBufferColorDesc);
        gBufferColorDesc.dxDesc.format = TextureFormat::R32;
        gBufferColorDesc.dxDesc.channelCount = 1;
        gBufferColorDesc.dxDesc.name = L"GBuffer Color2 RT";
        gBufferColorDesc.clearColor = { 1.0f, 0.0f, 0.0f, 0.0f };
        m_gBufferRt2 = msp<RenderTexture>(gBufferColorDesc);

        GetGlobalCbuffer()->Write(GBUFFER_0_TEX, m_gBufferRt0->GetTextureIndex());
        GetGlobalCbuffer()->Write(GBUFFER_1_TEX, m_gBufferRt1->GetTextureIndex());
        GetGlobalCbuffer()->Write(GBUFFER_2_TEX, m_gBufferRt2->GetTextureIndex());

        RenderTextureDesc gBufferDepthDesc;
        gBufferDepthDesc.dxDesc.type = TextureType::TEXTURE_2D;
        gBufferDepthDesc.dxDesc.format = TextureFormat::DEPTH_STENCIL;
        gBufferDepthDesc.dxDesc.wrapMode = TextureWrapMode::CLAMP;
        gBufferDepthDesc.dxDesc.filterMode = TextureFilterMode::NEAREST;
        gBufferDepthDesc.dxDesc.width = Window::Ins()->GetWidth();
        gBufferDepthDesc.dxDesc.height = Window::Ins()->GetHeight();
        gBufferDepthDesc.dxDesc.channelCount = 1;
        gBufferDepthDesc.dxDesc.hasMipmap = false;
        gBufferDepthDesc.dxDesc.name = L"GBuffer Depth RT";
        gBufferDepthDesc.clearColor = { 1.0f, 0.0f, 0.0f, 0.0f };
        m_gBufferRtDepth = msp<RenderTexture>(gBufferDepthDesc);

        m_gBufferRenderTarget = RenderTarget::Create({ m_gBufferRt0, m_gBufferRt1, m_gBufferRt2 }, m_gBufferRtDepth);
    }

    void DeferredLitPass::PrepareContext(RenderResources* context)
    {
        context->litResultRt = m_litResultRt;
        context->gBufferRt1 = m_gBufferRt1;
        context->gBufferRenderTarget = m_gBufferRenderTarget;
    }

    func<void(ID3D12GraphicsCommandList*)> DeferredLitPass::ExecuteRenderThread()
    {
        return [this](ID3D12GraphicsCommandList* cmdList)
        {
            ZoneScopedN("Deferred Lit Pass");

            DxHelper::Blit(cmdList, m_litMaterial.get(), m_litResultRenderTarget);
        };
    }
}
