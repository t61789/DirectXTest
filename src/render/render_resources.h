#pragma once
#include "common/const.h"
#include "common/utils.h"

namespace dt
{
    class Cbuffer;
    class RenderComp;
    class Mesh;
    class Material;
    class Shader;

    struct RenderObject
    {
        sp<Shader> shader;
        sp<Material> material;
        sp<Mesh> mesh;
        sp<Cbuffer> perObjectCbuffer;
    };
    
    struct RenderResources
    {
        RenderResources() = default;
        
        vecsp<RenderObject> renderObjects;
    };
}
