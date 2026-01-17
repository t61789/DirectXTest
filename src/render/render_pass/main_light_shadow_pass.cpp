#include "main_light_shadow_pass.h"

#include "common/material.h"
#include "common/render_texture.h"
#include "objects/camera_comp.h"
#include "render/cbuffer.h"
#include "render/dx_helper.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"
#include "render/render_thread.h"
#include "render/batch_rendering/batch_renderer.h"

namespace dt
{
    MainLightShadowPass::MainLightShadowPass()
    {
        RenderTextureDesc desc;
        desc.dxDesc.type = TextureType::TEXTURE_2D;
        desc.dxDesc.format = TextureFormat::SHADOW_MAP;
        desc.dxDesc.wrapMode = TextureWrapMode::CLAMP;
        desc.dxDesc.filterMode = TextureFilterMode::NEAREST;
        desc.dxDesc.width = 4096;
        desc.dxDesc.height = desc.dxDesc.width;
        desc.dxDesc.channelCount = 1;
        desc.dxDesc.hasMipmap = false;
        desc.clearColor = { 1.0f, 0.0f, 0.0f, 0.0f};
        m_shadowmapRt = msp<RenderTexture>(desc);
        m_shadowmapRenderTarget = RenderTarget::Create(nullptr, m_shadowmapRt);

        m_drawShadowMtl = Material::CreateFromShader("shaders/draw_shadow.shader");

        m_shadowViewCbuffer = msp<Cbuffer>(GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER)->GetLayout());
    }

    void MainLightShadowPass::PrepareContext(RenderResources* context)
    {
        context->shadowmapRt = m_shadowmapRt;
    }

    void MainLightShadowPass::ExecuteMainThread()
    {
        auto shadowVp = CameraComp::GetMainCamera()->CreateShadowVPMatrix(
            RenderRes()->mainLightDir,
            static_cast<float>(RenderRes()->screenSize.x) / static_cast<float>(RenderRes()->screenSize.y),
            20,
            m_shadowmapRt->GetSize().x);

        shadowVp->WriteToCbuffer(m_shadowViewCbuffer.get());

        auto globalCbuffer = GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER);
        globalCbuffer->Write(MAIN_LIGHT_SHADOW_VP, Transpose(shadowVp->vpMatrix));
        globalCbuffer->Write(MAIN_LIGHT_SHADOW_TEX, m_shadowmapRt->GetTextureIndex());
        
        BatchRenderer::Ins()->GetShadowRenderGroup()->EncodeCmd();
    }

    func<void(ID3D12GraphicsCommandList*)> MainLightShadowPass::ExecuteRenderThread()
    {
        return BatchRenderer::Ins()->GetCommonRenderGroup()->CreateCmd(m_shadowViewCbuffer, m_shadowmapRenderTarget);
        // return [this](ID3D12GraphicsCommandList* cmdList)
        // {
        //     ZoneScopedN("Render Scene Pass");
        //
        //     DxHelper::RenderScene(
        //         cmdList,
        //         RenderRes()->renderObjects,
        //         m_shadowViewCbuffer,
        //         m_shadowmapRenderTarget,
        //         m_drawShadowMtl);
        // };
    }
}
