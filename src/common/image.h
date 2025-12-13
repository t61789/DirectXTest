#pragma once
#include "dx_texture.h"
#include "i_resource.h"

namespace dt
{
    enum class TextureFilterMode : uint8_t;
    enum class TextureWrapMode : uint8_t;

    class Image final : public IResource
    {
        struct ImportConfig
        {
            bool needFlipVertical = true;
            bool needMipmap = true;
            TextureWrapMode wrapMode = TextureWrapMode::CLAMP;
            TextureFilterMode filterMode = TextureFilterMode::BILINEAR;

            template <typename Archive>
            void serialize(Archive& ar, uint32_t const version);
        };
        
        struct ImageCache
        {
            DxTextureDesc desc;
            vec<uint8_t> data;

            template <typename Archive>
            void serialize(Archive& ar, uint32_t const version);
        };
        
    public:
        DxTexture* GetDxTexture() { return m_dxTexture.get();}
        cr<StringHandle> GetPath() override { return m_path;}
        uint32_t GetSrvDescIndex();

        static sp<Image> LoadFromFile(cr<StringHandle> path);

        static ImageCache CreateCacheFromAsset(crstr assetPath);
        static sp<Image> CreateAssetFromCache(ImageCache&& cache);

    private:
        static ImportConfig LoadImageImportConfig(crstr assetPath);
        
        StringHandle m_path = {};
        
        sp<DxTexture> m_dxTexture = nullptr;
    };

    template <typename Archive>
    void Image::ImportConfig::serialize(Archive& ar, uint32_t const version)
    {
        ar & needFlipVertical;
        ar & needMipmap;

        auto w = static_cast<uint8_t>(wrapMode);
        ar & w;
        wrapMode = static_cast<TextureWrapMode>(w);

        auto f = static_cast<uint8_t>(filterMode);
        ar & f;
        filterMode = static_cast<TextureFilterMode>(f);
    }

    template <typename Archive>
    void Image::ImageCache::serialize(Archive& ar, uint32_t const version)
    {
        ar & desc;
        ar & data;
    }
}
