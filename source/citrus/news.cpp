#include "citrus/news.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace news {
        static bool initialized = false;
    }
}

bool ctr::news::init() {
    ctr::err::parse((u32) newsInit());
    initialized = !ctr::err::has();
    return initialized;
}

void ctr::news::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    newsExit();
}

bool ctr::news::add(std::u16string title, std::u16string message, void* image, u32 imageSize, bool jpeg) {
    if(!initialized) {
        return false;
    }

    ctr::err::parse((u32) NEWSU_AddNotification((const u16*) title.c_str(), title.length(), (const u16*) message.c_str(), message.length(), image, imageSize, jpeg));
    return !ctr::err::has();
}