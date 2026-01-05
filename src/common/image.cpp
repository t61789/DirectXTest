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
        result->m_shaderResource = DescriptorPool::Ins()->AllocSrv(result->m_dxTexture.get());
        
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
        auto importConfig = LoadImageImportConfig(assetPath);

        stbi_set_flip_vertically_on_load(importConfig.needFlipVertical);
        
        int width = 0, height = 0, nChannels = 4;
        stbi_uc* data;
        try
        {
            ZoneScopedN("Load Image Data");
            
            data = stbi_load(Utils::ToAbsPath(assetPath).c_str(), &width, &height, &nChannels, 4);
            if(!data)
            {
                throw std::exception();
            }
        }
        catch(cr<std::exception>)
        {
            log_info("Failed to load texture: %s", assetPath.c_str());
            throw;
        }

        auto mipmaps = CreateMipmaps(data, width, height);

        stbi_image_free(data);

        DxTextureDesc dxTextureDesc;
        dxTextureDesc.type = TextureType::TEXTURE_2D;
        dxTextureDesc.format = TextureFormat::RGBA;
        dxTextureDesc.width = width;
        dxTextureDesc.height = height;
        dxTextureDesc.channelCount = 4;
        dxTextureDesc.wrapMode = importConfig.wrapMode;
        dxTextureDesc.filterMode = importConfig.filterMode;
        dxTextureDesc.hasMipmap = importConfig.needMipmap;

        ImageCache cache;
        cache.desc = dxTextureDesc;
        cache.mipmaps = std::move(mipmaps);

        return std::move(cache);
    }

    sp<Image> Image::CreateAssetFromCache(ImageCache&& cache)
    {
        auto dxTexture = DxTexture::CreateImage(cache.desc);

        RT()->AddCmd([dxTexture, cache=std::move(cache)](ID3D12GraphicsCommandList* cmdList)
        {
            auto mipLevels = cache.mipmaps.size();
            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(dxTexture->GetDxResource()->GetResource(), 0, mipLevels);
            
            auto uploadBuffer = DxResource::CreateUploadBuffer(nullptr, uploadBufferSize);

            std::vector<D3D12_SUBRESOURCE_DATA> subresources(mipLevels);
            for (UINT i = 0; i < mipLevels; ++i)
            {
                subresources[i].pData = cache.mipmaps[i].data();
                subresources[i].RowPitch = (cache.desc.width >> i) * cache.desc.channelCount;
                subresources[i].SlicePitch = subresources[i].RowPitch * cache.desc.height;
            }
            
            THROW_IF_FAILED(UpdateSubresources(
                cmdList,
                dxTexture->GetDxResource()->GetResource(),
                uploadBuffer->GetResource(),
                0,
                0,
                mipLevels,
                subresources.data()));
            
            DxHelper::AddTransition(dxTexture->GetDxResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        });

        auto result = msp<Image>();
        result->m_dxTexture = dxTexture;

        return result;
    }
}
