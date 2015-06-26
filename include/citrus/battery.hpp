#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace battery {
        bool isCharging();
        u8 getLevel();
    }
}