#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <directx/d3dx12_root_signature.h>
#include <wrl/client.h>

#include "common/const.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class DirectX
    {
    public:
        DirectX() = default;

        void Init(HWND windowHwnd);
        void Release();

        cr<DXGI_SWAP_CHAIN_DESC> GetSwapChainDesc() const { return m_swapChainDesc; }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetBackBufferHandle();

        template <typename F>
        void AddCommand(F&& func);
        void FlushCommand();
        
        void IncreaseFence();
        void WaitForFence();
        
        void PresentSwapChain();

    private:
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
        void CreateTextures();
        
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

        uint32_t m_swapChainBufferIndex = 0;

        HWND m_windowHwnd;
    };

    template <typename F>
    void DirectX::AddCommand(F&& func)
    {
        func(m_commandList.Get());
    }
}
