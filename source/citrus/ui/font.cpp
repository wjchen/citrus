#include "citrus/ui.hpp"

#include <cstring>

ctr::ui::Font::Font(void* pixels, u32 width, u32 height, u32 charWidth, u32 charHeight, ctr::gpu::PixelFormat format, u32 params) {
    ctr::gpu::createTexture(&this->texture);
    ctr::gpu::setTextureData(this->texture, pixels, width, height, format, params);

    this->width = width;
    this->height = height;
    this->charWidth = charWidth;
    this->charHeight = charHeight;
}

ctr::ui::Font::~Font() {
    if(this->texture != 0) {
        ctr::gpu::freeTexture(this->texture);
        this->texture = 0;
    }

    this->width = 0;
    this->height = 0;
    this->charWidth = 0;
    this->charHeight = 0;
}

u32 ctr::ui::Font::getTexture() {
    return this->texture;
}

u32 ctr::ui::Font::getWidth() {
    return this->width;
}

u32 ctr::ui::Font::getHeight() {
    return this->height;
}

u32 ctr::ui::Font::getCharWidth() {
    return this->charWidth;
}

u32 ctr::ui::Font::getCharHeight() {
    return this->charHeight;
}

u32 ctr::ui::Font::getStringWidth(const std::string str) {
    u32 len = str.length();
    if(len == 0) {
        return 0;
    }

    u32 longestLine = 0;
    u32 currLength = 0;
    for(u32 i = 0; i < len; i++) {
        if(str[i] == '\n') {
            if(currLength > longestLine) {
                longestLine = currLength;
            }

            currLength = 0;
            continue;
        }

        currLength++;
    }

    if(currLength > longestLine) {
        longestLine = currLength;
    }

    return longestLine * this->charWidth;
}

u32 ctr::ui::Font::getStringHeight(const std::string str) {
    u32 len = str.length();
    if(len == 0) {
        return 0;
    }

    u32 lines = 1;
    for(u32 i = 0; i < len; i++) {
        if(str[i] == '\n') {
            lines++;
        }
    }

    return lines * this->charHeight;
}