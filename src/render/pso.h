#pragma once
#include <directx/d3d12.h>
#include <wrl/client.h>

#include "render_state.h"
#include "common/const.h"

namespace dt
{
    class Shader;
    using namespace Microsoft::WRL;
    
    class Pso
    {
    public:
        explicit Pso(Shader* shader);

        ID3D12PipelineState* CurPso();

        void SetDepthMode(DepthMode mode);
        void SetDepthWrite(bool enable);
        void SetCullMode(CullMode mode);
        void SetRenderTargetFormat(uint32_t index, DXGI_FORMAT format);

        void Update();

    private:
        bool m_dirty = true;
        DepthMode m_depthMode = DepthMode::LESS;
        bool m_depthWrite = true;
        CullMode m_cullMode = CullMode::BACK;
        BlendMode m_blendMode = BlendMode::NONE;
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc;
        ComPtr<ID3D12PipelineState> m_curPso;

        umap<size_t, ComPtr<ID3D12PipelineState>> m_psoPool = {};
    };
}
