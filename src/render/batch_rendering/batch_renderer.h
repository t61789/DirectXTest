#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "common/const.h"
#include "common/utils.h"
#include "common/math.h"
#include "render/cbuffer.h"
#include "render/render_target.h"
#include "batch_matrix_buffer.h"

namespace dt
{
    struct BatchMatrix;
    class BatchMatrixBuffer;
    using namespace Microsoft::WRL;
    
    class Shader;
    class Material;
    class Mesh;
    class DxBuffer;
    class ManagedMemoryBlock;
    class ByteBuffer;
    struct RenderObject;
    class BatchMesh;
    
    struct IndirectArg
    {
        uint32_t batchIndicesBufferOffsetU;
        D3D12_DRAW_INDEXED_ARGUMENTS drawArg;
    };

    struct BatchRenderObject
    {
        size_t matrixKey;
        sp<RenderObject> ro;
    };

    struct BatchRenderSubCmd
    {
        sp<Mesh> mesh;
        vec<BatchRenderObject> ros;

        IndirectArg indirectArg;
        vec<uint32_t> batchIndices;
    };

    struct BatchRenderCmd
    {
        sp<Shader> shader;
        sp<Material> material;
        ComPtr<ID3D12CommandSignature> cmdSignature;

        vec<IndirectArg> indirectArgs;
        sp<DxBuffer> indirectArgsBuffer = nullptr;

        vec<BatchRenderSubCmd> subCmds;
    };

    struct CmdSigPool
    {
        ComPtr<ID3D12CommandSignature> GetCmdSig(crsp<Shader> shader);
        void ClearCmdSig();
        
    private:
        vecpair<wp<Shader>, ComPtr<ID3D12CommandSignature>> m_commandSignature;
    };

    class BatchRenderGroup : public std::enable_shared_from_this<BatchRenderGroup>
    {
    public:
        explicit BatchRenderGroup(
            crsp<Material> replaceMaterial,
            crsp<BatchMesh> batchMesh,
            crsp<BatchMatrixBuffer> batchMatrix,
            crsp<DxBuffer> batchIndices);

        void Register(crsp<RenderObject> ro, size_t matrixKey, crsp<CmdSigPool> cmdSigPool);
        void Unregister(crsp<RenderObject> ro);
        
        void EncodeCmd();
        func<void(ID3D12GraphicsCommandList*)> CreateCmd(crsp<Cbuffer> viewCbuffer, crsp<RenderTarget> renderTarget);

    private:
        sp<BatchMesh> m_batchMesh;
        sp<BatchMatrixBuffer> m_batchMatrix;
        sp<DxBuffer> m_batchIndices;
        sp<Material> m_replaceMaterial;
        vec<BatchRenderCmd> m_batchRenderCmds;
    };

    class BatchRenderer : public Singleton<BatchRenderer>, public std::enable_shared_from_this<BatchRenderer>
    {
    public:
        BatchRenderer();

        BatchRenderGroup* GetCommonRenderGroup() const { return m_commonGroup.get(); }
        BatchRenderGroup* GetShadowRenderGroup() const { return m_shadowGroup.get(); }

        void Register(crsp<RenderObject> renderObject);
        void Unregister(crsp<RenderObject> renderObject);
        void RegisterActually();

        void UpdateMatrix(crsp<RenderObject> ro, cr<BatchMatrix> matrix);
        void UpdateMatrixActually();

    private:
        struct RenderObjectInfo
        {
            sp<RenderObject> ro;
            size_t matrixKey;
        };

        vecsp<RenderObject> m_pendingRegisterRenderObjects;
        vecsp<RenderObject> m_pendingUnregisterRenderObjects;
        vecpair<sp<RenderObject>, BatchMatrix> m_dirtyRoMatrix;
        
        vec<RenderObjectInfo> m_renderObjects;
        sp<BatchMesh> m_batchMesh;
        sp<BatchMatrixBuffer> m_batchMatrix;
        sp<DxBuffer> m_batchIndices;

        sp<CmdSigPool> m_cmdSigPool;

        sp<Material> m_shadowMaterial;
        
        sp<BatchRenderGroup> m_commonGroup;
        sp<BatchRenderGroup> m_shadowGroup;
    };
}
