#pragma once

#include <directx/d3d12.h>

#include "dx_resource.h"
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
        static void SetViewport(ID3D12GraphicsCommandList* cmdList, uint32_t width, uint32_t height);
        
        static void BindRootSignature(ID3D12GraphicsCommandList* cmdList, const Shader* shader);
        static void BindPso(ID3D12GraphicsCommandList* cmdList, const Material* material);
        static void BindCbuffer(ID3D12GraphicsCommandList* cmdList, const Shader* shader, Cbuffer* cbuffer);
        static void BindBindlessTextures(ID3D12GraphicsCommandList* cmdList, const Shader* shader);
        static void BindMesh(ID3D12GraphicsCommandList* cmdList, const Mesh* mesh);
        
        static void AddTransition(crsp<DxResource> resource, D3D12_RESOURCE_STATES state);
        static void ApplyTransitions(ID3D12GraphicsCommandList* cmdList);
        
        static uint32_t GetRegisterType(D3D_SHADER_INPUT_TYPE resourceType);
        
        static void SetHeaps(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* const* heaps, uint32_t heapCount);

    private:
        struct Transition
        {
            sp<DxResource> resource;
            D3D12_RESOURCE_STATES state;
        };
        
        inline static vec<Transition> m_transitions;
    };
}
