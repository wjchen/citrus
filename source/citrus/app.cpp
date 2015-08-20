#include "citrus/app.hpp"
#include "citrus/core.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>

#include <3ds.h>
#include <citrus/app.hpp>

namespace ctr {
    namespace app {
        static bool initialized = false;
        static ctr::err::Error initError = {};

        AppPlatform platformFromId(u16 id);
        AppCategory categoryFromId(u16 id);
    }
}

bool ctr::app::init() {
    ctr::err::parse(ctr::err::SOURCE_APP_INIT, (u32) amInit());
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
        ctr::err::set(initError);
    }

    return initialized;
}

void ctr::app::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    initError = {};

    amExit();
}

u32 ctr::app::deviceId() {
    if(!initialized) {
        ctr::err::set(initError);
        return 0;
    }

    u32 deviceId;
    ctr::err::parse(ctr::err::SOURCE_APP_GET_DEVICE_ID, (u32) AM_GetDeviceId(&deviceId));
    if(ctr::err::has()) {
        return 0;
    }

    return deviceId;
}

ctr::app::App ctr::app::ciaInfo(const std::string file, ctr::fs::MediaType mediaType) {
    if(!initialized) {
        ctr::err::set(initError);
        return {};
    }

    FS_archive archive = (FS_archive) {ARCH_SDMC, (FS_path) {PATH_EMPTY, 1, (u8*) ""}};
    ctr::err::parse(ctr::err::SOURCE_APP_OPEN_ARCHIVE, (u32) FSUSER_OpenArchive(NULL, &archive));
    if(ctr::err::has()) {
        return {};
    }

    Handle handle = 0;
    ctr::err::parse(ctr::err::SOURCE_APP_OPEN_FILE, (u32) FSUSER_OpenFile(NULL, &handle, archive, FS_makePath(PATH_CHAR, file.c_str()), FS_OPEN_READ, FS_ATTRIBUTE_NONE));
    if(ctr::err::has()) {
        return {};
    }

    TitleList titleInfo;
    ctr::err::parse(ctr::err::SOURCE_APP_GET_TITLE_INFO, (u32) AM_GetCiaFileInfo(mediaType, &titleInfo, handle));
    if(ctr::err::has()) {
        return {};
    }

    FSFILE_Close(handle);
    FSUSER_CloseArchive(NULL, &archive);

    App app;
    app.titleId = titleInfo.titleID;
    app.uniqueId = ((u32*) &titleInfo.titleID)[0];
    strcpy(app.productCode, "<N/A>");
    app.mediaType = mediaType;
    app.platform = platformFromId(((u16*) &titleInfo.titleID)[3]);
    app.category = categoryFromId(((u16*) &titleInfo.titleID)[2]);
    app.version = titleInfo.titleVersion;
    app.size = titleInfo.size;

    return app;
}

bool ctr::app::installed(ctr::app::App app) {
    if(!initialized) {
        ctr::err::set(initError);
        return false;
    }

    u32 titleCount;
    ctr::err::parse(ctr::err::SOURCE_APP_GET_TITLE_COUNT, (u32) AM_GetTitleCount(app.mediaType, &titleCount));
    if(ctr::err::has()) {
        return false;
    }

    u64 titleIds[titleCount];
    ctr::err::parse(ctr::err::SOURCE_APP_GET_TITLE_ID_LIST, (u32) AM_GetTitleIdList(app.mediaType, titleCount, titleIds));
    if(ctr::err::has()) {
        return false;
    }

    for(u32 i = 0; i < titleCount; i++) {
        if(titleIds[i] == app.titleId) {
            return true;
        }
    }

    return false;
}

std::vector<ctr::app::App> ctr::app::list(ctr::fs::MediaType mediaType) {
    std::vector<ctr::app::App> apps;

    if(!initialized) {
        ctr::err::set(initError);
        return apps;
    }

    u32 titleCount;
    ctr::err::parse(ctr::err::SOURCE_APP_GET_TITLE_COUNT, (u32) AM_GetTitleCount(mediaType, &titleCount));
    if(ctr::err::has()) {
        return apps;
    }

    u64 titleIds[titleCount];
    ctr::err::parse(ctr::err::SOURCE_APP_GET_TITLE_ID_LIST, (u32) AM_GetTitleIdList(mediaType, titleCount, titleIds));
    if(ctr::err::has()) {
        return apps;
    }

    TitleList titleList[titleCount];
    ctr::err::parse(ctr::err::SOURCE_APP_GET_TITLE_INFO, (u32) AM_ListTitles(mediaType, titleCount, titleIds, titleList));
    if(ctr::err::has()) {
        return apps;
    }

    for(u32 i = 0; i < titleCount; i++) {
        u64 titleId = titleList[i].titleID;

        App app;
        app.titleId = titleId;
        app.uniqueId = ((u32*) &titleId)[0];
        AM_GetTitleProductCode(mediaType, titleId, app.productCode);
        if(strcmp(app.productCode, "") == 0) {
            strcpy(app.productCode, "<N/A>");
        }

        app.mediaType = mediaType;
        app.platform = platformFromId(((u16*) &titleId)[3]);
        app.category = categoryFromId(((u16*) &titleId)[2]);
        app.version = titleList[i].titleVersion;
        app.size = titleList[i].size;

        apps.push_back(app);
    }

    return apps;
}

void ctr::app::install(ctr::fs::MediaType mediaType, FILE* fd, u64 size, std::function<bool(u64 pos, u64 totalSize)> onProgress) {
    if(!initialized) {
        ctr::err::set(initError);
        return;
    }

    if(onProgress != NULL) {
        onProgress(0, size);
    }

    Handle ciaHandle;
    ctr::err::parse(ctr::err::SOURCE_APP_BEGIN_INSTALL, (u32) AM_StartCiaInstall(mediaType, &ciaHandle));
    if(ctr::err::has()) {
        return;
    }

    u32 bufSize = 128 * 1024; // 128KB
    u8* buf = new u8[bufSize];
    bool cancelled = false;
    u64 pos = 0;
    while(ctr::core::running()) {
        if(onProgress != NULL && !onProgress(pos, size)) {
            cancelled = true;
            break;
        }

        size_t bytesRead = fread(buf, 1, bufSize, fd);
        if(bytesRead > 0) {
            ctr::err::parse(ctr::err::SOURCE_APP_WRITE_CIA, (u32) FSFILE_Write(ciaHandle, NULL, pos, buf, (u32) bytesRead, FS_WRITE_NOFLUSH));
            if(ctr::err::has()) {
                AM_CancelCIAInstall(&ciaHandle);
                return;
            }

            pos += bytesRead;
        }

        if((ferror(fd) && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINPROGRESS) || (size != 0 && pos == size)) {
            break;
        }
    }

    delete buf;

    if(cancelled) {
        AM_CancelCIAInstall(&ciaHandle);
        ctr::err::set({ctr::err::SOURCE_OPERATION_CANCELLED, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_CANCELED, ctr::err::DESCRIPTION_CANCEL_REQUESTED});
        return;
    }

    if(!ctr::core::running()) {
        AM_CancelCIAInstall(&ciaHandle);
        ctr::err::set({ctr::err::SOURCE_PROCESS_CLOSING, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_STATUS_CHANGED, ctr::err::DESCRIPTION_CANCEL_REQUESTED});
        return;
    }

    if(size != 0 && pos != size) {
        AM_CancelCIAInstall(&ciaHandle);
        ctr::err::set({ctr::err::SOURCE_APP_IO_ERROR, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_INVALID_STATE, (ctr::err::Description) errno});
        return;
    }

    if(onProgress != NULL) {
        onProgress(size, size);
    }

    ctr::err::parse(ctr::err::SOURCE_APP_FINALIZE_INSTALL, (u32) AM_FinishCiaInstall(mediaType, &ciaHandle));
}

void ctr::app::uninstall(ctr::app::App app) {
    if(!initialized) {
        ctr::err::set(initError);
        return;
    }

    ctr::err::parse(ctr::err::SOURCE_APP_DELETE_TITLE, (u32) AM_DeleteTitle(app.mediaType, app.titleId));
}

void ctr::app::launch(ctr::app::App app) {
    u8 buf0[0x300];
    u8 buf1[0x20];

    aptOpenSession();
    ctr::err::parse(ctr::err::SOURCE_APP_PREPARE_LAUNCH, (u32) APT_PrepareToDoAppJump(NULL, 0, app.titleId, app.mediaType));
    if(!ctr::err::has()) {
        ctr::err::parse(ctr::err::SOURCE_APP_DO_LAUNCH, (u32) APT_DoAppJump(NULL, 0x300, 0x20, buf0, buf1));
    }

    aptCloseSession();
}

const std::string ctr::app::platformString(ctr::app::AppPlatform platform) {
    switch(platform) {
        case PLATFORM_WII:
            return "Wii";
        case PLATFORM_DSI:
            return "DSi";
        case PLATFORM_3DS:
            return "3DS";
        case PLATFORM_WIIU:
            return "Wii U";
        default:
            return "Unknown";
    }
}

const std::string ctr::app::categoryString(ctr::app::AppCategory category) {
    switch(category) {
        case CATEGORY_APP:
            return "App";
        case CATEGORY_DEMO:
            return "Demo";
        case CATEGORY_DLC:
            return "DLC";
        case CATEGORY_PATCH:
            return "Patch";
        case CATEGORY_SYSTEM:
            return "System";
        case CATEGORY_TWL:
            return "TWL";
        default:
            return "Unknown";
    }
}

ctr::app::AppPlatform ctr::app::platformFromId(u16 id) {
    switch(id) {
        case 1:
            return PLATFORM_WII;
        case 3:
            return PLATFORM_DSI;
        case 4:
            return PLATFORM_3DS;
        case 5:
            return PLATFORM_WIIU;
        default:
            return PLATFORM_UNKNOWN;
    }
}

ctr::app::AppCategory ctr::app::categoryFromId(u16 id) {
    if((id & 0x8000) == 0x8000) {
        return CATEGORY_TWL;
    } else if((id & 0x10) == 0x10) {
        return CATEGORY_SYSTEM;
    } else if((id & 0x6) == 0x6) {
        return CATEGORY_PATCH;
    } else if((id & 0x4) == 0x4) {
        return CATEGORY_DLC;
    } else if((id & 0x2) == 0x2) {
        return CATEGORY_DEMO;
    }

    return CATEGORY_APP;
}