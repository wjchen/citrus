#include "citrus/news.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace news {
        static bool initialized = false;
        static ctr::err::Error initError = {};
    }
}

bool ctr::news::init() {
    ctr::err::parse(ctr::err::SOURCE_NEWS_INIT, (u32) newsInit());
    initialized = !ctr::err::has();
    if(!initialized) {
        initError = ctr::err::get();
        ctr::err::set(initError);
    }

    return initialized;
}

void ctr::news::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    initError = {};

    newsExit();
}

void ctr::news::add(std::u16string title, std::u16string message, void* image, u32 imageSize, bool jpeg) {
    if(!initialized) {
        ctr::err::set(initError);
        return;
    }

    ctr::err::parse(ctr::err::SOURCE_NEWS_ADD_NOTIFICATION, (u32) NEWS_AddNotification((const u16*) title.c_str(), title.length(), (const u16*) message.c_str(), message.length(), image, imageSize, jpeg));
}