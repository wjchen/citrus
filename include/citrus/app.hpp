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

        u32 deviceId();

        App ciaInfo(const std::string file, ctr::fs::MediaType mediaType);
        bool installed(App app);
        std::vector<ctr::app::App> list(ctr::fs::MediaType mediaType);
        void install(ctr::fs::MediaType mediaType, FILE* fd, u64 size, std::function<bool(u64 pos, u64 totalSize)> onProgress);
        void uninstall(App app);
        void launch(App app);

        const std::string platformString(AppPlatform platform);
        const std::string categoryString(AppCategory category);
    }
}