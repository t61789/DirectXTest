#include "pso.h"

#include <directx/d3dx12_core.h>

#include "directx.h"
#include "common/shader.h"
#include "common/utils.h"

namespace dt
{
    Pso::Pso(Shader* shader)
    {
        m_desc = {};
        m_desc.pRootSignature = shader->GetRootSignature().Get();
        m_desc.VS = { shader->GetVSPointer(), shader->GetVSSize() };
        m_desc.PS = { shader->GetPSPointer(), shader->GetPSSize() };
        m_desc.InputLayout = { shader->GetVertexLayout().data(), static_cast<UINT>(shader->GetVertexLayout().size()) };
        m_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        m_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        m_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        m_desc.SampleMask = UINT_MAX;
        m_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        m_desc.NumRenderTargets = 1;
        m_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        m_desc.SampleDesc.Count = 1;
        m_desc.SampleDesc.Quality = 0;
    }

    ComPtr<ID3D12PipelineState> Pso::CurPso()
    {
        Update();

        return m_curPso;
    }

    void Pso::SetDepthMode(const DepthMode mode)
    {
        if (m_depthMode ==  mode)
        {
            return;
        }
        m_dirty = true;
        m_depthMode = mode;

        switch (mode)
        {
        case DepthMode::LESS:
            m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            break;
        case DepthMode::LESS_EQUAL:
            m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            break;
        case DepthMode::ALWAYS:
            m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            break;
        case DepthMode::EQUAL:
            m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
            break;
        case DepthMode::NOT_EQUAL:
            m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
            break;
        case DepthMode::GREATER:
            m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
            break;
        case DepthMode::GREATER_EQUAL:
            m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            break;
        }
    }

    void Pso::SetDepthWrite(const bool enable)
    {
        if (m_depthWrite == enable)
        {
            return;
        }
        m_dirty = true;
        m_depthWrite = enable;

        m_desc.DepthStencilState.DepthWriteMask = enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    }

    void Pso::SetCullMode(const CullMode mode)
    {
        if (m_cullMode == mode)
        {
            return;
        }
        m_dirty = true;
        m_cullMode = mode;

        switch (mode)
        {
        case CullMode::BACK:
            m_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            break;
        case CullMode::FRONT:
            m_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
            break;
        case CullMode::NONE:
            m_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            break;
        }
    }

    void Pso::SetRenderTargetFormat(const uint32_t index, const DXGI_FORMAT format)
    {
        if (m_desc.RTVFormats[index] == format)
        {
            return;
        }
        m_dirty = true;
        m_desc.RTVFormats[index] = format;
    }

    void Pso::Update()
    {
        if (!m_dirty)
        {
            return;
        }
        m_dirty = false;

        auto hash = Utils::GetMemoryHash(&m_desc, sizeof(m_desc));
        if (m_psoPool.find(hash) != m_psoPool.end())
        {
            m_curPso = m_psoPool[hash];
            return;
        }

        THROW_IF_FAILED(Dx()->GetDevice()->CreateGraphicsPipelineState(
            &m_desc,
            IID_ID3D12PipelineState,
            &m_curPso));
        m_psoPool[hash] = m_curPso;
    }
}
