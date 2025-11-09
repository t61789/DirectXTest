#pragma once
#include <d3dcommon.h>
#include <wrl/client.h>

#include "i_resource.h"
#include "const.h"

namespace dt
{
    using namespace Microsoft::WRL;
    
    class Shader : public IResource
    {
    public:
        void* GetVSPointer() const { return m_vs->GetBufferPointer(); }
        size_t GetVSSize() const { return m_vs->GetBufferSize(); }
        void* GetPSPointer() const { return m_ps->GetBufferPointer(); }
        size_t GetPSSize() const { return m_ps->GetBufferSize(); }
        cr<StringHandle> GetPath() override { return m_path; }
        
        static sp<Shader> LoadFromFile(cr<StringHandle> path);

    private:
        void DoLoad(cr<StringHandle> path);
        
        static ComPtr<ID3DBlob> CompileShader(crstr filePath, crstr entryPoint, crstr target);

        ComPtr<ID3DBlob> m_vs;
        ComPtr<ID3DBlob> m_ps;

        StringHandle m_path;
    };
}
