#pragma once

#include <directx/d3d12.h>

#include "common/material.h"

namespace dt
{
    class Pso;
    class Shader;
    class Cbuffer;
    class Material;
    class Mesh;

    class DxHelper
    {
    public:
        static void PrepareRendering(ID3D12GraphicsCommandList* cmdList);
        static void SetViewport(ID3D12GraphicsCommandList* cmdList, uint32_t width, uint32_t height);
        
        static void BindRootSignature(ID3D12GraphicsCommandList* cmdList, const Shader* shader);
        static void BindPso(ID3D12GraphicsCommandList* cmdList, const Material* material);
        static void BindCbuffer(ID3D12GraphicsCommandList* cmdList, const Shader* shader, Cbuffer* cbuffer);
        static void BindMesh(ID3D12GraphicsCommandList* cmdList, const Mesh* mesh);
        
        static void AddTransition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
        static void ApplyTransitions(ID3D12GraphicsCommandList* cmdList);

        static uint32_t GetRegisterType(D3D_SHADER_INPUT_TYPE resourceType);

    private:
        inline static vec<D3D12_RESOURCE_BARRIER> m_transitions;
    };
}
