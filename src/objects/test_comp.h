#pragma once
#include "comp.h"

namespace dt
{
    class TestComp final : public Comp
    {
    public:
        void Start() override;
        void Update() override;
    };
}
