#pragma once
#include <dxgi.h>
#include <wrl/client.h>

#include "common/const.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class DirectX
    {
    public:
        DirectX() = default;
        
        void Init();
        void Release();

    private:
        void LoadFactory();
        void LoadAdapter();
        void LoadOutput();
        void LoadOutputModes();
        
        ComPtr<IDXGIFactory> m_dxgiFactor;
        ComPtr<IDXGIAdapter> m_dxgiAdapter;
        ComPtr<IDXGIOutput> m_dxgiOutput;
        DXGI_ADAPTER_DESC m_dxgiAdapterDesc;
        DXGI_OUTPUT_DESC m_dxgiOutputDesc;
        vec<DXGI_MODE_DESC> m_dxgiOutputModes;
    };
}
