#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace fs {
        typedef enum {
            NAND = 0,
            SD = 1
        } MediaType;

        u64 freeSpace(MediaType mediaType);
    }
}