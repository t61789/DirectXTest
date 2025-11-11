#pragma once
#include <cstdint>

#include "common/const.h"
#include "common/string_handle.h"

namespace dt
{
    enum class CullMode : uint8_t
    {
        BACK,
        FRONT,
        NONE
    };

    enum class BlendMode : uint8_t
    {
        NONE,
        BLEND,
        ADD
    };

    enum class DepthMode : uint8_t
    {
        LESS_EQUAL,
        LESS,
        ALWAYS,
        EQUAL,
        NOT_EQUAL,
        GREATER,
        GREATER_EQUAL
    };

    class RenderState
    {
    public:
        static CullMode GetCullMode(cr<StringHandle> s);
        static BlendMode GetBlendMode(cr<StringHandle> s);
        static DepthMode GetDepthMode(cr<StringHandle> s);
    };
}
