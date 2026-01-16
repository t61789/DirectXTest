#pragma once
#include <cstdint>
#include <d3d12.h>
#include <memory>

#include "common/utils.h"

namespace dt
{
    class DxBuffer;
    class VertexBuffer;
    class ByteBuffer;
    class Mesh;
    
    class BatchMesh
    {
    public:
        BatchMesh(uint32_t initVertexCount, uint32_t initIndexCount);

        void RegisterMesh(crsp<Mesh> mesh);
        void BindMesh(ID3D12GraphicsCommandList* cmdList);
        void GetMeshInfo(Mesh* mesh, size_t& vertexOffsetB, size_t& vertexSizeB, size_t& indexOffsetB, size_t& indexSizeB);

    private:
        void RegisterMeshActually();
        
        struct MeshInfo
        {
            sp<Mesh> mesh;
            size_t vertexBufferBlockId;
            size_t indexBufferBlockId;
        };

        sp<DxBuffer> m_vertexBuffer;
        sp<DxBuffer> m_indexBuffer;

        vec<MeshInfo> m_meshes;
        vecsp<Mesh> m_pendingMeshes;
    };
}
