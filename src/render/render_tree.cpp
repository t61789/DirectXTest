#include "render_tree.h"

#include "render_resources.h"
#include "common/material.h"
#include "game/game_resource.h"
#include "objects/render_comp.h"

namespace dt
{
    void RenderTree::Register(const RenderComp* renderComp)
    {
        auto ro = renderComp->GetRenderObject();
        assert(!ExistsRenderObject(ro));
        
        m_renderObjects.push_back(ro);

        std::sort(m_renderObjects.begin(), m_renderObjects.end(), [](crsp<RenderObject> a, crsp<RenderObject> b)
        {
            if (a->shader == b->shader)
            {
                if (a->material == b->material)
                {
                    if (a->mesh == b->mesh)
                    {
                        return true;
                    }
                    return a->mesh < b->mesh;
                }
                return a->material < b->material;
            }
            return a->shader < b->shader;
        });
    }

    void RenderTree::UnRegister(const RenderComp* renderComp)
    {
        auto ro = renderComp->GetRenderObject();
        assert(ExistsRenderObject(ro));

        remove(m_renderObjects, ro);
    }

    bool RenderTree::ExistsRenderObject(crsp<RenderObject> renderObject)
    {
        return exists_if(m_renderObjects, [&renderObject](crsp<RenderObject> x){ return x == renderObject; });
    }
}
