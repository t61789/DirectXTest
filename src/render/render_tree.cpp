#include "render_tree.h"

#include "render_resources.h"
#include "common/material.h"
#include "game/game_resource.h"
#include "objects/render_comp.h"

namespace dt
{
    void RenderTree::Register(crsp<RenderObject> renderObject)
    {
        assert(!ExistsRenderObject(renderObject));
        
        m_renderObjects.push_back(renderObject);

        std::sort(m_renderObjects.begin(), m_renderObjects.end(), [](crsp<RenderObject> a, crsp<RenderObject> b)
        {
            if (a->shader == b->shader)
            {
                if (a->material == b->material)
                {
                    if (a->mesh == b->mesh)
                    {
                        return false;
                    }
                    return a->mesh < b->mesh;
                }
                return a->material < b->material;
            }
            return a->shader < b->shader;
        });
    }

    void RenderTree::UnRegister(crsp<RenderObject> renderObject)
    {
        assert(ExistsRenderObject(renderObject));

        remove(m_renderObjects, renderObject);
    }

    bool RenderTree::ExistsRenderObject(crsp<RenderObject> renderObject)
    {
        return exists_if(m_renderObjects, [&renderObject](crsp<RenderObject> x){ return x == renderObject; });
    }
}
