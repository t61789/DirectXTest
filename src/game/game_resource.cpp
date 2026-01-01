#include "game_resource.h"

#include <cassert>

#include "common/image.h"
#include "common/material.h"
#include "common/mesh.h"
#include "render/cbuffer.h"

namespace dt
{
    GameResource::GameResource()
    {
        errorTex = Image::LoadFromFile("built_in/textures/error.png");
        
        blitMat = Material::LoadFromFile("built_in/materials/blit.mtl");
        quadMesh = Mesh::LoadFromFile("built_in/meshes/quad.obj");
    }

    GameResource::~GameResource()
    {
        errorTex.reset();
        blitMat.reset();
        quadMesh.reset();
        
        m_predefinedCbuffers.clear();
    }

    sp<Cbuffer> GameResource::GetPredefinedCbuffer(cr<StringHandle> name) const
    {
        auto result = find_if(m_predefinedCbuffers, [&name](crsp<Cbuffer> cb)
        {
            return cb && cb->GetLayout()->name == name;
        });

        assert(result);

        if (result)
        {
            return *result;
        }

        return nullptr;
    }

    void GameResource::RegisterResource(cr<StringHandle> path, crsp<IResource> resource)
    {
        assert(!GetResource(path));

        m_resources[path] = resource;
    }

    void GameResource::UnregisterResource(cr<StringHandle> path)
    {
        assert(GetResource(path));
        
        m_resources.erase(path);
    }
    
    sp<IResource> GameResource::GetResource(cr<StringHandle> path)
    {
        auto it = m_resources.find(path);
        if (it == m_resources.end())
        {
            return nullptr;
        }

        if (it->second.expired())
        {
            m_resources.erase(it);
            return nullptr;
        }

        return it->second.lock();
    }
}
