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
    ctr::err::parse(ctr::err::SOURCE_BATTERY_INIT, (u32) ptmInit());
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
        ctr::err::set(initError);
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
    ctr::err::parse(ctr::err::SOURCE_BATTERY_GET_CHARGE_STATE, (u32) PTMU_GetBatteryChargeState(&charging));
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
    ctr::err::parse(ctr::err::SOURCE_BATTERY_GET_LEVEL, (u32) PTMU_GetBatteryLevel(&level));
    if(ctr::err::has()) {
        return 0;
    }

    return level;
}