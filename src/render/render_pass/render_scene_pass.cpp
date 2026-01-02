#include "render_scene_pass.h"

#include "common/mesh.h"
#include "objects/scene.h"
#include "render/dx_helper.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"
#include "render/render_thread.h"

namespace dt
{
    void RenderScenePass::Execute()
    {
        return;
        RT()->AddCmd([](ID3D12GraphicsCommandList* cmdList)
        {
            RenderRes()->SetVp(RenderRes()->mainCameraVp);
            DxHelper::SetRenderTarget(cmdList, RenderRes()->gBufferRenderTarget);
            
            Shader* shader = nullptr;
            Material* material = nullptr;
            Mesh* mesh = nullptr;

            for (const auto& ro : RenderRes()->renderObjects)
            {
                auto rebindRootSignature = ro->shader.get() != shader;
                auto rebindPso = ro->material.get() != material;
                auto rebindGlobalCbuffer = rebindRootSignature;
                auto rebindPerViewCbuffer = rebindRootSignature;
                auto rebindPerMaterialCbuffer = rebindRootSignature || ro->material.get() != material;
                auto rebindMesh = ro->mesh.get() != mesh;
                
                shader = ro->shader.get();
                material = ro->material.get();
                mesh = ro->mesh.get();
                
                if (rebindRootSignature)
                {
                    DxHelper::BindRootSignature(cmdList, shader);
                }
            
                if (rebindPso)
                {
                    DxHelper::BindPso(cmdList, material);
                }
            
                if (rebindGlobalCbuffer)
                {
                    DxHelper::BindCbuffer(cmdList, shader, GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get());
                }
            
                if (rebindPerViewCbuffer)
                {
                    DxHelper::BindCbuffer(cmdList, shader, GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER).get());
                }
            
                DxHelper::BindCbuffer(cmdList, shader, ro->perObjectCbuffer.get());
            
                if (rebindPerMaterialCbuffer)
                {
                    if (material->GetCbuffer())
                    {
                        DxHelper::BindCbuffer(cmdList, shader, material->GetCbuffer());
                    }
                }
            
                if (rebindMesh)
                {
                    DxHelper::BindMesh(cmdList, mesh);
                }
                
                cmdList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);
            }

            DxHelper::UnsetRenderTarget(RenderRes()->gBufferRenderTarget);
        });
    }
}
