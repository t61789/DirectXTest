#pragma once

#include "nlohmann/json.hpp"

#include "comp.h"
#include "common/math.h"
#include "common/event.h"
#include "render/cbuffer.h"

namespace dt
{
    struct RenderObject;
    class Mesh;
    class Material;

    class RenderComp final : public Comp
    {
    public:
        void Start() override;
        void OnEnable() override;
        void OnDisable() override;
        void OnDestroy() override;
        void UpdateTransform();
        cr<Bounds> GetWorldBounds();
        
        bool HasOddNegativeScale() const;
        sp<Mesh> GetMesh() const { return m_mesh;}
        sp<Material> GetMaterial() const { return m_material;}
        sp<RenderObject> GetRenderObject() const { return m_renderObject;}

        void LoadFromJson(const nlohmann::json& objJson) override;

    private:
        void CreateRenderObject();
        void ClearRenderObject();
        
        sp<Mesh> m_mesh = nullptr;
        sp<Material> m_material = nullptr;
        sp<RenderObject> m_renderObject = nullptr;

        bool m_enableBatch = false;
        bool m_transformDirty = true;
        Bounds m_worldBounds;
        EventHandler m_onTransformDirtyHandler = 0;

        void OnTransformDirty();

        void UpdateWorldBounds();
        void UpdatePerObjectBuffer();
    };
}
