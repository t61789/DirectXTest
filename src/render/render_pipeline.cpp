#include "render_pipeline.h"

#include <d3d12.h>

#include "directx.h"
#include "dx_helper.h"
#include "render_context.h"
#include "render_thread.h"
#include "window.h"
#include "common/material.h"
#include "common/mesh.h"
#include "game/game_resource.h"
#include "objects/scene.h"
#include "render/render_resources.h"
#include "render_pass/final_pass.h"
#include "render_pass/prepare_pass.h"

namespace dt
{
    RenderPipeline::RenderPipeline()
    {
        m_preparePass = msp<PreparePass>();
        m_finalPass = msp<FinalPass>();
        m_renderResources = msp<RenderResources>();
    }

    RenderPipeline::~RenderPipeline()
    {
        m_renderResources.reset();
        m_finalPass.reset();
        m_preparePass.reset();
    }

    void RenderPipeline::Render()
    {
        ZoneScopedN("Render");
        
        m_preparePass->Execute();
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
