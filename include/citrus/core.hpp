#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace core {
        bool init();
        void exit();
        bool running();
        bool launcher();

        bool execKernel(s32 (*func)());
    }
}