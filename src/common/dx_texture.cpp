#include "dx_texture.h"

#include <directx/d3dx12_core.h>

#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/dx_resource.h"

namespace dt
{
    uint32_t DxTextureDesc::GetMipmapCount() const
    {
        return hasMipmap ? (std::min)(Utils::Log2(width) + 1, Utils::Log2(height) + 1) : 1;
    }

    sp<DxTexture> DxTexture::CreateImage(cr<DxTextureDesc> desc)
    {
        ASSERT_THROW(desc.width > 0 && desc.height > 0);

        auto dxResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            GetDxgiFormat(desc.format),
            desc.width,
            desc.height,
            desc.type == TextureType::TEXTURE_CUBE ? 6 : 1,
            (std::min)(Utils::Log2(desc.width) + 1, Utils::Log2(desc.height) + 1),
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
        assert(desc.type == TextureType::TEXTURE_2D);
        
        auto dxResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            GetDxgiFormat(desc.format),
            desc.width,
            desc.height,
            1,
            1,
            1,
            0,
            GetRenderTargetResourceFlag(desc.format));

        D3D12_CLEAR_VALUE clearValue;
        clearValue.Format = GetClearFormat(desc.format);
        clearValue.Color[0] = clearColor.x;
        clearValue.Color[1] = clearColor.y;
        clearValue.Color[2] = clearColor.z;
        clearValue.Color[3] = clearColor.w;
        clearValue.DepthStencil.Depth = clearColor.x;
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
                GetInitState(desc.format),
                &clearValue,
                D3D12_HEAP_FLAG_NONE,
                L"Image");
        }

        auto result = msp<DxTexture>();
        result->m_desc = desc;
        result->m_dxResource = resultDxResource;

        return result;
    }
    
    DXGI_FORMAT DxTexture::GetDxgiFormat(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::RGBA:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA16:
                return DXGI_FORMAT_R16G16B16A16_UNORM;
            case TextureFormat::R32:
                return DXGI_FORMAT_R32_FLOAT;
            case TextureFormat::DEPTH:
                return DXGI_FORMAT_D32_FLOAT;
            case TextureFormat::DEPTH_STENCIL:
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            case TextureFormat::SHADOW_MAP:
                return DXGI_FORMAT_R32_TYPELESS;
            default:
                THROW_ERROR("Invalid texture format");
        }
    }

    D3D12_RESOURCE_STATES DxTexture::GetInitState(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::RGBA:
            case TextureFormat::RGBA16:
            case TextureFormat::R32:
            case TextureFormat::SHADOW_MAP:
                return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case TextureFormat::DEPTH:
            case TextureFormat::DEPTH_STENCIL:
                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            default:
                THROW_ERROR("Invalid texture format");
        }
    }

    D3D12_RESOURCE_FLAGS DxTexture::GetRenderTargetResourceFlag(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::RGBA:
            case TextureFormat::RGBA16:
            case TextureFormat::R32:
                return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            case TextureFormat::DEPTH:
            case TextureFormat::DEPTH_STENCIL:
            case TextureFormat::SHADOW_MAP:
                return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            default:
                THROW_ERROR("Invalid texture format");
        }
    }

    D3D12_SRV_DIMENSION DxTexture::GetSrvDimension(const TextureType type)
    {
        switch (type)
        {
            case TextureType::TEXTURE_2D:
                return D3D12_SRV_DIMENSION_TEXTURE2D;
            case TextureType::TEXTURE_CUBE:
                return D3D12_SRV_DIMENSION_TEXTURECUBE;
            default:
                THROW_ERROR("Invalid texture type")
        }
    }

    DXGI_FORMAT DxTexture::GetDsvFormat(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::SHADOW_MAP:
                return DXGI_FORMAT_D32_FLOAT;
            default:
                return GetDxgiFormat(format);
        }
    }
    
    DXGI_FORMAT DxTexture::GetSrvFormat(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::SHADOW_MAP:
                return DXGI_FORMAT_R32_FLOAT;
            default:
                return GetDxgiFormat(format);
        }
    }
    
    DXGI_FORMAT DxTexture::GetClearFormat(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::SHADOW_MAP:
                return DXGI_FORMAT_D32_FLOAT;
            default:
                return GetDxgiFormat(format);
        }
    }
}
