#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace wifi {
        bool isConnected();
        bool waitForInternet();
        u8 getStrength();
    }
}