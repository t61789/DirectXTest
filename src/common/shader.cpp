#include "shader.h"

#include <D3Dcompiler.h>
#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

#include "game/game_resource.h"
#include "render/directx.h"

namespace
{
    void GetFieldInfo(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc, DXGI_FORMAT& format, uint32_t& sizeB)
    {
        // 计算分量数量
        uint32_t componentCount = 0;
        if (paramDesc.Mask & 1) componentCount++;
        if (paramDesc.Mask & 2) componentCount++;
        if (paramDesc.Mask & 4) componentCount++;
        if (paramDesc.Mask & 8) componentCount++;

        using format_tuple = std::tuple<D3D_REGISTER_COMPONENT_TYPE, uint32_t, DXGI_FORMAT>;
        
        static dt::vec<format_tuple> componentTypeToFormat =
        {
            { D3D_REGISTER_COMPONENT_FLOAT32, 1 , DXGI_FORMAT_R32_FLOAT },
            { D3D_REGISTER_COMPONENT_FLOAT32, 2 , DXGI_FORMAT_R32G32_FLOAT },
            { D3D_REGISTER_COMPONENT_FLOAT32, 3 , DXGI_FORMAT_R32G32B32_FLOAT },
            { D3D_REGISTER_COMPONENT_FLOAT32, 4 , DXGI_FORMAT_R32G32B32A32_FLOAT },
            { D3D_REGISTER_COMPONENT_UINT32, 1 , DXGI_FORMAT_R32_UINT },
            { D3D_REGISTER_COMPONENT_UINT32, 2 , DXGI_FORMAT_R32G32_UINT },
            { D3D_REGISTER_COMPONENT_UINT32, 3 , DXGI_FORMAT_R32G32B32_UINT },
            { D3D_REGISTER_COMPONENT_UINT32, 4 , DXGI_FORMAT_R32G32B32A32_UINT },
            { D3D_REGISTER_COMPONENT_SINT32, 1 , DXGI_FORMAT_R32_SINT },
            { D3D_REGISTER_COMPONENT_SINT32, 2 , DXGI_FORMAT_R32G32_SINT },
            { D3D_REGISTER_COMPONENT_SINT32, 3 , DXGI_FORMAT_R32G32B32_SINT },
            { D3D_REGISTER_COMPONENT_SINT32, 4 , DXGI_FORMAT_R32G32B32A32_SINT },
        };

        auto ff = dt::find_if(componentTypeToFormat, [&paramDesc, componentCount](const format_tuple& t)
        {
            return std::get<0>(t) == paramDesc.ComponentType &&
                std::get<1>(t) == componentCount;
        });
        ASSERT_THROW(ff);

        format = std::get<2>(*ff);
        sizeB = componentCount * 4;
    }
}

namespace dt
{
    sp<Cbuffer> Shader::CreateCbuffer()
    {
        if (!m_localCbufferLayout)
        {
            return nullptr;
        }
        return msp<Cbuffer>(m_localCbufferLayout);
    }

    sp<Shader> Shader::LoadFromFile(cr<StringHandle> path)
    {
        {
            if (auto mesh = GR()->GetResource<Shader>(path))
            {
                return mesh;
            }
        }

        auto shader = msp<Shader>();
        shader->m_path = path;
        
        shader->DoLoad(path);

        GR()->RegisterResource(path, shader);

        return shader;
    }

    void Shader::DoLoad(cr<StringHandle> path)
    {
        log_info("Loading shader: %s", path.CStr());

        m_ps = CompileShader(path.CStr(), "PS_Main", "ps_5_0");
        m_vs = CompileShader(path.CStr(), "VS_Main", "vs_5_0");

        LoadShaderInfo();
    }

    void Shader::LoadShaderInfo()
    {
        ReflectionPack reflectionPack;
        
        LoadShaderStage(m_vs, reflectionPack);
        LoadInputResources(reflectionPack);
        LoadCbuffers(reflectionPack);
        LoadVertexLayout(reflectionPack);
        
        LoadShaderStage(m_ps, reflectionPack);
        LoadInputResources(reflectionPack);
        LoadCbuffers(reflectionPack);

        CreateRootSignature(reflectionPack);
        
        m_pso = msp<Pso>(this);
    }

    void Shader::LoadShaderStage(cr<ComPtr<ID3DBlob>> blob, ReflectionPack& reflectionPack)
    {
        THROW_IF_FAILED(D3DReflect(
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            IID_ID3D12ShaderReflection,
            &reflectionPack.shaderReflection));
        
        THROW_IF_FAILED(reflectionPack.shaderReflection->GetDesc(&reflectionPack.shaderDesc));
    }

    void Shader::CreateRootSignature(ReflectionPack& reflectionPack)
    {
        vec<CD3DX12_ROOT_PARAMETER> rootParameters;
        for (auto& registerInfo : reflectionPack.registers)
        {
            if (registerInfo.Empty())
            {
                continue;
            }
            auto registerIndex = registerInfo.registerIndex;
            
            rootParameters.emplace_back();

            if (registerInfo.resourceType == D3D_SIT_CBUFFER)
            {
                rootParameters.back().InitAsConstantBufferView(registerIndex);
                registerInfo.rootParameterIndex = static_cast<uint32_t>(rootParameters.size() - 1);
            }
            else
            {
                throw std::runtime_error("Unsupported resource type.");
            }

            m_resourceBindings.emplace_back(registerInfo.resourceName.Hash(), registerInfo);
        }
        
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = rootParameters.size();
        rootSignatureDesc.pParameters = rootParameters.data();
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ComPtr<ID3DBlob> rootSignatureBlob;
        ComPtr<ID3DBlob> errorBlob;

        if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob)))
        {
            DirectX::ThrowErrorBlob(errorBlob);
        }

        THROW_IF_FAILED(Dx()->GetDevice()->CreateRootSignature(
            0,
            rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(),
            IID_ID3D12RootSignature,
            &m_rootSignature));
    }

    void Shader::LoadInputResources(ReflectionPack& reflectionPack)
    {
        for (uint32_t i = 0; i < reflectionPack.shaderDesc.BoundResources; ++i)
        {
            D3D12_SHADER_INPUT_BIND_DESC resourceDesc;
            THROW_IF_FAILED(reflectionPack.shaderReflection->GetResourceBindingDesc(i, &resourceDesc));
            ASSERT_THROWM(resourceDesc.BindPoint < MAX_REGISTER_COUNT, "Shader resource register out of range.");

            Register registerInfo;
            registerInfo.resourceName = StringHandle(resourceDesc.Name);
            registerInfo.registerIndex = resourceDesc.BindPoint;
            registerInfo.resourceType = resourceDesc.Type;
            
            auto& destRegisterInfo = reflectionPack.GetRegisterGroup(resourceDesc.Type)[resourceDesc.BindPoint];
            THROW_IF(destRegisterInfo != registerInfo && !destRegisterInfo.Empty(), "Different shader stage bound different resource in same register");
            destRegisterInfo = registerInfo;
        }
    }

    void Shader::LoadVertexLayout(ReflectionPack& reflectionPack)
    {
        uint32_t curOffset = 0;
        m_semanticNames.resize(reflectionPack.shaderDesc.InputParameters);
        for (uint32_t i = 0; i < reflectionPack.shaderDesc.InputParameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC inputParamDesc;
            THROW_IF_FAILED(reflectionPack.shaderReflection->GetInputParameterDesc(i, &inputParamDesc));

            DXGI_FORMAT fieldFormat;
            uint32_t fieldSizeB;
            GetFieldInfo(inputParamDesc, fieldFormat, fieldSizeB);

            m_semanticNames[i] = str(inputParamDesc.SemanticName);
            
            D3D12_INPUT_ELEMENT_DESC inputElementDesc;
            inputElementDesc.SemanticName = m_semanticNames[i].c_str();
            inputElementDesc.SemanticIndex = inputParamDesc.SemanticIndex;
            inputElementDesc.Format = fieldFormat;
            inputElementDesc.InputSlot = 0;
            inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            inputElementDesc.AlignedByteOffset = curOffset;
            inputElementDesc.InstanceDataStepRate = 0;
            curOffset += fieldSizeB;

            m_vertexLayout.push_back(inputElementDesc);
        }
    }

    void Shader::LoadCbuffers(const ReflectionPack& reflectionPack)
    {
        if (m_localCbufferLayout)
        {
            return;
        }
        
        for (uint32_t i = 0; i < reflectionPack.shaderDesc.ConstantBuffers; ++i)
        {
            auto cbufferReflection = reflectionPack.shaderReflection->GetConstantBufferByIndex(i);

            D3D12_SHADER_BUFFER_DESC cbufferDesc;
            THROW_IF_FAILED(cbufferReflection->GetDesc(&cbufferDesc));
        
            auto predefinedCbufferIndex = find_index(PREDEFINED_CBUFFER, StringHandle(cbufferDesc.Name).Hash());
            if (predefinedCbufferIndex.has_value())
            {
                if (!GR()->m_predefinedCbuffers[predefinedCbufferIndex.value()])
                {
                    auto layout = msp<CbufferLayout>(cbufferReflection, cbufferDesc);
                    GR()->m_predefinedCbuffers[predefinedCbufferIndex.value()] = msp<Cbuffer>(std::move(layout));
                }

                continue;
            }

            m_localCbufferLayout = msp<CbufferLayout>(cbufferReflection, cbufferDesc);
            return;
        }
    }

    ComPtr<ID3DBlob> Shader::CompileShader(crstr filePath, crstr entryPoint, crstr target)
    {
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        uint32_t compileFlags = 0;
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        auto absFilePath = Utils::StringToWString(Utils::ToAbsPath(filePath));
        auto result = D3DCompileFromFile(
            absFilePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(),
            target.c_str(),
            compileFlags,
            0,
            &shaderBlob,
            &errorBlob);

        if (FAILED(result))
        {
            if (errorBlob)
            {
                log_error("Compile shader error:\n%s", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }

            throw std::runtime_error("Failed to compile shader.");
        }

        return shaderBlob;
    }

    Shader::Register* Shader::ReflectionPack::GetRegisterGroup(const D3D_SHADER_INPUT_TYPE resourceType)
    {
        static umap<D3D_SHADER_INPUT_TYPE, uint32_t> mapper = {
            { D3D_SIT_CBUFFER, 0 },
        };

        auto registerType = mapper.at(resourceType);
        return &registers[registerType * MAX_REGISTER_COUNT];
    }
}
