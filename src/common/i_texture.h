#pragma once

#include <cstdint>

namespace op
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

        virtual uint32_t GetWidth() = 0;
        virtual uint32_t GetHeight() = 0;
    };
}
