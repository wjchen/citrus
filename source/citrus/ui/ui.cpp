#include "citrus/ui.hpp"
#include "../internal.hpp"

#include <cstring>

#include "citrus_default_font_bin.h"

namespace ctr {
    namespace ui {
        Font* defaultFont = NULL;
    }
}

bool ctr::ui::init() {
    void* gpuFont = ctr::gpu::galloc(citrus_default_font_bin_size);
    std::memcpy(gpuFont, citrus_default_font_bin, citrus_default_font_bin_size);
    defaultFont = new Font(gpuFont, 128, 128, 8, 8, ctr::gpu::PIXEL_RGBA8, TEXTURE_MIN_FILTER(ctr::gpu::FILTER_NEAREST) | TEXTURE_MAG_FILTER(ctr::gpu::FILTER_NEAREST));
    ctr::gpu::gfree(gpuFont);

    return true;
}

void ctr::ui::exit() {
    if(defaultFont != NULL) {
        delete defaultFont;
        defaultFont = NULL;
    }
}

ctr::ui::Font* ctr::ui::getDefaultFont() {
    return defaultFont;
}