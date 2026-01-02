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
        XMFLOAT4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    class RenderTexture : public ITexture
    {
    public:
        explicit RenderTexture(cr<RenderTextureDesc> desc, crsp<DxResource> dxResource = nullptr);

        XMINT2 GetSize() override { return m_dxTexture->GetSize(); }
        DxTexture* GetDxTexture() override { return m_dxTexture.get(); }
        uint32_t GetSrvDescIndex() override { return m_shaderResource->GetSrvIndex(); }
        uint32_t GetSamplerDescIndex() override { return m_shaderResource->GetSamplerIndex(); }
        cr<XMFLOAT4> GetClearColor() { return m_desc.clearColor; }

    private:
        RenderTextureDesc m_desc;
        sp<DxTexture> m_dxTexture;
        sp<ShaderResource> m_shaderResource;
    };
}
