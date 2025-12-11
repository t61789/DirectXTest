#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <tracy/Tracy.hpp>

#include "dx_texture.h"
#include "utils.h"
#include "stb_image.h"
#include "common/asset_cache.h"
#include "game/game_resource.h"
#include "render/desc_handle_pool.h"
#include "render/dx_helper.h"
#include "render/dx_resource.h"

namespace dt
{
    Image::ImageCache Image::ImageCache::Create(const Image* image)
    {
        ImageCache cache;
        
        cache.desc = image->m_dxTexture->GetDesc();
        cache.importConfig = image->m_importConfig;

        return cache;
    }

    uint32_t Image::GetSrvDescIndex()
    {
        return m_dxTexture->GetSrvDesc()->GetIndex();
    }

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

    sp<Image> Image::LoadFromFileImp(cr<StringHandle> path)
    {
        if (!Utils::AssetExists(path))
        {
            return GR()->errorTex;
        }

        auto importConfig = LoadImageImportConfig(path);

        stbi_set_flip_vertically_on_load(importConfig.needFlipVertical);
        
        int width = 0, height = 0, nChannels = 4;
        stbi_uc* data;
        try
        {
            ZoneScopedN("Load Image Data");
            
            data = stbi_load(Utils::ToAbsPath(path).c_str(), &width, &height, &nChannels, 0);
            if(!data)
            {
                throw std::exception();
            }
        }
        catch(cr<std::exception>)
        {
            log_info("Failed to load texture: %s", path.CStr());
            return GR()->errorTex;
        }

        auto sizeB = width * height * nChannels;
        vec<uint8_t> dataVec(sizeB);
        memcpy(dataVec.data(), data, sizeB);

        stbi_image_free(data);

        DxTextureDesc dxTextureDesc;
        dxTextureDesc.type = TextureType::TEXTURE_2D;
        dxTextureDesc.format = nChannels == 4 ? TextureFormat::RGBA : TextureFormat::RGB;
        dxTextureDesc.width = width;
        dxTextureDesc.height = height;
        dxTextureDesc.wrapMode = importConfig.wrapMode;
        dxTextureDesc.filterMode = importConfig.filterMode;
        dxTextureDesc.hasMipmap = importConfig.needMipmap;
        auto dxTexture = DxTexture::CreateImage(dxTextureDesc);
        dxTexture->GetResource()->Upload(dataVec.data(), sizeB);
        DxHelper::AddTransition(dxTexture->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        auto result = msp<Image>();
        result->m_width = width;
        result->m_height = height;
        result->m_dxTexture = dxTexture;
        
        result->m_channels = nChannels;
        result->m_data = std::move(dataVec);
        result->m_importConfig = importConfig;

        return result;
    }

    Image::ImageCache Image::CreateCacheFromAsset(crstr assetPath)
    {
        auto asset = LoadFromFileImp(assetPath);

        return ImageCache::Create(asset.get());
    }

    sp<Image> Image::CreateAssetFromCache(ImageCache&& cache)
    {
        auto result = msp<Image>();
        result->m_width = cache.desc.width;
        result->m_height = cache.desc.height;
        result->m_dxTexture = DxTexture::CreateImage(cache.desc);

        return result;
    }
}
