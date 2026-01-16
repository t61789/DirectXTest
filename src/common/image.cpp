#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <directx/d3dx12_resource_helpers.h>
#include <tracy/Tracy.hpp>

#include "dx_texture.h"
#include "utils.h"
#include "stb_image.h"
#include "common/math.h"
#include "common/asset_cache.h"
#include "game/game_resource.h"
#include "render/descriptor_pool.h"
#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/dx_resource.h"
#include "render/render_thread.h"

namespace
{
    DirectX::XMFLOAT4 LoadPixel(dt::crvec<uint8_t> mip, const uint32_t x, uint32_t y, const uint32_t width, const uint32_t height)
    {
        return DirectX::XMFLOAT4(
            static_cast<float>(mip[x * 4 + y * width * 4 + 0]) / 255.0f,
            static_cast<float>(mip[x * 4 + y * width * 4 + 1]) / 255.0f,
            static_cast<float>(mip[x * 4 + y * width * 4 + 2]) / 255.0f,
            static_cast<float>(mip[x * 4 + y * width * 4 + 3]) / 255.0f
        );
    }

    void StorePixel(dt::vec<uint8_t>& mip, const uint32_t x, uint32_t y, const uint32_t width, const uint32_t height, const DirectX::XMFLOAT4& pixel)
    {
        mip[x * 4 + y * width * 4 + 0] = static_cast<uint8_t>(pixel.x * 255.0f);
        mip[x * 4 + y * width * 4 + 1] = static_cast<uint8_t>(pixel.y * 255.0f);
        mip[x * 4 + y * width * 4 + 2] = static_cast<uint8_t>(pixel.z * 255.0f);
        mip[x * 4 + y * width * 4 + 3] = static_cast<uint8_t>(pixel.w * 255.0f);
    }
}

namespace dt
{
    sp<Image> Image::LoadFromFile(cr<StringHandle> path)
    {
        {
            if(auto result = GR()->GetResource<Image>(path))
            {
                return result;
            }
        }

        if (!Utils::AssetExists(path))
        {
            return GR()->errorTex;
        }

        auto result = AssetCache::GetFromCache<Image, ImageCache>(path);
        result->m_shaderResource = DescriptorPool::Ins()->AllocTextureSrv(result->m_dxTexture.get());
        
        GR()->RegisterResource(path, result);
        result->m_path = path;

        log_info("Load texture: %s", path.CStr());
        
        return result;
    }

    Image::ImportConfig Image::LoadImageImportConfig(crstr assetPath)
    {
        auto config = Utils::GetResourceMeta(assetPath);

        ImportConfig importConfig;

        try_get_val(config, "need_flip_vertical", importConfig.needFlipVertical);
        
        try_get_val(config, "need_mipmap", importConfig.needMipmap);

        try_get_val(config, "wrap_mode", importConfig.wrapMode);

        try_get_val(config, "filter_mode", importConfig.filterMode);

        return importConfig;
    }

    vec<vec<uint8_t>> Image::LoadTexture2dWithMipmaps(crstr path, uint32_t& width, uint32_t& height, ImportConfig& importConfig)
    {
        importConfig = LoadImageImportConfig(path);
        
        uint8_t* data = nullptr;
        uint32_t nChannels = 0;
        LoadTextureData(path, importConfig, data, width, height, nChannels);
        auto mipmaps = CreateMipmaps(data, width, height);
        FreeTextureData(data);

        return mipmaps;
    }

    vec<vec<uint8_t>> Image::LoadTextureCubeWithMipmaps(crstr path, uint32_t& width, uint32_t& height, ImportConfig& importConfig)
    {
        importConfig = LoadImageImportConfig(path);
        width = height = 0;

        vec<vec<uint8_t>> result;
        
        static constexpr arr<cstr, 6> FACES = {"right", "left", "top", "bottom", "front", "back"};
        for (uint32_t i = 0; i < FACES.size(); ++i)
        {
            auto facePath = path + "/" + FACES[i] + ".png";
            
            uint8_t* data = nullptr;
            uint32_t nChannels = 0;
            uint32_t faceWidth = 0, faceHeight = 0;
            LoadTextureData(facePath, importConfig, data, faceWidth, faceHeight, nChannels);
            if (width != 0 && (width != faceWidth || height != faceHeight))
            {
                THROW_ERROR("TextureCube faces size mismatch");
            }
            width = faceWidth;
            height = faceHeight;
            
            auto mipmaps = CreateMipmaps(data, width, height);
            FreeTextureData(data);

            result.insert(result.end(), mipmaps.begin(), mipmaps.end());
        }

        return result;
    }

    vec<vec<uint8_t>> Image::CreateMipmaps(const uint8_t* rawData, const uint32_t width, const uint32_t height)
    {
        ASSERT_THROWM(width > 0 && height > 0 && (width & (width - 1)) == 0 && (height & (height - 1)) == 0, "Resolution needs to be power of 2");

        vec<vec<uint8_t>> mipmaps;

        auto curWidth = width, curHeight = height;
        for (uint32_t i = 0; i < 32; ++i)
        {
            if (curWidth == 0 || curHeight == 0)
            {
                break;
            }

            vec<uint8_t> curMip(curWidth * curHeight * 4);

            if (i == 0)
            {
                memcpy(curMip.data(), rawData, curMip.size());
            }
            else
            {
                for (uint32_t y = 0; y < curHeight; ++y)
                {
                    for (uint32_t x = 0; x < curWidth; ++x)
                    {
                        auto preMipBaseX = x << 1;
                        auto preMipBaseY = y << 1;

                        auto p0 = Load(LoadPixel(mipmaps[i - 1], preMipBaseX, preMipBaseY, curWidth << 1, curHeight << 1));
                        auto p1 = Load(LoadPixel(mipmaps[i - 1], preMipBaseX + 1, preMipBaseY, curWidth << 1, curHeight << 1));
                        auto p2 = Load(LoadPixel(mipmaps[i - 1], preMipBaseX, preMipBaseY + 1, curWidth << 1, curHeight << 1));
                        auto p3 = Load(LoadPixel(mipmaps[i - 1], preMipBaseX + 1, preMipBaseY + 1, curWidth << 1, curHeight << 1));

                        auto resultPixel = Store4(XMVectorScale(
                            XMVectorAdd(
                                XMVectorAdd(p0, p1),
                                XMVectorAdd(p2, p3)),
                            0.25f));

                        StorePixel(curMip, x, y, curWidth, curHeight, resultPixel);
                    }
                }
            }
            mipmaps.push_back(std::move(curMip));
            
            curWidth = curWidth >> 1;
            curHeight = curHeight >> 1;
        }

        return mipmaps;
    }

    Image::ImageCache Image::CreateCacheFromAsset(crstr assetPath)
    {
        auto type = Utils::IsDir(assetPath) ? TextureType::TEXTURE_CUBE : TextureType::TEXTURE_2D;
        
        ImportConfig importConfig;
        vec<vec<uint8_t>> mipmaps;
        uint32_t width = 0, height = 0;

        if (type == TextureType::TEXTURE_CUBE)
        {
            mipmaps = LoadTextureCubeWithMipmaps(assetPath, width, height, importConfig);
        }
        else if (type == TextureType::TEXTURE_2D)
        {
            mipmaps = LoadTexture2dWithMipmaps(assetPath, width, height, importConfig);
        }
        else
        {
            THROW_ERROR("Unsupported texture type");
        }

        DxTextureDesc dxTextureDesc;
        dxTextureDesc.type = type;
        dxTextureDesc.format = TextureFormat::RGBA;
        dxTextureDesc.width = width;
        dxTextureDesc.height = height;
        dxTextureDesc.channelCount = 4;
        dxTextureDesc.wrapMode = importConfig.wrapMode;
        dxTextureDesc.filterMode = importConfig.filterMode;
        dxTextureDesc.hasMipmap = true;

        ImageCache cache;
        cache.desc = dxTextureDesc;
        cache.mipmaps = std::move(mipmaps);

        return std::move(cache);
    }

    sp<Image> Image::CreateAssetFromCache(ImageCache&& cache)
    {
        auto dxTexture = DxTexture::CreateImage(cache.desc);
        auto mipmapCount = cache.desc.GetMipmapCount();
        auto subResourcesCount = cache.mipmaps.size();
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(dxTexture->GetDxResource()->GetResource(), 0, subResourcesCount);
        auto uploadBuffer = DxResource::GetUploadBuffer(nullptr, uploadBufferSize);

        RT()->AddCmd([dxTexture, cache=std::move(cache), subResourcesCount, mipmapCount, uploadBuffer](ID3D12GraphicsCommandList* cmdList)
        {
            std::vector<D3D12_SUBRESOURCE_DATA> subresources(subResourcesCount);
            for (UINT i = 0; i < subResourcesCount; ++i)
            {
                auto curMipmapLevel = i % mipmapCount;
                
                subresources[i].pData = cache.mipmaps[i].data();
                subresources[i].RowPitch = (cache.desc.width >> curMipmapLevel) * cache.desc.channelCount;
                subresources[i].SlicePitch = subresources[i].RowPitch * cache.desc.height;
            }
            
            THROW_IF_FAILED(UpdateSubresources(
                cmdList,
                dxTexture->GetDxResource()->GetResource(),
                uploadBuffer->GetResource(),
                0,
                0,
                subResourcesCount,
                subresources.data()));
            
            DxHelper::AddTransition(dxTexture->GetDxResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        });

        auto result = msp<Image>();
        result->m_dxTexture = dxTexture;

        return result;
    }

    void Image::LoadTextureData(crstr path, const ImportConfig importConfig, uint8_t*& data, uint32_t& width, uint32_t& height, uint32_t& nChannels)
    {
        stbi_set_flip_vertically_on_load(importConfig.needFlipVertical);

        int w = 0, h = 0, c = 0;
        try
        {
            ZoneScopedN("Load Image Data");
            
            data = stbi_load(Utils::ToAbsPath(path).c_str(), &w, &h, &c, 4);
            if(!data)
            {
                throw std::exception();
            }
        }
        catch(cr<std::exception>)
        {
            log_info("Failed to load texture: %s", path.c_str());
            throw;
        }

        width = static_cast<uint32_t>(w);
        height = static_cast<uint32_t>(h);
        nChannels = static_cast<uint32_t>(c);
    }

    void Image::FreeTextureData(uint8_t* data)
    {
        stbi_image_free(data);
    }
}
