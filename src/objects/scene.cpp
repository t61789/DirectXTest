#include "scene.h"

#include <filesystem>
#include <tracy/Tracy.hpp>

#include "common/math.h"
#include "object.h"
#include "game/game_resource.h"
#include "nlohmann/json.hpp"
#include "objects/comp.h"
#include "objects/transform_comp.h"

namespace dt
{
    using namespace std;

    void Scene::LoadChildren(crsp<Object> parent, cr<nlohmann::json> children)
    {
        for(auto& elem : children)
        {
            auto obj = Object::Create(elem, parent);
            
            if(elem.contains("children"))
            {
                LoadChildren(obj, elem["children"]);
            }
        }
    }

    Scene::~Scene()
    {
        m_sceneRoot->Destroy();
    }

    sp<Scene> Scene::LoadScene(cr<StringHandle> sceneJsonPath)
    {
        {
            if (auto result = GR()->GetResource<Scene>(sceneJsonPath))
            {
                GR()->mainScene = result.get();
                return result;
            }
        }
        
        ZoneScoped;

        auto scene = msp<Scene>();
        GR()->mainScene = scene.get();
        
        scene->m_registry = mup<SceneRegistry>(scene.get());
        scene->m_renderTree = mup<RenderTree>();
        
        nlohmann::json json = Utils::LoadJson(sceneJsonPath);

        // 合并替换json
        auto p = filesystem::path(sceneJsonPath.CStr());
        auto coverSceneJsonPath = p.parent_path() / (p.stem().generic_string() + "_cover.json");
        if (exists(coverSceneJsonPath))
        {
            nlohmann::json coverSceneJson = Utils::LoadJson(coverSceneJsonPath.generic_string());
            Utils::MergeJson(json, coverSceneJson);
        }

        if(json.contains("config"))
        {
            scene->LoadSceneConfig(json["config"]);
        }

        auto rootObj = msp<Object>();
        rootObj->name = "Scene Root";
        rootObj->m_scene = scene.get();
        rootObj->SetEnable(true);
        
        scene->m_sceneRoot = rootObj;
        scene->m_sceneRoot->GetOrAddComp("TransformComp");
        scene->m_registry->RegisterObject(scene->m_sceneRoot);
        LoadChildren(rootObj, json.at("root"));

        GR()->RegisterResource(sceneJsonPath, scene);
        scene->m_path = sceneJsonPath;
        
        return scene;
    }
    
    void Scene::LoadSceneConfig(cr<nlohmann::json> configJson)
    {
        try_get_val(configJson, "ambientLightColorSky", ambientLightColorSky);

        try_get_val(configJson, "ambientLightColorEquator", ambientLightColorEquator);

        try_get_val(configJson, "ambientLightColorGround", ambientLightColorGround);

        try_get_val(configJson, "tonemappingExposureMultiplier", tonemappingExposureMultiplier);

        try_get_val(configJson, "fog_intensity", fogIntensity);

        try_get_val(configJson, "fog_color", fogColor);
    }
}
