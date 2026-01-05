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
    
        if (Keyboard::Ins()->KeyPressed(KeyCode::S))
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

        auto r = XMQuaternionSlerp(GetOwner()->transform->GetRotation(), ToRotation(Load(m_targetRotation)), damp);
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
        auto projMatrix = XMMatrixPerspectiveFovLH(fov * DEG2RAD, aspect, nearClip, farClip);

        return ViewProjInfo::Create(viewMatrix, projMatrix, viewCenter, true);
    }
    
    sp<ViewProjInfo> CameraComp::CreateShadowVPMatrix(const XMFLOAT3 lightDirection, float screenAspect, float shadowRange, uint32_t shadowTexSize)
    {
        shadowRange = std::min(shadowRange, farClip);
        
        float range = 100;
        auto lightForward = -XMVector3Normalize(Load(lightDirection));
        
        XMVECTOR forward, right, up;
        GramSchmidtOrtho(
            XMVectorSet(0, 1, 0, 0),
            lightForward,
            right,
            up,
            forward);

        auto& cameraL2W = GetOwner()->transform->GetLocalToWorld();
        auto cameraRight = cameraL2W.r[0];
        auto cameraUp = cameraL2W.r[1];
        auto cameraForward = cameraL2W.r[2];
        auto cameraPos = cameraL2W.r[3];
        
        XMVECTOR corners[8];
        GetFrustumCorners(cameraRight, cameraUp, cameraForward, cameraPos, fov, nearClip, shadowRange, screenAspect, corners);

        // Gui::Ins()->DrawFrustumPlanes(corners, IM_COL32(0, 255, 0, 255));

        auto shadowWidth = Length3(corners[6] - corners[0]) * 0.5f;
        shadowWidth = std::max(shadowWidth, Length3(corners[4] - corners[5]) * 0.5f);
        shadowWidth = std::max(shadowWidth, Length3(corners[5] - corners[6]) * 0.5f);

        float minU, maxU, minF, maxF;
        float minR = minU = minF = 999999999.0f;
        float maxR = maxU = maxF = -999999999.0f;
        for (auto corner : corners)
        {
            auto r = Dot3(corner, right);
            minR = std::min(minR, r);
            maxR = std::max(maxR, r);
            auto u = Dot3(corner, up);
            minU = std::min(minU, u);
            maxU = std::max(maxU, u);
            auto f = Dot3(corner, forward);
            minF = std::min(minF, f);
            maxF = std::max(maxF, f);
        }

        auto depthRange = maxF - minF + range;
        auto distancePerTexel = shadowWidth * 2 / static_cast<float>(shadowTexSize);
        auto rightMove = (minR + maxR) * 0.5f;
        rightMove = std::floor(rightMove / distancePerTexel) * distancePerTexel;
        auto upMove = (minU + maxU) * 0.5f;
        upMove = std::floor(upMove / distancePerTexel) * distancePerTexel;
        auto forwardMove = maxF - depthRange;
        auto shadowCameraPos = right * rightMove + up * upMove + forward * forwardMove;

        right = XMVectorSetW(right, 0);
        up = XMVectorSetW(up, 0);
        forward = XMVectorSetW(forward, 0);
        shadowCameraPos = XMVectorSetW(shadowCameraPos, 1);
        
        // 计算阴影矩阵
        auto shadowCameraToWorld = XMMATRIX(right, up, forward, shadowCameraPos);
        auto view = Inverse(shadowCameraToWorld);

        auto proj = XMMatrixOrthographicLH(shadowWidth * 2, shadowWidth * 2, 0, depthRange);

        return ViewProjInfo::Create(view, proj, shadowCameraPos);
    }
}
