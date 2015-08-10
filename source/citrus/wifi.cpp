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
    ctr::err::parse((u32) acInit());
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
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

bool ctr::wifi::isConnected() {
    if(!initialized) {
        ctr::err::set(initError);
        return false;
    }

    u32 status;
    ctr::err::parse((u32) ACU_GetWifiStatus(NULL, &status));
    if(ctr::err::has()) {
        return false;
    }

    return status != 0;
}

bool ctr::wifi::waitForInternet() {
    if(!initialized) {
        ctr::err::set(initError);
        return false;
    }

    ctr::err::parse((u32) ACU_WaitInternetConnection());
    return !ctr::err::has();
}

u8 ctr::wifi::getStrength() {
    if(!initialized) {
        ctr::err::set(initError);
        return 0;
    }

    return isConnected() ? osGetWifiStrength() : (u8) 0;
}