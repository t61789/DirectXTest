#include "render_texture.h"

namespace dt
{
    RenderTexture::RenderTexture(cr<RenderTextureDesc> desc, crsp<DxResource> dxResource)
    {
        m_dxTexture = DxTexture::CreateRenderTexture(desc.dxDesc, desc.clearColor, dxResource);
        m_shaderResource = DescriptorPool::Ins()->AllocSrv(m_dxTexture.get());
    }
}
