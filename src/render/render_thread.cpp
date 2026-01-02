#include "render_thread.h"

#include <tracy/Tracy.hpp>

#include "cbuffer.h"
#include "directx.h"
#include "utils/recycle_bin.h"

namespace dt
{
    RenderThread::RenderThread(const char* name)
    {
        m_thread = mup<ConsumerThread<RenderCmd>>([this](cr<RenderCmd> cmd) { cmd(m_cmdList.Get()); }, name);
        m_thread->Enqueue([this](const ID3D12GraphicsCommandList*) { ThreadInit(); });
        
        THROW_IF_FAILED(Dx()->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, &m_fence));
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        THROW_IF(m_fenceEvent == nullptr, "Failed to create fence event.");
    }

    RenderThread::~RenderThread()
    {
        m_thread->Enqueue([this](const ID3D12GraphicsCommandList*) { ThreadRelease(); });
        m_thread->Stop(false);
        m_thread->Join();
        
        ReleaseCmdResources();

        WaitForFence();

        CloseHandle(m_fenceEvent);
    }
    
    void RenderThread::AddCmd(RenderCmd&& cmd)
    {
        assert(Utils::IsMainThread() && !m_thread->IsStopped());

        if (!m_recording.load())
        {
            m_pendingCmds.push_back(std::move(cmd));
        }
        else
        {
            m_thread->Enqueue(cmd);
            m_cmds.push_back(std::move(cmd));
        }
    }

    void RenderThread::ReleaseCmdResources()
    {
        m_cmds.clear();
    }

    void RenderThread::StopRecording()
    {
        assert(Utils::IsMainThread() && m_recording.load());

        AddCmd([this](ID3D12GraphicsCommandList* cmdList)
        {
            ZoneScopedN("Execute");
            
            THROW_IF_FAILED(cmdList->Close());
            
            vec<ID3D12CommandList*> cmdLists;
            cmdLists.push_back(m_cmdList.Get());

            Dx()->GetCommandQueue()->ExecuteCommandLists(static_cast<UINT>(cmdLists.size()), cmdLists.data());

            WaitForFence();

            Dx()->PresentSwapChain();
        });
        
        m_recording.store(false);
    }

    void RenderThread::StartRecording()
    {
        assert(Utils::IsMainThread() && !m_recording.load());

        if (!m_first)
        {
            THROW_IF_FAILED(m_cmdAllocator->Reset());
            THROW_IF_FAILED(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));
        }
        m_first = false;

        ReleaseCmdResources();

        m_recording.store(true);
        
        for (auto& cmd : m_pendingCmds)
        {
            AddCmd(std::move(cmd));
        }

        m_pendingCmds.clear();
    }

    void RenderThread::ThreadInit()
    {
        THROW_IF_FAILED(Dx()->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_ID3D12CommandAllocator, &m_cmdAllocator));
        THROW_IF_FAILED(Dx()->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), nullptr, IID_ID3D12GraphicsCommandList, &m_cmdList));
    }

    void RenderThread::ThreadRelease()
    {
        m_cmdList.Reset();
        m_cmdAllocator.Reset();
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

    RenderThreadMgr::RenderThreadMgr()
    {
        m_renderThread = new RenderThread("Common Render Thread");

        // m_presentThread = new PresentThread([](cr<std::function<void()>> product){ product(); }, "Present Thread");
        
    }

    RenderThreadMgr::~RenderThreadMgr()
    {
        // m_presentThread->Stop(false);
        // m_presentThread->Wait();
        // delete m_presentThread;
        delete m_renderThread;
    }

    void RenderThreadMgr::ExecuteAllThreads()
    {
        assert(Utils::IsMainThread() && !m_executing);
        
        m_executing = true;
        
        m_renderThread->StopRecording();

        // m_presentThread->Enqueue([this]
        // {
        // });
    }

    void RenderThreadMgr::WaitForDone()
    {
        assert(Utils::IsMainThread() && m_executing);

        {
            ZoneScopedNC("Wait for pre frame", TRACY_IDLE_COLOR);

            // m_presentThread->Wait();
            m_renderThread->Wait();
        }

        RecycleBin::Ins()->Flush();
        Cbuffer::UpdateDirtyCbuffers();

        m_renderThread->StartRecording();

        m_executing = false;
    }
}
