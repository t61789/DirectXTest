#include "shader.h"

#include <D3Dcompiler.h>

#include "game/game_resource.h"

namespace dt
{
    sp<Shader> Shader::LoadFromFile(cr<StringHandle> path)
    {
        {
            if (auto mesh = GR()->GetResource<Shader>(path))
            {
                return mesh;
            }
        }

        auto shader = msp<Shader>();
        shader->m_path = path;
        
        shader->DoLoad(path);

        GR()->RegisterResource(path, shader);

        return shader;
    }

    void Shader::DoLoad(cr<StringHandle> path)
    {
        log_info("Loading shader: %s", path.CStr());

        m_ps = CompileShader(path.CStr(), "PS_Main", "ps_5_0");
        m_vs = CompileShader(path.CStr(), "VS_Main", "vs_5_0");
    }
    
    ComPtr<ID3DBlob> Shader::CompileShader(crstr filePath, crstr entryPoint, crstr target)
    {
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        uint32_t compileFlags = 0;
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        auto absFilePath = Utils::StringToWString(Utils::ToAbsPath(filePath));
        auto result = D3DCompileFromFile(
            absFilePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(),
            target.c_str(),
            compileFlags,
            0,
            &shaderBlob,
            &errorBlob);

        if (FAILED(result))
        {
            if (errorBlob)
            {
                log_error("Compile shader error:\n%s", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }

            throw std::runtime_error("Failed to compile shader.");
        }

        return shaderBlob;
    }
}
