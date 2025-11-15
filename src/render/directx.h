#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <directx/d3dx12_root_signature.h>
#include <wrl/client.h>

#include "common/const.h"
#include "common/utils.h"

namespace dt
{
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
        cr<DXGI_SWAP_CHAIN_DESC> GetSwapChainDesc() const { return m_swapChainDesc; }
        ComPtr<ID3D12Resource> GetBackBuffer() const { return m_swapChainBuffers[m_swapChainBufferIndex]; }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetBackBufferHandle();

        template <typename F>
        void AddCommand(F&& func);
        void FlushCommand();

        void AddTransition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
        void ApplyTransitions(ID3D12GraphicsCommandList* cmdList);
        
        void IncreaseFence();
        void WaitForFence();
        
        void PresentSwapChain();

        ComPtr<ID3D12Resource> CreateUploadBuffer(const void* data, size_t sizeB);
        ComPtr<ID3D12Resource> CreateCommittedResource(
            cr<D3D12_HEAP_PROPERTIES> pHeapProperties,
            cr<D3D12_RESOURCE_DESC> pDesc,
            D3D12_RESOURCE_STATES initialResourceState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr,
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE,
            const char* name = nullptr);

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
        void CreateCommandAllocator();
        void CreateCommandList();
        void CreateFence();
        void CreateSwapChain();
        void CreateDescriptorHeaps();
        void CreateRenderTargets();
        // void CreateTextures();

        ComPtr<ID3D12Debug> m_debugLayer;
        ComPtr<IDXGIFactory> m_dxgiFactor;
        ComPtr<IDXGIAdapter> m_dxgiAdapter;
        ComPtr<IDXGIOutput> m_dxgiOutput;
        ComPtr<IDXGISwapChain> m_dxgiSwapChain;
        vec<ComPtr<ID3D12Resource>> m_swapChainBuffers;
        ComPtr<ID3D12Resource> m_depthStencilBuffer;
        DXGI_ADAPTER_DESC m_dxgiAdapterDesc;
        DXGI_OUTPUT_DESC m_dxgiOutputDesc;
        vec<DXGI_MODE_DESC> m_dxgiOutputModes;
        DXGI_SWAP_CHAIN_DESC m_swapChainDesc;

        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

        uint32_t m_rtvDescriptorSize;
        uint32_t m_dsvDescriptorSize;
        uint32_t m_cbvDescriptorSize;
        
        ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent;
        uint64_t m_fenceValue = 0;

        vec<D3D12_RESOURCE_BARRIER> m_transitions;
        vec<std::function<void(ID3D12GraphicsCommandList*)>> m_cmds;

        uint32_t m_swapChainBufferIndex = 0;

        HWND m_windowHwnd;
    };

    template <typename F>
    void DirectX::AddCommand(F&& func)
    {
        m_cmds.push_back(func);
        func(m_commandList.Get());
    }

    static DirectX* Dx() { return DirectX::Ins(); }
}
