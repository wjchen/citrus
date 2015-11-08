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
        initError = {ctr::err::SOURCE_IR_ALLOCATE_BUFFER, ctr::err::MODULE_NN_IR, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_OUT_OF_RESOURCE, ctr::err::DESCRIPTION_OUT_OF_MEMORY};
        ctr::err::set(initError);
        return false;
    }

    ctr::err::parse(ctr::err::SOURCE_IR_INIT, (u32) iruInit(buffer, 0x1000));
    if(ctr::err::has()) {
        free(buffer);
        buffer = NULL;

        initError = ctr::err::get();
        ctr::err::set(initError);
        return false;
    }

    return true;
}

void ctr::ir::exit() {
    if(buffer == NULL) {
        return;
    }

    iruExit();

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
    ctr::err::parse(ctr::err::SOURCE_IR_GET_STATE, (u32) IRU_GetIRLEDRecvState(&state));
    if(ctr::err::has()) {
        return 0;
    }

    return state;
}

void ctr::ir::set(u32 state) {
    if(buffer == NULL) {
        ctr::err::set(initError);
        return;
    }

    ctr::err::parse(ctr::err::SOURCE_IR_SET_STATE, (u32) IRU_SetIRLEDState(state));
}