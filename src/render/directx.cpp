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
#include "render_target.h"
#include "render_thread.h"
#include "window.h"
#include "common/utils.h"
#include "common/math.h"
#include "common/render_texture.h"

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
        CreateSwapChain();
        
        m_recycleBin = new RecycleBin();
        m_descriptorPool = new DescriptorPool();
        m_renderThread = new RenderThreadMgr();

        CreateSwapChainTextures();
    }

    DirectX::~DirectX()
    {
        m_swapChainRenderTargets.clear();
        m_swapChainRenderTextures.clear();
        
        delete m_renderThread;
        delete m_descriptorPool;
        m_recycleBin->Flush();
        delete m_recycleBin;
        
        m_dxgiSwapChain3.Reset();
        m_device.Reset();
        m_commandQueue.Reset();
        m_dxgiOutput.Reset();
        m_dxgiAdapter1.Reset();
        m_dxgiFactor4.Reset();
        m_debugLayer.Reset();
    }

    void DirectX::PresentSwapChain()
    {
        THROW_IF_FAILED(m_dxgiSwapChain3->Present(1, 0));
        m_backBufferIndex = m_dxgiSwapChain3->GetCurrentBackBufferIndex();
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
        auto format = TextureFormat::RGBA;
        auto dxFormat = DxTexture::GetDxgiFormat(format);
        
        m_swapChainDesc1 = {};
        m_swapChainDesc1.Width = 1600;
        m_swapChainDesc1.Height = 900;
        m_swapChainDesc1.Format = dxFormat;
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
    }

    void DirectX::CreateSwapChainTextures()
    {
        m_swapChainRenderTextures.resize(m_swapChainDesc1.BufferCount);
        m_swapChainRenderTargets.resize(m_swapChainDesc1.BufferCount);
        for (uint32_t i = 0; i < m_swapChainDesc1.BufferCount; i++)
        {
            ComPtr<ID3D12Resource> buffer;
            THROW_IF_FAILED(m_dxgiSwapChain3->GetBuffer(i, IID_ID3D12Resource, &buffer));

            auto dxResource = DxResource::Create(buffer, D3D12_RESOURCE_STATE_PRESENT, L"Swap Cain Buffer");

            RenderTextureDesc rtDesc;
            rtDesc.dxDesc.width = m_swapChainDesc1.Width;
            rtDesc.dxDesc.height = m_swapChainDesc1.Height;
            rtDesc.dxDesc.channelCount = 4;
            rtDesc.dxDesc.format = TextureFormat::RGBA;
            rtDesc.dxDesc.wrapMode = TextureWrapMode::CLAMP;
            rtDesc.dxDesc.filterMode = TextureFilterMode::BILINEAR;
            rtDesc.dxDesc.hasMipmap = false;
            rtDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_swapChainRenderTextures[i] = msp<RenderTexture>(rtDesc, dxResource);

            m_swapChainRenderTargets[i] = RenderTarget::Create({ m_swapChainRenderTextures[i] }, {});
        }
    }
}
