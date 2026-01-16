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
    BatchRenderer::BatchRenderer()
    {
        m_batchMesh = msp<BatchMesh>(500000, 100000);
        
        m_gpuCmdBuffer = DxBuffer::Create(32 * sizeof(IndirectArg), L"Batch Cmd Buffer");
        m_batchMatrix = DxBuffer::Create(200 * sizeof(BatchMatrix), L"Batch Matrix Buffer");
        m_batchIndices = DxBuffer::Create(32 * sizeof(uint32_t), L"Batch Indices Buffer");
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
        
        for (auto& renderObject : m_pendingRegisterRenderObjects)
        {
            if (exists_if(m_renderObjects, [renderObject](cr<BatchRenderObject> a) { return a.renderObject == renderObject; }))
            {
                return;
            }

            m_batchMesh->RegisterMesh(renderObject->mesh);

            BatchRenderObject batchRenderObject;
            batchRenderObject.matrixKey = m_batchMatrix->Alloc(sizeof(BatchMatrix));
            batchRenderObject.renderObject = renderObject;

            size_t offsetB, sizeB;
            m_batchMatrix->GetBlock(batchRenderObject.matrixKey, offsetB, sizeB);
            batchRenderObject.matrixIndex = offsetB / sizeof(BatchMatrix);

            m_renderObjects.push_back(batchRenderObject);
        }

        for (auto& renderObject : m_pendingUnregisterRenderObjects)
        {
            auto it = std::find_if(m_renderObjects.begin(), m_renderObjects.end(), [renderObject](cr<BatchRenderObject> a)
            {
                return a.renderObject == renderObject;
            });
            if (it == m_renderObjects.end())
            {
                return;
            }

            m_renderObjects.erase(it);
        }

        m_pendingRegisterRenderObjects.clear();
        m_pendingUnregisterRenderObjects.clear();

        std::sort(m_renderObjects.begin(), m_renderObjects.end(), [](cr<BatchRenderObject> a, cr<BatchRenderObject> b)
        {
            if (a.renderObject->shader == b.renderObject->shader)
            {
                if (a.renderObject->material == b.renderObject->material)
                {
                    if (a.renderObject->mesh == b.renderObject->mesh)
                    {
                        return true;
                    }
                    return a.renderObject->mesh < b.renderObject->mesh;
                }
                return a.renderObject->material < b.renderObject->material;
            }
            return a.renderObject->shader < b.renderObject->shader;
        });

        for (auto& [ro, matrix] : m_dirtyRoMatrix)
        {
            auto batchRo = find_if(m_renderObjects, [ro](cr<BatchRenderObject> a) { return a.renderObject == ro; });
            assert(batchRo);

            BatchMatrix transposedMatrix;
            transposedMatrix.localToWorld = Transpose(matrix.localToWorld);
            transposedMatrix.worldToLocal = Transpose(matrix.worldToLocal);
            m_batchMatrix->Write(batchRo->matrixKey, &transposedMatrix);
        }
        m_dirtyRoMatrix.clear();
        
        GetGlobalCbuffer()->Write(BATCH_MATRICES, m_batchMatrix->GetShaderResource()->GetSrvIndex());
    }

    void BatchRenderer::EncodeCmd()
    {
        RegisterActually();
        
        if (m_renderObjects.empty())
        {
            return;
        }
        
        m_cpuCmdBuffer.clear();

        uint32_t sumInstanceCount = 0;
        uint32_t instanceCount = 0;
        Mesh* curMesh = nullptr;
        for (auto it = m_renderObjects.begin();; ++it)
        {
            if (it == m_renderObjects.end() || (curMesh != nullptr && it->renderObject->mesh.get() != curMesh))
            {
                size_t vertexOffsetB, vertexSizeB, indexOffsetB, indexSizeB;
                m_batchMesh->GetMeshInfo(curMesh, vertexOffsetB, vertexSizeB, indexOffsetB, indexSizeB);
                
                IndirectArg indirectArg;
                indirectArg.baseInstanceId = sumInstanceCount;
                indirectArg.drawArg.IndexCountPerInstance = indexSizeB / sizeof(uint32_t);
                indirectArg.drawArg.InstanceCount = instanceCount;
                indirectArg.drawArg.StartIndexLocation = indexOffsetB / sizeof(uint32_t);
                indirectArg.drawArg.BaseVertexLocation = vertexOffsetB / (MAX_VERTEX_ATTR_STRIDE_F * sizeof(float));
                indirectArg.drawArg.StartInstanceLocation = sumInstanceCount;
                m_cpuCmdBuffer.push_back(indirectArg);

                sumInstanceCount += instanceCount;

                if (it == m_renderObjects.end())
                {
                    break;
                }

                instanceCount = 0;
            }

            curMesh = it->renderObject->mesh.get();
            instanceCount++;
        }

        auto cmdBufferSizeB = m_cpuCmdBuffer.size() * sizeof(IndirectArg);
        auto indicesBufferSizeB = m_renderObjects.size() * sizeof(uint32_t);
        
        m_gpuCmdBuffer->Reserve(cmdBufferSizeB);
        m_batchIndices->Reserve(indicesBufferSizeB);

        vec<uint32_t> indicesBuffer(m_renderObjects.size());
        for (uint32_t i = 0; i < m_renderObjects.size(); i++)
        {
            indicesBuffer[i] = m_renderObjects[i].matrixIndex;
        }

        m_gpuCmdBuffer->Write(0, cmdBufferSizeB, m_cpuCmdBuffer.data());
        m_batchIndices->Write(0, indicesBufferSizeB, indicesBuffer.data());
        
        auto globalCbuffer = GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get();
        globalCbuffer->Write(BATCH_MATRICES, m_batchMatrix->GetShaderResource()->GetSrvIndex());
        globalCbuffer->Write(BATCH_INDICES, m_batchIndices->GetShaderResource()->GetSrvIndex());
    }

    func<void(ID3D12GraphicsCommandList*)> BatchRenderer::GetCmd()
    {
        if (m_cpuCmdBuffer.empty())
        {
            return {};
        }
        
        return [self=shared_from_this()](ID3D12GraphicsCommandList* cmdList)
        {
            DxHelper::SetRenderTarget(cmdList, RenderRes()->gBufferRenderTarget);

            auto ro = self->m_renderObjects[0].renderObject;
            auto shader = ro->shader.get();
            auto material = ro->material.get();
            auto rootConstantsCbufferRootParamIndex = shader->GetRootConstantCbufferRootParamIndex();
            auto cmdSigIt = self->m_commandSignature.find(rootConstantsCbufferRootParamIndex);
            if (cmdSigIt == self->m_commandSignature.end())
            {
                D3D12_INDIRECT_ARGUMENT_DESC indirectParams[2];
                indirectParams[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
                indirectParams[0].Constant.RootParameterIndex = rootConstantsCbufferRootParamIndex;
                indirectParams[0].Constant.DestOffsetIn32BitValues = ROOT_CONSTANTS_BASE_INSTANCE_ID_32_BIT_OFFSET;
                indirectParams[0].Constant.Num32BitValuesToSet = 1;
                
                indirectParams[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

                D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
                sigDesc.ByteStride = sizeof(IndirectArg);
                sigDesc.NumArgumentDescs = _countof(indirectParams);
                sigDesc.pArgumentDescs = indirectParams;

                ComPtr<ID3D12CommandSignature> commandSignature;
                THROW_IF_FAILED(Dx()->GetDevice()->CreateCommandSignature(&sigDesc, shader->GetRootSignature().Get(), IID_PPV_ARGS(&commandSignature)));

                self->m_commandSignature.emplace(rootConstantsCbufferRootParamIndex, commandSignature);

                cmdSigIt = self->m_commandSignature.find(rootConstantsCbufferRootParamIndex);
            }
            
            DxHelper::BindRootSignature(cmdList, shader);
            DxHelper::BindPso(cmdList, material);
            DxHelper::BindBindlessTextures(cmdList, shader);

            self->m_batchMesh->BindMesh(cmdList);
            DxHelper::BindCbuffer(cmdList, shader, GR()->GetPredefinedCbuffer(GLOBAL_CBUFFER).get());
            DxHelper::BindCbuffer(cmdList, shader, RenderRes()->mainCameraViewCbuffer.get());
            if (material->GetCbuffer())
            {
                DxHelper::BindCbuffer(cmdList, shader, material->GetCbuffer());
            }

            cmdList->ExecuteIndirect(
                cmdSigIt->second.Get(),
                self->m_cpuCmdBuffer.size(),
                self->m_gpuCmdBuffer->GetDxResource()->GetResource(),
                0,
                nullptr,
                0);
        };
    }
}
