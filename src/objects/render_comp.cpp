#include "render_comp.h"

#include "game/game_resource.h"
#include "common/material.h"
#include "common/mesh.h"

#include "object.h"
#include "scene.h"
#include "transform_comp.h"
#include "render/render_resources.h"
#include "render/batch_rendering/batch_renderer.h"

namespace dt
{
    void RenderComp::Start()
    {
        m_onTransformDirtyHandler = GetOwner()->transform->matrixChangedEvent.Add(this, &RenderComp::OnTransformDirty);
    }

    void RenderComp::OnEnable()
    {
        CreateRenderObject();
    }

    void RenderComp::OnDisable()
    {
        ClearRenderObject();
    }

    void RenderComp::OnDestroy()
    {
        if (m_onTransformDirtyHandler)
        {
            GetOwner()->transform->matrixChangedEvent.Remove(m_onTransformDirtyHandler);
        }
    }

    void RenderComp::LoadFromJson(const nlohmann::json& objJson)
    {
        if (str meshPath; try_get_val(objJson, "mesh", meshPath))
        {
            m_mesh = Mesh::LoadFromFile(meshPath);
        }

        if (str matPath; try_get_val(objJson, "material", matPath))
        {
            m_material = Material::LoadFromFile(matPath);
        }

        try_get_val(objJson, "enable_batch", m_enableBatch);
    }

    void RenderComp::CreateRenderObject()
    {
        ClearRenderObject();
        
        ASSERT_THROW(m_mesh && m_material);

        m_renderObject = msp<RenderObject>();
        m_renderObject->mesh = m_mesh;
        m_renderObject->material = m_material;
        m_renderObject->shader = m_material->GetShader();
        m_renderObject->perObjectCbuffer = msp<Cbuffer>(GR()->GetPredefinedCbuffer(PER_OBJECT_CBUFFER)->GetLayout());

        if (m_enableBatch)
        {
            BatchRenderer::Ins()->Register(m_renderObject);
        }
        else
        {
            GetOwner()->GetScene()->GetRenderTree()->Register(m_renderObject);
        }
        
        OnTransformDirty();
    }

    void RenderComp::ClearRenderObject()
    {
        if (!m_renderObject)
        {
            return;
        }

        if (m_enableBatch)
        {
            BatchRenderer::Ins()->Unregister(m_renderObject);
        }
        else
        {
            GetOwner()->GetScene()->GetRenderTree()->UnRegister(m_renderObject);
        }
        m_renderObject.reset();
    }

    const Bounds& RenderComp::GetWorldBounds()
    {
        UpdateTransform();

        return m_worldBounds;
    }

    bool RenderComp::HasOddNegativeScale() const
    {
        return GetOwner()->transform->HasOddNegativeScale();
    }

    void RenderComp::OnTransformDirty()
    {
        m_transformDirty = true;
        UpdateTransform();
    }

    void RenderComp::UpdateTransform()
    {
        if (!m_transformDirty)
        {
            return;
        }
        m_transformDirty = false;

        UpdateWorldBounds();
        UpdatePerObjectBuffer();
    }

    void RenderComp::UpdateWorldBounds()
    {
        m_worldBounds = m_mesh->GetBounds().ToWorld(GetOwner()->transform->GetLocalToWorld());
    }

    void RenderComp::UpdatePerObjectBuffer()
    {
        if (!m_renderObject)
        {
            return;
        }
        
        auto m = Transpose(Store(GetOwner()->transform->GetLocalToWorld()));
        auto im = Transpose(Store(GetOwner()->transform->GetWorldToLocal()));

        if (m_enableBatch)
        {
            BatchRenderer::Ins()->UpdateMatrix(m_renderObject, {
                m, im
            });
        }
        else
        {
            m_renderObject->perObjectCbuffer->Write(M, m);
            m_renderObject->perObjectCbuffer->Write(IM, im);
        }
    }
}
