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
        struct Register;
        struct ReflectionPack;
        
    public:
        void* GetVSPointer() const { return m_vs->GetBufferPointer(); }
        size_t GetVSSize() const { return m_vs->GetBufferSize(); }
        void* GetPSPointer() const { return m_ps->GetBufferPointer(); }
        size_t GetPSSize() const { return m_ps->GetBufferSize(); }
        cr<ComPtr<ID3D12RootSignature>> GetRootSignature() const { return m_rootSignature; }
        crvec<D3D12_INPUT_ELEMENT_DESC> GetVertexLayout() const { return m_vertexLayout; }
        cr<StringHandle> GetPath() override { return m_path; }
        Pso* GetPso() const { return m_pso.get(); }
        
        sp<Cbuffer> CreateCbuffer();
        
        static sp<Shader> LoadFromFile(cr<StringHandle> path);

    private:
        void DoLoad(cr<StringHandle> path);
        void LoadShaderInfo();
        void LoadVertexLayout(ReflectionPack& reflectionPack);
        void LoadCbuffers(const ReflectionPack& reflectionPack);
        void CreateRootSignature(ReflectionPack& reflectionPack);
        
        static void LoadShaderStage(cr<ComPtr<ID3DBlob>> blob, ReflectionPack& reflectionPack);
        static void LoadInputResources(ReflectionPack& reflectionPack);
        static ComPtr<ID3DBlob> CompileShader(crstr filePath, crstr entryPoint, crstr target);

        ComPtr<ID3DBlob> m_vs;
        ComPtr<ID3DBlob> m_ps;

        StringHandle m_path;

        vec<D3D12_INPUT_ELEMENT_DESC> m_vertexLayout;
        vecpair<string_hash, Register> m_resourceBindings;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoTemplateDesc = {};

        sp<CbufferLayout> m_localCbufferLayout = nullptr;

        sp<Pso> m_pso = nullptr;

        vec<str> m_semanticNames = {};

        struct Register
        {
            uint32_t rootParameterIndex = ~0u;
            uint32_t registerIndex = ~0u;
            StringHandle resourceName;
            D3D_SHADER_INPUT_TYPE resourceType;

            bool Empty() const { return registerIndex == ~0u; }

            friend bool operator==(const Register& lhs, const Register& rhs)
            {
                return lhs.rootParameterIndex == rhs.rootParameterIndex
                    && lhs.resourceName == rhs.resourceName
                    && lhs.resourceName == rhs.resourceName
                    && lhs.resourceType == rhs.resourceType;
            }

            friend bool operator!=(const Register& lhs, const Register& rhs)
            {
                return !(lhs == rhs);
            }
        };

        struct ReflectionPack
        {
            ReflectionPack() = default;
            
            ComPtr<ID3D12ShaderReflection> shaderReflection;
            D3D12_SHADER_DESC shaderDesc;
            ID3D12ShaderReflectionConstantBuffer* cbReflection;
            D3D12_SHADER_BUFFER_DESC cbDesc;

            Register registers[MAX_REGISTER_COUNT * 4];

            Register* GetRegisterGroup(D3D_SHADER_INPUT_TYPE resourceType);
        };
    };
}
