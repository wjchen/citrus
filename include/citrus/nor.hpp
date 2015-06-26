#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace nor {
        bool read(u32 offset, void* data, u32 size);
        bool write(u32 offset, void* data, u32 size);
    }
}