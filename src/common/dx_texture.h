#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

#include "const.h"

namespace dt
{
    class DxResource;
    struct SrvDesc;
    using namespace Microsoft::WRL;
    
    enum class TextureType : uint8_t
    {
        TEXTURE_2D
    };

    enum class TextureFormat : uint8_t
    {
        RGBA,
        RGB
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
    
    struct DxTextureDesc
    {
        TextureType type = TextureType::TEXTURE_2D;
        TextureFormat format = TextureFormat::RGBA;
        TextureWrapMode wrapMode = TextureWrapMode::CLAMP;
        TextureFilterMode filterMode = TextureFilterMode::BILINEAR;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channelCount = 0;
        bool hasMipmap = false;

        template <typename Archive>
        void serialize(Archive& ar, uint32_t const version);
    };

    class DxTexture
    {
    public:
        SrvDesc* GetSrvDesc() const { return m_srvDescHandle.get(); }
        crsp<DxResource> GetResource() const { return m_dxResource; }
        DxTextureDesc GetDesc() const { return m_desc; }
        
        static sp<DxTexture> CreateImage(cr<DxTextureDesc> desc);
        
    private:
        DxTextureDesc m_desc;
        sp<SrvDesc> m_srvDescHandle;
        sp<DxResource> m_dxResource;
    };

    template <typename Archive>
    void DxTextureDesc::serialize(Archive& ar, uint32_t const version)
    {
        auto t = static_cast<uint8_t>(type);
        ar & t;
        type = static_cast<TextureType>(t);
        
        auto f = static_cast<uint8_t>(format);
        ar & f;
        format = static_cast<TextureFormat>(f);

        auto w = static_cast<uint8_t>(wrapMode);
        ar & w;
        wrapMode = static_cast<TextureWrapMode>(w);

        auto fm = static_cast<uint8_t>(filterMode);
        ar & fm;
        filterMode = static_cast<TextureFilterMode>(fm);

        ar & width;
        ar & height;
        ar & channelCount;
        ar & hasMipmap;
    }
}
