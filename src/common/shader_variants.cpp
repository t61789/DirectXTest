#include "shader_variants.h"

#include "shader.h"
#include "utils.h"
#include "variant_keyword.h"
#include "game/game_resource.h"

namespace dt
{
    sp<Shader> ShaderVariants::GetShader(cr<VariantKeyword> keyword)
    {
        if (auto result = find(m_shaders, keyword.GetHash()))
        {
            return *result;
        }

        auto shader = Shader::Create(m_path, keyword);
        m_shaders.emplace_back(keyword.GetHash(), shader);

        return shader;
    }

    sp<ShaderVariants> ShaderVariants::LoadFromFile(cr<StringHandle> path)
    {
        {
            if (auto result = GR()->GetResource<ShaderVariants>(path))
            {
                return result;
            }
        }

        auto result = msp<ShaderVariants>();
        result->m_path = path;

        GR()->RegisterResource(path, result);
        
        log_info("Load shader variants: %s", path.CStr());
        
        return result;
    }
}
