#pragma once
#include "cbuffer.h"
#include "common/const.h"

namespace dt
{
    class RenderComp;
    class Mesh;
    class Material;
    class Shader;
    
    struct RenderObject
    {
        Shader* shader;
        Material* material;
        Mesh* mesh;
        RenderComp* renderComp;

        up<Cbuffer> perObjectCbuffer;
    };

    class RenderTree
    {
    public:
        void Register(RenderComp* renderComp);
        void UnRegister(RenderComp* renderComp);

        template <typename F>
        void Foreach(F&& func);

    private:
        vecsp<RenderObject> m_renderObjects;
        umap<RenderComp*, sp<RenderObject>> m_registeredComps;
    };

    template <typename F>
    void RenderTree::Foreach(F&& func)
    {
        for (const auto& renderObject : m_renderObjects)
        {
            func(renderObject.get());
        }
    }
}
