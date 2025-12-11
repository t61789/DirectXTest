#pragma once
#include <d3d12.h>

namespace dt
{
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
        virtual void Execute() = 0;
    };
}
