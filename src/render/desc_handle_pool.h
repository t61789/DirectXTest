#pragma once
#include <cstdint>
#include <d3d12.h>
#include <directx/d3dx12_root_signature.h>
#include <wrl/client.h>

#include "common/utils.h"
#include "common/const.h"
#include "common/image.h"

namespace dt
{
    using namespace Microsoft::WRL;

    class SrvDescPool;
    
    struct SrvDesc
    {
        SrvDesc() = default;
        ~SrvDesc();
        SrvDesc(const SrvDesc& other) = delete;
        SrvDesc(SrvDesc&& other) noexcept = delete;
        SrvDesc& operator=(const SrvDesc& other) = delete;
        SrvDesc& operator=(SrvDesc&& other) noexcept = delete;

        uint32_t GetIndex() const { return m_index; }
        
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuHandle();
        
    private:
        
        uint32_t m_index;
        SrvDescPool* m_pool;

        friend class SrvDescPool;
    };
    
    class SrvDescPool : public Singleton<SrvDescPool>
    {
    public:
        SrvDescPool();
        ~SrvDescPool() = default;
        SrvDescPool(const SrvDescPool& other) = delete;
        SrvDescPool(SrvDescPool&& other) noexcept = delete;
        SrvDescPool& operator=(const SrvDescPool& other) = delete;
        SrvDescPool& operator=(SrvDescPool&& other) noexcept = delete;

        ID3D12DescriptorHeap* GetHeap() const { return m_descHeap.Get(); }

        sp<SrvDesc> Alloc();
        void Bind(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex);

    private:
        void Free(const SrvDesc* handle);
        
        struct DescInfo
        {
            bool inUse = false;
        };
        
        uint32_t m_firstFreeIndex;
        uint32_t m_descSizeB;
        ComPtr<ID3D12DescriptorHeap> m_descHeap;
        vec<DescInfo> m_descInfos;

        friend struct SrvDesc;
    };

    class SamplerDescPool;

    struct SamplerDesc
    {
        uint32_t GetIndex() const { return m_index; }

    private:
        uint32_t m_index;
        SamplerDescPool* m_pool;
        TextureWrapMode m_wrapMode;
        TextureFilterMode m_filterMode;

        friend class SamplerDescPool;
    };

    class SamplerDescPool : public Singleton<SamplerDescPool>
    {
    public:
        explicit SamplerDescPool();

        ID3D12DescriptorHeap* GetHeap() const { return m_descHeap.Get(); }
        
        sp<SamplerDesc> Alloc(TextureFilterMode filterMode, TextureWrapMode wrapMode);
        void Release(uint32_t index);
        void Bind(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex);

    private:
        uint32_t m_firstFreeIndex = 0;
        uint32_t m_descSizeB;
        vecwp<SamplerDesc> m_samplerDesc;
        ComPtr<ID3D12DescriptorHeap> m_descHeap;
    };
}
