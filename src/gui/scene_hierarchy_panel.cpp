#include "scene_hierarchy_panel.h"

#include <imgui.h>

#include "gui.h"
#include "common/const.h"
#include "game/game_resource.h"
#include "objects/object.h"
#include "objects/scene.h"
#include "objects/transform_comp.h"

namespace dt
{
    SceneHierarchyPanel::SceneHierarchyPanel(Scene* scene)
    {
        m_scene = scene;
    }

    void SceneHierarchyPanel::DrawSceneGui()
    {
        if (!ImGui::CollapsingHeader("Scene Info"))
        {
            return;
        }

        ImGui::BeginChild("Hierarchy", ImVec2(0, 150), true);
        DrawHierarchyObject(m_scene->GetRoot());
        ImGui::EndChild();
        
        DrawProperties(m_selected.lock().get());
    }

    void SceneHierarchyPanel::DrawHierarchyObject(crsp<Object> obj)
    {
        ImGui::TextUnformatted(obj->name.CStr());
        ImGui::SameLine();
        auto pathInScene = obj->GetPathInScene();
        if (ImGui::Button(("pick##" + pathInScene).c_str()))
        {
            if (m_selected.expired() || m_selected.lock() != obj)
            {
                m_selected = obj;
            }
        }

        if (!obj->GetChildren().empty())
        {
            auto foldout = m_foldout.GetOrAdd(pathInScene);
            ImGui::SameLine();
            auto foldoutLabel = foldout ? "-##" + pathInScene : "+##" + pathInScene;
            if (ImGui::Button(foldoutLabel.c_str()))
            {
                foldout = !foldout;
                m_foldout.Set(pathInScene, foldout);
            }

            if (foldout)
            {
                ImGui::Indent(s_intent);
                for (auto& child : obj->GetChildren())
                {
                    DrawHierarchyObject(child);
                }
                ImGui::Unindent(s_intent);
            }
        }
    }

    void SceneHierarchyPanel::DrawProperties(const Object* obj)
    {
        ImGui::BeginChild("Properties", ImVec2(0, 150), true);
        
        if (obj)
        {
            ImGui::TextUnformatted(obj->name.CStr());
            if (auto val = Store3(obj->transform->GetPosition()); Gui::DragFloat3("position", val, 0.02f))
            {
                obj->transform->SetPosition(Load(val));
            }

            if (auto val = Store3(obj->transform->GetScale()); Gui::DragFloat3("scale", val, 0.02f))
            {
                obj->transform->SetScale(Load(val));
            }

            if (auto val = Store3(obj->transform->GetEulerAngles()); Gui::DragFloat3("rotation", val, 1.0f))
            {
                obj->transform->SetEulerAngles(Load(val));
            }
        }
        else
        {
            ImGui::Text("未选择任何物体");
        }
    
        ImGui::EndChild();
    }
}
