#pragma once

#include "nlohmann/json.hpp"

#include "comp.h"
#include "common/math.h"
#include "common/event.h"
#include "render/cbuffer.h"

namespace dt
{
    class Mesh;
    class Material;

    class RenderComp final : public Comp
    {
    public:
        void Awake() override;
        void Start() override;
        void OnEnable() override;
        void OnDisable() override;
        void OnDestroy() override;
        void UpdateTransform();
        cr<Bounds> GetWorldBounds();
        
        bool HasOddNegativeScale() const;
        sp<Mesh> GetMesh() const { return m_mesh;}
        sp<Material> GetMaterial() const { return m_material;}
        sp<Cbuffer> GetPerObjectCbuffer() const { return m_perObjectCbuffer;}

        void LoadFromJson(const nlohmann::json& objJson) override;

    private:
        sp<Mesh> m_mesh = nullptr;
        sp<Material> m_material = nullptr;
    
        EventHandler m_onTransformDirtyHandler = 0;
        bool m_transformDirty = true;
        Bounds m_worldBounds;
        sp<Cbuffer> m_perObjectCbuffer;

        void OnTransformDirty();

        void UpdateWorldBounds();
        void UpdatePerObjectBuffer();
    };
}
