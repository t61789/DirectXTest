#include "comp.h"

#include "object.h"
#include "render_comp.h"
#include "scene_registry.h"
#include "transform_comp.h"
#include "camera_comp.h"
#include "light_comp.h"
#include "skybox_comp.h"
#include "common/utils.h"

namespace dt
{
    void Comp::LoadFromJson(cr<nlohmann::json> objJson)
    {
        try_get_val(objJson, "name", m_name);
    }

    sp<Comp> Comp::Create(cr<StringHandle> compName)
    {
        std::unordered_map<string_hash, std::function<sp<Comp>()>> compConstructors;
        if (compConstructors.empty())
        {
            #define REGISTER_COMP(t) \
                compConstructors[StringHandle(#t)] = []() -> sp<Comp> { \
                    auto result = std::make_shared<t>(); \
                    result->m_name = StringHandle(#t); \
                    result->m_type = std::type_index(typeid(t)); \
                    return result; \
                }; \
                CompStorage::RegisterComp<t>();

            REGISTER_COMP(RenderComp)
            REGISTER_COMP(TransformComp)
            REGISTER_COMP(CameraComp)
            REGISTER_COMP(LightComp)
            REGISTER_COMP(SkyboxComp)

            #undef REGISTER_COMP
        }

        return compConstructors.at(compName.Hash())();
    }

    void Comp::CallUpdate()
    {
        if (!m_hasStart)
        {
            Start();
            m_hasStart = true;
        }

        Update();
    }

    void Comp::Destroy()
    {
        SetEnable(false);

        if (m_hasAwake)
        {
            OnDestroy();
        }
    }

    void Comp::SetEnable(const bool enable)
    {
        m_enable = enable;
        UpdateRealEnable();
    }

    void Comp::UpdateRealEnable()
    {
        auto preRealEnable = m_realEnable;
        m_realEnable = m_enable && GetOwner()->IsEnable();

        if (m_realEnable != preRealEnable)
        {
            if (m_realEnable)
            {
                if (!m_hasAwake)
                {
                    Awake();
                    m_hasAwake = false;
                }
                
                OnEnable();
            }
            else
            {
                OnDisable();
            }
        }
    }
}
