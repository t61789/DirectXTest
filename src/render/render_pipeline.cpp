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
        m_gBufferRt0 = msp<RenderTexture>(gBufferColorDesc);
        m_gBufferRt1 = msp<RenderTexture>(gBufferColorDesc);
        m_gBufferRt2 = msp<RenderTexture>(gBufferColorDesc);

        RenderTextureDesc gBufferDepthDesc;
        gBufferDepthDesc.dxDesc.type = TextureType::TEXTURE_2D;
        gBufferDepthDesc.dxDesc.format = TextureFormat::DEPTH_STENCIL;
        gBufferDepthDesc.dxDesc.wrapMode = TextureWrapMode::CLAMP;
        gBufferDepthDesc.dxDesc.filterMode = TextureFilterMode::NEAREST;
        gBufferDepthDesc.dxDesc.width = Window::Ins()->GetWidth();
        gBufferDepthDesc.dxDesc.height = Window::Ins()->GetHeight();
        gBufferDepthDesc.dxDesc.channelCount = 1;
        gBufferColorDesc.dxDesc.hasMipmap = false;
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

    void RenderPipeline::RenderScene(ID3D12GraphicsCommandList* cmdList, const Scene* scene)
    {
        // Shader* shader = nullptr;
        // Material* material = nullptr;
        // Mesh* mesh = nullptr;
        //
        // scene->GetRenderTree()->Foreach([&](const RenderObject* ro)
        // {
        //     auto rebindRootSignature = ro->shader != shader;
        //     auto rebindPso = ro->material != material;
        //     auto rebindGlobalCbuffer = rebindRootSignature;
        //     auto rebindPerViewCbuffer = rebindRootSignature;
        //     auto rebindPerMaterialCbuffer = rebindRootSignature || ro->material != material;
        //     auto rebindMesh = ro->mesh != mesh;
        //     
        //     if (rebindRootSignature)
        //     {
        //         DxHelper::BindRootSignature(cmdList, ro->material->GetShader());
        //     }
        //
        //     if (rebindPso)
        //     {
        //         DxHelper::BindPso(cmdList, ro->material);
        //     }
        //
        //     if (rebindGlobalCbuffer)
        //     {
        //         DxHelper::BindCbuffer(cmdList, ro->shader, GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get());
        //     }
        //
        //     if (rebindPerViewCbuffer)
        //     {
        //         DxHelper::BindCbuffer(cmdList, ro->shader, GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER).get());
        //     }
        //
        //     DxHelper::BindCbuffer(cmdList, ro->shader, ro->perObjectCbuffer.get());
        //
        //     if (rebindPerMaterialCbuffer)
        //     {
        //         if (ro->material->GetCbuffer())
        //         {
        //             DxHelper::BindCbuffer(cmdList, ro->shader, ro->material->GetCbuffer());
        //         }
        //     }
        //
        //     if (rebindMesh)
        //     {
        //         DxHelper::BindMesh(cmdList, ro->mesh);
        //     }
        //     
        //     cmdList->DrawIndexedInstanced(ro->mesh->GetIndicesCount(), 1, 0, 0, 0);
        //
        //     shader = ro->shader;
        //     material = ro->material;
        //     mesh = ro->mesh;
        // });
    }
}
