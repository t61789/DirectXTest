// #include "dx_buffer.h"
//
// #include <directx/d3dx12_core.h>
//
// #include "directx.h"
//
// namespace dt
// {
//     void DxBuffer::UploadData(const uint32_t destOffsetB, const uint32_t sizeB, const void* data)
//     {
//         assert((m_writeDest && destOffsetB + sizeB <= m_sizeB) || (!m_writeDest && destOffsetB == 0 && m_sizeB == sizeB));
//
//         if (m_writeDest)
//         {
//             memcpy(m_writeDest + destOffsetB, data, sizeB);
//         }
//         else
//         {
//             auto uploadBuffer = CreateUploadBuffer();
//             void* writeDest = nullptr;
//             THROW_IF_FAILED(uploadBuffer->Map(0, nullptr, &writeDest));
//             memcpy(writeDest, data, sizeB);
//             uploadBuffer->Unmap(0, nullptr);
//
//             Dx()->AddCommand([uploadBuffer, self=shared_from_this()](ID3D12GraphicsCommandList* cmdList)
//             {
//                 if (self->m_resourceStates != D3D12_RESOURCE_STATE_COPY_DEST)
//                 {
//                     Dx()->AddTransition(self->m_buffer.Get(), self->m_resourceStates, D3D12_RESOURCE_STATE_COPY_DEST);
//                     Dx()->ApplyTransitions(cmdList);
//                 }
//                 
//                 cmdList->CopyResource(self->m_buffer.Get(), uploadBuffer.Get());
//
//                 if (self->m_resourceStates != D3D12_RESOURCE_STATE_COPY_DEST)
//                 {
//                     Dx()->AddTransition(self->m_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, self->m_resourceStates);
//                 }
//             });
//         }
//     }
//
//     sp<DxBuffer> DxBuffer::CreateStatic(const D3D12_RESOURCE_STATES state, const uint32_t sizeB, const void* initData)
//     {
//         auto result = msp<DxBuffer>();
//
//         result->m_sizeB = sizeB;
//         result->m_resourceStates = state;
//         result->m_buffer = Dx()->CreateCommittedResource(
//             CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
//             CD3DX12_RESOURCE_DESC::Buffer(sizeB),
//             state,
//             nullptr,
//             D3D12_HEAP_FLAG_NONE,
//             L"Buffer");
//
//         if (initData)
//         {
//             result->UploadData(0, sizeB, initData);
//         }
//
//         return result;
//     }
//
//     sp<DxBuffer> DxBuffer::CreateDynamic(const uint32_t sizeB)
//     {
//         auto result = msp<DxBuffer>();
//
//         result->m_sizeB = sizeB;
//         result->m_buffer = Dx()->CreateCommittedResource(
//             CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
//             CD3DX12_RESOURCE_DESC::Buffer(result->m_sizeB),
//             D3D12_RESOURCE_STATE_GENERIC_READ,
//             nullptr,
//             D3D12_HEAP_FLAG_NONE,
//             "Upload Buffer");
//
//         void* writeDest;
//         THROW_IF_FAILED(result->m_buffer->Map(0, nullptr, &writeDest));
//         result->m_writeDest = static_cast<uint8_t*>(writeDest);
//
//         return result;
//     }
//
//     ComPtr<ID3D12Resource> DxBuffer::CreateUploadBuffer()
//     {
//         return Dx()->CreateCommittedResource(
//             CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
//             CD3DX12_RESOURCE_DESC::Buffer(m_sizeB),
//             D3D12_RESOURCE_STATE_GENERIC_READ,
//             nullptr,
//             D3D12_HEAP_FLAG_NONE,
//             "Upload Buffer");
//     }
// }
