#include "render_pipeline.h"

#include <d3d12.h>
#include <pix.h>

#include "cbuffer.h"
#include "directx.h"
#include "dx_buffer.h"
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
#include "render_pass/deferred_lit_pass.h"
#include "render_pass/final_pass.h"
#include "render_pass/main_light_shadow_pass.h"
#include "render_pass/prepare_pass.h"
#include "render_pass/render_scene_pass.h"
#include "render/render_thread.h"

namespace dt
{
    RenderPipeline::RenderPipeline()
    {
        m_passes.push_back(msp<PreparePass>());
        m_passes.push_back(msp<MainLightShadowPass>());
        m_passes.push_back(msp<RenderScenePass>());
        m_passes.push_back(msp<DeferredLitPass>());
        m_passes.push_back(msp<FinalPass>());
        
        m_renderResources = msp<RenderResources>();
    }

    RenderPipeline::~RenderPipeline()
    {
        m_renderResources.reset();
        
        m_passes.clear();
    }

    void RenderPipeline::Render()
    {
        ZoneScopedN("Render");

        RenderThread::Ins()->Wait();
        RenderThread::Ins()->ReleaseCmdResources();

        DxResource::ClearUploadBuffers();
        
        *RenderRes() = RenderResources();

        {
            ZoneScopedN("Prepare Context");
            for (auto& pass : m_passes)
            {
                ZoneScopedN("Pass");
                ZoneText(pass->GetName(), strlen(pass->GetName()));
                
                pass->PrepareContext(RenderRes());
            }
        }

        {
            ZoneScopedN("Execute Pass");
            for (auto& pass : m_passes)
            {
                ZoneText(pass->GetName(), strlen(pass->GetName()));
                
                pass->ExecuteMainThread();
            }
        }

        {
            ZoneScopedN("Submit Dirty Data");
            DxBuffer::SubmitDirtyData();
        }

        {
            ZoneScopedN("Record Cmds");
            for (auto& pass : m_passes)
            {
                ZoneScopedN("Pass");
                ZoneText(pass->GetName(), strlen(pass->GetName()));
                
                RT()->AddCmd([name=pass->GetName(), cmd=pass->ExecuteRenderThread()](ID3D12GraphicsCommandList* cmdList)
                {
                    PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, name);
                    cmd(cmdList);
                    PIXEndEvent(cmdList);
                });
            }
        }

        {
            ZoneScopedN("Dispatch Cmds");
            RenderThread::Ins()->ExecuteCmds();
        }
    }
}
