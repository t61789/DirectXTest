#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <directx/d3dx12_resource_helpers.h>
#include <tracy/Tracy.hpp>

#include "dx_texture.h"
#include "utils.h"
#include "stb_image.h"
#include "common/asset_cache.h"
#include "game/game_resource.h"
#include "render/descriptor_pool.h"
#include "render/directx.h"
#include "render/dx_helper.h"
#include "render/dx_resource.h"
#include "render/render_thread.h"

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

    Image::ImageCache Image::CreateCacheFromAsset(crstr assetPath)
    {
        auto importConfig = LoadImageImportConfig(assetPath);

        stbi_set_flip_vertically_on_load(importConfig.needFlipVertical);
        
        int width = 0, height = 0, nChannels = 4;
        stbi_uc* data;
        try
        {
            ZoneScopedN("Load Image Data");
            
            data = stbi_load(Utils::ToAbsPath(assetPath).c_str(), &width, &height, &nChannels, 0);
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

        auto sizeB = width * height * nChannels;
        vec<uint8_t> dataVec(sizeB);
        memcpy(dataVec.data(), data, sizeB);

        stbi_image_free(data);

        DxTextureDesc dxTextureDesc;
        dxTextureDesc.type = TextureType::TEXTURE_2D;
        dxTextureDesc.format = nChannels == 4 ? TextureFormat::RGBA : TextureFormat::RGB;
        dxTextureDesc.width = width;
        dxTextureDesc.height = height;
        dxTextureDesc.channelCount = nChannels;
        dxTextureDesc.wrapMode = importConfig.wrapMode;
        dxTextureDesc.filterMode = importConfig.filterMode;
        dxTextureDesc.hasMipmap = importConfig.needMipmap;

        ImageCache cache;
        cache.desc = dxTextureDesc;
        cache.data = std::move(dataVec);

        return std::move(cache);
    }

    sp<Image> Image::CreateAssetFromCache(ImageCache&& cache)
    {
        auto dxTexture = DxTexture::CreateImage(cache.desc);

        RT()->AddCmd([dxTexture, cache=std::move(cache)](ID3D12GraphicsCommandList* cmdList, RenderThreadContext& context)
        {
            auto dxTextureResourceDesc = dxTexture->GetDxResource()->GetDesc();
            
            size_t uploadBytes;
            Dx()->GetDevice()->GetCopyableFootprints(
                &dxTextureResourceDesc,
                0,
                1,
                0,
                nullptr,
                nullptr,
                nullptr,
                &uploadBytes);
            
            auto uploadBuffer = DxResource::CreateUploadBuffer(nullptr, uploadBytes);
            
            D3D12_SUBRESOURCE_DATA srcData;
            srcData.pData = cache.data.data();
            srcData.RowPitch = cache.desc.width * cache.desc.channelCount;
            srcData.SlicePitch = srcData.RowPitch * cache.desc.height;
            
            THROW_IF_FAILED(UpdateSubresources(
                cmdList,
                dxTexture->GetDxResource()->GetResource(),
                uploadBuffer->GetResource(),
                0,
                0,
                1,
                &srcData));
            
            DxHelper::AddTransition(context, dxTexture->GetDxResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        });

        auto result = msp<Image>();
        result->m_dxTexture = dxTexture;

        return result;
    }
}
