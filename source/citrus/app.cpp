#include "citrus/app.hpp"
#include "citrus/core.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>

#include <3ds.h>

namespace ctr {
    namespace app {
        static bool initialized = false;

        AppPlatform platformFromId(u16 id);
        AppCategory categoryFromId(u16 id);
    }
}

bool ctr::app::init() {
    ctr::err::parse((u32) amInit());
    initialized = !ctr::err::has();
    return initialized;
}

void ctr::app::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    amExit();
}

ctr::app::AppResult ctr::app::ciaInfo(ctr::app::App& app, const std::string file) {
    if(!initialized) {
        return APP_AM_INIT_FAILED;
    }

    FS_archive archive = (FS_archive) {ARCH_SDMC, (FS_path) {PATH_EMPTY, 1, (u8*) ""}};
    ctr::err::parse((u32) FSUSER_OpenArchive(NULL, &archive));
    if(ctr::err::has()) {
        return APP_OPEN_ARCHIVE_FAILED;
    }

    Handle handle = 0;
    ctr::err::parse((u32) FSUSER_OpenFile(NULL, &handle, archive, FS_makePath(PATH_CHAR, file.c_str()), FS_OPEN_READ, FS_ATTRIBUTE_NONE));
    if(ctr::err::has()) {
        return APP_OPEN_FILE_FAILED;
    }

    TitleList titleInfo;
    ctr::err::parse((u32) AM_GetCiaFileInfo(mediatype_SDMC, &titleInfo, handle));
    if(ctr::err::has()) {
        return APP_TITLE_INFO_FAILED;
    }

    FSFILE_Close(handle);
    FSUSER_CloseArchive(NULL, &archive);

    app.titleId = titleInfo.titleID;
    app.uniqueId = ((u32*) &titleInfo.titleID)[0];
    strcpy(app.productCode, "<N/A>");
    app.mediaType = ctr::fs::SD;
    app.platform = platformFromId(((u16*) &titleInfo.titleID)[3]);
    app.category = categoryFromId(((u16*) &titleInfo.titleID)[2]);
    app.version = titleInfo.titleVersion;
    app.size = titleInfo.size;

    return APP_SUCCESS;
}

ctr::app::AppResult ctr::app::list(std::vector<ctr::app::App>& apps, ctr::fs::MediaType mediaType) {
    if(!initialized) {
        return APP_AM_INIT_FAILED;
    }

    u32 titleCount;
    ctr::err::parse((u32) AM_GetTitleCount(mediaType, &titleCount));
    if(ctr::err::has()) {
        return APP_TITLE_COUNT_FAILED;
    }

    u64 titleIds[titleCount];
    ctr::err::parse((u32) AM_GetTitleIdList(mediaType, titleCount, titleIds));
    if(ctr::err::has()) {
        return APP_TITLE_ID_LIST_FAILED;
    }

    TitleList titleList[titleCount];
    ctr::err::parse((u32) AM_ListTitles(mediaType, titleCount, titleIds, titleList));
    if(ctr::err::has()) {
        return APP_TITLE_INFO_FAILED;
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

    return APP_SUCCESS;
}

ctr::app::AppResult ctr::app::install(ctr::fs::MediaType mediaType, FILE* fd, u64 size, std::function<bool(u64 pos, u64 totalSize)> onProgress) {
    if(!initialized) {
        return APP_AM_INIT_FAILED;
    }

    if(onProgress != NULL) {
        onProgress(0, size);
    }

    Handle ciaHandle;
    ctr::err::parse((u32) AM_StartCiaInstall(mediaType, &ciaHandle));
    if(ctr::err::has()) {
        return APP_BEGIN_INSTALL_FAILED;
    }

    u32 bufSize = 128 * 1024; // 128KB
    u8* buf = new u8[bufSize];
    bool cancelled = false;
    u64 pos = 0;
    while(ctr::running()) {
        if(onProgress != NULL && !onProgress(pos, size)) {
            cancelled = true;
            break;
        }

        size_t bytesRead = fread(buf, 1, bufSize, fd);
        if(bytesRead > 0) {
            ctr::err::parse((u32) FSFILE_Write(ciaHandle, NULL, pos, buf, (u32) bytesRead, FS_WRITE_NOFLUSH));
            if(ctr::err::has()) {
                AM_CancelCIAInstall(&ciaHandle);
                return APP_INSTALL_ERROR;
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
        return APP_OPERATION_CANCELLED;
    }

    if(!ctr::running()) {
        AM_CancelCIAInstall(&ciaHandle);
        return APP_PROCESS_CLOSING;
    }

    if(size != 0 && pos != size) {
        AM_CancelCIAInstall(&ciaHandle);
        return APP_IO_ERROR;
    }

    if(onProgress != NULL) {
        onProgress(size, size);
    }

    ctr::err::parse((u32) AM_FinishCiaInstall(mediaType, &ciaHandle));
    if(ctr::err::has()) {
        return APP_FINALIZE_INSTALL_FAILED;
    }

    return APP_SUCCESS;
}

ctr::app::AppResult ctr::app::uninstall(ctr::app::App app) {
    if(!initialized) {
        return APP_AM_INIT_FAILED;
    }

    ctr::err::parse((u32) AM_DeleteTitle(app.mediaType, app.titleId));
    if(ctr::err::has()) {
        return APP_DELETE_FAILED;
    }

    return APP_SUCCESS;
}

ctr::app::AppResult ctr::app::launch(ctr::app::App app) {
    u8 buf0[0x300];
    u8 buf1[0x20];

    aptOpenSession();
    ctr::err::parse((u32) APT_PrepareToDoAppJump(NULL, 0, app.titleId, app.mediaType));
    if(ctr::err::has()) {
        aptCloseSession();
        return APP_LAUNCH_FAILED;
    }

    ctr::err::parse((u32) APT_DoAppJump(NULL, 0x300, 0x20, buf0, buf1));
    if(ctr::err::has()) {
        aptCloseSession();
        return APP_LAUNCH_FAILED;
    }

    aptCloseSession();
    return APP_SUCCESS;
}

const std::string ctr::app::resultString(ctr::app::AppResult result) {
    std::stringstream resultMsg;
    if(result == APP_SUCCESS) {
        resultMsg << "Operation succeeded.";
    } else if(result == APP_PROCESS_CLOSING) {
        resultMsg << "Process closing.";
    } else if(result == APP_OPERATION_CANCELLED) {
        resultMsg << "Operation cancelled.";
    } else if(result == APP_AM_INIT_FAILED) {
        resultMsg << "Could not initialize AM service.";
    } else if(result == APP_IO_ERROR) {
        resultMsg << "I/O Error." << "\n" << strerror(errno);
    } else if(result == APP_OPEN_FILE_FAILED) {
        resultMsg << "Could not open file." << "\n" << strerror(errno);
    } else if(result == APP_BEGIN_INSTALL_FAILED) {
        resultMsg << "Could not begin installation." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_INSTALL_ERROR) {
        resultMsg << "Could not install app." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_FINALIZE_INSTALL_FAILED) {
        resultMsg << "Could not finalize installation." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_DELETE_FAILED) {
        resultMsg << "Could not delete app." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_LAUNCH_FAILED) {
        resultMsg << "Could not launch app." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_TITLE_COUNT_FAILED) {
        resultMsg << "Could not get title count." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_TITLE_ID_LIST_FAILED) {
        resultMsg << "Could not get title id list." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_TITLE_INFO_FAILED) {
        resultMsg << "Could not get title info." << "\n" << ctr::err::toString(ctr::err::get());
    } else if(result == APP_OPEN_ARCHIVE_FAILED) {
        resultMsg << "Could not open SD archive." << "\n" << ctr::err::toString(ctr::err::get());
    } else {
        resultMsg << "Unknown error.";
    }

    return resultMsg.str();
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