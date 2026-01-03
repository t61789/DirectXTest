#pragma once
#include <cstdint>
#include <d3d12shader.h>
#include <DirectXMath.h>
#include <stdexcept>

#include "common/const.h"

namespace dt
{
    enum class ParamType : uint8_t
    {
        INT,
        FLOAT,
        VEC4,
        MATRIX,
        VEC4_ARRAY,
        FLOAT_ARRAY
    };

    template <typename T>
    static ParamType GetParamType()
    {
        if constexpr (std::is_same_v<T, int32_t>)
        {
            return ParamType::INT;
        }
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            return ParamType::INT;
        }
        else if constexpr (std::is_same_v<T, size_t>)
        {
            return ParamType::INT;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return ParamType::FLOAT;
        }
        else if constexpr (std::is_same_v<T, XMFLOAT4>)
        {
            return ParamType::VEC4;
        }
        else if constexpr (std::is_same_v<T, XMFLOAT4X4>)
        {
            return ParamType::MATRIX;
        }
        else if constexpr (std::is_same_v<T, float*>)
        {
            return ParamType::FLOAT_ARRAY;
        }
        
        throw std::runtime_error("Unsupported parameter type");
    }

    static ParamType GetParamType(cr<D3D12_SHADER_TYPE_DESC> typeDesc, uint32_t& repeatCount)
    {
        repeatCount = 1;
        
        if (typeDesc.Class == D3D_SVC_SCALAR)
        {
            if (typeDesc.Type == D3D_SVT_FLOAT && typeDesc.Rows == 1 && typeDesc.Columns == 1)
            {
                if (typeDesc.Elements == 0)
                {
                    return ParamType::FLOAT;
                }

                repeatCount = typeDesc.Elements;
                return ParamType::FLOAT_ARRAY;
            }
            
            if ((typeDesc.Type == D3D_SVT_INT || typeDesc.Type == D3D_SVT_UINT) && typeDesc.Rows == 1 && typeDesc.Columns == 1)
            {
                return ParamType::INT;
            }
        }
        else if (typeDesc.Class == D3D_SVC_VECTOR)
        {
            if (typeDesc.Type == D3D_SVT_FLOAT && typeDesc.Rows == 1 && typeDesc.Columns == 4)
            {
                if (typeDesc.Elements == 0)
                {
                    return ParamType::VEC4;
                }

                repeatCount = typeDesc.Elements;
                return ParamType::VEC4_ARRAY;
            }
        }
        else if (typeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
        {
            if (typeDesc.Type == D3D_SVT_FLOAT && typeDesc.Rows == 4 && typeDesc.Columns == 4)
            {
                return ParamType::MATRIX;
            }
        }

        throw std::runtime_error("Unsupported dx parameter type");
    }

    static uint32_t GetLogicSize(const ParamType type, const uint32_t repeatCount = 1)
    {
        switch (type)
        {
            case ParamType::INT:
                return sizeof(int32_t);
            case ParamType::FLOAT:
                return sizeof(float);
            case ParamType::VEC4:
                return sizeof(float) * 4;
            case ParamType::MATRIX:
                return sizeof(float) * 16;
            case ParamType::VEC4_ARRAY:
                return sizeof(float) * 4 * repeatCount;
            case ParamType::FLOAT_ARRAY:
                return sizeof(float) * 4 * repeatCount;
        }
    }
}
