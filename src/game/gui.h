#pragma once
#include "render/descriptor_pool.h"

struct ImGuiContext;

namespace dt
{
    class Gui : public Singleton<Gui>
    {
    public:
        Gui();

        ImGuiContext* GetMainThreadContext() { return m_mainThreadContext; }
        ImGuiContext* GetRenderThreadContext() { return m_renderThreadContext; }

    private:
        sp<SrvPool::Handle> m_imguiSrvHandle;
        ImGuiContext* m_mainThreadContext;
        ImGuiContext* m_renderThreadContext;
    };
}
