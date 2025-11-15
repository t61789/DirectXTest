#pragma once

#include "common/math.h"
#include "comp.h"

namespace dt
{
    class LightComp final : public Comp
    {
    public:
        XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f);
        float intensity = 1;
        int lightType = 0; // 0 parallel, 1 point
        float radius = 10;

        XMFLOAT3 GetColor() const;

        void LoadFromJson(const nlohmann::json& objJson) override;
    };
}
