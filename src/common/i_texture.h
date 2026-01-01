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
        virtual XMINT2 GetSize() = 0;
    };
}
