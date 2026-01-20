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
        bool hasOddNegativeScale;
        sp<RenderObject> ro;

        friend bool operator==(const BatchRenderObject& lhs, const BatchRenderObject& rhs)
        {
            return lhs.matrixKey == rhs.matrixKey
                && lhs.hasOddNegativeScale == rhs.hasOddNegativeScale
                && lhs.ro == rhs.ro;
        }

        friend bool operator!=(const BatchRenderObject& lhs, const BatchRenderObject& rhs)
        {
            return !(lhs == rhs);
        }
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
        bool hasOddNegativeScale;
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

        void Register(cr<BatchRenderObject> batchRo, crsp<CmdSigPool> cmdSigPool);
        void Unregister(cr<BatchRenderObject> batchRo);
        
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
        void ReRegister(cr<BatchRenderObject> batchRo);
        void RegisterActually();

        void UpdateMatrix(crsp<RenderObject> ro);
        void UpdateMatrixActually();

    private:

        vecsp<RenderObject> m_pendingRegisterRenderObjects;
        vecsp<RenderObject> m_pendingUnregisterRenderObjects;
        vecsp<RenderObject> m_dirtyRoMatrix;
        
        vec<BatchRenderObject> m_renderObjects;
        sp<BatchMesh> m_batchMesh;
        sp<BatchMatrixBuffer> m_batchMatrix;
        sp<DxBuffer> m_batchIndices;

        sp<CmdSigPool> m_cmdSigPool;

        sp<Material> m_shadowMaterial;
        
        sp<BatchRenderGroup> m_commonGroup;
        sp<BatchRenderGroup> m_shadowGroup;
    };
}
