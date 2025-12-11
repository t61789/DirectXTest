#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <directx/d3dx12_root_signature.h>
#include <wrl/client.h>

#include "desc_handle_pool.h"
#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    class DxBuffer;
    class RenderThread;
    using namespace Microsoft::WRL;

    class DirectX : public Singleton<DirectX>
    {
    public:
        DirectX();
        ~DirectX();
        DirectX(const DirectX& other) = delete;
        DirectX(DirectX&& other) noexcept = delete;
        DirectX& operator=(const DirectX& other) = delete;
        DirectX& operator=(DirectX&& other) noexcept = delete;

        ComPtr<ID3D12Device> GetDevice() const { return m_device; }
        cr<DXGI_SWAP_CHAIN_DESC1> GetSwapChainDesc() const { return m_swapChainDesc1; }
        sp<DxResource> GetBackBuffer() const { return m_swapChainBuffers[m_backBufferIndex]; }
        ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetBackBufferHandle();

        void PresentSwapChain();
        void EndFrame();

        ComPtr<ID3D12Resource> CreateUploadBuffer(const void* data, size_t sizeB);
        ComPtr<ID3D12Resource> CreateCommittedResource(
            cr<D3D12_HEAP_PROPERTIES> pHeapProperties,
            cr<D3D12_RESOURCE_DESC> pDesc,
            D3D12_RESOURCE_STATES initialResourceState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr,
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE,
            const wchar_t* name = nullptr);

        static ComPtr<ID3DBlob> CompileShader(crstr filePath, crstr entryPoint, crstr target);
        static void ThrowErrorBlob(cr<ComPtr<ID3DBlob>> blob);

    private:
        void EnableDebugLayer();
        void LoadFactory();
        void LoadAdapter();
        void LoadOutput();
        void LoadOutputModes();
        void CreateDevice();
        void CreateCommandQueue();
        void CreateSwapChain();
        void CreateDescriptorHeaps();
        void CreateRenderTargets();
        // void CreateTextures();

        ComPtr<ID3D12Debug> m_debugLayer;
        ComPtr<IDXGIFactory4> m_dxgiFactor4;
        ComPtr<IDXGIAdapter> m_dxgiAdapter;
        ComPtr<IDXGIOutput> m_dxgiOutput;
        ComPtr<IDXGISwapChain3> m_dxgiSwapChain3;
        vecsp<DxResource> m_swapChainBuffers;
        ComPtr<ID3D12Resource> m_depthStencilBuffer;
        DXGI_ADAPTER_DESC m_dxgiAdapterDesc;
        DXGI_OUTPUT_DESC m_dxgiOutputDesc;
        vec<DXGI_MODE_DESC> m_dxgiOutputModes;
        DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc1;

        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

        uint32_t m_rtvDescriptorSize;
        uint32_t m_dsvDescriptorSize;
        uint32_t m_cbvDescriptorSize;
        
        uint32_t m_backBufferIndex = 0;

        sp<SrvDescPool> m_descHandlePool;

        HWND m_windowHwnd;

        sp<RenderThread> m_renderThread;
    };

    static DirectX* Dx() { return DirectX::Ins(); }
}
