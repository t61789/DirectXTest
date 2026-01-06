#pragma once

#include "comp.h"

namespace dt
{
    class Material;
    
    class SkyboxComp final : public Comp
    {
    public:
        void Awake() override;
        void LateUpdate() override;

    private:
        sp<Object> m_skyboxObject;
    };
}
