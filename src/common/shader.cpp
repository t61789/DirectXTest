#include "shader.h"

#include <D3Dcompiler.h>
#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

#include "game/game_resource.h"
#include "render/directx.h"
#include "render/dx_helper.h"

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

    sp<Shader> Shader::Create(cr<StringHandle> path, VariantKeyword keywords)
    {
        auto shader = msp<Shader>();
        shader->m_path = path;
        shader->m_keywords = std::move(keywords);
        
        shader->DoLoad();

        return shader;
    }

    void Shader::DoLoad()
    {
        log_info("Load shader: %s, variants %s", m_path.CStr(), m_keywords.GetStr().c_str());

        m_vsDxcResult = CompileShader(m_path.CStr(), L"VS_Main", L"vs_6_0", m_keywords);
        m_psDxcResult = CompileShader(m_path.CStr(), L"PS_Main", L"ps_6_0", m_keywords);

        THROW_IF_FAILED(m_psDxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&m_ps), nullptr));
        THROW_IF_FAILED(m_vsDxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&m_vs), nullptr));

        LoadShaderInfo();

        m_defaultParams = Utils::GetResourceMeta(m_path);
    }

    void Shader::LoadShaderInfo()
    {
        ReflectionPack reflectionPack;
        
        LoadShaderStage(m_vsDxcResult, reflectionPack);
        LoadInputResources(reflectionPack);
        LoadCbuffers(reflectionPack);
        LoadVertexLayout(reflectionPack);
        
        LoadShaderStage(m_psDxcResult, reflectionPack);
        LoadInputResources(reflectionPack);
        LoadCbuffers(reflectionPack);
        LoadOutputResources(reflectionPack);

        CreateRootSignature(reflectionPack);
        
        m_pso = msp<Pso>(this);
    }

    void Shader::LoadShaderStage(cr<ComPtr<IDxcResult>> dxcResult, ReflectionPack& reflectionPack)
    {
        ComPtr<IDxcBlob> pReflectionData;
        THROW_IF_FAILED(dxcResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&pReflectionData), nullptr));
        assert(pReflectionData != nullptr);
        
        DxcBuffer reflectionBuffer;
        reflectionBuffer.Ptr = pReflectionData->GetBufferPointer();
        reflectionBuffer.Size = pReflectionData->GetBufferSize();
        reflectionBuffer.Encoding = 0;

        THROW_IF_FAILED(Dx()->GetDxcUtils()->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflectionPack.shaderReflection)));
        
        THROW_IF_FAILED(reflectionPack.shaderReflection->GetDesc(&reflectionPack.shaderDesc));
    }

    void Shader::CreateRootSignature(const ReflectionPack& reflectionPack)
    {
        vecup<CD3DX12_DESCRIPTOR_RANGE1> rootParamRanges;
        vec<CD3DX12_ROOT_PARAMETER1> rootParams;
        rootParamRanges.reserve(reflectionPack.bindResources.size());
        rootParams.reserve(reflectionPack.bindResources.size());
        
        // Build root parameters from bind resources
        for (auto& bindResource : reflectionPack.bindResources)
        {
            rootParams.emplace_back();

            switch (bindResource.resourceType)
            {
            case D3D_SIT_CBUFFER:
                {
                    if (bindResource.registerIndex == ROOT_CONSTANTS_CBUFFER_REGISTER_INDEX && bindResource.registerSpace == ROOT_CONSTANTS_CBUFFER_REGISTER_SPACE)
                    {
                        rootParams.back().InitAsConstants(ROOT_CONSTANTS_CBUFFER_SIZE_DWORD, bindResource.registerIndex, bindResource.registerSpace);
                        m_rootConstantCbufferRootParamIndex = rootParams.size() - 1;
                    }
                    else
                    {
                        rootParams.back().InitAsConstantBufferView(bindResource.registerIndex, bindResource.registerSpace);
                    }
                    break;
                }
            case D3D_SIT_BYTEADDRESS:
            case D3D_SIT_TEXTURE:
                { 
                    D3D12_SHADER_VISIBILITY shaderVisibility;
                    
                    if (bindResource.resourceDimension == D3D_SRV_DIMENSION_TEXTURE2D)
                    {
                        if (bindResource.resourceName != BINDLESS_2D_TEXTURES || bindResource.registerIndex != 0 || bindResource.registerSpace != 1)
                        {
                            THROW_ERROR("Texture2d resource needs to be bindless with t0 and space1")
                        }

                        shaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                    }
                    else if (bindResource.resourceDimension == D3D_SRV_DIMENSION_TEXTURECUBE)
                    {
                        if (bindResource.resourceName != BINDLESS_CUBE_TEXTURES || bindResource.registerIndex != 0 || bindResource.registerSpace != 2)
                        {
                            THROW_ERROR("TextureCube resource needs to be bindless with t0 and space2")
                        }

                        shaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                    }
                    else if (bindResource.resourceDimension == D3D_SRV_DIMENSION_BUFFER)
                    {
                        if (bindResource.resourceName != BINDLESS_BYTE_BUFFERS || bindResource.registerIndex != 0 || bindResource.registerSpace != 3)
                        {
                            THROW_ERROR("Byte buffer resource needs to be bindless with t0 and space3")
                        }

                        shaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                    }
                    else
                    {
                        THROW_ERROR("Unsupported texture resource dimension")
                    }
                    
                    auto range = mup<CD3DX12_DESCRIPTOR_RANGE1>();
                    range->Init(
                        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                        bindResource.bindCount,
                        bindResource.registerIndex,
                        bindResource.registerSpace,
                        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
                        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                    rootParamRanges.push_back(std::move(range));

                    rootParams.back().InitAsDescriptorTable(1, rootParamRanges.back().get(), shaderVisibility);
                    break;
                }
            case D3D_SIT_SAMPLER:
                {
                    if (bindResource.resourceName != BINDLESS_SAMPLERS || bindResource.registerIndex != 0 || bindResource.registerSpace != 1)
                    {
                        THROW_ERROR("Sampler resource needs to be bindless with s0 and space1")
                    }
                    
                    auto range = mup<CD3DX12_DESCRIPTOR_RANGE1>();
                    range->Init(
                        D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
                        bindResource.bindCount,
                        bindResource.registerIndex,
                        bindResource.registerSpace,
                        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
                        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                    rootParamRanges.push_back(std::move(range));
                    
                    rootParams.back().InitAsDescriptorTable(1, rootParamRanges.back().get(), D3D12_SHADER_VISIBILITY_PIXEL);
                    break;
                }
            default:
                {
                    THROW_ERROR("Unsupported resource type")
                }
            }
        }

        // Record bind resources for future binding query
        m_bindResources = reflectionPack.bindResources;

        // Create actual root signature by root parameters for dx
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(
            rootParams.size(),
            rootParams.data(),
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        
        ComPtr<ID3DBlob> rootSignatureBlob;
        ComPtr<ID3DBlob> errorBlob;

        if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &rootSignatureBlob, &errorBlob)))
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

            BindResource bindResource;
            bindResource.resourceName = StringHandle(resourceDesc.Name);
            bindResource.registerIndex = resourceDesc.BindPoint;
            bindResource.registerSpace = resourceDesc.Space;
            bindResource.resourceType = resourceDesc.Type;
            bindResource.resourceDimension = resourceDesc.Dimension;
            bindResource.registerType = DxHelper::GetRegisterType(bindResource.resourceType);
            bindResource.bindCount = resourceDesc.BindCount;

            auto found = find_if(reflectionPack.bindResources, [&bindResource](cr<BindResource> x)
            {
                if (x.registerIndex == bindResource.registerIndex &&
                    x.registerType == bindResource.registerType &&
                    x.registerSpace == bindResource.registerSpace)
                {
                    if (x.resourceName != bindResource.resourceName ||
                        x.resourceType != bindResource.resourceType ||
                        x.bindCount != bindResource.bindCount ||
                        x.resourceDimension != bindResource.resourceDimension)
                    {
                        throw std::runtime_error("Different shader stage bound different resource in same register");
                    }
                    return true;
                }
                return false;
            });

            if (!found)
            {
                bindResource.rootParameterIndex = reflectionPack.bindResources.size();
                reflectionPack.bindResources.push_back(bindResource);
            }
        }
    }

    void Shader::LoadOutputResources(const ReflectionPack& reflectionPack)
    {
        m_outputCount = reflectionPack.shaderDesc.OutputParameters;
    }

    void Shader::LoadVertexLayout(const ReflectionPack& reflectionPack)
    {
        uint32_t curOffset = 0;
        m_semanticNames.resize(reflectionPack.shaderDesc.InputParameters);
        for (uint32_t i = 0; i < reflectionPack.shaderDesc.InputParameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC inputParamDesc;
            THROW_IF_FAILED(reflectionPack.shaderReflection->GetInputParameterDesc(i, &inputParamDesc));

            if (inputParamDesc.SystemValueType == D3D_NAME_INSTANCE_ID || inputParamDesc.SystemValueType == D3D_NAME_VERTEX_ID)
            {
                continue;
            }

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

    ComPtr<IDxcResult> Shader::CompileShader(crstr filePath, const wchar_t* entryPoint, const wchar_t* target, cr<VariantKeyword> keywords)
    {
        ASSERT_THROW(Utils::AssetExists(filePath));

        vec<const wchar_t*> pszArgs = {
            L"shader.hlsl",
            L"-E", entryPoint,
            L"-T", target,
            L"-Zi", // 启用调试
            L"-flegacy-resource-reservation"
        };

        for (auto& keyword : keywords.GetKeywordsWStr())
        {
            pszArgs.push_back(L"-D");
            pszArgs.push_back(keyword.c_str());
        }

        ComPtr<IDxcBlobEncoding> pSourceBlob;
        THROW_IF_FAILED(Dx()->GetDxcUtils()->LoadFile(Utils::StringToWString(Utils::ToAbsPath(filePath)).c_str(), nullptr, &pSourceBlob));

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = pSourceBlob->GetBufferPointer();
        sourceBuffer.Size = pSourceBlob->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        ComPtr<IDxcResult> result;
        THROW_IF_FAILED(Dx()->GetDxcCompiler()->Compile(&sourceBuffer, pszArgs.data(), pszArgs.size(), Dx()->GetDxcIncludeHandler(), IID_PPV_ARGS(&result)));

        ComPtr<IDxcBlobUtf8> pErrors;
        THROW_IF_FAILED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
        if (pErrors && pErrors->GetStringLength() > 0)
        {
            log_error("Compile shader error:\n%s", pErrors->GetStringPointer());
            throw std::runtime_error("Failed to compile shader.");
        }

        return result;
    }
}
