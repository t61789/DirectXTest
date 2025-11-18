#pragma once
#include <d3d12shader.h>
#include <d3dcommon.h>
#include <optional>
#include <directx/d3d12.h>
#include <wrl/client.h>

#include "i_resource.h"
#include "const.h"
#include "render/cbuffer.h"
#include "render/pso.h"

namespace dt
{
    class Cbuffer;
    using namespace Microsoft::WRL;
    
    class Shader final : public IResource
    {
        struct ReflectionPack;
        
    public:
        struct BindResource
        {
            uint32_t rootParameterIndex;
            uint32_t registerIndex;
            uint32_t registerType;
            StringHandle resourceName;
            D3D_SHADER_INPUT_TYPE resourceType;
        };

        void* GetVSPointer() const { return m_vs->GetBufferPointer(); }
        size_t GetVSSize() const { return m_vs->GetBufferSize(); }
        void* GetPSPointer() const { return m_ps->GetBufferPointer(); }
        size_t GetPSSize() const { return m_ps->GetBufferSize(); }
        cr<ComPtr<ID3D12RootSignature>> GetRootSignature() const { return m_rootSignature; }
        crvec<D3D12_INPUT_ELEMENT_DESC> GetVertexLayout() const { return m_vertexLayout; }
        cr<StringHandle> GetPath() override { return m_path; }
        Pso* GetPso() const { return m_pso.get(); }
        crvec<BindResource> GetBindResources() const { return m_bindResources; }
        
        sp<Cbuffer> CreateCbuffer();
        
        static sp<Shader> LoadFromFile(cr<StringHandle> path);

    private:
        void DoLoad(cr<StringHandle> path);
        void LoadShaderInfo();
        void LoadVertexLayout(ReflectionPack& reflectionPack);
        void LoadCbuffers(const ReflectionPack& reflectionPack);
        void CreateRootSignature(const ReflectionPack& reflectionPack);
        
        static void LoadShaderStage(cr<ComPtr<ID3DBlob>> blob, ReflectionPack& reflectionPack);
        static void LoadInputResources(ReflectionPack& reflectionPack);
        static ComPtr<ID3DBlob> CompileShader(crstr filePath, crstr entryPoint, crstr target);

        ComPtr<ID3DBlob> m_vs;
        ComPtr<ID3DBlob> m_ps;

        StringHandle m_path;

        vec<D3D12_INPUT_ELEMENT_DESC> m_vertexLayout;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        vec<BindResource> m_bindResources;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoTemplateDesc = {};

        sp<CbufferLayout> m_localCbufferLayout = nullptr;

        sp<Pso> m_pso = nullptr;

        vec<str> m_semanticNames = {};

        struct ReflectionPack
        {
            ReflectionPack() = default;
            
            ComPtr<ID3D12ShaderReflection> shaderReflection;
            D3D12_SHADER_DESC shaderDesc;
            ID3D12ShaderReflectionConstantBuffer* cbReflection;
            D3D12_SHADER_BUFFER_DESC cbDesc;

            vec<BindResource> bindResources;
        };
    };
}
