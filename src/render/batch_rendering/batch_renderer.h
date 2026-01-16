#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "common/const.h"
#include "common/utils.h"
#include "common/math.h"

namespace dt
{
    class DxBuffer;
    class ManagedMemoryBlock;
    class ByteBuffer;
    struct RenderObject;
    class BatchMesh;
    class BatchMatrix;

    class BatchRenderer : public Singleton<BatchRenderer>, public std::enable_shared_from_this<BatchRenderer>
    {
    public:
        struct BatchMatrix
        {
            XMFLOAT4X4 localToWorld;
            XMFLOAT4X4 worldToLocal;
        };
        
        BatchRenderer();

        void Register(crsp<RenderObject> renderObject);
        void Unregister(crsp<RenderObject> renderObject);

        void EncodeCmd();
        func<void(ID3D12GraphicsCommandList*)> GetCmd();
        void UpdateMatrix(crsp<RenderObject> ro, cr<BatchMatrix> matrix);

    private:
        void RegisterActually();

        struct IndirectArg
        {
            uint32_t baseInstanceId;
            D3D12_DRAW_INDEXED_ARGUMENTS drawArg;
        };
        
        struct BatchRenderObject
        {
            size_t matrixKey;
            uint32_t matrixIndex;
            sp<RenderObject> renderObject;
        };

        vecsp<RenderObject> m_pendingRegisterRenderObjects;
        vecsp<RenderObject> m_pendingUnregisterRenderObjects;
        vecpair<sp<RenderObject>, BatchMatrix> m_dirtyRoMatrix;
        
        vec<BatchRenderObject> m_renderObjects;
        sp<BatchMesh> m_batchMesh;
        sp<DxBuffer> m_batchMatrix;
        sp<DxBuffer> m_gpuCmdBuffer = nullptr;
        sp<DxBuffer> m_batchIndices = nullptr;
        vec<IndirectArg> m_cpuCmdBuffer;

        umap<uint32_t, Microsoft::WRL::ComPtr<ID3D12CommandSignature>> m_commandSignature;
    };
}
