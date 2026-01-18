#pragma once
#include "const.h"
#include "i_resource.h"

namespace dt
{
    struct VariantKeyword;
    class Shader;
    
    class ShaderVariants final : public IResource, public std::enable_shared_from_this<ShaderVariants>
    {
    public:
        cr<StringHandle> GetPath() override { return m_path; }

        sp<Shader> GetShader(cr<VariantKeyword> keyword);

        static sp<ShaderVariants> LoadFromFile(cr<StringHandle> path);

    private:
        StringHandle m_path;
        vecpair<size_t, sp<Shader>> m_shaders;
    };
}
