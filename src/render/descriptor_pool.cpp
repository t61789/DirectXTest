#include "descriptor_pool.h"

#include <algorithm>

#include "directx.h"
#include "dx_helper.h"
#include "dx_resource.h"
#include "common/shader.h"

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
    DescriptorPool::DescriptorPool():
        m_srvPool(SRV_DESC_POOL_SIZE),
        m_samplerPool(SAMPLER_DESC_POOL_SIZE),
        m_rtvPool(RTV_DESC_POOL_SIZE),
        m_dsvPool(RTV_DESC_POOL_SIZE)
    {
        // shader resource
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.NumDescriptors = SRV_DESC_POOL_SIZE;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvHeapDesc.NodeMask = 0;
        THROW_IF_FAILED(Dx()->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_ID3D12DescriptorHeap, &m_srvDescHeap));
        m_srvDescSizeB = Dx()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // sampler
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.NumDescriptors = SAMPLER_DESC_POOL_SIZE;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        samplerHeapDesc.NodeMask = 0;
        THROW_IF_FAILED(Dx()->GetDevice()->CreateDescriptorHeap(&samplerHeapDesc, IID_ID3D12DescriptorHeap, &m_samplerDescHeap));
        m_samplerDescSizeB = Dx()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        // render target
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = RTV_DESC_POOL_SIZE;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0;
        THROW_IF_FAILED(Dx()->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_ID3D12DescriptorHeap, &m_rtvDescHeap));
        m_rtvDescSizeB = Dx()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // depth stencil
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = RTV_DESC_POOL_SIZE;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;
        THROW_IF_FAILED(Dx()->GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_ID3D12DescriptorHeap, &m_dsvDescHeap));
        m_dsvDescSizeB = Dx()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    sp<ShaderResource> DescriptorPool::AllocSrv(const DxTexture* dxTexture)
    {
        auto srvHandle = m_srvPool.Alloc();
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = {
            m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(srvHandle->GetIndex()),
            m_srvDescSizeB
        };

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DxTexture::GetSrvFormat(dxTexture->GetDesc().format);
        srvDesc.ViewDimension = DxTexture::GetSrvDimension(dxTexture->GetDesc().type);
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (dxTexture->GetDesc().type == TextureType::TEXTURE_2D)
        {
            srvDesc.Texture2D.MipLevels = 1; // TODO mipmap
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            THROW_ERROR("Not implemented")
        }
        
        Dx()->GetDevice()->CreateShaderResourceView(
            dxTexture->GetDxResource()->GetResource(),
            &srvDesc,
            srvCpuHandle);
        srvHandle->data = srvCpuHandle;

        auto filterMode = dxTexture->GetDesc().filterMode;
        auto wrapMode = dxTexture->GetDesc().wrapMode;
        auto samplerHandle = m_samplerPool.Find([this, filterMode, wrapMode](crsp<SamplerPool::Handle> poolHandle)
        {
            return poolHandle->data.filterMode == filterMode && poolHandle->data.wrapMode == wrapMode;
        });
        if (!samplerHandle)
        {
            samplerHandle = m_samplerPool.Alloc();
            samplerHandle->data.filterMode = filterMode;
            samplerHandle->data.wrapMode = wrapMode;
            
            D3D12_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = ToD3D12Filter(filterMode);
            samplerDesc.AddressU = ToD3D12AddressMode(wrapMode);
            samplerDesc.AddressV = ToD3D12AddressMode(wrapMode);
            samplerDesc.AddressW = ToD3D12AddressMode(wrapMode);
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = 1;

            CD3DX12_CPU_DESCRIPTOR_HANDLE dxSamplerHandle(
                m_samplerDescHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(samplerHandle->GetIndex()),
                m_samplerDescSizeB);
            Dx()->GetDevice()->CreateSampler(&samplerDesc, dxSamplerHandle);
        }

        auto descHandle = msp<ShaderResource>();
        descHandle->m_pool = this;
        descHandle->m_srvPoolHandle = srvHandle;
        descHandle->m_samplerPoolHandle = samplerHandle;

        return descHandle;
    }

    void DescriptorPool::AllocRtv(
        crvec<DxTexture*> colorAttachments,
        const DxTexture* depthAttachment,
        vecsp<RtvPool::Handle>& rtvHandles,
        sp<DsvPool::Handle>& dsvHandle)
    {
        assert(colorAttachments.size() > 0 || depthAttachment);

        if (colorAttachments.size() > 0)
        {
            rtvHandles.resize(colorAttachments.size());
            for (uint32_t i = 0; i < colorAttachments.size(); ++i)
            {
                auto rtvHandle = m_rtvPool.Alloc(colorAttachments.size() > 1);
                CD3DX12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle = {
                    m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                    static_cast<INT>(rtvHandle->GetIndex()),
                    m_rtvDescSizeB
                };
                
                Dx()->GetDevice()->CreateRenderTargetView(
                    colorAttachments[i]->GetDxResource()->GetResource(),
                    nullptr,
                    rtvCpuHandle);
                rtvHandle->data = rtvCpuHandle;
                rtvHandles[i] = rtvHandle;
            }
        }

        if (depthAttachment)
        {
            dsvHandle = m_dsvPool.Alloc();
            CD3DX12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = {
                m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(dsvHandle->GetIndex()),
                m_dsvDescSizeB
            };

            D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format = DxTexture::GetDsvFormat(depthAttachment->GetDesc().format);
            desc.Flags = D3D12_DSV_FLAG_NONE;
            desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;

            Dx()->GetDevice()->CreateDepthStencilView(
                depthAttachment->GetDxResource()->GetResource(),
                &desc,
                dsvCpuHandle);
            dsvHandle->data = dsvCpuHandle;
        }
    }

    void DescriptorPool::SetHeaps(ID3D12GraphicsCommandList* cmdList)
    {
        ID3D12DescriptorHeap* heaps[] = {
            m_srvDescHeap.Get(),
            m_samplerDescHeap.Get()
        };
        cmdList->SetDescriptorHeaps(2, heaps);
    }
}
