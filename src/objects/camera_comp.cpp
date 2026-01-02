#include "camera_comp.h"

#include "common/utils.h"
#include "transform_comp.h"
#include "common/keyboard.h"
#include "game/game_resource.h"
#include "render/render_context.h"

namespace dt
{
    using namespace std;

    void CameraComp::Awake()
    {
        m_cameras.push_back(this);
    }

    void CameraComp::OnDestroy()
    {
        dt::remove(m_cameras, this);
    }

    void CameraComp::Start()
    {
        XMStoreFloat3(&m_targetPosition, GetOwner()->transform->GetWorldPosition());
        XMStoreFloat3(&m_targetRotation, GetOwner()->transform->GetEulerAngles());
    }

    void CameraComp::Update()
    {
        const XMMATRIX& localToWorld = GetOwner()->transform->GetLocalToWorld();
        auto forward = GetForward(localToWorld);
        auto right = GetRight(localToWorld);
        auto up = GetUp(localToWorld);

        float moveSpeed = 6;
        float accleration = 8;
        float rotateSpeed = 125;
        float damp = 0.07f;

        auto deltaTime = GR()->GetDeltaTime();

        if (Keyboard::Ins()->KeyPressed(KeyCode::LeftShift))
        {
            m_curSpeedAdd += deltaTime * accleration;
            moveSpeed += m_curSpeedAdd;
        }
        else
        {
            m_curSpeedAdd = 0;
        }

        if (Keyboard::Ins()->KeyPressed(KeyCode::W))
        {
            XMStoreFloat3(&m_targetPosition, XMLoadFloat3(&m_targetPosition) + forward * deltaTime * moveSpeed);
        }
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::W))
        {
            XMStoreFloat3(&m_targetPosition, XMLoadFloat3(&m_targetPosition) - forward * deltaTime * moveSpeed);
        }
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::A))
        {
            XMStoreFloat3(&m_targetPosition, XMLoadFloat3(&m_targetPosition) - right * deltaTime * moveSpeed);
        }
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::D))
        {
            XMStoreFloat3(&m_targetPosition, XMLoadFloat3(&m_targetPosition) + right * deltaTime * moveSpeed);
        }
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::E))
        {
            XMStoreFloat3(&m_targetPosition, XMLoadFloat3(&m_targetPosition) + up * deltaTime * moveSpeed);
        }

        if (Keyboard::Ins()->KeyPressed(KeyCode::Q))
        {
            XMStoreFloat3(&m_targetPosition, XMLoadFloat3(&m_targetPosition) - up * deltaTime * moveSpeed);
        }

        auto newPositionWS = XMVectorLerp(GetOwner()->transform->GetWorldPosition(), XMLoadFloat3(&m_targetPosition), damp);
        GetOwner()->transform->SetWorldPosition(newPositionWS);
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::Up))
        {
            m_targetRotation.x -= deltaTime * rotateSpeed;
        }
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::Down))
        {
            m_targetRotation.x += deltaTime * rotateSpeed;
        }
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::Left))
        {
            m_targetRotation.y -= deltaTime * rotateSpeed;
        }
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::Right))
        {
            m_targetRotation.y += deltaTime * rotateSpeed;
        }

        auto targetRotationQ = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_targetRotation) * XMVectorReplicate(DEG2RAD));
        auto r = XMQuaternionSlerp(GetOwner()->transform->GetRotation(), targetRotationQ, damp);
        GetOwner()->transform->SetRotation(r);
    }

    void CameraComp::LoadFromJson(const nlohmann::json& objJson)
    {
        Comp::LoadFromJson(objJson);
        
        try_get_val(objJson, "fov", fov);

        try_get_val(objJson, "nearClip", nearClip);

        try_get_val(objJson, "farClip", farClip);
    }

    CameraComp* CameraComp::GetMainCamera()
    {
        ASSERT_THROW(!m_cameras.empty());

        return m_cameras[0];
    }

    sp<ViewProjInfo> CameraComp::CreateVPMatrix(const float aspect)
    {
        auto& localToWorld = GetOwner()->transform->GetLocalToWorld();
        auto viewCenter = GetPosition(localToWorld);
        auto viewMatrix = XMMatrixLookToLH(GetPosition(localToWorld), GetForward(localToWorld), GetUp(localToWorld));
        auto projMatrix = XMMatrixPerspectiveFovLH(fov, aspect, nearClip, farClip);

        return ViewProjInfo::Create(viewMatrix, projMatrix, viewCenter, true);
    }
}
