#pragma once
#include <atomic>
#include <d3d12.h>
#include <mutex>
#include <thread>
#include <wrl/client.h>

#include "common/utils.h"
#include "game/game_resource.h"
#include "utils/consumer_thread.h"

namespace dt
{
    class RenderTarget;
    class DxResource;
    using namespace Microsoft::WRL;

    class RenderThread : public Singleton<RenderThread>
    {
    public:
        using RenderCmd = std::function<void(ID3D12GraphicsCommandList*)>;

        explicit RenderThread(const char* name = nullptr);
        ~RenderThread();
        RenderThread(const RenderThread& other) = delete;
        RenderThread(RenderThread&& other) noexcept = delete;
        RenderThread& operator=(const RenderThread& other) = delete;
        RenderThread& operator=(RenderThread&& other) noexcept = delete;

        ID3D12CommandList* GetCmdList() { return m_cmdList.Get(); }

        void Wait() { m_thread->Wait(); }

        void AddCmd(RenderCmd&& cmd);

        void ExecuteCmds();

        void ResetCmdList();
        void ReleaseCmdResources();

    private:
        void ThreadInit();
        void ThreadRelease();

        ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_cmdList;

        up<ConsumerThread<RenderCmd>> m_thread;
        vec<RenderCmd> m_pendingCmds;
        vec<RenderCmd> m_cmds;

        bool m_first = true;
    };

    static RenderThread* RT()
    {
        return RenderThread::Ins();
    }
}
