#include "citrus/nor.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace nor {
        static bool initialized = false;
    }
}

bool ctr::nor::init() {
    ctr::err::parse((u32) CFGNOR_Initialize(1));
    initialized = !ctr::err::has();
    return initialized;
}

void ctr::nor::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    CFGNOR_Shutdown();
}

bool ctr::nor::read(u32 offset, void* data, u32 size) {
    if(!initialized) {
        return false;
    }

    ctr::err::parse((u32) CFGNOR_ReadData(offset, (u32*) data, size));
    return !ctr::err::has();
}

bool ctr::nor::write(u32 offset, void* data, u32 size) {
    if(!initialized) {
        return false;
    }

    ctr::err::parse((u32) CFGNOR_WriteData(offset, (u32*) data, size));
    return !ctr::err::has();
}