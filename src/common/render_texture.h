#pragma once
#include "const.h"
#include "dx_texture.h"
#include "i_texture.h"
#include "render/descriptor_pool.h"

namespace dt
{
    struct RenderTextureDesc
    {
        DxTextureDesc dxDesc;
        XMFLOAT4 clearColor;
    };

    class RenderTexture : public ITexture
    {
    public:
        explicit RenderTexture(cr<RenderTextureDesc> desc, crsp<DxResource> dxResource = nullptr);

        XMINT2 GetSize() override { return m_dxTexture->GetSize(); }
        DxTexture* GetDxTexture() override { return m_dxTexture.get(); }
        uint32_t GetSrvDescIndex() override { return m_shaderResource->GetSrvIndex(); }
        uint32_t GetSamplerDescIndex() override { return m_shaderResource->GetSamplerIndex(); }

    private:
        sp<DxTexture> m_dxTexture;
        sp<ShaderResource> m_shaderResource;
    };
}
