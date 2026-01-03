#include "transform_comp.h"

#include "object.h"
#include "common/utils.h"

namespace dt
{
    using namespace std;

    void TransformComp::Awake()
    {
        GetOwner()->transform = this;
    }

    XMVECTOR TransformComp::GetWorldPosition()
    {
        if (m_position.needUpdateFromWorld)
        {
            return m_position.worldVal;
        }
        
        UpdateMatrix();
        
        return m_position.worldVal;
    }

    void TransformComp::SetWorldPosition(const XMVECTOR& pos)
    {
        if (m_position.needUpdateFromWorld && XMVector3Equal(m_position.worldVal, pos))
        {
            return;
        }
        
        m_position.worldVal = pos;
        m_position.needUpdateFromWorld = true;

        SetDirty(GetOwner());
    }

    XMVECTOR TransformComp::GetPosition()
    {
        if (m_position.needUpdateFromWorld)
        {
            UpdateMatrix();
        }
        
        return m_position.localVal;
    }

    void TransformComp::SetPosition(const XMVECTOR& pos)
    {
        m_position.needUpdateFromWorld = false;
        
        if (XMVector3Equal(m_position.localVal, pos))
        {
            return;
        }

        SetDirty(GetOwner());

        m_position.localVal = pos;
    }

    XMVECTOR TransformComp::GetScale()
    {
        return m_scale.localVal;
    }

    void TransformComp::SetScale(const XMVECTOR& scale)
    {
        if (XMVector3Equal(m_scale.localVal, scale))
        {
            return;
        }

        SetDirty(GetOwner());

        m_scale.localVal = scale;
    }

    XMVECTOR TransformComp::GetRotation()
    {
        return m_rotation.localVal;
    }

    void TransformComp::SetRotation(const XMVECTOR& rotation)
    {
        if (XMVector4Equal(m_rotation.localVal, rotation))
        {
            return;
        }

        SetDirty(GetOwner());

        m_eulerAngles.localVal = QuaternionToEuler(rotation);
        m_rotation.localVal = rotation;
    }

    XMVECTOR TransformComp::GetEulerAngles()
    {
        return m_eulerAngles.localVal;
    }

    void TransformComp::SetEulerAngles(const XMVECTOR& ea)
    {
        if (XMVector3Equal(m_eulerAngles.localVal, ea))
        {
            return;
        }

        SetDirty(GetOwner());

        m_eulerAngles.localVal = ea;
        m_rotation.localVal = ToRotation(ea);
    }

    bool TransformComp::HasOddNegativeScale()
    {
        UpdateMatrix();

        return m_hasOddNegativeScale;
    }

    const XMMATRIX& TransformComp::GetLocalToWorld()
    {
        UpdateMatrix();

        return m_matrix.localVal;
    }
    
    const XMMATRIX& TransformComp::GetWorldToLocal()
    {
        UpdateMatrix();

        return m_matrix.worldVal;
    }

    void TransformComp::LoadFromJson(const nlohmann::json& objJson)
    {
        Comp::LoadFromJson(objJson);
        
        try_get_val(objJson, "position", m_position.localVal);

        try_get_val(objJson, "rotation", m_eulerAngles.localVal);
        m_rotation.localVal = ToRotation(m_eulerAngles.localVal);

        try_get_val(objJson, "scale", m_scale.localVal);
    }

    void TransformComp::UpdateMatrix()
    {
        if (!m_dirty)
        {
            return;
        }
        m_dirty = false;
        
        XMMATRIX parentLocalToWorld;
        if (!GetOwner()->parent.expired())
        {
            // 如果parent还是dirty的话，GetLocalToWorld会继续往上递归地UpdateMatrix
            parentLocalToWorld = GetOwner()->parent.lock()->transform->GetLocalToWorld();
            if (m_position.needUpdateFromWorld)
            {
                auto& parentWorldToLocal = GetOwner()->parent.lock()->transform->GetWorldToLocal();
                m_position.localVal = XMVector3TransformCoord(m_position.worldVal, parentWorldToLocal);
                m_position.needUpdateFromWorld = false;
            }
        }
        else
        {
            parentLocalToWorld = XMMatrixIdentity();
            if (m_position.needUpdateFromWorld)
            {
                m_position.localVal = m_position.worldVal;
                m_position.needUpdateFromWorld = false;
            }
        }

        auto objectMatrix = XMMatrixTransformation(
            XMVectorZero(),
            XMQuaternionIdentity(),
            m_scale.localVal,
            XMVectorZero(),
            m_rotation.localVal,
            m_position.localVal);

        m_matrix.localVal = objectMatrix * parentLocalToWorld;

        XMVECTOR determinant;
        m_matrix.worldVal = XMMatrixInverse(&determinant, m_matrix.localVal);
        auto det = XMVectorGetX(determinant);
        ASSERT_THROW(std::abs(det) > EPSILON);

        m_hasOddNegativeScale = det < 0;

        m_position.worldVal = m_matrix.localVal.r[3];
    }

    /// 递归地将当前物体和它的子物体都标记为dirty
    void TransformComp::SetDirty(const Object* object)
    {
        if (object->transform->m_dirty)
        {
            return;
        }
        
        object->transform->m_dirty = true;
        object->transform->dirtyEvent.Invoke();
    
        if (object->GetChildren().empty())
        {
            return;
        }

        for (auto& child : object->GetChildren())
        {
            if (child->transform->m_dirty)
            {
                continue;
            }

            SetDirty(child.get());
        }
    }
}
