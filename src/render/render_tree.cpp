#include "render_tree.h"

#include "common/material.h"
#include "game/game_resource.h"
#include "objects/render_comp.h"

namespace dt
{
    void RenderTree::Register(RenderComp* renderComp)
    {
        if (m_registeredComps.find(renderComp) != m_registeredComps.end())
        {
            return;
        }

        auto renderObject = msp<RenderObject>();
        renderObject->shader = renderComp->GetMaterial()->GetShader();
        renderObject->material = renderComp->GetMaterial().get();
        renderObject->mesh = renderComp->GetMesh().get();
        renderObject->renderComp = renderComp;
        renderObject->perObjectCbuffer = mup<Cbuffer>(GR()->GetPredefinedCbuffer(PER_OBJECT_CBUFFER)->GetLayout());
        
        m_registeredComps[renderComp] = renderObject;
        m_renderObjects.push_back(renderObject);

        std::sort(m_renderObjects.begin(), m_renderObjects.end(), [](crsp<RenderObject> a, crsp<RenderObject> b)
        {
            if (a->shader == b->shader)
            {
                if (a->material == b->material)
                {
                    if (a->mesh == b->mesh)
                    {
                        return true;
                    }
                    return a->mesh < b->mesh;
                }
                return a->material < b->material;
            }
            return a->shader < b->shader;
        });
    }

    void RenderTree::UnRegister(RenderComp* renderComp)
    {
        auto it = m_registeredComps.find(renderComp);
        if (it == m_registeredComps.end())
        {
            return;
        }
        m_registeredComps.erase(it);

        remove_if(m_renderObjects, [renderComp](crsp<RenderObject> renderObject)
        {
            return renderObject->renderComp == renderComp;
        });
    }

    void RenderTree::UpdateTransform(RenderComp* renderComp, cr<XMFLOAT4X4> localToWorld, cr<XMFLOAT4X4> worldToLocal)
    {
        auto ro = m_registeredComps.at(renderComp);
        
        ro->perObjectCbuffer->Write(M, &localToWorld, sizeof(XMFLOAT4X4));
        ro->perObjectCbuffer->Write(IM, &worldToLocal, sizeof(XMFLOAT4X4));
    }
}
