#include "batch_renderer.h"

#include "batch_mesh.h"
#include "common/shader.h"
#include "render/directx.h"
#include "render/dx_buffer.h"
#include "render/dx_helper.h"
#include "render/dx_resource.h"
#include "render/render_pipeline.h"
#include "render/render_resources.h"
#include "render/render_thread.h"

namespace dt
{
    BatchRenderGroup::BatchRenderGroup(crsp<Material> replaceMaterial, crsp<BatchMesh> batchMesh, crsp<DxBuffer> batchMatrix)
    {
        m_batchMesh = batchMesh;
        m_batchMatrix = batchMatrix;
        m_replaceMaterial = replaceMaterial;
    }

    void BatchRenderGroup::Register(crsp<RenderObject> ro, const size_t matrixKey, crsp<CmdSigPool> cmdSigPool)
    {
        // Get or create cmd
        auto material = m_replaceMaterial ? m_replaceMaterial : ro->material;
        auto batchRenderCmd = find_if(m_batchRenderCmds, [material](cr<BatchRenderCmd> a)
        {
            return a.material == material;
        });
        if (!batchRenderCmd)
        {
            BatchRenderCmd b;
            b.material = material;
            b.cmdSignature = cmdSigPool->GetCmdSig(material->GetShader());
            b.indirectArgsBuffer = DxBuffer::Create(32 * sizeof(IndirectArg), L"Batch Indirect Arg Buffer");
            
            m_batchRenderCmds.push_back(b);
            batchRenderCmd = &m_batchRenderCmds.back();
        }

        // Get or create subCmd
        auto mesh = ro->mesh;
        auto batchRenderSubCmd = find_if(batchRenderCmd->subCmds, [mesh](cr<BatchRenderSubCmd> a)
        {
            return a.mesh == mesh;
        });
        if (!batchRenderSubCmd)
        {
            size_t vertexOffsetB, vertexSizeB, indexOffsetB, indexSizeB;
            m_batchMesh->GetMeshInfo(mesh.get(), vertexOffsetB, vertexSizeB, indexOffsetB, indexSizeB);
            
            BatchRenderSubCmd b;
            b.mesh = mesh;
            b.indirectArg.drawArg.IndexCountPerInstance = indexSizeB / sizeof(uint32_t);
            b.indirectArg.drawArg.StartIndexLocation = indexOffsetB / sizeof(uint32_t);
            b.indirectArg.drawArg.BaseVertexLocation = vertexOffsetB / (MAX_VERTEX_ATTR_STRIDE_F * sizeof(float));
            b.indirectArg.drawArg.StartInstanceLocation = 0;
            b.batchIndicesBuffer = DxBuffer::Create(32 * sizeof(uint32_t), L"Batch Indices Buffer");
            
            batchRenderCmd->subCmds.push_back(b);
            batchRenderSubCmd = &batchRenderCmd->subCmds.back();
        }

        // Try add ro
        auto roExists = exists_if(batchRenderSubCmd->ros, [ro](cr<BatchRenderObject> a)
        {
            return a.ro == ro;
        });
        if (!roExists)
        {
            size_t offsetB, sizeB;
            m_batchMatrix->GetBlock(matrixKey, offsetB, sizeB);
            uint32_t matrixIndex = offsetB / sizeof(BatchMatrix);
            
            batchRenderSubCmd->ros.push_back({
                matrixIndex,
                ro
            });
        }
    }

    void BatchRenderGroup::Unregister(crsp<RenderObject> ro)
    {
        for (auto batchRenderCmdIt = m_batchRenderCmds.begin(); batchRenderCmdIt != m_batchRenderCmds.end(); ++batchRenderCmdIt)
        {
            for (auto batchRenderSubCmdIt = batchRenderCmdIt->subCmds.begin(); batchRenderSubCmdIt != batchRenderCmdIt->subCmds.end(); ++batchRenderSubCmdIt)
            {
                for (auto roIt = batchRenderSubCmdIt->ros.begin(); roIt != batchRenderSubCmdIt->ros.end(); ++roIt)
                {
                    if (roIt->ro == ro)
                    {
                        batchRenderSubCmdIt->ros.erase(roIt);
                        break;
                    }
                }

                if (batchRenderSubCmdIt->ros.empty())
                {
                    batchRenderCmdIt->subCmds.erase(batchRenderSubCmdIt);
                    break;
                }
            }

            if (batchRenderCmdIt->subCmds.empty())
            {
                m_batchRenderCmds.erase(batchRenderCmdIt);
                break;
            }
        }
    }
    
    void BatchRenderGroup::EncodeCmd()
    {
        for (auto& batchRenderCmd : m_batchRenderCmds)
        {
            batchRenderCmd.indirectArgs.clear();
            auto sumInstanceCount = 0;
            
            for (auto& batchRenderSubCmd : batchRenderCmd.subCmds)
            {
                batchRenderSubCmd.batchIndices.clear();
                
                for (auto& ro : batchRenderSubCmd.ros)
                {
                    batchRenderSubCmd.batchIndices.push_back(ro.matrixIndex);
                }
                auto indicesBufferSizeB = batchRenderSubCmd.batchIndices.size() * sizeof(uint32_t);
                batchRenderSubCmd.batchIndicesBuffer->Reserve(indicesBufferSizeB);
                batchRenderSubCmd.batchIndicesBuffer->Write(0, indicesBufferSizeB, batchRenderSubCmd.batchIndices.data());
                
                batchRenderSubCmd.indirectArg.batchIndicesBuffer = batchRenderSubCmd.batchIndicesBuffer->GetShaderResource()->GetSrvIndex();
                batchRenderSubCmd.indirectArg.drawArg.InstanceCount = batchRenderSubCmd.batchIndices.size();
                sumInstanceCount += batchRenderSubCmd.indirectArg.drawArg.InstanceCount;

                if (batchRenderSubCmd.indirectArg.drawArg.InstanceCount > 0)
                {
                    batchRenderCmd.indirectArgs.push_back(batchRenderSubCmd.indirectArg);
                }
            }

            if (!batchRenderCmd.indirectArgs.empty())
            {
                auto indirectArgsBufferSizeB = batchRenderCmd.indirectArgs.size() * sizeof(IndirectArg);
                batchRenderCmd.indirectArgsBuffer->Reserve(indirectArgsBufferSizeB);
                batchRenderCmd.indirectArgsBuffer->Write(0, indirectArgsBufferSizeB, batchRenderCmd.indirectArgs.data());
            }
        }
    }

    func<void(ID3D12GraphicsCommandList*)> BatchRenderGroup::CreateCmd(crsp<Cbuffer> viewCbuffer, crsp<RenderTarget> renderTarget)
    {
        return [self=shared_from_this(), renderTarget, viewCbuffer](ID3D12GraphicsCommandList* cmdList)
        {
            DxHelper::SetRenderTarget(cmdList, renderTarget);

            for (auto& batchRenderCmd : self->m_batchRenderCmds)
            {
                if (batchRenderCmd.indirectArgs.empty())
                {
                    continue;
                }
                
                auto shader = batchRenderCmd.material->GetShader().get();
                auto material = batchRenderCmd.material.get();
                auto cmdSig = batchRenderCmd.cmdSignature.Get();
                
                DxHelper::BindRootSignature(cmdList, shader);
                DxHelper::BindPso(cmdList, material);
                DxHelper::BindBindlessTextures(cmdList, shader);
                
                self->m_batchMesh->BindMesh(cmdList);
                DxHelper::BindCbuffer(cmdList, shader, GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get());
                DxHelper::BindCbuffer(cmdList, shader, viewCbuffer.get());
                if (material->GetCbuffer())
                {
                    DxHelper::BindCbuffer(cmdList, shader, material->GetCbuffer());
                }

                cmdList->ExecuteIndirect(
                    cmdSig,
                    batchRenderCmd.indirectArgs.size(),
                    batchRenderCmd.indirectArgsBuffer->GetDxResource()->GetResource(),
                    0,
                    nullptr,
                    0);
            }
        };
    }

    ComPtr<ID3D12CommandSignature> CmdSigPool::GetCmdSig(crsp<Shader> shader)
    {
        auto pair = find_if(m_commandSignature, [shader](cr<decltype(m_commandSignature)::value_type> a)
        {
            return a.first.lock() == shader;
        });
        if (pair)
        {
            return pair->second;
        }
        
        D3D12_INDIRECT_ARGUMENT_DESC indirectParams[2];
        indirectParams[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        indirectParams[0].Constant.RootParameterIndex = shader->GetRootConstantCbufferRootParamIndex();
        indirectParams[0].Constant.DestOffsetIn32BitValues = ROOT_CONSTANTS_BATCH_INDICES_OFFSET_DWORD;
        indirectParams[0].Constant.Num32BitValuesToSet = 1;
        
        indirectParams[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
        sigDesc.ByteStride = sizeof(IndirectArg);
        sigDesc.NumArgumentDescs = _countof(indirectParams);
        sigDesc.pArgumentDescs = indirectParams;

        ComPtr<ID3D12CommandSignature> commandSignature;
        THROW_IF_FAILED(Dx()->GetDevice()->CreateCommandSignature(&sigDesc, shader->GetRootSignature().Get(), IID_PPV_ARGS(&commandSignature)));

        m_commandSignature.push_back(std::make_pair(std::weak_ptr(shader), commandSignature));

        return commandSignature;
    }
    
    void CmdSigPool::ClearCmdSig()
    {
        remove_if(m_commandSignature, [](cr<decltype(m_commandSignature)::value_type> a)
        {
            return a.first.expired();
        });
    }

    BatchRenderer::BatchRenderer()
    {
        m_batchMesh = msp<BatchMesh>(500000, 100000);
        
        m_batchMatrix = DxBuffer::Create(200 * sizeof(BatchMatrix), L"Batch Matrix Buffer");
        m_cmdSigPool = msp<CmdSigPool>();

        m_shadowMaterial = Material::CreateFromShader("shaders/draw_shadow_batch.shader");

        m_commonGroup = msp<BatchRenderGroup>(nullptr, m_batchMesh, m_batchMatrix);
        m_shadowGroup = msp<BatchRenderGroup>(m_shadowMaterial, m_batchMesh, m_batchMatrix);
    }

    void BatchRenderer::Register(crsp<RenderObject> renderObject)
    {
        remove(m_pendingUnregisterRenderObjects, renderObject);
        
        m_pendingRegisterRenderObjects.push_back(renderObject);
    }

    void BatchRenderer::Unregister(crsp<RenderObject> renderObject)
    {
        remove(m_pendingRegisterRenderObjects, renderObject);
        
        m_pendingUnregisterRenderObjects.push_back(renderObject);
    }

    void BatchRenderer::UpdateMatrix(crsp<RenderObject> ro, cr<BatchMatrix> matrix)
    {
        m_dirtyRoMatrix.emplace_back(ro, matrix);
    }

    void BatchRenderer::RegisterActually()
    {
        if (m_pendingRegisterRenderObjects.empty() && m_pendingUnregisterRenderObjects.empty())
        {
            return;
        }
        
        for (auto& ro : m_pendingRegisterRenderObjects)
        {
            if (find(m_renderObjects, &RenderObjectInfo::ro, ro))
            {
                return;
            }

            m_batchMesh->RegisterMesh(ro->mesh);
            auto matrixKey = m_batchMatrix->Alloc(sizeof(BatchMatrix));

            m_commonGroup->Register(ro, matrixKey, m_cmdSigPool);
            m_shadowGroup->Register(ro, matrixKey, m_cmdSigPool);

            m_renderObjects.push_back({
                ro,
                matrixKey
            });
        }

        for (auto& ro : m_pendingUnregisterRenderObjects)
        {
            if (!find(m_renderObjects, &RenderObjectInfo::ro, ro))
            {
                return;
            }
            
            m_commonGroup->Unregister(ro);
            m_shadowGroup->Unregister(ro);

            remove_if(m_renderObjects, [ro](CR_ELEM_TYPE(m_renderObjects) a){
                return a.ro == ro;
            });
        }

        m_pendingRegisterRenderObjects.clear();
        m_pendingUnregisterRenderObjects.clear();

        m_cmdSigPool->ClearCmdSig();
    }

    void BatchRenderer::UpdateMatrixActually()
    {
        for (auto& [ro, matrix] : m_dirtyRoMatrix)
        {
            auto roInfo = find(m_renderObjects, &RenderObjectInfo::ro, ro);
            assert(roInfo);

            BatchMatrix transposedMatrix;
            transposedMatrix.localToWorld = Transpose(matrix.localToWorld);
            transposedMatrix.worldToLocal = Transpose(matrix.worldToLocal);
            m_batchMatrix->Write(roInfo->matrixKey, &transposedMatrix);
        }
        m_dirtyRoMatrix.clear();
        
        GetGlobalCbuffer()->Write(BATCH_MATRICES, m_batchMatrix->GetShaderResource()->GetSrvIndex());
    }
}
