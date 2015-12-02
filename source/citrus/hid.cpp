#include "citrus/hid.hpp"
#include "internal.hpp"

#include <unordered_map>

#include <3ds.h>

namespace ctr {
    namespace hid {
        static std::unordered_map<u32, const std::string> buttonNames = {
                {BUTTON_NONE, "None"},
                {BUTTON_A, "A"},
                {BUTTON_B, "B"},
                {BUTTON_SELECT, "Select"},
                {BUTTON_START, "Start"},
                {BUTTON_DRIGHT, "D-Pad Right"},
                {BUTTON_DLEFT, "D-Pad Left"},
                {BUTTON_DUP, "D-Pad Up"},
                {BUTTON_DDOWN, "D-Pad Down"},
                {BUTTON_R, "R"},
                {BUTTON_L, "L"},
                {BUTTON_X, "X"},
                {BUTTON_Y, "Y"},
                {BUTTON_ZL, "ZL"},
                {BUTTON_ZR, "ZR"},
                {BUTTON_TOUCH, "Touch"},
                {BUTTON_CSTICK_RIGHT, "C-Stick Right"},
                {BUTTON_CSTICK_LEFT, "C-Stick Left"},
                {BUTTON_CSTICK_UP, "C-Stick Up"},
                {BUTTON_CSTICK_DOWN, "C-Stick Down"},
                {BUTTON_CPAD_RIGHT, "Circle-Pad Right"},
                {BUTTON_CPAD_LEFT, "Circle-Pad Left"},
                {BUTTON_CPAD_UP, "Circle-Pad Up"},
                {BUTTON_CPAD_DOWN, "Circle-Pad Down"},
                {BUTTON_UP, "Up"},
                {BUTTON_DOWN, "Down"},
                {BUTTON_LEFT, "Left"},
                {BUTTON_RIGHT, "Right"}
        };
    }
}

bool ctr::hid::init() {
    return true;
}

void ctr::hid::exit() {
}

void ctr::hid::poll() {
    hidScanInput();
}

ctr::hid::Button ctr::hid::anyPressed() {
    for(u32 i = 0; i < 32; i++) {
        Button button = (ctr::hid::Button) (1 << i);
        if(pressed(button)) {
            return button;
        }
    }

    return BUTTON_NONE;
}

bool ctr::hid::released(ctr::hid::Button button) {
    return (hidKeysUp() & button) != 0;
}

bool ctr::hid::pressed(ctr::hid::Button button) {
    return (hidKeysDown() & button) != 0;
}

bool ctr::hid::held(ctr::hid::Button button) {
    return (hidKeysHeld() & button) != 0;
}

ctr::hid::Touch ctr::hid::touch() {
    touchPosition pos;
    hidTouchRead(&pos);
    return {pos.px, pos.py};
}

ctr::hid::Analog ctr::hid::circlePad() {
    circlePosition pos;
    hidCircleRead(&pos);
    return {pos.dx, pos.dy};
}

ctr::hid::Analog ctr::hid::cStick() {
    circlePosition pos;
    irrstCstickRead(&pos);
    return {pos.dx, pos.dy};
}

void enableAccelerometer() {
    ctr::err::parse(ctr::err::SOURCE_HIDUSER_ENABLE_ACCELEROMETER, (u32) HIDUSER_EnableAccelerometer());
}

void disableAccelerometer() {
    ctr::err::parse(ctr::err::SOURCE_HIDUSER_DISABLE_ACCELEROMETER, (u32) HIDUSER_DisableAccelerometer());
}

ctr::hid::Axis ctr::hid::accelerometer() {
    accelVector vec;
    hidAccelRead(&vec);
    return {vec.x, vec.y, vec.z};
}

void enableGyroscope() {
    ctr::err::parse(ctr::err::SOURCE_HIDUSER_ENABLE_GYROSCOPE, (u32) HIDUSER_EnableGyroscope());
}

void disableGyroscope() {
    ctr::err::parse(ctr::err::SOURCE_HIDUSER_DISABLE_GYROSCOPE, (u32) HIDUSER_DisableGyroscope());
}

ctr::hid::Axis ctr::hid::gyroscope() {
    angularRate rate;
    hidGyroRead(&rate);
    return {rate.x, rate.y, rate.z};
}

const std::string ctr::hid::buttonName(ctr::hid::Button button) {
    std::unordered_map<u32, const std::string>::iterator result = buttonNames.find(button);
    return result != buttonNames.end() ? result->second : "<unknown>";
}