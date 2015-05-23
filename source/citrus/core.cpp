#include "citrus/core.hpp"
#include "internal.hpp"

#include "../libkhax/khax.h"

#include <sys/iosupport.h>

#include <cstdio>

#include <3ds.h>

namespace ctr {
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

bool ctr::init() {
    oldErrTab = devoptab_list[STD_ERR];
    devoptab_list[STD_ERR] = &debugOpTab;
    setvbuf(stderr, NULL, _IOLBF, 0);

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
        soc::init();
        app::init();
        news::init();
        ir::init();
    }

    return ret;
}

void ctr::exit() {
    ir::exit();
    news::exit();
    app::exit();
    soc::exit();
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
    setvbuf(stderr, NULL, _IONBF, 0);
    oldErrTab = NULL;
}

bool ctr::running() {
    return aptMainLoop();
}

bool ctr::launcher() {
    return hasLauncher;
}

bool ctr::execKernel(s32 (*func)()) {
    if(!hasKernel) {
        return false;
    }

    svcBackdoor(func);
    return true;
}