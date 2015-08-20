#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace wifi {
        bool connected();
        void waitForInternet();
        u8 strength();
    }
}