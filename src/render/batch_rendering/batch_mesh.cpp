#include "batch_mesh.h"

#include "common/mesh.h"
#include "render/descriptor_pool.h"
#include "render/dx_buffer.h"

namespace dt
{
    BatchMesh::BatchMesh(const uint32_t initVertexCount, const uint32_t initIndexCount)
    {
        m_vertexBuffer = DxBuffer::CreateVertexBuffer(initVertexCount * MAX_VERTEX_ATTR_STRIDE_F * sizeof(float), MAX_VERTEX_ATTR_STRIDE_F * sizeof(float), L"Batch Mesh Vertex Buffer");
        m_indexBuffer = DxBuffer::Create(initIndexCount * sizeof(uint32_t), L"Batch Mesh Index Buffer");
    }

    void BatchMesh::RegisterMesh(crsp<Mesh> mesh)
    {
        m_pendingMeshes.push_back(mesh);
    }

    void BatchMesh::BindMesh(ID3D12GraphicsCommandList* cmdList)
    {
        cmdList->IASetIndexBuffer(&m_indexBuffer->GetIndexBufferView());
        cmdList->IASetVertexBuffers(0, 1, &m_vertexBuffer->GetVertexBufferView());
    }

    void BatchMesh::GetMeshInfo(Mesh* mesh, size_t& vertexOffsetB, size_t& vertexSizeB, size_t& indexOffsetB, size_t& indexSizeB)
    {
        RegisterMeshActually();

        auto meshInfo = find_if(m_meshes, [mesh](cr<MeshInfo> x){ return x.mesh.get() == mesh;});
        if (!meshInfo)
        {
            THROW_ERROR("Unregistered mesh!")
        }

        m_vertexBuffer->GetBlock(meshInfo->vertexBufferBlockId, vertexOffsetB, vertexSizeB);
        m_indexBuffer->GetBlock(meshInfo->indexBufferBlockId, indexOffsetB, indexSizeB);
    }

    void BatchMesh::RegisterMeshActually()
    {
        if (m_pendingMeshes.empty())
        {
            return;
        }

        vecsp<Mesh> needAddMeshes;
        size_t addVertexDataSizeB = 0;
        size_t addIndexDataSizeB = 0;
        for (auto& mesh : m_pendingMeshes)
        {
            if (!exists_if(m_meshes, [mesh](cr<MeshInfo> x){ return x.mesh == mesh; }) &&
                !exists(needAddMeshes, mesh))
            {
                needAddMeshes.push_back(mesh);
                addVertexDataSizeB += MAX_VERTEX_ATTR_STRIDE_F * mesh->GetVertexCount() * sizeof(float);
                addIndexDataSizeB += mesh->GetIndicesCount() * sizeof(uint32_t);
            }
        }
        m_pendingMeshes.clear();

        m_vertexBuffer->Reserve(m_vertexBuffer->GetSizeB() + addVertexDataSizeB);
        m_indexBuffer->Reserve(m_indexBuffer->GetSizeB() + addIndexDataSizeB);

        for (const auto& mesh : needAddMeshes)
        {
            auto& vertexData = mesh->GetVertexData();
            auto& indexData = mesh->GetIndexData();

            auto vertexBufferId = m_vertexBuffer->Alloc(vertexData.size() * sizeof(float));
            m_vertexBuffer->Write(vertexBufferId, vertexData.data());
            
            auto indexBufferId = m_indexBuffer->Alloc(indexData.size() * sizeof(uint32_t));
            m_indexBuffer->Write(indexBufferId, indexData.data());

            m_meshes.push_back({ mesh, vertexBufferId, indexBufferId });
        }
    }
}
