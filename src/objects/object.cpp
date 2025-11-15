#include "object.h"

#include "scene.h"
#include "game/game_resource.h"
#include "objects/transform_comp.h"

namespace dt
{
    sp<Object> Object::Create(cr<StringHandle> name, crsp<Object> parent)
    {
        auto json = nlohmann::json::object();
        json["name"] = name.Str();
        return Create(json, parent);
    }
    
    sp<Object> Object::Create(const nlohmann::json& objJson, crsp<Object> parent)
    {
        auto result = msp<Object>();
        
        if (parent)
        {
            result->SetParent(parent);
        }
        else
        {
            result->SetParent(GR()->mainScene->GetRoot());
        }
        
        if(objJson.contains("name"))
        {
            result->name = objJson["name"].get<std::string>();
        }

        try_get_val(objJson, "name", result->name);

        try_get_val(objJson, "enable", result->m_enable);
        
        result->AddCompsFromJsons(objJson["comps"]);
        
        return result;
    }

    void Object::SetEnable(const bool enable)
    {
        if (enable && !m_enable)
        {
            m_enable = true;

            UpdateRealEnable();
        }
        else if (!enable && m_enable)
        {
            m_enable = false;

            UpdateRealEnable();
        }
    }

    void Object::SetParent(crsp<Object> obj)
    {
        if (!CheckIfCanBeNewParent(obj))
        {
            return;
        }
        auto newParent = obj ? obj : GR()->mainScene->GetRoot();

        // Remove from the old parent when parent exists
        if (!parent.expired())
        {
            remove(parent.lock()->m_children, shared_from_this());
        }
        parent = newParent;

        // Add to the new parent
        newParent->m_children.push_back(shared_from_this());

        // Register to the new scene if needed
        if (newParent->m_scene != m_scene)
        {
            if (m_scene)
            {
                m_scene->GetRegistry()->UnregisterObject(shared_from_this());
            }

            m_scene = newParent->m_scene;

            if (m_scene)
            {
                m_scene->GetRegistry()->RegisterObject(shared_from_this());
            }
        }

        UpdateRealEnable();
    }

    void Object::Destroy()
    {
        auto self = shared_from_this();

        auto children = m_children;
        for (auto& child : children)
        {
            child->Destroy();
        }
        
        for (auto& comp: m_comps)
        {
            comp->Destroy();
        }
        
        if (!parent.expired())
        {
            remove(parent.lock()->m_children, shared_from_this());
            parent.reset();
        }

        if (m_scene)
        {
            m_scene->GetRegistry()->UnregisterObject(shared_from_this());
        }

        m_children.clear();
    }

    std::string Object::GetPathInScene() const
    {
        vec<std::string> path;
        
        auto curObj = this;
        while (curObj)
        {
            path.push_back(curObj->name);
            curObj = curObj->parent.lock().get();
        }

        return Utils::Join(path, "/");
    }

    void Object::BindComp(crsp<Comp> comp)
    {
        ASSERT_THROW(!comp->m_owner);
        ASSERT_THROW(m_scene);

        if (comp->GetName() == "TransformComp")
        {
            transform = static_cast<TransformComp*>(comp.get());
        }
        
        comp->m_owner = this;
        m_comps.push_back(comp);
        m_scene->GetRegistry()->GetCompStorage()->AddComp(comp);
        comp->UpdateRealEnable();
    }

    void Object::UpdateRealEnable()
    {
        m_realEnable = m_enable;
        if (!parent.expired())
        {
            m_realEnable = m_realEnable && parent.lock()->IsEnable();
        }

        for (auto& comp: m_comps)
        {
            comp->UpdateRealEnable();
        }

        for (auto& child : m_children)
        {
            child->UpdateRealEnable();
        }
    }

    bool Object::HasComp(cr<StringHandle> compName)
    {
        return GetComp(compName) != nullptr;
    }

    sp<Comp> Object::GetComp(cr<StringHandle> compName)
    {
        auto result = find_if(m_comps, [compName](crsp<Comp> x)
        {
            return x->m_name == compName;
        });
        
        if (!result)
        {
            return nullptr;
        }
        return *result;
    }

    void Object::AddCompsFromJsons(nlohmann::json compJsons)
    {
        umap<string_hash, sp<Comp>> comps;

        nlohmann::json transformCompJson;
        transformCompJson["name"] = "TransformComp";
        compJsons.insert(compJsons.begin(), transformCompJson);
        
        for (auto& compJson : compJsons)
        {
            auto compName = get_val<StringHandle>(compJson, "name");
            sp<Comp> comp;
            if (auto it = comps.find(compName.Hash()); it != comps.end())
            {
                comp = it->second;
            }
            else
            {
                comp = Comp::Create(compName);
                comps[compName.Hash()] = comp;
            }

            comp->LoadFromJson(compJson);
        }

        for (auto& comp : comps)
        {
            BindComp(comp.second);
        }

        ASSERT_THROW(transform != nullptr);
    }

    bool Object::IsAnyAncestorOf(crsp<Object> obj)
    {
        auto curObj = obj.get();
        while (curObj)
        {
            if (curObj == this)
            {
                return true;
            }

            curObj = curObj->parent.lock().get();
        }

        return false;
    }

    sp<Comp> Object::GetOrAddComp(cr<StringHandle> compName)
    {
        if (auto comp = GetComp<Comp>(compName))
        {
            return comp;
        }

        auto comp = Comp::Create(compName);
        BindComp(comp);

        return comp;
    }

    bool Object::CheckIfCanBeNewParent(crsp<Object> obj)
    {
        auto newParent = obj ? obj : GR()->mainScene->GetRoot();
        
        if (obj.get() == this || newParent == this->parent.lock())
        {
            return false;
        }

        if (IsAnyAncestorOf(obj))
        {
            log_warning("The current node cannot be any ancestor of the target node");
            return false;
        }

        if (exists(newParent->m_children, shared_from_this()))
        {
            return false;
        }

        return true;
    }
}
