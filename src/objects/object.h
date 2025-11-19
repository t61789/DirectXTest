#pragma once
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "objects/comp.h"

namespace dt
{
    class TransformComp;
    class Scene;

    class Object final : public std::enable_shared_from_this<Object>
    {
        friend class Scene;
        friend class SceneRegistry;
        
    public:
        StringHandle name = UNNAMED_OBJECT;

        TransformComp* transform = nullptr;
        wp<Object> parent;
        
        Object() = default;

        bool IsEnable() const { return m_realEnable; }
        crvecsp<Object> GetChildren() const { return m_children; }
        crvecsp<Comp> GetComps() const { return m_comps; }
        Scene* GetScene() const { return m_scene; }
        std::string GetPathInScene() const;

        void SetEnable(bool enable);
        void SetParent(crsp<Object> obj);
        void Destroy();

        bool HasComp(cr<StringHandle> compName);
        sp<Comp> GetComp(cr<StringHandle> compName);
        sp<Comp> GetOrAddComp(cr<StringHandle> compName);
        
        template <typename T>
        sp<T> GetComp(cr<StringHandle> compName);
        template <typename T>
        sp<T> GetOrAddComp(cr<StringHandle> compName);
        
        static sp<Object> Create(cr<StringHandle> name = UNNAMED_OBJECT, crsp<Object> parent = nullptr);
        static sp<Object> Create(const nlohmann::json& objJson, crsp<Object> parent = nullptr);

    private:
        vecsp<Comp> m_comps;
        vecsp<Object> m_children;
        Scene* m_scene = nullptr;
        bool m_enable = true;
        bool m_realEnable = true;
        
        void BindComp(crsp<Comp> comp);
        void UpdateRealEnable();
        void AddCompsFromJsons(nlohmann::json compJsons);
        bool CheckIfCanBeNewParent(crsp<Object> obj);
        bool IsAnyAncestorOf(crsp<Object> obj);
    };

    template <typename T>
    sp<T> Object::GetComp(cr<StringHandle> compName)
    {
        return std::dynamic_pointer_cast<T>(GetComp(compName));
    }
    
    template <typename T>
    sp<T> Object::GetOrAddComp(cr<StringHandle> compName)
    {
        return std::dynamic_pointer_cast<T>(GetOrAddComp(compName));
    }
}
