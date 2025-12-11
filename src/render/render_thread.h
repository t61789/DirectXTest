#pragma once
#include <atomic>
#include <d3d12.h>
#include <mutex>
#include <thread>
#include <wrl/client.h>

#include "common/utils.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class RenderThread : public Singleton<RenderThread>
    {
    public:
        using RenderCmd = std::function<void(ID3D12GraphicsCommandList*)>;

        explicit RenderThread(cr<ComPtr<ID3D12Device>> device);
        ~RenderThread();
        RenderThread(const RenderThread& other) = delete;
        RenderThread(RenderThread&& other) noexcept = delete;
        RenderThread& operator=(const RenderThread& other) = delete;
        RenderThread& operator=(RenderThread&& other) noexcept = delete;

        template <typename F>
        void AddCmd(F&& f);
        void FlushCmds();
        void WaitForDone();

        void ReleaseCmdResources();

    private:
        void ThreadMain();
        void AddCmd(sp<RenderCmd>&& cmd);
        void StopThread();
        void WaitForFence();

        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_cmdList;
        
        std::mutex m_mutex;
        std::condition_variable m_execCmdCond;
        std::condition_variable m_waitForDoneCond;
        std::thread m_thread;
        std::atomic<bool> m_stop = false;
        vecsp<RenderCmd> m_cmds;
        uint32_t m_curCmdIndex = 0;
        
        ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent;
        uint64_t m_fenceValue = 0;
    };

    template <typename F>
    void RenderThread::AddCmd(F&& f)
    {
        AddCmd(msp<RenderCmd>(std::forward<F>(f)));
    }

    static RenderThread* RT() { return RenderThread::Ins(); }
}
