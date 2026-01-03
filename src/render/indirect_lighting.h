#pragma once
#include <array>

#include "common/math.h"

namespace dt
{
    class Shc
    {
    public:
        void SetShc(int channel, int term, float val);
        void AddShc(int channel, int term, float val);
        float GetShc(int channel, int term);
        float* GetData();
        
    private:
        std::array<float, 27> m_data = {};
    };

    class IndirectLighting
    {
    public:
        static void SetGradientAmbientColor(cr<XMFLOAT3> sky, cr<XMFLOAT3> equator, cr<XMFLOAT3> ground);
        
        static Shc CalcShc(cr<XMFLOAT3> sky, cr<XMFLOAT3> equator, cr<XMFLOAT3> ground);

        static std::array<float, 9> CalcBaseShc(cr<XMFLOAT3> direction);
        static XMFLOAT3 SampleColor(cr<XMFLOAT3> direction, cr<XMFLOAT3> sky, cr<XMFLOAT3> equator, cr<XMFLOAT3> ground);
    };
}
