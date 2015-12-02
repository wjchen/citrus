#include "citrus/utf.hpp"
#include "internal.hpp"

#include <3ds.h>

bool ctr::utf::init() {
    return true;
}

void ctr::utf::exit() {
}

const std::u16string ctr::utf::utf8to16(const std::string utf8) {
    char16_t buf[utf8.length()];
    ssize_t units = utf8_to_utf16((u16*) buf, (u8*) utf8.c_str(), utf8.length());
    if(units == -1) {
        return u"";
    }

    return std::u16string(buf, (size_t) units);
}

const std::u32string ctr::utf::utf8to32(const std::string utf8) {
    char32_t buf[utf8.length()];
    ssize_t units = utf8_to_utf32((u32*) buf, (u8*) utf8.c_str(), utf8.length());
    if(units == -1) {
        return U"";
    }

    return std::u32string(buf, (size_t) units);
}

const std::string ctr::utf::utf16to8(const std::u16string utf16) {
    char buf[utf16.length()];
    ssize_t units = utf16_to_utf8((u8*) buf, (u16*) utf16.c_str(), utf16.length());
    if(units == -1) {
        return "";
    }

    return std::string(buf, (size_t) units);
}

const std::u32string ctr::utf::utf16to32(const std::u16string utf16) {
    char32_t buf[utf16.length()];
    ssize_t units = utf16_to_utf32((u32*) buf, (u16*) utf16.c_str(), utf16.length());
    if(units == -1) {
        return U"";
    }

    return std::u32string(buf, (size_t) units);
}

const std::string ctr::utf::utf32to8(const std::u32string utf32) {
    char buf[utf32.length()];
    ssize_t units = utf32_to_utf8((u8*) buf, (u32*) utf32.c_str(), utf32.length());
    if(units == -1) {
        return "";
    }

    return std::string(buf, (size_t) units);
}

const std::u16string ctr::utf::utf32to16(const std::u32string utf32) {
    char16_t buf[utf32.length()];
    ssize_t units = utf32_to_utf16((u16*) buf, (u32*) utf32.c_str(), utf32.length());
    if(units == -1) {
        return u"";
    }

    return std::u16string(buf, (size_t) units);
}