#include "light_comp.h"

#include "common/math.h"
#include "common/utils.h"

namespace dt
{
    XMFLOAT3 LightComp::GetColor() const
    {
        return {color.x * intensity, color.y * intensity, color.z * intensity};
    }

    void LightComp::LoadFromJson(const nlohmann::json& objJson)
    {
        Comp::LoadFromJson(objJson);
        
        try_get_val(objJson, "color", color);

        try_get_val(objJson, "light_type", lightType);

        try_get_val(objJson, "intensity", intensity);

        try_get_val(objJson, "radius", radius);
    }
}
