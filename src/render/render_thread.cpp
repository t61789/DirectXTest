#include "render_thread.h"

#include <tracy/Tracy.hpp>

#include "directx.h"

namespace dt
{
    RenderThread::RenderThread(cr<ComPtr<ID3D12Device>> device)
    {
        m_device = device;
        THROW_IF_FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, &m_fence));
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        THROW_IF(m_fenceEvent == nullptr, "Failed to create fence event.");
        
        m_thread = std::thread(&RenderThread::ThreadMain, this);
    }

    RenderThread::~RenderThread()
    {
        StopThread();

        CloseHandle(m_fenceEvent);
        m_device.Reset();
    }

    void RenderThread::FlushCmds()
    {
        AddCmd([this](ID3D12GraphicsCommandList* cmdList)
        {
            THROW_IF_FAILED(cmdList->Close());
            ID3D12CommandList* c[] = { cmdList };
            Dx()->GetCommandQueue()->ExecuteCommandLists(1, c);

            WaitForFence();

            THROW_IF_FAILED(m_cmdAllocator->Reset());
            THROW_IF_FAILED(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));

            Dx()->PresentSwapChain();
        });
    }

    void RenderThread::ReleaseCmdResources()
    {
        std::lock_guard lock(m_mutex);
        m_cmds.clear();
        m_curCmdIndex = 0;
    }

    void RenderThread::WaitForDone()
    {
        std::unique_lock lock(m_mutex);
        m_waitForDoneCond.wait(lock, [this]
        {
            return m_stop.load() || m_curCmdIndex >= m_cmds.size();
        });
    }

    void RenderThread::AddCmd(sp<RenderCmd>&& cmd)
    {
        assert(!m_stop.load());
        
        std::lock_guard lock(m_mutex);
        m_cmds.push_back(std::move(cmd));
        m_execCmdCond.notify_one();
    }

    void RenderThread::ThreadMain()
    {
        THROW_IF_FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_ID3D12CommandAllocator, &m_cmdAllocator));
        THROW_IF_FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), nullptr, IID_ID3D12GraphicsCommandList, &m_cmdList));
        
        while (true)
        {
            sp<RenderCmd> cmd;
            
            {
                std::unique_lock lock(m_mutex);
                if (m_curCmdIndex >= m_cmds.size())
                {
                    m_waitForDoneCond.notify_one();
                }

                m_execCmdCond.wait(lock, [this]
                {
                    return m_stop.load() || m_curCmdIndex < m_cmds.size();
                });

                if (m_stop.load() && m_curCmdIndex >= m_cmds.size())
                {
                    break;
                }

                cmd = m_cmds[m_curCmdIndex];
                m_curCmdIndex++;
            }

            (*cmd)(m_cmdList.Get());
        }

        WaitForFence();
        
        m_cmdList.Reset();
        m_cmdAllocator.Reset();
    }
    
    void RenderThread::StopThread()
    {
        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
            m_execCmdCond.notify_one();
        }
        m_thread.join();
        
        ReleaseCmdResources();
    }

    void RenderThread::WaitForFence()
    {
        m_fenceValue++;
        THROW_IF_FAILED(Dx()->GetCommandQueue()->Signal(m_fence.Get(), m_fenceValue));
        
        if (m_fence->GetCompletedValue() < m_fenceValue)
        {
            THROW_IF_FAILED(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));

            {
                ZoneScopedNC("Wait for fence", TRACY_IDLE_COLOR);
                   
                THROW_IF(WaitForSingleObject(m_fenceEvent, INFINITE) != WAIT_OBJECT_0, "Call wait for fence event failed.");
            }
        }
    }
}
