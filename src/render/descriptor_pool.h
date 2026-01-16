#pragma once
#include <cstdint>
#include <d3d12.h>
#include <directx/d3dx12_root_signature.h>
#include <wrl/client.h>

#include "common/utils.h"
#include "common/const.h"
#include "utils/single_elem_pool.h"

namespace dt
{
    class DxResource;
    class DxTexture;
    enum class TextureFilterMode : uint8_t;
    enum class TextureWrapMode : uint8_t;
    class Shader;
    class DescriptorPool;
    using namespace Microsoft::WRL;

    struct SamplerInfo
    {
        TextureWrapMode wrapMode;
        TextureFilterMode filterMode;
    };

    struct SrvInfo
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    };
    
    using SrvPool = SingleElemPool<SrvInfo>;
    using SamplerPool = SingleElemPool<SamplerInfo>;
    using RtvPool = SingleElemPool<CD3DX12_CPU_DESCRIPTOR_HANDLE>;
    using DsvPool = SingleElemPool<CD3DX12_CPU_DESCRIPTOR_HANDLE>;

    struct ShaderResource
    {
        uint32_t GetSrvIndex() const { return m_srvPoolHandle->GetIndex(); }
        uint32_t GetSamplerIndex() const { return m_samplerPoolHandle->GetIndex(); }

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvCpuHandle();

    private:
        DescriptorPool* m_pool;
        sp<SrvPool::Handle> m_srvPoolHandle;
        sp<SamplerPool::Handle> m_samplerPoolHandle;

        friend class DescriptorPool;
    };

    class DescriptorPool : public Singleton<DescriptorPool>
    {
    public:
        DescriptorPool();

        ID3D12DescriptorHeap* GetSrvDescHeap() const { return m_srvDescHeap.Get(); }
        ID3D12DescriptorHeap* GetSamplerDescHeap() const { return m_samplerDescHeap.Get(); }

        sp<ShaderResource> AllocTextureSrv(const DxTexture* dxTexture);
        sp<ShaderResource> AllocBufferSrv(const DxResource* dxResource, uint32_t sizeB);
        sp<SrvPool::Handle> AllocEmptySrvHandle();
        void AllocRtv(
            crvec<DxTexture*> colorAttachments,
            const DxTexture* depthAttachment,
            vecsp<RtvPool::Handle>& rtvHandles, sp<DsvPool::
            Handle>& dsvHandle);
        
        void SetHeaps(ID3D12GraphicsCommandList* cmdList);

    private:
        SrvPool m_srvPool;
        ComPtr<ID3D12DescriptorHeap> m_srvDescHeap;
        uint32_t m_srvDescSizeB;
        
        SamplerPool m_samplerPool;
        ComPtr<ID3D12DescriptorHeap> m_samplerDescHeap;
        uint32_t m_samplerDescSizeB;

        RtvPool m_rtvPool;
        ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;
        uint32_t m_rtvDescSizeB;

        DsvPool m_dsvPool;
        ComPtr<ID3D12DescriptorHeap> m_dsvDescHeap;
        uint32_t m_dsvDescSizeB;
    };
}
