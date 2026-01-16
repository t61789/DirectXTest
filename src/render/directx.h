#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <directx/d3dx12_root_signature.h>
#include <wrl/client.h>
#include <dxcapi.h>

#include "descriptor_pool.h"
#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    class RenderThread;
    class Gui;
    class RecycleBin;
    class RenderTarget;
    class RenderTexture;
    class DxBuffer;
    class RenderThreadMgr;
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
        crsp<RenderTexture> GetBackBuffer() const { return m_swapChainRenderTextures[m_backBufferIndex]; }
        crsp<RenderTarget> GetBackBufferRenderTarget() const { return m_swapChainRenderTargets[m_backBufferIndex]; }
        ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
        IDxcUtils* GetDxcUtils() const { return m_dxcUtils.Get(); }
        IDxcCompiler3* GetDxcCompiler() const { return m_dxcCompiler.Get(); }
        IDxcIncludeHandler* GetDxcIncludeHandler() const { return m_dxcIncludeHandler.Get(); }

        void WaitForFence();
        void PresentSwapChain();

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
        void CreateDebugLayer();
        void CreateDxc();
        void CreateFactory();
        void CreateAdapter();
        void CreateOutput();
        void LoadOutputModes();
        void CreateDevice();
        void CreateCommandQueue();
        void CreateSwapChain();
        void CreateSwapChainTextures();
        void CreateFence();

        ComPtr<ID3D12Debug> m_debugLayer;
        ComPtr<IDXGIFactory4> m_dxgiFactor4;
        ComPtr<IDXGIAdapter1> m_dxgiAdapter1;
        ComPtr<IDXGIOutput> m_dxgiOutput;
        ComPtr<IDXGISwapChain3> m_dxgiSwapChain3;
        DXGI_ADAPTER_DESC m_dxgiAdapterDesc;
        DXGI_OUTPUT_DESC m_dxgiOutputDesc;
        vec<DXGI_MODE_DESC> m_dxgiOutputModes;
        DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc1;
        ComPtr<IDxcUtils> m_dxcUtils;
        ComPtr<IDxcCompiler3> m_dxcCompiler;
        ComPtr<IDxcIncludeHandler> m_dxcIncludeHandler;

        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        
        uint32_t m_backBufferIndex = 0;

        HWND m_windowHwnd;
        
        vecsp<RenderTexture> m_swapChainRenderTextures;
        vecsp<RenderTarget> m_swapChainRenderTargets;

        RecycleBin* m_recycleBin;
        DescriptorPool* m_descriptorPool;
        RenderThread* m_renderThread;
        Gui* m_gui;
        
        ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent;
        uint64_t m_fenceValue = 0;
    };

    static DirectX* Dx() { return DirectX::Ins(); }
}
