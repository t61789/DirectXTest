#pragma once

#include "common/math.h"
#include "object.h"
#include "render/render_context.h"

namespace dt
{
    class CameraComp final : public Comp
    {
    public:
        void Awake() override;
        void OnDestroy() override;
        void Start() override;
        void Update() override;

        sp<ViewProjInfo> CreateVPMatrix(float aspect);
        
        void LoadFromJson(cr<nlohmann::json> objJson) override;
        
        static CameraComp* GetMainCamera();
        
        float fov = 45.0f;
        float nearClip = 0.1f;
        float farClip = 1000.0f;

    private:
        XMFLOAT3 m_targetPosition = {};
        XMFLOAT3 m_targetRotation = {};

        float m_curSpeedAdd = 0;
        
        inline static vec<CameraComp*> m_cameras;
    };
}
