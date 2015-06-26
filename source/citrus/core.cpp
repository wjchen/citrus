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

bool ctr::core::init() {
    oldErrTab = devoptab_list[STD_ERR];
    devoptab_list[STD_ERR] = &debugOpTab;

    hasLauncher = hbInit() == 0;
    if(hasLauncher) {
        hbExit();

        // We've been launched from Ninjhax (hopefully), so try to acquire kernel access and extra services.
        hasKernel = khaxInit() == 0;
    }

    bool ret = err::init() && gpu::init() && gput::init() && fs::init() && hid::init();
    if(ret) {
        // Not required.
        snd::init();
        wifi::init();
        soc::init();
        app::init();
        news::init();
        nor::init();
        battery::init();
        ir::init();
        ui::init();
    }

    return ret;
}

void ctr::core::exit() {
    ui::exit();
    ir::exit();
    battery::exit();
    nor::exit();
    news::exit();
    app::exit();
    soc::exit();
    wifi::exit();
    snd::exit();
    hid::exit();
    fs::exit();
    gput::exit();
    gpu::exit();
    err::exit();

    if(hasKernel) {
        khaxExit();
    }

    hasKernel = false;
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

bool ctr::core::execKernel(s32 (*func)()) {
    if(!hasKernel) {
        return false;
    }

    svcBackdoor(func);
    return true;
}