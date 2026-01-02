#pragma once

#include <cstdint>

namespace dt
{
    class ITexture
    {
    public:
        ITexture() = default;
        virtual ~ITexture() = default;
        ITexture(const ITexture& other) = delete;
        ITexture(ITexture&& other) noexcept = delete;
        ITexture& operator=(const ITexture& other) = delete;
        ITexture& operator=(ITexture&& other) noexcept = delete;

        virtual DxTexture* GetDxTexture() = 0;
        virtual uint32_t GetSrvDescIndex() = 0;
        virtual uint32_t GetSamplerDescIndex() = 0;
        virtual XMINT2 GetSize() = 0;

        uint32_t GetTextureIndex()
        {
            uint32_t srvDescIndex = GetSrvDescIndex();
            uint32_t samplerDescIndex = GetSamplerDescIndex();

            constexpr uint32_t SRV_MASK = 0xFFFF;
            return (srvDescIndex & SRV_MASK) | (samplerDescIndex << 20);
        }
    };
}
