#include "mesh.h"

#include <d3d12.h>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <fstream>
#include <mutex>
#include <directx/d3dx12_barriers.h>
#include <directx/d3dx12_core.h>
#include <tracy/Tracy.hpp>

#include "common/math.h"
#include "common/asset_cache.h"
#include "game/game_resource.h"
#include "render/directx.h"

namespace dt
{
    static crumap<VertexAttr, Mesh::VertexAttrInfo> GetFullVertexAttribInfo()
    {
        static umap<VertexAttr, Mesh::VertexAttrInfo> vertexAttribInfo;
        if (vertexAttribInfo.empty())
        {
            for (auto& attrDefine : VERTEX_ATTR_DEFINES)
            {
                vertexAttribInfo[attrDefine.attr] = {
                    true,
                    attrDefine.offsetF * sizeof(float)
                };
            }
        }

        return vertexAttribInfo;
    }
    
    static vec<D3D12_INPUT_ELEMENT_DESC> GetD3dVertexLayout(crumap<VertexAttr, Mesh::VertexAttrInfo> vertexAttribInfo)
    {
        static umap<VertexAttr, D3D12_INPUT_ELEMENT_DESC> INPUT_ELEMENT_DESC_MAPPER =
        {
            {VertexAttr::POSITION_OS, { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }},
            {VertexAttr::NORMAL_OS, { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }},
            {VertexAttr::TANGENT_OS, { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }},
            {VertexAttr::UV0, { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }},
            {VertexAttr::UV1, { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }},
        };

        vec<D3D12_INPUT_ELEMENT_DESC> inputLayout;
        for (auto& attrInfo : vertexAttribInfo)
        {
            if (!attrInfo.second.enabled)
            {
                continue;
            }
                
            auto desc = INPUT_ELEMENT_DESC_MAPPER.at(attrInfo.first);
            desc.AlignedByteOffset = attrInfo.second.offsetB;
            inputLayout.push_back(desc);
        }

        return inputLayout;
    }
    
    sp<Mesh> Mesh::LoadFromFile(crstr modelPath)
    {
        {
            if (auto mesh = GR()->GetResource<Mesh>(modelPath))
            {
                return mesh;
            }
        }
        
        ZoneScoped;

        auto result = AssetCache::GetFromCache<Mesh, Cache>(modelPath);

        GR()->RegisterResource(modelPath, result);
        result->m_path = modelPath;
        
        log_info("Load mesh: %s", modelPath.c_str());
        
        return result;
    }

    sp<Mesh> Mesh::LoadFromFileImp(crstr modelPath)
    {
        auto importer = ImportFile(modelPath);
        auto scene = importer->GetScene();

        auto mesh = scene->mMeshes[0];
        auto verticesCount = mesh->mNumVertices;
        
        std::unordered_map<VertexAttr, VertexAttrInfo> vertexAttribInfo;
        vertexAttribInfo[VertexAttr::POSITION_OS] = {true};
        vertexAttribInfo[VertexAttr::NORMAL_OS] = {mesh->mNormals != nullptr};
        vertexAttribInfo[VertexAttr::TANGENT_OS] = {mesh->mTangents != nullptr};
        vertexAttribInfo[VertexAttr::UV0] = {mesh->HasTextureCoords(0)};
        vertexAttribInfo[VertexAttr::UV1] = {mesh->HasTextureCoords(1)};

        auto boundsMin = XMVectorReplicate((std::numeric_limits<float>::max)());
        auto boundsMax = XMVectorReplicate((std::numeric_limits<float>::min)());

        // 遍历每个顶点，得到交错的顶点数据
        std::vector<float> vertexData;
        for (uint32_t i = 0; i < verticesCount; ++i)
        {
            // Load positionOS
            auto positionOS = XMVectorSet(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z, 0);
            boundsMin = XMVectorMin(boundsMin, positionOS);
            boundsMax = XMVectorMax(boundsMax, positionOS);
            vertexData.push_back(XMVectorGetX(positionOS));
            vertexData.push_back(XMVectorGetY(positionOS));
            vertexData.push_back(XMVectorGetZ(positionOS));
            vertexData.push_back(1.0f);

            // Load normalOS
            if (vertexAttribInfo[VertexAttr::NORMAL_OS].enabled)
            {
                vertexData.push_back(mesh->mNormals[i].x);
                vertexData.push_back(mesh->mNormals[i].y);
                vertexData.push_back(mesh->mNormals[i].z);
                vertexData.push_back(0.0f);
            }
            else
            {
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
            }

            // Load tangentOS
            if (vertexAttribInfo[VertexAttr::TANGENT_OS].enabled)
            {
                vertexData.push_back(mesh->mTangents[i].x);
                vertexData.push_back(mesh->mTangents[i].y);
                vertexData.push_back(mesh->mTangents[i].z);
                vertexData.push_back(1.0f);
            }
            else
            {
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
                vertexData.push_back(1.0f);
            }

            // Load uv
            VertexAttr uvAttr[] = {VertexAttr::UV0, VertexAttr::UV1};
            for (size_t j = 0; j < 2; ++j)
            {
                if (vertexAttribInfo[uvAttr[j]].enabled)
                {
                    vertexData.push_back(mesh->mTextureCoords[j][i].x);
                    vertexData.push_back(mesh->mTextureCoords[j][i].y);
                }
                else
                {
                    vertexData.push_back(0.0f);
                    vertexData.push_back(0.0f);
                }
            }
        }

        // Load indices
        std::vector<uint32_t> indices;
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            auto face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; ++j)
            {
                indices.push_back(face.mIndices[j]);
            }
        }
        
        for (auto& attrInfo : vertexAttribInfo)
        {
            attrInfo.second.enabled = true;
        }
        CalcVertexAttrOffset(vertexAttribInfo);

        auto bounds = Bounds((boundsMax + boundsMin) * 0.5f, (boundsMax - boundsMin) * 0.5f);
        bounds.extents = XMVectorMax(bounds.extents, XMVectorReplicate(0.01f));

        auto result = CreateMesh(
            std::move(vertexData),
            std::move(indices),
            verticesCount,
            std::move(vertexAttribInfo),
            bounds);

        return result;
    }

    up<Assimp::Importer> Mesh::ImportFile(crstr modelPath)
    {
        float initScale;
        bool flipWindingOrder;
        GetMeshLoadConfig(modelPath, initScale, flipWindingOrder);
        
        auto importer = mup<Assimp::Importer>();
        importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, initScale);

        unsigned int pFlags = 
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_GlobalScale;

        if (flipWindingOrder)
        {
            pFlags |= aiProcess_FlipWindingOrder;
        }
        
        const aiScene *scene = importer->ReadFile(Utils::ToAbsPath(modelPath).c_str(), pFlags);
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            THROW_ERRORF("Load model failed: %s", importer->GetErrorString())
        }

        return importer;
    }
    
    sp<Mesh> Mesh::CreateMesh(
        vec<float>&& vertexData,
        vec<uint32_t>&& indices,
        const uint32_t vertexCount,
        crumap<VertexAttr, VertexAttrInfo> vertexAttribInfo,
        cr<Bounds> bounds)
    {
        auto vertexDataStrideB = static_cast<int>(vertexData.size() / vertexCount * sizeof(float));
        
        auto vbUploadBuffer = Dx()->CreateUploadBuffer(
            vertexData.data(),
            vertexData.size() * sizeof(float));
        auto vbBuffer = Dx()->CreateCommittedResource(
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            CD3DX12_RESOURCE_DESC::Buffer(vbUploadBuffer->GetDesc().Width),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            D3D12_HEAP_FLAG_NONE,
            "Vertex Buffer");
        D3D12_VERTEX_BUFFER_VIEW vbView = {};
        vbView.SizeInBytes = vbUploadBuffer->GetDesc().Width;
        vbView.StrideInBytes = vertexDataStrideB;
        vbView.BufferLocation = vbBuffer->GetGPUVirtualAddress();

        auto ibUploadBuffer = Dx()->CreateUploadBuffer(
            indices.data(),
            indices.size() * sizeof(uint32_t));
        auto ibBuffer = Dx()->CreateCommittedResource(
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            CD3DX12_RESOURCE_DESC::Buffer(ibUploadBuffer->GetDesc().Width),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            D3D12_HEAP_FLAG_NONE,
            "Index Buffer");
        D3D12_INDEX_BUFFER_VIEW ibView = {};
        ibView.Format = DXGI_FORMAT_R32_UINT;
        ibView.SizeInBytes = ibUploadBuffer->GetDesc().Width;
        ibView.BufferLocation = ibBuffer->GetGPUVirtualAddress();

        Dx()->AddCommand([vbUploadBuffer, vbBuffer, ibUploadBuffer, ibBuffer](ID3D12GraphicsCommandList* cmdList)
        {
            Dx()->AddTransition(vbBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            Dx()->AddTransition(ibBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            Dx()->ApplyTransitions(cmdList);
            
            cmdList->CopyResource(vbBuffer.Get(), vbUploadBuffer.Get());
            cmdList->CopyResource(ibBuffer.Get(), ibUploadBuffer.Get());

            Dx()->AddTransition(vbBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            Dx()->AddTransition(ibBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
            Dx()->ApplyTransitions(cmdList);
        });

        sp<Mesh> result = msp<Mesh>();
        result->m_vertexBufferView = vbView;
        result->m_indexBufferView = ibView;
        result->m_vertexCount = vertexCount;
        result->m_vertexDataStrideB = static_cast<int>(vertexData.size() / vertexCount * sizeof(float));
        result->m_bounds = bounds;
        result->m_inputLayout = GetD3dVertexLayout(vertexAttribInfo);
        result->m_vertexData = std::move(vertexData);
        result->m_vertexBuffer = std::move(vbBuffer);
        result->m_indexData = std::move(indices);
        result->m_indexBuffer = std::move(ibBuffer);
        result->m_vertexAttribInfo = std::move(vertexAttribInfo);

        return result;
    }

    void Mesh::CalcVertexAttrOffset(umap<VertexAttr, VertexAttrInfo>& vertexAttribInfo)
    {
        size_t curOffset = 0;
        for (uint8_t i = 0; i < static_cast<uint8_t>(VertexAttr::COUNT); ++i)
        {
            auto attr = static_cast<VertexAttr>(i);
            if (!vertexAttribInfo[attr].enabled)
            {
                continue;
            }

            vertexAttribInfo[attr].offsetB = curOffset * sizeof(float);
            curOffset += VERTEX_ATTR_STRIDE[attr];
        }
    }

    crumap<VertexAttr, Mesh::VertexAttrInfo> Mesh::GetFullVertexAttribInfo()
    {
        static umap<VertexAttr, VertexAttrInfo> vertexAttribInfo;
        if (vertexAttribInfo.empty())
        {
            for (auto& attrInfo : VERTEX_ATTR_DEFINES)
            {
                VertexAttrInfo meshVertexAttrInfo;
                meshVertexAttrInfo.enabled = true;
                meshVertexAttrInfo.offsetB = attrInfo.offsetF * sizeof(float);
                vertexAttribInfo.emplace(attrInfo.attr, meshVertexAttrInfo);
            }
        }

        return vertexAttribInfo;
    }

    void Mesh::GetMeshLoadConfig(crstr modelPath, float& initScale, bool& flipWindingOrder)
    {
        auto config = Utils::GetResourceMeta(modelPath);

        initScale = 1.0f;
        try_get_val(config, "init_scale", initScale);

        flipWindingOrder = false;
        try_get_val(config, "flip_winding_order", flipWindingOrder);
    }
    
    crvec<float> Mesh::GetFullVertexData(const Mesh* mesh)
    {
        auto vertexCount = mesh->GetVertexCount();
        auto& rawVertexData = mesh->GetVertexData();
        auto rawVertexStrideF = mesh->GetVertexDataStrideB() / sizeof(float);
        
        static std::vector<float> vertexData;
        vertexData.resize(MAX_VERTEX_ATTR_STRIDE_F * vertexCount);

        uint32_t curOffsetF = 0;
        for (auto& attrInfo : VERTEX_ATTR_DEFINES)
        {
            auto aa = attrInfo.name;
            auto& meshVertexAttrInfo = mesh->GetVertexAttribInfo().at(attrInfo.attr);
            if (meshVertexAttrInfo.enabled)
            {
                auto rawAttrOffsetF = meshVertexAttrInfo.offsetB / sizeof(float);
                for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    for (uint32_t j = 0; j < attrInfo.strideF; ++j)
                    {
                        auto dstIndex = vertexIndex * MAX_VERTEX_ATTR_STRIDE_F + curOffsetF + j;
                        auto srcIndex = vertexIndex * rawVertexStrideF + rawAttrOffsetF + j;
                        vertexData[dstIndex] = rawVertexData[srcIndex];
                    }
                }
            }

            curOffsetF += attrInfo.strideF;
        }

        return vertexData;
    }

    Mesh::Cache Mesh::CreateCacheFromAsset(crstr assetPath)
    {
        auto mesh = LoadFromFileImp(assetPath);

        Cache fullMesh;
        fullMesh.bounds = mesh->GetBounds();
        fullMesh.vertexCount = mesh->GetVertexCount();
        // fullMesh.vertexData = GetFullVertexData(mesh.get());
        fullMesh.vertexData = mesh->GetVertexData();
        fullMesh.indices = mesh->GetIndexData();
        fullMesh.vertexAttribInfo = mesh->GetVertexAttribInfo();

        return fullMesh;
    }

    sp<Mesh> Mesh::CreateAssetFromCache(Cache&& cache)
    {
        auto c = std::move(cache);
    
        auto mesh = CreateMesh(
            std::move(c.vertexData),
            std::move(c.indices),
            c.vertexCount,
            c.vertexAttribInfo,
            c.bounds);

        return mesh;
    }
}
