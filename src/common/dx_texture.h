#pragma once
#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "const.h"
#include "render/texture_format.h"

namespace dt
{
    class DxResource;
    struct SrvDesc;
    using namespace Microsoft::WRL;
    using namespace DirectX;
    
    struct DxTextureDesc
    {
        TextureType type = TextureType::TEXTURE_2D;
        TextureFormat format = TextureFormat::RGBA;
        TextureWrapMode wrapMode = TextureWrapMode::CLAMP;
        TextureFilterMode filterMode = TextureFilterMode::BILINEAR;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channelCount = 0;
        uint32_t hasMipmap = false;
        const wchar_t* name = nullptr;

        template <typename Archive>
        void serialize(Archive& ar, uint32_t const version);

        uint32_t GetMipmapCount() const;
    };

    class DxTexture
    {
    public:
        crsp<DxResource> GetDxResource() const { return m_dxResource; }
        DxTextureDesc GetDesc() const { return m_desc; }
        XMINT2 GetSize() const { return XMINT2(m_desc.width, m_desc.height); }
        
        static sp<DxTexture> CreateImage(cr<DxTextureDesc> desc);
        static sp<DxTexture> CreateRenderTexture(cr<DxTextureDesc> desc, cr<XMFLOAT4> clearColor, crsp<DxResource> dxResource = nullptr);

    private:
        DxTextureDesc m_desc;
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
