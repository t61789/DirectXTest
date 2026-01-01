#include "render_target.h"

#include "common/render_texture.h"
#include "common/math.h"

namespace dt
{
    sp<RenderTarget> RenderTarget::Create(crsp<RenderTexture> colorAttachment, crsp<RenderTexture> depthAttachment)
    {
        static vecsp<RenderTexture> tempColorAttachments;
        tempColorAttachments.push_back(colorAttachment);

        auto result = Create(tempColorAttachments, depthAttachment);
        tempColorAttachments.clear();

        return result;
    }

    sp<RenderTarget> RenderTarget::Create(crvecsp<RenderTexture> colorAttachments, crsp<RenderTexture> depthAttachment)
    {
        // Extract dx textures from render textures
        static vec<DxTexture*> tempColorAttachments;
        tempColorAttachments.clear();
        for (auto& rt : colorAttachments)
        {
            tempColorAttachments.push_back(rt->GetDxTexture());
        }
        DxTexture* tempDepthAttachment = nullptr;
        if (depthAttachment)
        {
            tempDepthAttachment = depthAttachment->GetDxTexture();
        }

        // Allocate rtv
        vecsp<RtvPool::Handle> rtvHandles;
        sp<DsvPool::Handle> dsvHandle = nullptr;
        DescriptorPool::Ins()->AllocRtv(tempColorAttachments, tempDepthAttachment, rtvHandles, dsvHandle);

        // Calculate size
        tempColorAttachments.push_back(tempDepthAttachment);
        auto size = XMINT2(0, 0);
        for (auto& rt : tempColorAttachments)
        {
            if (rt)
            {
                assert(size == XMINT2(0, 0) || size == rt->GetSize());
                size = rt->GetSize();
            }
        }

        // Assemble result
        auto result = msp<RenderTarget>();
        result->m_colorAttachments = colorAttachments;
        result->m_depthAttachment = depthAttachment;
        result->m_rtvHandles = rtvHandles;
        result->m_dsvHandle = dsvHandle;
        result->m_size = size;

        return result;
    }
}
