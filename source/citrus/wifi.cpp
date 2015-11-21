#include "citrus/wifi.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace wifi {
        static bool initialized = false;
        static ctr::err::Error initError = {};
    }
}

bool ctr::wifi::init() {
    ctr::err::parse(ctr::err::SOURCE_WIFI_INIT, (u32) acInit());
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
        ctr::err::set(initError);
    }

    return initialized;
}

void ctr::wifi::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    initError = {};

    acExit();
}

bool ctr::wifi::connected() {
    if(!initialized) {
        ctr::err::set(initError);
        return false;
    }

    u32 status;
    ctr::err::parse(ctr::err::SOURCE_WIFI_GET_STATUS, (u32) ACU_GetWifiStatus(&status));
    if(ctr::err::has()) {
        return false;
    }

    return status != 0;
}

u8 ctr::wifi::strength() {
    if(!initialized) {
        ctr::err::set(initError);
        return 0;
    }

    return connected() ? osGetWifiStrength() : (u8) 0;
}