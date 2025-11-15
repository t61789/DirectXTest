#include "render_comp.h"

#include "game/game_resource.h"
#include "common/material.h"
#include "common/mesh.h"

#include "object.h"
#include "transform_comp.h"
#include "render/cbuffer.h"

namespace dt
{
    void RenderComp::Awake()
    {
        m_perObjectCbuffer = msp<Cbuffer>(GR()->GetPredefinedCbuffer(PER_OBJECT_CBUFFER)->GetLayout());
    }

    void RenderComp::Start()
    {
        m_onTransformDirtyHandler = GetOwner()->transform->dirtyEvent.Add(this, &RenderComp::OnTransformDirty);
        OnTransformDirty();
        UpdateTransform();
    }

    void RenderComp::OnEnable()
    {
        OnTransformDirty();
        UpdateTransform();
    }

    void RenderComp::OnDisable()
    {
    }

    void RenderComp::OnDestroy()
    {
        m_perObjectCbuffer.reset();

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

        m_perObjectCbuffer->Write(M, &localToWorld, sizeof(XMFLOAT4X4));
        m_perObjectCbuffer->Write(IM, &worldToLocal, sizeof(XMFLOAT4X4));
    }
}
