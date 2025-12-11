#pragma once
#include <d3d12.h>
#include <assimp/Importer.hpp>
#include <wrl/client.h>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "i_resource.h"
#include "math.h"
#include "utils.h"
#include "render/dx_resource.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class Mesh final : public IResource
    {
        struct Cache;
        
    public:
        struct VertexAttrInfo;

        Mesh() = default;
        
        cr<StringHandle> GetPath() override { return m_path; }
        Bounds GetBounds() const { return m_bounds; }
        uint32_t GetVertexDataStrideB() const { return m_vertexDataStrideB; }
        crvec<float> GetVertexData() const { return m_vertexData; }
        crvec<uint32_t> GetIndexData() const { return m_indexData; }
        crumap<VertexAttr, VertexAttrInfo> GetVertexAttribInfo() const { return m_vertexAttribInfo; }
        crvec<D3D12_INPUT_ELEMENT_DESC> GetInputLayout() const { return m_inputLayout; }
        cr<D3D12_VERTEX_BUFFER_VIEW> GetVertexBufferView() const { return m_vertexBufferView; }
        cr<D3D12_INDEX_BUFFER_VIEW> GetIndexBufferView() const { return m_indexBufferView; }
        
        uint32_t GetVertexCount() const { return static_cast<uint32_t>(GetVertexData().size() * sizeof(float) / GetVertexDataStrideB()); }
        uint32_t GetIndicesCount() const { return static_cast<uint32_t>(GetIndexData().size()); }

        static sp<Mesh> LoadFromFile(crstr modelPath);
        
        static Cache CreateCacheFromAsset(crstr assetPath);
        static sp<Mesh> CreateAssetFromCache(Cache&& cache);
        
        struct VertexAttrInfo
        {
            bool enabled = false;
            uintptr_t offsetB = 0;

            template <class Archive>
            void serialize(Archive& ar, unsigned int version);
        };
        
    private:
        static sp<Mesh> LoadFromFileImp(crstr modelPath);
        static up<Assimp::Importer> ImportFile(crstr modelPath);
        static void GetMeshLoadConfig(crstr modelPath, float& initScale, bool& flipWindingOrder);
        static void CalcVertexAttrOffset(umap<VertexAttr, VertexAttrInfo>& vertexAttribInfo);
        static crumap<VertexAttr, VertexAttrInfo> GetFullVertexAttribInfo();
        static sp<Mesh> CreateMesh(
            vec<float>&& vertexData,
            vec<uint32_t>&& indices,
            uint32_t vertexCount,
            crumap<VertexAttr, VertexAttrInfo> vertexAttribInfo,
            cr<Bounds> bounds);
        
        static crvec<float> GetFullVertexData(const Mesh* mesh);
        
        struct Cache
        {
            Bounds bounds = {};
            uint32_t vertexCount = 0;
            vec<float> vertexData = {};
            vec<uint32_t> indices = {};
            umap<VertexAttr, VertexAttrInfo> vertexAttribInfo = {};

            template <class Archive>
            void serialize(Archive& ar, unsigned int version);
        };
        
        Bounds m_bounds;
        StringHandle m_path;
        uint32_t m_vertexDataStrideB;
        uint32_t m_vertexCount;
        umap<VertexAttr, VertexAttrInfo> m_vertexAttribInfo;
        vec<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
        
        vec<float> m_vertexData;
        vec<uint32_t> m_indexData;

        sp<DxResource> m_vertexBuffer;
        sp<DxResource> m_indexBuffer;

        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        
        friend class MeshCacheMgr;
    };

    template <class Archive>
    void Mesh::VertexAttrInfo::serialize(Archive& ar, unsigned int version)
    {
        ar & enabled;
        ar & offsetB;
    }

    template <class Archive>
    void Mesh::Cache::serialize(Archive& ar, unsigned int version)
    {
        ar & bounds;
        ar & vertexCount;
        ar & vertexData;
        ar & indices;
        ar & vertexAttribInfo;
    }
}
