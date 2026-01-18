#include "scene_registry.h"

#include "common/material.h"
#include "object.h"
#include "objects/render_comp.h"

namespace dt
{
    umap<std::type_index, CompStorage::CompsAccessor> CompStorage::m_compAccessors = {};
    
    SceneRegistry::SceneRegistry(Scene* scene)
    {
        m_scene = scene;
    }

    void SceneRegistry::RegisterObject(crsp<Object> obj)
    {
        assert(!ObjectExists(obj));
        
        for (const auto& child : obj->GetChildren())
        {
            RegisterObject(child);
        }
        
        for (const auto& comp : obj->GetComps())
        {
            UnregisterComp(comp);
        }
        
        m_objects.push_back(obj);
    }

    void SceneRegistry::UnregisterObject(crsp<Object> obj)
    {
        assert(ObjectExists(obj));

        for (const auto& child : obj->GetChildren())
        {
            UnregisterObject(child);
        }
        
        for (const auto& comp : obj->GetComps())
        {
            UnregisterComp(comp);
        }

        m_objects.erase(
            std::remove_if(
                m_objects.begin(), m_objects.end(),
                [&obj](crwp<Object> x){ return x.lock() == obj; }),
            m_objects.end());
    }

    void SceneRegistry::RegisterComp(crsp<Comp> comp)
    {
        m_compStorage.AddComp(comp);

        if (auto renderComp = std::dynamic_pointer_cast<RenderComp>(comp))
        {
            RegisterRenderComp(renderComp);
        }
    }

    void SceneRegistry::UnregisterComp(crsp<Comp> comp)
    {
        if (auto renderComp = std::dynamic_pointer_cast<RenderComp>(comp))
        {
            UnRegisterRenderComp(renderComp);
        }
        
        m_compStorage.RemoveComp(comp);
    }

    bool SceneRegistry::ObjectExists(crsp<Object> obj)
    {
        return exists_if(m_objects, [&obj](crwp<Object> x){ return x.lock() == obj;});
    }

    void SceneRegistry::RegisterRenderComp(crsp<RenderComp> comp)
    {
        assert(comp != nullptr);
        auto material = comp->GetMaterial();
        assert(material != nullptr);

        auto& comps = GetRenderComps(material->GetBlendMode());
        assert(!std::any_of(comps.begin(), comps.end(), [&comp](crwp<RenderComp> x){ return x.lock() == comp; }));

        comps.push_back(comp);
    }

    void SceneRegistry::UnRegisterRenderComp(crsp<RenderComp> comp)
    {
        assert(comp != nullptr);
        auto material = comp->GetMaterial();
        assert(material != nullptr);

        auto& comps = GetRenderComps(material->GetBlendMode());
        remove_if(comps, [&comp](crwp<RenderComp> x){ return x.lock() == comp; });
    }

    vecwp<RenderComp>& SceneRegistry::GetRenderComps(const BlendMode blendMode)
    {
        switch (blendMode)
        {
            case BlendMode::NONE:
                return m_opaqueComps;
            case BlendMode::BLEND:
            case BlendMode::ADD:
                return m_transparentComps;
            default:
                throw std::runtime_error("Unsupported blend mode");
        }
    }
}
 