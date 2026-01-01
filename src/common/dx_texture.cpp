#include "dx_texture.h"

#include <directx/d3dx12_core.h>

#include "render/directx.h"
#include "render/dx_resource.h"

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

        auto result = msp<DxTexture>();
        result->m_desc = desc;
        result->m_dxResource = dxResource;

        return result;
    }

    sp<DxTexture> DxTexture::CreateRenderTexture(cr<DxTextureDesc> desc, cr<XMFLOAT4> clearColor, crsp<DxResource> dxResource)
    {
        auto dxResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            ToDxgiFormat(desc.format),
            desc.width,
            desc.height,
            1,
            1,
            1,
            0,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        D3D12_CLEAR_VALUE clearValue;
        clearValue.Format = dxResourceDesc.Format;
        clearValue.Color[0] = clearColor.x;
        clearValue.Color[1] = clearColor.y;
        clearValue.Color[2] = clearColor.z;
        clearValue.Color[3] = clearColor.w;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        sp<DxResource> resultDxResource;
        if (dxResource)
        {
            resultDxResource = dxResource;
        }
        else
        {
            resultDxResource = DxResource::Create(
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                dxResourceDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &clearValue,
                D3D12_HEAP_FLAG_NONE,
                L"Image");
        }

        auto result = msp<DxTexture>();
        result->m_desc = desc;
        result->m_dxResource = resultDxResource;

        return result;
    }

    DXGI_FORMAT DxTexture::ToDxgiFormat(const TextureFormat format)
    {
        static const umap<TextureFormat, DXGI_FORMAT> FORMAT_MAP =
        {
            { TextureFormat::RGBA, DXGI_FORMAT_R8G8B8A8_UNORM },
            { TextureFormat::RGB, DXGI_FORMAT_R8G8B8A8_UNORM },
            { TextureFormat::Depth, DXGI_FORMAT_D32_FLOAT },
            { TextureFormat::DepthStencil, DXGI_FORMAT_D32_FLOAT_S8X24_UINT },
        };

        return FORMAT_MAP.at(format);
    }
}
