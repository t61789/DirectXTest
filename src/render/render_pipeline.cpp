#include "render_pipeline.h"

#include <d3d12.h>

#include "cbuffer.h"
#include "directx.h"
#include "dx_helper.h"
#include "render_context.h"
#include "render_thread.h"
#include "window.h"
#include "common/material.h"
#include "common/mesh.h"
#include "common/render_texture.h"
#include "game/game_resource.h"
#include "objects/scene.h"
#include "render/render_resources.h"
#include "render_pass/final_pass.h"
#include "render_pass/prepare_pass.h"
#include "render_pass/render_scene_pass.h"

namespace dt
{
    RenderPipeline::RenderPipeline()
    {
        m_preparePass = msp<PreparePass>();
        m_finalPass = msp<FinalPass>();
        m_renderScenePass = msp<RenderScenePass>();
        m_renderResources = msp<RenderResources>();

        RenderTextureDesc gBufferColorDesc;
        gBufferColorDesc.dxDesc.type = TextureType::TEXTURE_2D;
        gBufferColorDesc.dxDesc.format = TextureFormat::RGBA;
        gBufferColorDesc.dxDesc.wrapMode = TextureWrapMode::CLAMP;
        gBufferColorDesc.dxDesc.filterMode = TextureFilterMode::BILINEAR;
        gBufferColorDesc.dxDesc.width = Window::Ins()->GetWidth();
        gBufferColorDesc.dxDesc.height = Window::Ins()->GetHeight();
        gBufferColorDesc.dxDesc.channelCount = 4;
        gBufferColorDesc.dxDesc.hasMipmap = false;
        gBufferColorDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_gBufferRt0 = msp<RenderTexture>(gBufferColorDesc);
        m_gBufferRt1 = msp<RenderTexture>(gBufferColorDesc);
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
        gBufferDepthDesc.clearColor = { 1.0f, 0.0f, 0.0f, 0.0f };
        m_gBufferRtDepth = msp<RenderTexture>(gBufferDepthDesc);

        m_gBufferRenderTarget = RenderTarget::Create({ m_gBufferRt0, m_gBufferRt1, m_gBufferRt2 }, m_gBufferRtDepth);
    }

    RenderPipeline::~RenderPipeline()
    {
        m_gBufferRenderTarget.reset();
        
        m_gBufferRt0.reset();
        m_gBufferRt1.reset();
        m_gBufferRt2.reset();
        m_gBufferRtDepth.reset();
        
        m_renderResources.reset();
        m_finalPass.reset();
        m_renderScenePass.reset();
        m_preparePass.reset();
    }

    void RenderPipeline::Render()
    {
        ZoneScopedN("Render");
        
        m_preparePass->Execute();
        m_renderScenePass->Execute();
        m_finalPass->Execute();
    }
}
