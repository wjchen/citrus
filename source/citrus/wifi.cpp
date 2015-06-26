#include "citrus/wifi.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace wifi {
        static bool initialized = false;
    }
}

bool ctr::wifi::init() {
    ctr::err::parse((u32) acInit());
    initialized = !ctr::err::has();
    return initialized;
}

void ctr::wifi::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    acExit();
}

bool ctr::wifi::isConnected() {
    u32 status;
    ctr::err::parse((u32) ACU_GetWifiStatus(NULL, &status));
    if(ctr::err::has()) {
        return false;
    }

    return status != 0;
}

bool ctr::wifi::waitForInternet() {
    ctr::err::parse((u32) ACU_WaitInternetConnection());
    return !ctr::err::has();
}

u8 ctr::wifi::getStrength() {
    return isConnected() ? osGetWifiStrength() : (u8) 0;
}