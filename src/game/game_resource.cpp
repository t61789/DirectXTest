#include "game_resource.h"

#include <cassert>

namespace dt
{
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
