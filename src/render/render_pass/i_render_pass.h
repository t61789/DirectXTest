#pragma once

#include <d3d12.h>

#include "common/const.h"

namespace dt
{
    struct RenderResources;
    
    class IRenderPass
    {
    public:
        IRenderPass() = default;
        virtual ~IRenderPass() = default;
        IRenderPass(const IRenderPass& other) = delete;
        IRenderPass(IRenderPass&& other) noexcept = delete;
        IRenderPass& operator=(const IRenderPass& other) = delete;
        IRenderPass& operator=(IRenderPass&& other) noexcept = delete;
        
        virtual const char* GetName() = 0;
        virtual void PrepareContext(RenderResources* context) {}
        virtual void ExecuteMainThread() {}
        virtual func<void(ID3D12GraphicsCommandList*)> ExecuteRenderThread() { return nullptr; }
    };
}
