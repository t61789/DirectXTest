#pragma once
#include <d3d12.h>
#include <d3dcommon.h>

#include "common/const.h"

namespace dt
{
    class DirectX;

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();
        RenderPipeline(const RenderPipeline& other) = delete;
        RenderPipeline(RenderPipeline&& other) noexcept = delete;
        RenderPipeline& operator=(const RenderPipeline& other) = delete;
        RenderPipeline& operator=(RenderPipeline&& other) noexcept = delete;

        void Render();

    private:
        void CreateMesh();
        void CreateRootSignature();
        void CreatePso();
        
        sp<DirectX> m_directx;
        ComPtr<ID3DBlob> m_vertexShader;
        ComPtr<ID3DBlob> m_pixelShader;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pso;
        vec<D3D12_INPUT_ELEMENT_DESC> m_layout;
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    };
}
