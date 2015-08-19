#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace wifi {
        bool connected();
        bool waitForInternet();
        u8 strength();
    }
}