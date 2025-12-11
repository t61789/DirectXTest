#include "desc_handle_pool.h"

#include <algorithm>

#include "directx.h"

namespace
{
    using namespace dt;

    D3D12_FILTER ToD3D12Filter(const TextureFilterMode filterMode)
    {
        switch (filterMode)
        {
        case TextureFilterMode::NEAREST:
            return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case TextureFilterMode::BILINEAR:
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        default:
            THROW_ERROR("Invalid texture filter mode")
        }
    }

    D3D12_TEXTURE_ADDRESS_MODE ToD3D12AddressMode(const TextureWrapMode addressMode)
    {
        switch (addressMode)
        {
        case TextureWrapMode::CLAMP:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case TextureWrapMode::REPEAT:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        default:
            THROW_ERROR("Invalid texture wrap mode")
        }
    }
}

namespace dt
{
    SrvDesc::~SrvDesc()
    {
        m_pool->Free(this);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE SrvDesc::GetCpuHandle()
    {
        return { m_pool->m_descHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(m_index), m_pool->m_descSizeB };
    }

    SrvDescPool::SrvDescPool(cr<ComPtr<ID3D12Device>> device)
    {
        m_firstFreeIndex = 0;
        m_descInfos.resize(DESC_HANDLE_POOL_SIZE);
        
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        rtvHeapDesc.NumDescriptors = DESC_HANDLE_POOL_SIZE;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        rtvHeapDesc.NodeMask = 0;

        THROW_IF_FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_ID3D12DescriptorHeap, &m_descHeap));

        m_descSizeB = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    sp<SrvDesc> SrvDescPool::Alloc()
    {
        uint32_t index = ~0u;
        
        for (uint32_t i = m_firstFreeIndex; i < m_descInfos.size(); ++i)
        {
            if (m_descInfos[i].inUse)
            {
                continue;
            }
            
            index = i;
            m_firstFreeIndex = i + 1;
            m_descInfos[i].inUse = false;
            break;
        }

        ASSERT_THROW(index != ~0u);

        auto descHandle = msp<SrvDesc>();
        descHandle->m_index = index;
        descHandle->m_pool = this;

        return descHandle;
    }

    void SrvDescPool::SetHeap(ID3D12GraphicsCommandList* cmdList)
    {
        ID3D12DescriptorHeap* heaps[] = { m_descHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);
    }

    void SrvDescPool::Bind(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex)
    {
        cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, m_descHeap->GetGPUDescriptorHandleForHeapStart()); // TODO samplers
    }

    void SrvDescPool::Free(const SrvDesc* handle)
    {
        m_descInfos[handle->m_index].inUse = false;

        m_firstFreeIndex = std::min<uint32_t>(handle->m_index, m_firstFreeIndex);
    }

    SamplerDescPool::SamplerDescPool()
    {
        m_firstFreeIndex = 0;
        m_samplerDesc.resize(SAMPLER_DESC_POOL_SIZE);
        m_descSizeB = Dx()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    sp<SamplerDesc> SamplerDescPool::Alloc(const TextureFilterMode filterMode, const TextureWrapMode wrapMode)
    {
        for (auto& wp : m_samplerDesc)
        {
            auto desc = wp.lock();
            if (desc != nullptr && desc->m_filterMode == filterMode && desc->m_wrapMode == wrapMode)
            {
                return desc;
            }
        }

        auto index = m_firstFreeIndex;
        for (; index <= m_samplerDesc.size(); ++index)
        {
            if (index == m_samplerDesc.size())
            {
                if (index == SAMPLER_DESC_POOL_SIZE)
                {
                    THROW_ERROR("Sampler desc pool is full")
                }
                
                m_samplerDesc.push_back({});
            }
            
            if (m_samplerDesc[index].expired())
            {
                break;
            }
        }

        D3D12_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = ToD3D12Filter(filterMode);
        samplerDesc.AddressU = ToD3D12AddressMode(wrapMode);
        samplerDesc.AddressV = ToD3D12AddressMode(wrapMode);
        samplerDesc.AddressW = ToD3D12AddressMode(wrapMode);
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(m_descHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(index), m_descSizeB);
        Dx()->GetDevice()->CreateSampler(&samplerDesc, samplerHandle);

        auto descHandle = msp<SamplerDesc>();
        descHandle->m_filterMode = filterMode;
        descHandle->m_wrapMode = wrapMode;
        descHandle->m_index = index;
        descHandle->m_pool = this;

        m_samplerDesc[index] = descHandle;

        return descHandle;
    }

    void SamplerDescPool::Release(const uint32_t index)
    {
        m_firstFreeIndex = std::min<uint32_t>(index, m_firstFreeIndex);
    }
}
