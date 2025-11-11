#include "render_state.h"

namespace dt
{
    CullMode RenderState::GetCullMode(cr<StringHandle> s)
    {
        static const umap<string_hash, CullMode> CULL_MODE_MAP =
        {
            {StringHandle("None"), CullMode::NONE},
            {StringHandle("Front"), CullMode::FRONT},
            {StringHandle("Back"), CullMode::BACK},
        };

        return CULL_MODE_MAP.at(s);
    }

    BlendMode RenderState::GetBlendMode(cr<StringHandle> s)
    {
        static const umap<string_hash, BlendMode> BLEND_MODE_MAP =
        {
            {StringHandle("None"), BlendMode::NONE},
            {StringHandle("Blend"), BlendMode::BLEND},
            {StringHandle("Add"), BlendMode::ADD},
        };

        return BLEND_MODE_MAP.at(s);
    }

    DepthMode RenderState::GetDepthMode(cr<StringHandle> s)
    {
        static const umap<string_hash, DepthMode> DEPTH_MODE_MAP =
        {
            {StringHandle("Always"), DepthMode::ALWAYS},
            {StringHandle("Less"), DepthMode::LESS},
            {StringHandle("LessEqual"), DepthMode::LESS_EQUAL},
            {StringHandle("Equal"), DepthMode::EQUAL},
            {StringHandle("NotEqual"), DepthMode::NOT_EQUAL},
            {StringHandle("Greater"), DepthMode::GREATER},
            {StringHandle("GreaterEqual"), DepthMode::GREATER_EQUAL}
        };

        return DEPTH_MODE_MAP.at(s);
    }
}
