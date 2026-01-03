#include "indirect_lighting.h"

#include "cbuffer.h"
#include "game/game_resource.h"

namespace dt
{
    void Shc::SetShc(const int channel, const int term, const float val)
    {
        m_data[term * 3 + channel] = val;
    }

    void Shc::AddShc(const int channel, const int term, const float val)
    {
        m_data[term * 3 + channel] += val;
    }

    float Shc::GetShc(const int channel, const int term)
    {
        return m_data[term * 3 + channel];
    }

    float* Shc::GetData()
    {
        return m_data.data();
    }

    void IndirectLighting::SetGradientAmbientColor(cr<XMFLOAT3> sky, cr<XMFLOAT3> equator, cr<XMFLOAT3> ground)
    {
        auto shc = CalcShc(sky, equator, ground);
        GetGlobalCbuffer()->Write(SHC, shc.GetData(), 27 * sizeof(float));
    }

    Shc IndirectLighting::CalcShc(cr<XMFLOAT3> sky, cr<XMFLOAT3> equator, cr<XMFLOAT3> ground)
    {
        static XMFLOAT3 directions[6] =
        {
            XMFLOAT3(1.0f, 0.0f, 0.0f),
            XMFLOAT3(-1.0f, 0.0f, 0.0f),
            XMFLOAT3(0.0f, 1.0f, 0.0f),
            XMFLOAT3(0.0f, -1.0f, 0.0f),
            XMFLOAT3(0.0f, 0.0f, 1.0f),
            XMFLOAT3(0.0f, 0.0f, -1.0f)
        };

        Shc result;
        for (auto direction : directions)
        {
            auto baseShc = CalcBaseShc(direction);
            auto color = SampleColor(direction, sky, equator, ground);
            
            for (int term = 0; term < 9; ++term)
            {
                for (int channel = 0; channel < 3; ++channel)
                {
                    result.AddShc(channel, term, baseShc[term] * (&color.x)[channel]);
                }
            }
        }

        return result;
    }

    std::array<float, 9> IndirectLighting::CalcBaseShc(cr<XMFLOAT3> direction)
    {
        return {
            0.282095f,
            0.488603f * direction.y,
            0.488603f * direction.z,
            0.488603f * direction.x,
            1.092548f * direction.y * direction.x,
            1.092548f * direction.y * direction.z,
            0.315392f * (-direction.x * direction.x - direction.y * direction.y + 2 * direction.z * direction.z),
            1.092548f * direction.z * direction.x,
            0.546274f * (direction.x * direction.x - direction.y * direction.y)
        };
    }

    XMFLOAT3 IndirectLighting::SampleColor(cr<XMFLOAT3> direction, cr<XMFLOAT3> sky, cr<XMFLOAT3> equator, cr<XMFLOAT3> ground)
    {
        if (direction.y > 0.5f)
        {
            return sky;
        }
        
        if (direction.y < -0.5f)
        {
            return ground;
        }
        
        return equator;
    }
}
