#include "citrus/ir.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <malloc.h>

#include <3ds.h>

namespace ctr {
    namespace ir {
        static u32* buffer = NULL;
        static ctr::err::Error initError = {};
    }
}

bool ctr::ir::init() {
    buffer = (u32*) memalign(0x1000, 0x1000);
    if(buffer == NULL) {
        initError = {ctr::err::MODULE_NN_IR, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_OUT_OF_RESOURCE, ctr::err::DESCRIPTION_OUT_OF_MEMORY};
        return false;
    }

    ctr::err::parse((u32) IRU_Initialize(buffer, 0x1000));
    if(ctr::err::has()) {
        free(buffer);
        buffer = NULL;

        initError = ctr::err::get();
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
    initError = {};
}

u32 ctr::ir::get() {
    if(buffer == NULL) {
        ctr::err::set(initError);
        return 0;
    }

    u32 state;
    IRU_GetIRLEDRecvState(&state);
    return state;
}

void ctr::ir::set(u32 state) {
    if(buffer == NULL) {
        ctr::err::set(initError);
        return;
    }

    IRU_SetIRLEDState(state);
}