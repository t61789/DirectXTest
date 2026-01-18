#include "skybox_comp.h"

#include "camera_comp.h"
#include "transform_comp.h"
#include "common/material.h"
#include "common/mesh.h"
#include "game/game_resource.h"

namespace dt
{
    void SkyboxComp::Awake()
    {
        m_skyboxObject = Object::Create(StringHandle("Skybox"));
        m_skyboxObject->AddComp("RenderComp", {
            { "mesh", GR()->sphereMesh->GetPath().CStr() },
            { "material", "built_in/materials/skybox.mtl" },
            { "enable_batch", true }
        });
    }

    void SkyboxComp::LateUpdate()
    {
        auto mainCamera = CameraComp::GetMainCamera();
        m_skyboxObject->transform->SetWorldPosition(mainCamera->GetOwner()->transform->GetWorldPosition());
    }
}
