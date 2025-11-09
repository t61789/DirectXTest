#include "directx.h"

#include <stdexcept>

#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <directx/d3dx12_root_signature.h>
#include <directx/d3dx12_barriers.h>
#include <directx/d3dx12_core.h>

#include "common/utils.h"
#include "common/math.h"

namespace dt
{
    void DirectX::Init(const HWND windowHwnd)
    {
        m_windowHwnd = windowHwnd;

        LoadFactory();
        LoadAdapter();
        LoadOutput();
        LoadOutputModes();

        CreateDevice();
        CreateCommandQueue();
        CreateCommandAllocator();
        CreateCommandList();
        CreateFence();
        CreateSwapChain();
        CreateDescriptorHeaps();
        CreateRenderTargets();

        CreateTextures();
    }

    void DirectX::Release()
    {
        CloseHandle(m_fenceEvent);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE DirectX::GetBackBufferHandle()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        rtvHandle.Offset(static_cast<int>(m_swapChainBufferIndex * m_rtvDescriptorSize));

        return rtvHandle;
    }

    void DirectX::PresentSwapChain()
    {
        THROW_IF_FAILED(m_dxgiSwapChain->Present(1, 0));

        m_swapChainBufferIndex = (m_swapChainBufferIndex + 1) % m_swapChainDesc.BufferCount;
    }

    void DirectX::DelayRelease(const ComPtr<ID3D12Resource>&& obj)
    {
        m_delayedReleaseObjs.push_back(std::move(obj));
    }

    ComPtr<ID3D12Resource> DirectX::CreateUploadBuffer(const void* data, const size_t sizeB)
    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeB);
        
        ComPtr<ID3D12Resource> buffer = CreateCommittedResource(heapProps, bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ);

        void* mappedData;
        THROW_IF_FAILED(buffer->Map(0, nullptr, &mappedData));
        memcpy(mappedData, data, sizeB);
        buffer->Unmap(0, nullptr);

        return buffer;
    }
    
    ComPtr<ID3D12Resource> DirectX::CreateCommittedResource(
        cr<D3D12_HEAP_PROPERTIES> pHeapProperties,
        cr<D3D12_RESOURCE_DESC> pDesc,
        const D3D12_RESOURCE_STATES initialResourceState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        const D3D12_HEAP_FLAGS heapFlags)
    {
        ComPtr<ID3D12Resource> resource;
        THROW_IF_FAILED(m_device->CreateCommittedResource(
            &pHeapProperties,
            heapFlags,
            &pDesc,
            initialResourceState,
            pOptimizedClearValue,
            IID_ID3D12Resource,
            &resource));

        return resource;
    }

    ComPtr<ID3DBlob> DirectX::CompileShader(crstr filePath, crstr entryPoint, crstr target)
    {
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        uint32_t compileFlags = 0;
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        auto absFilePath = Utils::StringToWString(Utils::ToAbsPath(filePath));
        auto result = D3DCompileFromFile(
            absFilePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(),
            target.c_str(),
            compileFlags,
            0,
            &shaderBlob,
            &errorBlob);

        if (FAILED(result))
        {
            if (errorBlob)
            {
                log_error("Compile shader error: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }

            throw std::runtime_error("Failed to compile shader.");
        }

        return shaderBlob;
    }

    void DirectX::FlushCommand()
    {
        THROW_IF_FAILED(m_commandList->Close());
        ID3D12CommandList* cmdList[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, cmdList);
    }

    void DirectX::IncreaseFence()
    {
        m_fenceValue++;
        THROW_IF_FAILED(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
    }

    void DirectX::WaitForFence()
    {
        if (m_fence->GetCompletedValue() < m_fenceValue)
        {
            THROW_IF_FAILED(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
            
            THROW_IF(WaitForSingleObject(m_fenceEvent, INFINITE) != WAIT_OBJECT_0, "Call wait for fence event failed.");
        }

        THROW_IF_FAILED(m_commandAllocator->Reset());
        THROW_IF_FAILED(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

        m_delayedReleaseObjs.clear();
    }

    void DirectX::LoadFactory()
    {
        THROW_IF_FAILED(CreateDXGIFactory(IID_IDXGIFactory, &m_dxgiFactor));
    }

    void DirectX::LoadAdapter()
    {
        for (uint32_t i = 0;; i++)
        {
            THROW_IF(m_dxgiFactor->EnumAdapters(i, &m_dxgiAdapter) == DXGI_ERROR_NOT_FOUND, "Failed to find DXGI adapter.");

            if (SUCCEEDED(m_dxgiAdapter->GetDesc(&m_dxgiAdapterDesc)))
            {
                break;
            }

            m_dxgiAdapter.Reset();
        }
        
        log_info("Adapter: %s", Utils::WStringToString(m_dxgiAdapterDesc.Description).c_str());
    }

    void DirectX::LoadOutput()
    {
        for (uint32_t i = 0;; i++)
        {
            THROW_IF(m_dxgiAdapter->EnumOutputs(i, &m_dxgiOutput) == DXGI_ERROR_NOT_FOUND, "Failed to find DXGI output.");

            if (SUCCEEDED(m_dxgiOutput->GetDesc(&m_dxgiOutputDesc)))
            {
                break;
            }

            m_dxgiOutput.Reset();
        }

        log_info("Output: %s", Utils::WStringToString(m_dxgiOutputDesc.DeviceName).c_str());
    }

    void DirectX::LoadOutputModes()
    {
        auto count = 0u;
        auto result = m_dxgiOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &count, nullptr);
        THROW_IF(FAILED(result) || count == 0, "Failed to get display mode list.");
        m_dxgiOutputModes = vec<DXGI_MODE_DESC>(count);
        
        THROW_IF_FAILED(m_dxgiOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &count, m_dxgiOutputModes.data()));
        
        auto mode = m_dxgiOutputModes.back();
        log_info(
            "Max display mode: size %dx%d, refresh rate %f, scaling %d",
            mode.Width, mode.Height,
            static_cast<float>(mode.RefreshRate.Numerator) / static_cast<float>(mode.RefreshRate.Denominator),
            mode.Scaling);

        // for (uint32_t i = 0; i < count; i++)
        // {
        //     mode = m_dxgiOutputModes[i];
        //     log_info(
        //         "Display mode%d: size %dx%d, refresh rate %f, scaling %d",
        //         i,
        //         mode.Width, mode.Height,
        //         static_cast<float>(mode.RefreshRate.Numerator) / static_cast<float>(mode.RefreshRate.Denominator),
        //         mode.Scaling);
        // }
    }

    void DirectX::CreateDevice()
    {
        THROW_IF_FAILED(D3D12CreateDevice(m_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_ID3D12Device, &m_device));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_cbvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    void DirectX::CreateCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        THROW_IF_FAILED(m_device->CreateCommandQueue(&desc, IID_ID3D12CommandQueue, &m_commandQueue));
    }

    void DirectX::CreateCommandAllocator()
    {
        THROW_IF_FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_ID3D12CommandAllocator, &m_commandAllocator));
    }

    void DirectX::CreateCommandList()
    {
        THROW_IF_FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_ID3D12GraphicsCommandList, &m_commandList));
    }

    void DirectX::CreateFence()
    {
        THROW_IF_FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, &m_fence));

        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        THROW_IF(m_fenceEvent == nullptr, "Failed to create fence event.");
    }

    void DirectX::CreateSwapChain()
    {
        DXGI_MODE_DESC modeDesc = {};
        modeDesc.Width = 1600;
        modeDesc.Height = 900;
        modeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        modeDesc.RefreshRate.Numerator = 144;
        modeDesc.RefreshRate.Denominator = 1;
        modeDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
        
        m_swapChainDesc = {};
        m_swapChainDesc.BufferDesc = modeDesc;
        m_swapChainDesc.SampleDesc.Quality = 0;
        m_swapChainDesc.SampleDesc.Count = 1;
        m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        m_swapChainDesc.BufferCount = 2;
        m_swapChainDesc.OutputWindow = m_windowHwnd;
        m_swapChainDesc.Windowed = TRUE;
        m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        m_swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        THROW_IF_FAILED(m_dxgiFactor->CreateSwapChain(m_commandQueue.Get(), &m_swapChainDesc, &m_dxgiSwapChain));

        m_swapChainBuffers.resize(m_swapChainDesc.BufferCount);
        for (uint32_t i = 0; i < m_swapChainDesc.BufferCount; i++)
        {
            THROW_IF_FAILED(m_dxgiSwapChain->GetBuffer(i, IID_ID3D12Resource, &m_swapChainBuffers[i]));
        }
    }

    void DirectX::CreateDescriptorHeaps()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = m_swapChainDesc.BufferCount;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0;

        THROW_IF_FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_ID3D12DescriptorHeap, &m_rtvHeap));

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;

        THROW_IF_FAILED(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_ID3D12DescriptorHeap, &m_dsvHeap));
    }

    void DirectX::CreateRenderTargets()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (uint32_t i = 0; i < m_swapChainDesc.BufferCount; i++)
        {
            m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(static_cast<int>(m_rtvDescriptorSize));
        }
    }

    void DirectX::CreateTextures()
    {
        
        D3D12_RESOURCE_DESC depthTexDesc = {};
        depthTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthTexDesc.Alignment = 0;
        depthTexDesc.Width = m_swapChainDesc.BufferDesc.Width;
        depthTexDesc.Height = m_swapChainDesc.BufferDesc.Height;
        depthTexDesc.DepthOrArraySize = 1;
        depthTexDesc.MipLevels = 1;
        depthTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthTexDesc.SampleDesc.Count = 1;
        depthTexDesc.SampleDesc.Quality = 0;
        depthTexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES depthTexHeapProps = {};
        depthTexHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        THROW_IF_FAILED(m_device->CreateCommittedResource(
            &depthTexHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthTexDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_ID3D12Resource,
            &m_depthStencilBuffer));

        m_device->CreateDepthStencilView(
            m_depthStencilBuffer.Get(),
            nullptr,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        AddCommand([this](ID3D12GraphicsCommandList* cmdList)
        {
            auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
                this->m_depthStencilBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_DEPTH_WRITE);
            cmdList->ResourceBarrier(1, &transition);
        });
        
        FlushCommand();
        WaitForFence();
    }
}
