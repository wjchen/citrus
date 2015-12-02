#include "citrus/ir.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <malloc.h>

#include <3ds.h>

namespace ctr {
    namespace ir {
        static u32* iruBuffer = NULL;
        static ctr::err::Error initError = {};
    }
}

bool ctr::ir::init() {
    iruBuffer = (u32*) memalign(0x1000, 0x1000);
    if(iruBuffer == NULL) {
        initError = {ctr::err::SOURCE_ALLOCATE_BUFFER, ctr::err::MODULE_NN_IR, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_OUT_OF_RESOURCE, ctr::err::DESCRIPTION_OUT_OF_MEMORY};
        ctr::err::set(initError);
        return false;
    }

    ctr::err::parse(ctr::err::SOURCE_IRU_INIT, (u32) iruInit(iruBuffer, 0x1000));
    if(ctr::err::has()) {
        free(iruBuffer);
        iruBuffer = NULL;

        initError = ctr::err::get();
        ctr::err::set(initError);
        return false;
    }

    return true;
}

void ctr::ir::exit() {
    if(iruBuffer == NULL) {
        return;
    }

    iruExit();

    free(iruBuffer);
    iruBuffer = NULL;
    initError = {};
}

u32 ctr::ir::read(void* buffer, u32 size) {
    if(iruBuffer == NULL) {
        ctr::err::set(initError);
        return 0;
    }

    u32 bytesRead = 0;
    ctr::err::parse(ctr::err::SOURCE_IRU_RECV_DATA, (u32) iruRecvData((u8*) buffer, size, 0, &bytesRead, true));
    return bytesRead;
}

void ctr::ir::write(void* buffer, u32 size, bool wait) {
    if(iruBuffer == NULL) {
        ctr::err::set(initError);
        return;
    }

    ctr::err::parse(ctr::err::SOURCE_IRU_SEND_DATA, (u32) iruSendData((u8*) buffer, size, wait));
}