#include "render_comp.h"

#include "game/game_resource.h"
#include "common/material.h"
#include "common/mesh.h"

#include "object.h"
#include "scene.h"
#include "transform_comp.h"

namespace dt
{
    void RenderComp::Start()
    {
        m_onTransformDirtyHandler = GetOwner()->transform->dirtyEvent.Add(this, &RenderComp::OnTransformDirty);
    }

    void RenderComp::OnEnable()
    {
        GetOwner()->GetScene()->GetRenderTree()->Register(this);
        
        OnTransformDirty();
    }

    void RenderComp::OnDisable()
    {
        GetOwner()->GetScene()->GetRenderTree()->UnRegister(this);
    }

    void RenderComp::OnDestroy()
    {
        if (m_onTransformDirtyHandler)
        {
            GetOwner()->transform->dirtyEvent.Remove(m_onTransformDirtyHandler);
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
        XMFLOAT4X4 localToWorld;
        XMStoreFloat4x4(&localToWorld, GetOwner()->transform->GetLocalToWorld());
        XMFLOAT4X4 worldToLocal;
        XMStoreFloat4x4(&worldToLocal, GetOwner()->transform->GetWorldToLocal());
        
        GetOwner()->GetScene()->GetRenderTree()->UpdateTransform(this, localToWorld, worldToLocal);
    }
}
