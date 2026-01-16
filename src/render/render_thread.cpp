#include "render_thread.h"

#include <tracy/Tracy.hpp>

#include "cbuffer.h"
#include "directx.h"
#include "utils/recycle_bin.h"

namespace dt
{
    RenderThread::RenderThread(const char* name)
    {
        m_thread = mup<ConsumerThread<RenderCmd>>([this](cr<RenderCmd> cmd) { if (cmd) cmd(m_cmdList.Get()); }, name);
        m_thread->Enqueue([this](const ID3D12GraphicsCommandList*) { ThreadInit(); });
    }

    RenderThread::~RenderThread()
    {
        Wait();
        
        m_thread->Enqueue([this](const ID3D12GraphicsCommandList*) { ThreadRelease(); });
        m_thread->Stop(false);
        m_thread->Join();
    }
    
    void RenderThread::AddCmd(RenderCmd&& cmd)
    {
        assert(Utils::IsMainThread() && !m_thread->IsStopped());

        if (cmd == nullptr)
        {
            return;
        }

        m_pendingCmds.push_back(std::move(cmd));
    }

    void RenderThread::ReleaseCmdResources()
    {
        m_cmds.clear();
    }

    void RenderThread::ExecuteCmds()
    {
        for (auto& cmd : m_pendingCmds)
        {
            m_thread->Enqueue(cmd);
            m_cmds.push_back(std::move(cmd));
        }
        m_pendingCmds.clear();
    }

    void RenderThread::ResetCmdList()
    {
        THROW_IF_FAILED(m_cmdAllocator->Reset());
        THROW_IF_FAILED(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));
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
}
