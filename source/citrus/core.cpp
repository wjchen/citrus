#include "citrus/core.hpp"
#include "internal.hpp"

#include "../libkhax/khax.h"

#include <sys/iosupport.h>

#include <cstdio>

#include <3ds.h>

namespace ctr {
    namespace core {
        ssize_t debugWrite(struct _reent* r, int fd, const char* ptr, size_t len) {
            svcOutputDebugString(ptr, len);
            return len;
        }

        static const devoptab_t debugOpTab = {
                "3dmoo",
                0,
                NULL,
                NULL,
                debugWrite,
                NULL,
                NULL,
                NULL
        };

        static const devoptab_t* oldErrTab = NULL;
        static bool hasLauncher = false;
        static bool hasKernel = false;
    }
}

bool ctr::core::init(int argc) {
    oldErrTab = devoptab_list[STD_ERR];
    devoptab_list[STD_ERR] = &debugOpTab;
    setvbuf(stderr, NULL, _IOLBF, 0);

    hasLauncher = argc > 0;

    bool ret = err::init() && utf::init() && gpu::init() && gput::init() && hid::init() && fs::init();
    if(ret) {
        // Try to acquire kernel access for additional service access.
        // libkhax currently only works through Ninjhax 1.x, so only
        // attempt to use it when we have access to the HB service.
        if(hasLauncher) {
            u8 n3ds = 0;
            bool isN3ds = osGetKernelVersion() >= SYSTEM_VERSION(2, 44, 6) && APT_CheckNew3DS(NULL, &n3ds) != 0 && n3ds != 0;
            Result hbResult = -1;
            if(!isN3ds || (hbResult = hbInit()) == 0) {
                hasKernel = khaxInit() == 0;
                if(hbResult == 0) {
                    hbExit();
                }
            }
        }

        // Not required.
        battery::init();
        wifi::init();
        soc::init();
        ir::init();
        nor::init();
        snd::init();
        app::init();
        news::init();

        ui::init();
    }

    return ret;
}

void ctr::core::exit() {
    ui::exit();

    news::exit();
    app::exit();
    snd::exit();
    nor::exit();
    ir::exit();
    soc::exit();
    wifi::exit();
    battery::exit();

    if(hasKernel) {
        khaxExit();
        hasKernel = false;
    }

    fs::exit();
    hid::exit();
    gput::exit();
    gpu::exit();
    utf::exit();
    err::exit();

    hasLauncher = false;

    devoptab_list[STD_ERR] = oldErrTab;
    oldErrTab = NULL;
}

bool ctr::core::running() {
    return aptMainLoop();
}

bool ctr::core::launcher() {
    return hasLauncher;
}

u64 ctr::core::time() {
    return osGetTime();
}