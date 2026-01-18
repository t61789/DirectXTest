#pragma once

#include "common/math.h"
#include "nlohmann/json.hpp"

#include "common/i_resource.h"
#include "scene_registry.h"
#include "common/event.h"
#include "render/render_tree.h"

namespace dt
{
    class SceneHierarchyPanel;
    class CameraComp;
    class Object;

    class Scene final : public IResource, public std::enable_shared_from_this<Scene>
    {
    public:
        XMFLOAT3 ambientLightColorSky = XMFLOAT3(0, 0, 0);
        XMFLOAT3 ambientLightColorEquator = XMFLOAT3(0, 0, 0);
        XMFLOAT3 ambientLightColorGround = XMFLOAT3(0, 0, 0);
        float tonemappingExposureMultiplier = 1.0f;
        float fogIntensity = 0.01f;
        XMFLOAT3 fogColor = XMFLOAT3(1.0f, 1.0f, 1.0f);

        Scene();
        ~Scene() override;
        Scene(const Scene& other) = delete;
        Scene(Scene&& other) noexcept = delete;
        Scene& operator=(const Scene& other) = delete;
        Scene& operator=(Scene&& other) noexcept = delete;

        SceneRegistry* GetRegistry() const { return m_registry.get();}
        RenderTree* GetRenderTree() const { return m_renderTree.get();}
        cr<StringHandle> GetPath() override { return m_path;}
        sp<Object> GetRoot() const { return m_sceneRoot;}
        
        static sp<Scene> LoadScene(cr<StringHandle> sceneJsonPath);

    private:
        void LoadSceneConfig(cr<nlohmann::json> configJson);

        void DrawSceneGui();

        static void LoadChildren(crsp<Object> parent, cr<nlohmann::json> children);
        
        StringHandle m_path;
        
        sp<Object> m_sceneRoot = nullptr;
        up<SceneRegistry> m_registry;
        up<RenderTree> m_renderTree;

        EventHandler m_drawGuiEventHandler;
        sp<SceneHierarchyPanel> m_hierarchyPanel;
    };
}
