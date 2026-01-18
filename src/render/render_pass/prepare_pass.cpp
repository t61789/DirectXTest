#include "prepare_pass.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include "window.h"
#include "game/game_resource.h"
#include "gui/gui.h"
#include "objects/camera_comp.h"
#include "objects/light_comp.h"
#include "objects/scene.h"
#include "objects/transform_comp.h"
#include "render/cbuffer.h"
#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/indirect_lighting.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"
#include "render/render_thread.h"
#include "render/batch_rendering/batch_renderer.h"

namespace dt
{
    PreparePass::PreparePass()
    {
        m_mainCameraViewCbuffer = msp<Cbuffer>(GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER)->GetLayout());
    }

    void PreparePass::PrepareContext(RenderResources* context)
    {
        RenderRes()->screenSize = { Window::Ins()->GetWidth(), Window::Ins()->GetHeight() };
        auto aspect = static_cast<float>(RenderRes()->screenSize.x) / static_cast<float>(RenderRes()->screenSize.y);
        RenderRes()->mainCameraVp = CameraComp::GetMainCamera()->CreateVPMatrix(aspect);
        RenderRes()->mainCameraViewCbuffer = m_mainCameraViewCbuffer;
        RenderRes()->mainCameraVp->WriteToCbuffer(RenderRes()->mainCameraViewCbuffer.get());
        RenderRes()->mainCameraVp->WriteToCbuffer(GR()->GetPredefinedCbuffer(PER_VIEW_CBUFFER).get());
        RenderRes()->renderObjects = GR()->mainScene->GetRenderTree()->GetRenderObjects();
        GetGlobalCbuffer()->Write(SKYBOX_TEX, GR()->skyboxTex->GetTextureIndex());

        PrepareLights();
    }

    void PreparePass::ExecuteMainThread()
    {
        TransformComp::UpdateAllDirtyComps();

        RecycleBin::Ins()->Flush();

        Gui::Ins()->Render();
        
        BatchRenderer::Ins()->RegisterActually();
        BatchRenderer::Ins()->UpdateMatrixActually();
    }

    func<void(ID3D12GraphicsCommandList*)> PreparePass::ExecuteRenderThread()
    {
        return [](ID3D12GraphicsCommandList* cmdList)
        {
            ZoneScopedN("Prepare Pass");

            ImGui::SetCurrentContext(Gui::Ins()->GetMainThreadContext());

            DxHelper::PrepareCmdList(cmdList);
        };
    }

    void PreparePass::PrepareLights()
    {
        ZoneScoped;

        XMFLOAT3 lightDir = Store3(XMVector3Normalize(XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f)));
        XMFLOAT3 lightColor = { 0.0f, 0.0f, 0.0f };

        auto& lightComps = GR()->mainScene->GetRegistry()->GetCompStorage()->GetComps<LightComp>();
        
        auto mainLightCompWp = find_if(lightComps, [](crwp<LightComp> comp)
        {
            return comp.lock() && comp.lock()->lightType == 0;
        });
        if (mainLightCompWp)
        {
            auto lightComp = mainLightCompWp->lock();
            lightDir = Store3(-GetForward(lightComp->GetOwner()->transform->GetLocalToWorld()));
            lightColor = lightComp->GetColor();
        }

        uint32_t pointLightCount = 0;
        vec<XMFLOAT4> pointLightInfos(MAX_POINT_LIGHT_COUNT);
        for (auto& compWp : lightComps)
        {
            if (!compWp.lock() || compWp.lock()->lightType != 1)
            {
                continue;
            }
            
            auto pointLightComp = compWp.lock();
            
            auto positionWS = Store3(pointLightComp->GetOwner()->transform->GetWorldPosition());
            auto radius = pointLightComp->radius;
            auto color = pointLightComp->GetColor();

            constexpr uint32_t pointLightStrideVec4 = 2;
            pointLightInfos[pointLightCount * pointLightStrideVec4 + 0] = { positionWS.x, positionWS.y, positionWS.z, radius };
            pointLightInfos[pointLightCount * pointLightStrideVec4 + 1] = { color.x, color.y, color.z, 0.0f };

            pointLightCount++;
        }

        RenderRes()->mainLightDir = lightDir;

        GetGlobalCbuffer()->Write(MAIN_LIGHT_DIR, lightDir);
        GetGlobalCbuffer()->Write(MAIN_LIGHT_COLOR, lightColor);
        GetGlobalCbuffer()->Write(POINT_LIGHT_COUNT, pointLightCount);
        GetGlobalCbuffer()->Write(POINT_LIGHT_INFOS, pointLightInfos.data(), pointLightInfos.size() * sizeof(XMFLOAT4));

        IndirectLighting::SetGradientAmbientColor(
            GR()->mainScene->ambientLightColorSky,
            GR()->mainScene->ambientLightColorEquator,
            GR()->mainScene->ambientLightColorGround);
    }
}
