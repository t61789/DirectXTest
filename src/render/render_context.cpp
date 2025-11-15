#include "render_context.h"

#include "cbuffer.h"
#include "common/material.h"
#include "game/game_resource.h"

#include "objects/camera_comp.h"

namespace dt
{
    void ViewProjInfo::UpdateIVP()
    {
        XMStoreFloat4x4(&vpMatrix, Inverse(XMLoadFloat4x4(&vpMatrix)));
    }

    sp<ViewProjInfo> ViewProjInfo::Create(cr<XMMATRIX> vMatrix, cr<XMMATRIX> pMatrix, const bool useIVP)
    {
        auto invV = Inverse(vMatrix);
        auto viewCenter = GetPosition(invV);

        return Create(vMatrix, pMatrix, viewCenter, useIVP);
    }

    sp<ViewProjInfo> ViewProjInfo::Create(cr<XMMATRIX> vMatrix, cr<XMMATRIX> pMatrix, cr<XMVECTOR> viewCenter, const bool useIVP)
    {
        auto info = msp<ViewProjInfo>();
        XMStoreFloat4x4(&info->vMatrix, vMatrix);
        XMStoreFloat4x4(&info->pMatrix, pMatrix);
        XMStoreFloat4x4(&info->vpMatrix, vMatrix * pMatrix);
        XMStoreFloat4(&info->viewCenter, viewCenter);
        if (useIVP)
        {
            info->UpdateIVP();
        }

        return info;
    }

    void RenderContext::PushViewProjMatrix(crsp<ViewProjInfo> viewProjInfo)
    {
        m_vpMatrixStack.push_back(viewProjInfo);

        SetViewProjMatrix(viewProjInfo);
    }

    void RenderContext::PopViewProjMatrix()
    {
        if (m_vpMatrixStack.empty())
        {
            return;
        }
        
        this->m_vpMatrixStack.pop_back();
        if (!m_vpMatrixStack.empty())
        {
            SetViewProjMatrix(m_vpMatrixStack.back());
        }
    }

    UsingObject RenderContext::UsingViewProjMatrix(crsp<ViewProjInfo> viewProjInfo)
    {
        PushViewProjMatrix(viewProjInfo);

        return UsingObject([this]
        {
            this->PopViewProjMatrix();
        });
    }

    crsp<ViewProjInfo> RenderContext::CurViewProjMatrix() const
    {
        assert(!m_vpMatrixStack.empty());

        return m_vpMatrixStack.back();
    }

    void RenderContext::SetViewProjMatrix(crsp<ViewProjInfo> viewProjInfo)
    {
        auto perViewCbuffer = GetPerViewCbuffer();
        perViewCbuffer->Write(VP, &viewProjInfo->vpMatrix, sizeof(XMFLOAT4X4));
        perViewCbuffer->Write(CAMERA_POSITION_WS, &viewProjInfo->viewCenter, sizeof(XMFLOAT4));

        if (viewProjInfo->ivpMatrix.has_value())
        {
            perViewCbuffer->Write(IVP, &viewProjInfo->ivpMatrix, sizeof(XMFLOAT4X4));
        }
    }
}
