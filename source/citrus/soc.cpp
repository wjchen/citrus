#include "citrus/err.hpp"
#include "internal.hpp"

#include <malloc.h>

#include <3ds.h>

namespace ctr {
    namespace soc {
        static u32* buffer = NULL;
    }
}

bool ctr::soc::init() {
    buffer = (u32*) memalign(0x1000, 0x100000);
    if(buffer == NULL) {
        return false;
    }

    ctr::err::parse((u32) SOC_Initialize(buffer, 0x100000));
    if(ctr::err::has()) {
        free(buffer);
        buffer = NULL;
        return false;
    }

    return true;
}

void ctr::soc::exit() {
    if(buffer == NULL) {
        return;
    }

    SOC_Shutdown();

    free(buffer);
    buffer = NULL;
}