#include "citrus/fs.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

bool ctr::fs::init() {
    return true;
}

void ctr::fs::exit() {
}

u64 ctr::fs::freeSpace(MediaType mediaType) {
    u32 clusterSize;
    u32 freeClusters;
    if(mediaType == NAND) {
        ctr::err::parse((u32) FSUSER_GetNandArchiveResource(NULL, NULL, &clusterSize, NULL, &freeClusters));
    } else {
        ctr::err::parse((u32) FSUSER_GetSdmcArchiveResource(NULL, NULL, &clusterSize, NULL, &freeClusters));
    }

    if(ctr::err::has()) {
        return 0;
    }

    return (u64) clusterSize * (u64) freeClusters;
}