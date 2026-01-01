#pragma once
#include "common/math.h"
#include "common/const.h"
#include "descriptor_pool.h"

namespace dt
{
    class RenderTexture;

    class RenderTarget
    {
    public:
        crvecsp<RtvPool::Handle> GetRtvHandles() const { return m_rtvHandles; }
        crsp<DsvPool::Handle> GetDsvHandle() const { return m_dsvHandle; }
        crvecsp<RenderTexture> GetColorAttachments() const { return m_colorAttachments; }
        crsp<RenderTexture> GetDepthAttachment() const { return m_depthAttachment; }
        cr<XMINT2> GetSize() const { return m_size; }
        
        static sp<RenderTarget> Create(crsp<RenderTexture> colorAttachment, crsp<RenderTexture> depthAttachment);
        static sp<RenderTarget> Create(crvecsp<RenderTexture> colorAttachments, crsp<RenderTexture> depthAttachment);

    private:
        vecsp<RenderTexture> m_colorAttachments;
        sp<RenderTexture> m_depthAttachment;
        vecsp<RtvPool::Handle> m_rtvHandles;
        sp<DsvPool::Handle> m_dsvHandle;
        XMINT2 m_size = {0, 0};
    };
}
