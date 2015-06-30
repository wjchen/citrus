#pragma once

#include "citrus/types.hpp"
#include "citrus/fs.hpp"

#include <functional>
#include <vector>

namespace ctr {
    namespace app {
        typedef enum {
            PLATFORM_WII,
            PLATFORM_DSI,
            PLATFORM_3DS,
            PLATFORM_WIIU,
            PLATFORM_UNKNOWN
        } AppPlatform;

        typedef enum {
            CATEGORY_APP,
            CATEGORY_DEMO,
            CATEGORY_DLC,
            CATEGORY_PATCH,
            CATEGORY_SYSTEM,
            CATEGORY_TWL
        } AppCategory;

        typedef struct {
            u64 titleId;
            u32 uniqueId;
            char productCode[16];
            ctr::fs::MediaType mediaType;
            AppPlatform platform;
            AppCategory category;
            u16 version;
            u64 size;
        } App;

        typedef enum {
            APP_SUCCESS,
            APP_PROCESS_CLOSING,
            APP_OPERATION_CANCELLED,
            APP_AM_INIT_FAILED,
            APP_IO_ERROR,
            APP_OPEN_FILE_FAILED,
            APP_BEGIN_INSTALL_FAILED,
            APP_INSTALL_ERROR,
            APP_FINALIZE_INSTALL_FAILED,
            APP_DELETE_FAILED,
            APP_LAUNCH_FAILED,
            APP_TITLE_COUNT_FAILED,
            APP_TITLE_ID_LIST_FAILED,
            APP_TITLE_INFO_FAILED,
            APP_OPEN_ARCHIVE_FAILED,
            APP_GET_DEVICE_ID_FAILED
        } AppResult;

        AppResult getDeviceId(u32* deviceId);

        AppResult ciaInfo(App* app, const std::string file, ctr::fs::MediaType mediaType);
        AppResult isInstalled(bool* result, App app);
        AppResult list(std::vector<ctr::app::App>* apps, ctr::fs::MediaType mediaType);
        AppResult install(ctr::fs::MediaType mediaType, FILE* fd, u64 size, std::function<bool(u64 pos, u64 totalSize)> onProgress);
        AppResult uninstall(App app);
        AppResult launch(App app);

        const std::string resultString(AppResult result);
        const std::string platformString(AppPlatform platform);
        const std::string categoryString(AppCategory category);
    }
}