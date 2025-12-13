#include "dx_texture.h"

#include <directx/d3dx12_core.h>

#include "render/directx.h"
#include "render/dx_resource.h"

namespace
{
    DXGI_FORMAT ToDxgiFormat(const dt::TextureFormat format)
    {
        static const dt::umap<dt::TextureFormat, DXGI_FORMAT> FORMAT_MAP =
        {
            { dt::TextureFormat::RGBA, DXGI_FORMAT_R8G8B8A8_UNORM },
            { dt::TextureFormat::RGB, DXGI_FORMAT_R8G8B8A8_UNORM },
        };

        return FORMAT_MAP.at(format);
    }
}

namespace dt
{
    sp<DxTexture> DxTexture::CreateImage(cr<DxTextureDesc> desc)
    {
        ASSERT_THROW(desc.width > 0 && desc.height > 0);

        auto dxResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            ToDxgiFormat(desc.format),
            desc.width,
            desc.height,
            1,
            1,
            1,
            0,
            D3D12_RESOURCE_FLAG_NONE);

        auto dxResource = DxResource::Create(
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            dxResourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            D3D12_HEAP_FLAG_NONE,
            L"Image");

        auto srvDescHandle = SrvDescPool::Ins()->Alloc();
        srvDescHandle->GetCpuHandle();
        Dx()->GetDevice()->CreateShaderResourceView(dxResource->GetResource(), nullptr, srvDescHandle->GetCpuHandle());

        auto result = msp<DxTexture>();
        result->m_desc = desc;
        result->m_srvDescHandle = srvDescHandle;
        result->m_dxResource = dxResource;

        return result;
    }
}
