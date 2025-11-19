#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include <wrl/client.h>

#include "common/const.h"

namespace dt
{
    class Scene;
    class Material;
    class Mesh;
    class DirectX;
    class Shader;
    
    using namespace Microsoft::WRL;

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();
        RenderPipeline(const RenderPipeline& other) = delete;
        RenderPipeline(RenderPipeline&& other) noexcept = delete;
        RenderPipeline& operator=(const RenderPipeline& other) = delete;
        RenderPipeline& operator=(RenderPipeline&& other) noexcept = delete;

        void Render();
        static void RenderScene(ID3D12GraphicsCommandList* cmdList, const Scene* scene);

    private:
        
        sp<Mesh> m_testMesh;
        sp<Material> m_testMaterial;
    };
}
