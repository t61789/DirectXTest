#include "render_resources.h"

#include "cbuffer.h"
#include "game/game_resource.h"

namespace dt
{
    void RenderResources::SetVp(crsp<ViewProjInfo> vp)
    {
        if (curVp == vp)
        {
            return;
        }
        curVp = vp;
        
        auto perViewCbuffer = GetPerViewCbuffer();
        perViewCbuffer->Write(VP, &vp->vpMatrix, sizeof(XMFLOAT4X4));
        perViewCbuffer->Write(CAMERA_POSITION_WS, &vp->viewCenter, sizeof(XMFLOAT4));

        if (vp->ivpMatrix.has_value())
        {
            perViewCbuffer->Write(IVP, &vp->ivpMatrix, sizeof(XMFLOAT4X4));
        }
    }
}
