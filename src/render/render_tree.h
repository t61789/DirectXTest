#pragma once

#include "common/const.h"

namespace dt
{
    struct RenderObject;
    class RenderComp;
    class Mesh;
    class Material;
    class Shader;
    
    class RenderTree
    {
    public:
        void Register(crsp<RenderObject> renderObject);
        void UnRegister(crsp<RenderObject> renderObject);

        crvecsp<RenderObject> GetRenderObjects() const { return m_renderObjects; }

    private:
        bool ExistsRenderObject(crsp<RenderObject> renderObject);
        
        vecsp<RenderObject> m_renderObjects;
    };
}
