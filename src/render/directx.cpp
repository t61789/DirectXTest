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
#include <tracy/Tracy.hpp>

#include "dx_resource.h"
#include "render_thread.h"
#include "window.h"
#include "common/utils.h"
#include "common/math.h"

namespace dt
{
    DirectX::DirectX()
    {
        m_windowHwnd = Window::Ins()->GetHandle();

        CreateDebugLayer();

        CreateFactory();
        CreateAdapter();
        CreateOutput();
        LoadOutputModes();

        CreateDevice();
        CreateCommandQueue();

        m_renderThread = msp<RenderThread>(m_device);
        m_srvDescPool = msp<SrvDescPool>();
        m_samplerDescPool = msp<SamplerDescPool>();
        
        CreateSwapChain();
        CreateDescriptorHeaps();
        CreateRenderTargets();

        // CreateTextures();
    }

    DirectX::~DirectX()
    {
        m_samplerDescPool.reset();
        m_srvDescPool.reset();
        m_renderThread.reset();

        m_dsvHeap.Reset();
        m_rtvHeap.Reset();
        m_depthStencilBuffer.Reset();
        m_swapChainBuffers.clear();
        m_dxgiSwapChain3.Reset();
        m_device.Reset();
        m_commandQueue.Reset();
        m_dxgiOutput.Reset();
        m_dxgiAdapter1.Reset();
        m_dxgiFactor4.Reset();
        m_debugLayer.Reset();
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE DirectX::GetBackBufferHandle()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        rtvHandle.Offset(static_cast<int>(m_backBufferIndex * m_rtvDescriptorSize));

        return rtvHandle;
    }

    void DirectX::PresentSwapChain()
    {
        THROW_IF_FAILED(m_dxgiSwapChain3->Present(1, 0));
        m_backBufferIndex = m_dxgiSwapChain3->GetCurrentBackBufferIndex();
    }

    void DirectX::EndFrame()
    {
        RenderThread::Ins()->ReleaseCmdResources();
    }

    ComPtr<ID3D12Resource> DirectX::CreateUploadBuffer(const void* data, const size_t sizeB)
    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeB);
        
        ComPtr<ID3D12Resource> buffer = CreateCommittedResource(
            heapProps,
            bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            D3D12_HEAP_FLAG_NONE,
            L"Upload Buffer");

        if (data)
        {
            void* mappedData;
            THROW_IF_FAILED(buffer->Map(0, nullptr, &mappedData));
            memcpy(mappedData, data, sizeB);
            buffer->Unmap(0, nullptr);
        }

        return buffer;
    }
    
    ComPtr<ID3D12Resource> DirectX::CreateCommittedResource(
        cr<D3D12_HEAP_PROPERTIES> pHeapProperties,
        cr<D3D12_RESOURCE_DESC> pDesc,
        const D3D12_RESOURCE_STATES initialResourceState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        const D3D12_HEAP_FLAGS heapFlags,
        const wchar_t* name)
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

        if (name)
        {
            THROW_IF_FAILED(resource->SetName(name));
        }

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

    void DirectX::ThrowErrorBlob(cr<ComPtr<ID3DBlob>> blob)
    {
        if (!blob)
        {
            throw std::runtime_error("Failed to parse error blob.");
        }
        
        auto msg = format_log(LOG_ERROR, "Error: %s", static_cast<const char*>(blob->GetBufferPointer()));
        throw std::runtime_error(msg);
    }

    void DirectX::CreateDebugLayer()
    {
        THROW_IF_FAILED(D3D12GetDebugInterface(IID_ID3D12Debug, &m_debugLayer));
        m_debugLayer->EnableDebugLayer();
    }

    void DirectX::CreateFactory()
    {
        THROW_IF_FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_IDXGIFactory4, &m_dxgiFactor4));
    }

    void DirectX::CreateAdapter()
    {
        for (uint32_t i = 0;; i++)
        {
            THROW_IF(m_dxgiFactor4->EnumAdapters1(i, &m_dxgiAdapter1) == DXGI_ERROR_NOT_FOUND, "Failed to find DXGI adapter.");

            if (SUCCEEDED(m_dxgiAdapter1->GetDesc(&m_dxgiAdapterDesc)))
            {
                break;
            }

            m_dxgiAdapter1.Reset();
        }
        
        log_info("Adapter: %s", Utils::WStringToString(m_dxgiAdapterDesc.Description).c_str());
    }

    void DirectX::CreateOutput()
    {
        for (uint32_t i = 0;; i++)
        {
            THROW_IF(m_dxgiAdapter1->EnumOutputs(i, &m_dxgiOutput) == DXGI_ERROR_NOT_FOUND, "Failed to find DXGI output.");

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
        THROW_IF_FAILED(D3D12CreateDevice(m_dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_ID3D12Device, &m_device));

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

    void DirectX::CreateSwapChain()
    {
        m_swapChainDesc1 = {};
        m_swapChainDesc1.Width = 1600;
        m_swapChainDesc1.Height = 900;
        m_swapChainDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        m_swapChainDesc1.Stereo = FALSE;
        m_swapChainDesc1.SampleDesc.Count = 1;
        m_swapChainDesc1.SampleDesc.Quality = 0;
        m_swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        m_swapChainDesc1.BufferCount = 2;
        m_swapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
        m_swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        m_swapChainDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        ComPtr<IDXGISwapChain1> swapChain1;
        THROW_IF_FAILED(m_dxgiFactor4->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_windowHwnd,
            &m_swapChainDesc1,
            nullptr,
            nullptr,
            &swapChain1));
        
        THROW_IF_FAILED(swapChain1.As(&m_dxgiSwapChain3));
        m_backBufferIndex = m_dxgiSwapChain3->GetCurrentBackBufferIndex();

        m_swapChainBuffers.resize(m_swapChainDesc1.BufferCount);
        for (uint32_t i = 0; i < m_swapChainDesc1.BufferCount; i++)
        {
            ComPtr<ID3D12Resource> buffer;
            THROW_IF_FAILED(m_dxgiSwapChain3->GetBuffer(i, IID_ID3D12Resource, &buffer));

            m_swapChainBuffers[i] = DxResource::Create(buffer, D3D12_RESOURCE_STATE_PRESENT, L"Swap Cain Buffer");
        }
    }

    void DirectX::CreateDescriptorHeaps()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = m_swapChainDesc1.BufferCount;
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
        for (uint32_t i = 0; i < m_swapChainDesc1.BufferCount; i++)
        {
            m_device->CreateRenderTargetView(m_swapChainBuffers[i]->GetResource(), nullptr, rtvHandle);
            rtvHandle.Offset(static_cast<int>(m_rtvDescriptorSize));
        }
    }

    // void DirectX::CreateTextures()
    // {
    //     
    //     D3D12_RESOURCE_DESC depthTexDesc = {};
    //     depthTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    //     depthTexDesc.Alignment = 0;
    //     depthTexDesc.Width = m_swapChainDesc.BufferDesc.Width;
    //     depthTexDesc.Height = m_swapChainDesc.BufferDesc.Height;
    //     depthTexDesc.DepthOrArraySize = 1;
    //     depthTexDesc.MipLevels = 1;
    //     depthTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    //     depthTexDesc.SampleDesc.Count = 1;
    //     depthTexDesc.SampleDesc.Quality = 0;
    //     depthTexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    //     depthTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    //
    //     D3D12_HEAP_PROPERTIES depthTexHeapProps = {};
    //     depthTexHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    //
    //     THROW_IF_FAILED(m_device->CreateCommittedResource(
    //         &depthTexHeapProps,
    //         D3D12_HEAP_FLAG_NONE,
    //         &depthTexDesc,
    //         D3D12_RESOURCE_STATE_COPY_DEST,
    //         nullptr,
    //         IID_ID3D12Resource,
    //         &m_depthStencilBuffer));
    //
    //     m_depthStencilBuffer->SetName(L"Depth Stencil Buffer");
    //
    //     m_device->CreateDepthStencilView(
    //         m_depthStencilBuffer.Get(),
    //         nullptr,
    //         m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    //
    //     AddCommand([this](ID3D12GraphicsCommandList* cmdList)
    //     {
    //         auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
    //             this->m_depthStencilBuffer.Get(),
    //             D3D12_RESOURCE_STATE_COPY_DEST,
    //             D3D12_RESOURCE_STATE_DEPTH_WRITE);
    //         cmdList->ResourceBarrier(1, &transition);
    //     });
    // }
}
