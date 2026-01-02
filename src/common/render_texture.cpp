#include "render_texture.h"

namespace dt
{
    RenderTexture::RenderTexture(cr<RenderTextureDesc> desc, crsp<DxResource> dxResource)
    {
        m_desc = desc;
        m_dxTexture = DxTexture::CreateRenderTexture(desc.dxDesc, desc.clearColor, dxResource);

        if (desc.dxDesc.format != TextureFormat::DEPTH_STENCIL)
        {
            m_shaderResource = DescriptorPool::Ins()->AllocSrv(m_dxTexture.get());
        }
    }
}
