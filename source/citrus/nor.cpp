#include "citrus/nor.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace nor {
        static bool initialized = false;
        static ctr::err::Error initError = {};
    }
}

bool ctr::nor::init() {
    ctr::err::parse(ctr::err::SOURCE_NOR_INIT, (u32) CFGNOR_Initialize(1));
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
        ctr::err::set(initError);
    }

    return initialized;
}

void ctr::nor::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    initError = {};

    CFGNOR_Shutdown();
}

void ctr::nor::read(u32 offset, void* data, u32 size) {
    if(!initialized) {
        ctr::err::set(initError);
        return;
    }

    ctr::err::parse(ctr::err::SOURCE_NOR_READ_DATA, (u32) CFGNOR_ReadData(offset, (u32*) data, size));
}

void ctr::nor::write(u32 offset, void* data, u32 size) {
    if(!initialized) {
        ctr::err::set(initError);
        return;
    }

    ctr::err::parse(ctr::err::SOURCE_NOR_WRITE_DATA, (u32) CFGNOR_WriteData(offset, (u32*) data, size));
}