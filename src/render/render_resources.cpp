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
        perViewCbuffer->Write(V, Transpose(vp->vMatrix));
        perViewCbuffer->Write(VP, Transpose(vp->vpMatrix));
        perViewCbuffer->Write(CAMERA_POSITION_WS, vp->viewCenter);

        if (vp->ivpMatrix.has_value())
        {
            perViewCbuffer->Write(IVP, Transpose(vp->ivpMatrix.value()));
        }
    }
}
