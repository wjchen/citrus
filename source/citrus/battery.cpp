#include "citrus/battery.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace battery {
        static bool initialized = false;
        static ctr::err::Error initError = {};
    }
}

bool ctr::battery::init() {
    ctr::err::parse((u32) ptmInit());
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
    }

    return initialized;
}

void ctr::battery::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    initError = {};

    ptmExit();
}

bool ctr::battery::charging() {
    if(!initialized) {
        ctr::err::set(initError);
        return false;
    }

    u8 charging;
    ctr::err::parse((u32) PTMU_GetBatteryChargeState(NULL, &charging));
    if(ctr::err::has()) {
        return false;
    }

    return charging != 0;
}

u8 ctr::battery::level() {
    if(!initialized) {
        ctr::err::set(initError);
        return 0;
    }

    u8 level;
    ctr::err::parse((u32) PTMU_GetBatteryLevel(NULL, &level));
    if(ctr::err::has()) {
        return 0;
    }

    return level;
}