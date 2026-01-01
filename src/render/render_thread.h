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

    struct RenderThreadContext
    {
        sp<RenderTarget> curRenderTarget = nullptr;
        vecpair<sp<DxResource>, D3D12_RESOURCE_STATES> transitions;
    };
    
    class RenderThread
    {
    public:
        using RenderCmd = std::function<void(ID3D12GraphicsCommandList*, RenderThreadContext&)>;

        explicit RenderThread(const char* name = nullptr);
        ~RenderThread();
        RenderThread(const RenderThread& other) = delete;
        RenderThread(RenderThread&& other) noexcept = delete;
        RenderThread& operator=(const RenderThread& other) = delete;
        RenderThread& operator=(RenderThread&& other) noexcept = delete;

        void Wait() { m_thread->Wait(); }

        void AddCmd(RenderCmd&& cmd);

        void StopRecording();
        void StartRecording();

        void ReleaseCmdResources();

    private:
        void ThreadInit();
        void ThreadRelease();
        void WaitForFence();

        ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_cmdList;

        up<ConsumerThread<RenderCmd>> m_thread;
        vec<RenderCmd> m_cmds;
        vec<RenderCmd> m_pendingCmds;

        std::atomic<bool> m_recording = false;

        RenderThreadContext m_context;

        bool m_first = true;
        
        ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent;
        uint64_t m_fenceValue = 0;
    };

    class RenderThreadMgr : public Singleton<RenderThreadMgr>
    {
        using PresentThread = ConsumerThread<std::function<void()>>;
        
    public:
        RenderThreadMgr();
        ~RenderThreadMgr();
        RenderThreadMgr(const RenderThreadMgr& other) = delete;
        RenderThreadMgr(RenderThreadMgr&& other) noexcept = delete;
        RenderThreadMgr& operator=(const RenderThreadMgr& other) = delete;
        RenderThreadMgr& operator=(RenderThreadMgr&& other) noexcept = delete;

        RenderThread* GetRenderThread() const { return m_renderThread; }

        void ExecuteAllThreads();
        void WaitForDone();

    private:
        bool m_executing = true;
        
        RenderThread* m_renderThread;
        // PresentThread* m_presentThread;
    };

    static RenderThread* RT()
    {
        return RenderThreadMgr::Ins()->GetRenderThread();
    }
}
