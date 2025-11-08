#include "directx.h"

#include <stdexcept>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <initguid.h>
#include <dxgi.h>

#include "common/utils.h"

namespace dt
{
    void DirectX::Init()
    {
        LoadFactory();
        LoadAdapter();
        LoadOutput();
        LoadOutputModes();
    }

    void DirectX::Release()
    {
    }

    void DirectX::LoadFactory()
    {
        auto result = CreateDXGIFactory(IID_IDXGIFactory, &m_dxgiFactor);
        THROW_IF(FAILED(result), "Failed to create DXGI factory.");
    }

    void DirectX::LoadAdapter()
    {
        for (uint32_t i = 0;; i++)
        {
            auto result = m_dxgiFactor->EnumAdapters(i, &m_dxgiAdapter);
            THROW_IF(result == DXGI_ERROR_NOT_FOUND, "Failed to find DXGI adapter.");

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
            auto result = m_dxgiAdapter->EnumOutputs(i, &m_dxgiOutput);
            THROW_IF(result == DXGI_ERROR_NOT_FOUND, "Failed to find DXGI output.");

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
        
        result = m_dxgiOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &count, m_dxgiOutputModes.data());
        THROW_IF(FAILED(result), "Failed to get display mode list.");
        
        auto& mode = m_dxgiOutputModes.back();
        log_info(
            "Max display mode: size %dx%d, refresh rate %f, scaling %d",
            mode.Width, mode.Height,
            static_cast<float>(mode.RefreshRate.Numerator) / static_cast<float>(mode.RefreshRate.Denominator),
            mode.Scaling);
    }
}
