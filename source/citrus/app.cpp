#include "citrus/app.hpp"
#include "citrus/core.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <unordered_map>

#include <3ds.h>

namespace ctr {
    namespace app {
        static bool initialized = false;
        static ctr::err::Error initError = {};

        AppPlatform platformFromId(u16 id);
        AppCategory categoryFromId(u16 id);
    }
}

bool ctr::app::init() {
    ctr::err::parse(ctr::err::SOURCE_AM_INIT, (u32) amInit());
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
        ctr::err::set(initError);
    } else {
        ctr::err::parse(ctr::err::SOURCE_AM_INITIALIZE_EXTERNAL_TITLE_DATABASE, (u32) AM_InitializeExternalTitleDatabase(false));
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
    ctr::err::parse(ctr::err::SOURCE_AM_GET_DEVICE_ID, (u32) AM_GetDeviceId(&deviceId));
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

    FS_Archive archive = (FS_Archive) {ARCHIVE_SDMC, (FS_Path) {PATH_EMPTY, 1, (u8*) ""}};
    ctr::err::parse(ctr::err::SOURCE_FSUSER_OPEN_ARCHIVE, (u32) FSUSER_OpenArchive(&archive));
    if(ctr::err::has()) {
        return {};
    }

    Handle handle = 0;
    ctr::err::parse(ctr::err::SOURCE_FSUSER_OPEN_FILE, (u32) FSUSER_OpenFile(&handle, archive, (FS_Path) {PATH_ASCII, file.length() + 1, (const u8*) file.c_str()}, FS_OPEN_READ, 0));
    if(ctr::err::has()) {
        return {};
    }

    AM_TitleEntry titleInfo;
    ctr::err::parse(ctr::err::SOURCE_AM_GET_CIA_FILE_INFO, (u32) AM_GetCiaFileInfo(mediaType, &titleInfo, handle));
    if(ctr::err::has()) {
        return {};
    }

    FSFILE_Close(handle);
    FSUSER_CloseArchive(&archive);

    App app;
    app.titleId = titleInfo.titleID;
    app.uniqueId = ((u32*) &titleInfo.titleID)[0];
    strcpy(app.productCode, "<N/A>");
    app.mediaType = mediaType;
    app.platform = platformFromId(((u16*) &titleInfo.titleID)[3]);
    app.category = categoryFromId(((u16*) &titleInfo.titleID)[2]);
    app.version = titleInfo.version;
    app.size = titleInfo.size;

    return app;
}

ctr::app::SMDH ctr::app::ciaSMDH(const std::string file) {
    FILE* fd = fopen(file.c_str(), "rb");
    if(!fd) {
        ctr::err::set({ctr::err::SOURCE_IO_ERROR, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_INVALID_STATE, (ctr::err::Description) errno});
        return {};
    }

    if(fseek(fd, -0x36C0, SEEK_END) != 0) {
        fclose(fd);

        ctr::err::set({ctr::err::SOURCE_IO_ERROR, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_INVALID_STATE, (ctr::err::Description) errno});
        return {};
    }

    SMDH smdh;
    size_t bytesRead = fread(&smdh, sizeof(SMDH), 1, fd);
    if(bytesRead < 0) {
        fclose(fd);

        ctr::err::set({ctr::err::SOURCE_IO_ERROR, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_INVALID_STATE, (ctr::err::Description) errno});
        return {};
    }

    fclose(fd);

    if(smdh.magic[0] != 'S' || smdh.magic[1] != 'M' || smdh.magic[2] != 'D' || smdh.magic[3] != 'H') {
        ctr::err::set({ctr::err::SOURCE_VALIDATE_SMDH, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_INVALID_ARGUMENT, ctr::err::DESCRIPTION_INVALID_SELECTION});
        return {};
    }

    return smdh;
}

std::vector<ctr::app::App> ctr::app::list(ctr::fs::MediaType mediaType) {
    std::vector<ctr::app::App> apps;

    if(!initialized) {
        ctr::err::set(initError);
        return apps;
    }

    u32 titleCount;
    ctr::err::parse(ctr::err::SOURCE_AM_GET_TITLE_COUNT, (u32) AM_GetTitleCount(mediaType, &titleCount));
    if(ctr::err::has()) {
        return apps;
    }

    u64 titleIds[titleCount];
    ctr::err::parse(ctr::err::SOURCE_AM_GET_TITLE_ID_LIST, (u32) AM_GetTitleIdList(mediaType, titleCount, titleIds));
    if(ctr::err::has()) {
        return apps;
    }

    AM_TitleEntry titleList[titleCount];
    ctr::err::parse(ctr::err::SOURCE_AM_LIST_TITLES, (u32) AM_ListTitles(mediaType, titleCount, titleIds, titleList));
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
        app.version = titleList[i].version;
        app.size = titleList[i].size;

        apps.push_back(app);
    }

    return apps;
}

ctr::app::SMDH ctr::app::smdh(ctr::app::App app) {
    static const u32 filePath[] = {0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000};
    static const FS_Path path = (FS_Path) {PATH_BINARY, 0x14, (u8*) filePath};

    u32 archivePath[] = {(u32) (app.titleId & 0xFFFFFFFF), (u32) ((app.titleId >> 32) & 0xFFFFFFFF), app.mediaType, 0x00000000};
    FS_Archive archive = (FS_Archive) {0x2345678a, (FS_Path) {PATH_BINARY, 0x10, (u8*) archivePath}};

    Handle handle;
    ctr::err::parse(ctr::err::SOURCE_FSUSER_OPEN_FILE, (u32) FSUSER_OpenFileDirectly(&handle, archive, path, FS_OPEN_READ, 0));
    if(ctr::err::has()) {
        return {};
    }

    SMDH smdh;
    u32 bytesRead;
    ctr::err::parse(ctr::err::SOURCE_FSFILE_READ, (u32) FSFILE_Read(handle, &bytesRead, 0x0, &smdh, sizeof(SMDH)));
    if(ctr::err::has()) {
        return {};
    }

    FSFILE_Close(handle);

    if(smdh.magic[0] != 'S' || smdh.magic[1] != 'M' || smdh.magic[2] != 'D' || smdh.magic[3] != 'H') {
        ctr::err::set({ctr::err::SOURCE_VALIDATE_SMDH, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_INVALID_ARGUMENT, ctr::err::DESCRIPTION_INVALID_SELECTION});
        return {};
    }

    return smdh;
}

bool ctr::app::installed(ctr::app::App app) {
    if(!initialized) {
        ctr::err::set(initError);
        return false;
    }

    u32 titleCount;
    ctr::err::parse(ctr::err::SOURCE_AM_GET_TITLE_COUNT, (u32) AM_GetTitleCount(app.mediaType, &titleCount));
    if(ctr::err::has()) {
        return false;
    }

    u64 titleIds[titleCount];
    ctr::err::parse(ctr::err::SOURCE_AM_GET_TITLE_ID_LIST, (u32) AM_GetTitleIdList(app.mediaType, titleCount, titleIds));
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

class BlockReader
{
private:
    int bufSize;
    int blockSize;
    int remainLen;

    unsigned char *remainBuf;
public:
    unsigned char *buf;
    unsigned char *availPtr;

    BlockReader(int bufSize, int blockSize) {
        if(bufSize <= 64) {
            this->bufSize = 64;
        }
        if(blockSize <= 64) {
            this->blockSize = 64;
        }
        if((blockSize & (blockSize - 1)) != 0) { //blockSize not 2^n
            this->blockSize = 64;
        }
        if(this->blockSize > this->bufSize) {
            this->blockSize = this->bufSize;
        }
        this->availPtr = NULL;
        this->remainLen = 0;
        this->remainBuf = 0;
        this->bufSize = bufSize + (64 - (bufSize & 63)); //ensure bufSize % 64 == 0
        this->buf = new unsigned char[this->bufSize + this->blockSize + 1];
        this->remainBuf = new unsigned char[this->blockSize + 1];
    }

    ~BlockReader() {
        delete[] this->remainBuf;
        delete[] this->buf;
    }

    int Read(FILE* fd, int readSize) {
        if(fd == NULL || readSize <= 0) return -1;
        if(readSize > this->bufSize) readSize = this->bufSize;
        int nread = fread(this->buf + this->blockSize, 1, readSize, fd);
        if(nread <= 0) {
            this->availPtr = NULL;
            return nread;
        }
        int availLen = nread + this->remainLen;
        //no enough, add new read to the end of remainbuf
        if(availLen < this->blockSize) {
            memcpy(this->remainBuf + this->remainLen, this->buf + this->blockSize, nread);
            this->remainLen = availLen;
            errno = EWOULDBLOCK;
            this->availPtr = NULL;
            return -1;
        }
        //copy remain to head
        unsigned char *availPtr = this->buf + this->blockSize - this->remainLen;
        if(this->remainLen > 0) {
            memcpy(availPtr, this->remainBuf, this->remainLen);
        }

        int nextRemainLen = availLen & (this->blockSize - 1);
        if(nextRemainLen > 0) {
            availLen -= nextRemainLen;
            //copy next remain to remanBuf
            memcpy(this->remainBuf, availPtr + availLen, nextRemainLen);
        }
        this->remainLen = nextRemainLen;
        this->availPtr = availPtr;
        return availLen;
    }
};

void ctr::app::install(ctr::fs::MediaType mediaType, FILE* fd, u64 size, std::function<bool(u64 pos, u64 totalSize)> onProgress) {
    if(!initialized) {
        ctr::err::set(initError);
        return;
    }

    if(onProgress != NULL) {
        onProgress(0, size);
    }

    Handle ciaHandle;
    ctr::err::parse(ctr::err::SOURCE_AM_START_CIA_INSTALL, (u32) AM_StartCiaInstall(mediaType, &ciaHandle));
    if(ctr::err::has()) {
        return;
    }

    u32 bufSize = 128 * 1024; // 128KB
    u64 pos = 0;
    BlockReader *reader = new BlockReader(bufSize, 64);

    while(true) {
        if(!ctr::core::running()) {
            ctr::err::set({ctr::err::SOURCE_PROCESS_CLOSING, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_STATUS_CHANGED, ctr::err::DESCRIPTION_CANCEL_REQUESTED});
            break;
        }

        if(onProgress != NULL && !onProgress(pos, size)) {
            ctr::err::set({ctr::err::SOURCE_OPERATION_CANCELLED, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_CANCELED, ctr::err::DESCRIPTION_CANCEL_REQUESTED});
            break;
        }

        u32 readSize = bufSize;
        if(size != 0) {
            u64 remaining = size - pos;
            if(remaining < readSize) {
                readSize = (u32) remaining;
            }
        }

        size_t bytesRead = reader->Read(fd, readSize);
        u8 *buf = reader->availPtr;
        if(bytesRead > 0) {
            ctr::err::parse(ctr::err::SOURCE_FSFILE_WRITE, (u32) FSFILE_Write(ciaHandle, NULL, pos, buf, (u32) bytesRead, 0));
            if(ctr::err::has()) {
                break;
            }

            pos += bytesRead;
        }

        if(feof(fd) || (size != 0 && pos >= size)) {
            break;
        }

        if(ferror(fd) && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINPROGRESS) {
            ctr::err::set({ctr::err::SOURCE_IO_ERROR, ctr::err::MODULE_APPLICATION, ctr::err::LEVEL_PERMANENT, ctr::err::SUMMARY_INVALID_STATE, (ctr::err::Description) errno});
            break;
        }
    }

    delete reader;

    if(ctr::err::has()) {
        AM_CancelCIAInstall(&ciaHandle);
        return;
    }

    if(onProgress != NULL) {
        onProgress(size, size);
    }

    ctr::err::parse(ctr::err::SOURCE_AM_FINISH_CIA_INSTALL, (u32) AM_FinishCiaInstall(mediaType, &ciaHandle));
}

void ctr::app::uninstall(ctr::app::App app) {
    if(!initialized) {
        ctr::err::set(initError);
        return;
    }

    ctr::err::parse(ctr::err::SOURCE_AM_DELETE_TITLE, (u32) AM_DeleteTitle(app.mediaType, app.titleId));
}

void ctr::app::launch(ctr::app::App app) {
    u8 buf0[0x300];
    u8 buf1[0x20];

    aptOpenSession();
    ctr::err::parse(ctr::err::SOURCE_APT_PREPARE_TO_DO_APP_JUMP, (u32) APT_PrepareToDoAppJump(0, app.titleId, app.mediaType));
    if(!ctr::err::has()) {
        ctr::err::parse(ctr::err::SOURCE_APT_DO_APP_JUMP, (u32) APT_DoAppJump(0x300, 0x20, buf0, buf1));
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

extern std::unordered_map<u64, std::string> gameInfoMap;
const std::string ctr::app::nameFromTitleid(u64 titleid) {
    std::unordered_map<u64, std::string>::iterator it;
    it = gameInfoMap.find(titleid);
    if(it != gameInfoMap.end()) {
        return it->second;
    }
    return "<N/A>";
}