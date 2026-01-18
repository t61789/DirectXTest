#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "common/const.h"
#include "common/utils.h"
#include "common/math.h"
#include "render/cbuffer.h"
#include "render/render_target.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class Shader;
    class Material;
    class Mesh;
    class DxBuffer;
    class ManagedMemoryBlock;
    class ByteBuffer;
    struct RenderObject;
    class BatchMesh;
    
    struct BatchMatrix
    {
        XMFLOAT4X4 localToWorld;
        XMFLOAT4X4 worldToLocal;
    };
    
    struct IndirectArg
    {
        uint32_t batchIndicesBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS drawArg;
    };

    struct BatchRenderObject
    {
        uint32_t matrixIndex;
        sp<RenderObject> ro;
    };

    struct BatchRenderSubCmd
    {
        sp<Mesh> mesh;
        vec<BatchRenderObject> ros;

        IndirectArg indirectArg;
        vec<uint32_t> batchIndices;
        sp<DxBuffer> batchIndicesBuffer = nullptr;
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
            crsp<DxBuffer> batchMatrix);

        void Register(crsp<RenderObject> ro, size_t matrixKey, crsp<CmdSigPool> cmdSigPool);
        void Unregister(crsp<RenderObject> ro);
        
        void EncodeCmd();
        func<void(ID3D12GraphicsCommandList*)> CreateCmd(crsp<Cbuffer> viewCbuffer, crsp<RenderTarget> renderTarget);

    private:
        sp<BatchMesh> m_batchMesh;
        sp<DxBuffer> m_batchMatrix;
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
        sp<DxBuffer> m_batchMatrix;

        sp<CmdSigPool> m_cmdSigPool;

        sp<Material> m_shadowMaterial;
        
        sp<BatchRenderGroup> m_commonGroup;
        sp<BatchRenderGroup> m_shadowGroup;
    };
}
