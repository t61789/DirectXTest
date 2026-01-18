#pragma once
#include "common/const.h"
#include "utils/time_out_buffer.h"

namespace dt
{
    class Object;

    class SceneHierarchyPanel
    {
    public:
        explicit SceneHierarchyPanel(Scene* scene);
        
        void DrawSceneGui();
        void DrawHierarchyObject(crsp<Object> obj);
        void DrawProperties(const Object* obj);

    private:
        TimeOutBuffer<std::string, bool> m_foldout = TimeOutBuffer<std::string, bool>(true);
        
        wp<Object> m_selected;
        Scene* m_scene;
        
        inline static float s_intent = 20.0f;
    };
}
