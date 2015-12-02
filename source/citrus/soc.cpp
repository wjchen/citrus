#include "citrus/err.hpp"
#include "internal.hpp"

#include <malloc.h>

#include <3ds.h>

namespace ctr {
    namespace soc {
        static u32* socBuffer = NULL;
    }
}

bool ctr::soc::init() {
    socBuffer = (u32*) memalign(0x1000, 0x100000);
    if(socBuffer == NULL) {
        ctr::err::set({ctr::err::SOURCE_ALLOCATE_BUFFER, ctr::err::MODULE_NN_SOCKET, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_OUT_OF_RESOURCE, ctr::err::DESCRIPTION_OUT_OF_MEMORY});
        return false;
    }

    ctr::err::parse(ctr::err::SOURCE_SOC_INIT, (u32) socInit(socBuffer, 0x100000));
    if(ctr::err::has()) {
        free(socBuffer);
        socBuffer = NULL;

        return false;
    }

    return true;
}

void ctr::soc::exit() {
    if(socBuffer == NULL) {
        return;
    }

    socExit();

    free(socBuffer);
    socBuffer = NULL;
}