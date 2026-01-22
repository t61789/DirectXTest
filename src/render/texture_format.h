#pragma once
#include <cstdint>
#include <d3d12.h>
#include <dxgiformat.h>

#include "common/const.h"

namespace dt
{
    enum class TextureType : uint8_t
    {
        TEXTURE_2D,
        TEXTURE_CUBE
    };

    enum class TextureFormat : uint8_t
    {
        RGBA,
        RGBA_S,
        RGBA16,
        R32,
        DEPTH,
        DEPTH_STENCIL,
        SHADOW_MAP
    };

    enum class TextureWrapMode : uint8_t
    {
        REPEAT,
        CLAMP
    };

    enum class TextureFilterMode : uint8_t
    {
        NEAREST,
        BILINEAR
    };

    struct TextureFormatInfo
    {
        TextureFormat format;
        
        DXGI_FORMAT dxgiFormat;
        DXGI_FORMAT rtvFormat;
        DXGI_FORMAT dsvFormat;
        DXGI_FORMAT srvFormat;
        DXGI_FORMAT clearFormat;
        
        D3D12_RESOURCE_STATES initState;
        D3D12_RESOURCE_FLAGS renderTargetResourceFlag;
    };

    struct TextureTypeInfo
    {
        TextureType type;

        D3D12_SRV_DIMENSION srvDimension;
        D3D12_RTV_DIMENSION rtvDimension;
        D3D12_DSV_DIMENSION dsvDimension;
        D3D12_RESOURCE_DIMENSION resourceDimension;
    };

    inline cr<TextureFormatInfo> GetTextureFormatInfo(const TextureFormat format)
    {
        static umap<TextureFormat, TextureFormatInfo> kTextureFormatInfo;
        if (kTextureFormatInfo.empty())
        {
            TextureFormatInfo info;
            
            info.format = TextureFormat::RGBA;
            info.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            info.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            info.dsvFormat = DXGI_FORMAT_UNKNOWN;
            info.srvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            info.clearFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            info.initState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            info.renderTargetResourceFlag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            kTextureFormatInfo.emplace(info.format, info);

            info.format = TextureFormat::RGBA_S;
            info.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            info.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            info.dsvFormat = DXGI_FORMAT_UNKNOWN;
            info.srvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            info.clearFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            info.initState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            info.renderTargetResourceFlag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            kTextureFormatInfo.emplace(info.format, info);

            info.format = TextureFormat::RGBA16;
            info.dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
            info.rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
            info.dsvFormat = DXGI_FORMAT_UNKNOWN;
            info.srvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
            info.clearFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
            info.initState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            info.renderTargetResourceFlag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            kTextureFormatInfo.emplace(info.format, info);

            info.format = TextureFormat::R32;
            info.dxgiFormat = DXGI_FORMAT_R32_FLOAT;
            info.rtvFormat = DXGI_FORMAT_R32_FLOAT;
            info.dsvFormat = DXGI_FORMAT_UNKNOWN;
            info.srvFormat = DXGI_FORMAT_R32_FLOAT;
            info.clearFormat = DXGI_FORMAT_R32_FLOAT;
            info.initState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            info.renderTargetResourceFlag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            kTextureFormatInfo.emplace(info.format, info);

            info.format = TextureFormat::DEPTH;
            info.dxgiFormat = DXGI_FORMAT_D32_FLOAT;
            info.rtvFormat = DXGI_FORMAT_UNKNOWN;
            info.dsvFormat = DXGI_FORMAT_D32_FLOAT;
            info.srvFormat = DXGI_FORMAT_UNKNOWN;
            info.clearFormat = DXGI_FORMAT_D32_FLOAT;
            info.initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            info.renderTargetResourceFlag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            kTextureFormatInfo.emplace(info.format, info);

            info.format = TextureFormat::DEPTH_STENCIL;
            info.dxgiFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            info.rtvFormat = DXGI_FORMAT_UNKNOWN;
            info.dsvFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            info.srvFormat = DXGI_FORMAT_UNKNOWN;
            info.clearFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            info.initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            info.renderTargetResourceFlag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            kTextureFormatInfo.emplace(info.format, info);

            info.format = TextureFormat::SHADOW_MAP;
            info.dxgiFormat = DXGI_FORMAT_R32_TYPELESS;
            info.rtvFormat = DXGI_FORMAT_UNKNOWN;
            info.dsvFormat = DXGI_FORMAT_D32_FLOAT;
            info.srvFormat = DXGI_FORMAT_R32_FLOAT;
            info.clearFormat = DXGI_FORMAT_D32_FLOAT;
            info.initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            info.renderTargetResourceFlag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            kTextureFormatInfo.emplace(info.format, info);
        }

        return kTextureFormatInfo.at(format);
    }

    inline cr<TextureTypeInfo> GetTextureTypeInfo(const TextureType type)
    {
        static umap<TextureType, TextureTypeInfo> kTextureTypeInfo;
        if (kTextureTypeInfo.empty())
        {
            TextureTypeInfo info;

            info.type = TextureType::TEXTURE_2D;
            info.srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            info.rtvDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            info.dsvDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            info.resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            kTextureTypeInfo.emplace(info.type, info);

            info.type = TextureType::TEXTURE_CUBE;
            info.srvDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            info.rtvDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            info.dsvDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            info.resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            kTextureTypeInfo.emplace(info.type, info);
        }

        return kTextureTypeInfo.at(type);
    }
}
