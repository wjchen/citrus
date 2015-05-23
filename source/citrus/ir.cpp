#include "citrus/ir.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <malloc.h>

#include <3ds.h>

namespace ctr {
    namespace ir {
        static u32* buffer = NULL;
    }
}

bool ctr::ir::init() {
    buffer = (u32*) memalign(0x1000, 0x1000);
    if(buffer == NULL) {
        return false;
    }

    ctr::err::parse((u32) IRU_Initialize(buffer, 0x1000));
    if(ctr::err::has()) {
        free(buffer);
        buffer = NULL;
        return false;
    }

    return true;
}

void ctr::ir::exit() {
    if(buffer == NULL) {
        return;
    }

    IRU_Shutdown();

    free(buffer);
    buffer = NULL;
}

u32 ctr::ir::get() {
    if(buffer == NULL) {
        return 0;
    }

    u32 state;
    IRU_GetIRLEDRecvState(&state);
    return state;
}

void ctr::ir::set(u32 state) {
    if(buffer == NULL) {
        return;
    }

    IRU_SetIRLEDState(state);
}