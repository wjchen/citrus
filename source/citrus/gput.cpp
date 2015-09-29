#include "citrus/gput.hpp"
#include "citrus/gpu.hpp"
#include "internal.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <stack>

#include <3ds.h>

#include "citrus_default_font_bin.h"
#include "citrus_default_shader_shbin.h"

using namespace ctr;

namespace ctr {
    namespace gput {
        static u32 defaultShader = 0;

        static u32 stringVbo = 0;

        static u32 fontTexture = 0;
        static u32 fontWidth = 0;
        static u32 fontHeight = 0;
        static u32 fontCharWidth = 0;
        static u32 fontCharHeight = 0;

        static float projection[16] = {0};
        static float modelview[16] = {0};

        static std::stack<float*> projectionStack;
        static std::stack<float*> modelviewStack;
    }
}

bool ctr::gput::init() {
    gpu::createShader(&defaultShader);
    gpu::loadShader(defaultShader, citrus_default_shader_shbin, citrus_default_shader_shbin_size);
    useDefaultShader();

    gpu::createVbo(&stringVbo);
    gpu::setVboAttributes(stringVbo, gpu::vboAttribute(0, 3, gpu::ATTR_FLOAT) | gpu::vboAttribute(1, 2, gpu::ATTR_FLOAT) | gpu::vboAttribute(2, 4, gpu::ATTR_FLOAT), 3);

    gpu::createTexture(&fontTexture);

    void* gpuFont = gpu::galloc(citrus_default_font_bin_size);
    std::memcpy(gpuFont, citrus_default_font_bin, citrus_default_font_bin_size);
    setFont(gpuFont, 128, 128, 8, 8, gpu::PIXEL_RGBA8);
    gpu::gfree(gpuFont);

    float identity[16];
    setIdentityMatrix(identity);
    setProjection(identity);
    setModelView(identity);

    return true;
}

void ctr::gput::exit() {
    if(defaultShader != 0) {
        gpu::freeShader(defaultShader);
        defaultShader = 0;
    }

    if(stringVbo != 0) {
        gpu::freeVbo(stringVbo);
        stringVbo = 0;
    }

    if(fontTexture != 0) {
        gpu::freeTexture(fontTexture);
        fontTexture = 0;
    }
}

void ctr::gput::useDefaultShader() {
    gpu::useShader(defaultShader);
}

void ctr::gput::multMatrix(float* out, const float* m1, const float* m2) {
    if(out == NULL || m1 == NULL || m2 == NULL) {
        return;
    }

    for(u32 x1 = 0; x1 < 4; x1++) {
        for(u32 y2 = 0; y2 < 4; y2++) {
            out[y2 * 4 + x1] = 0;
            for(u32 y1 = 0; y1 < 4; y1++) {
                out[y2 * 4 + x1] += m1[y1 * 4 + x1] * m2[y2 * 4 + y1];
            }
        }
    }
}

void ctr::gput::setIdentityMatrix(float* out) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));
    out[0] = 1.0f;
    out[5] = 1.0f;
    out[10] = 1.0f;
    out[15] = 1.0f;
}

void ctr::gput::setOrthoMatrix(float* out, float left, float right, float bottom, float top, float near, float far) {
    float orthoMatrix[16];

    orthoMatrix[0] = 2.0f / (right - left);
    orthoMatrix[1] = 0.0f;
    orthoMatrix[2] = 0.0f;
    orthoMatrix[3] = -((right + left) / (right - left));

    orthoMatrix[4] = 0.0f;
    orthoMatrix[5] = 2.0f / (top - bottom);
    orthoMatrix[6] = 0.0f;
    orthoMatrix[7] = -((top + bottom) / (top - bottom));

    orthoMatrix[8] = 0.0f;
    orthoMatrix[9] = 0.0f;
    orthoMatrix[10] = 2.0f / (far - near);
    orthoMatrix[11] = -((far + near) / (far - near));

    orthoMatrix[12] = 0.0f;
    orthoMatrix[13] = 0.0f;
    orthoMatrix[14] = 0.0f;
    orthoMatrix[15] = 1.0f;

    float correction[16];
    setRotationMatrixZ(correction, (float) M_PI / 2.0f);

    multMatrix(out, orthoMatrix, correction);
}

void ctr::gput::setPerspectiveMatrix(float* out, float fovy, float aspect, float near, float far) {
    float top = near * std::tan(fovy / 2);
    float right = top * aspect;

    float projectionMatrix[16];

    projectionMatrix[0] = near / right;
    projectionMatrix[1] = 0.0f;
    projectionMatrix[2] = 0.0f;
    projectionMatrix[3] = 0.0f;

    projectionMatrix[4] = 0.0f;
    projectionMatrix[5] = near / top;
    projectionMatrix[6] = 0.0f;
    projectionMatrix[7] = 0.0f;

    projectionMatrix[8] = 0.0f;
    projectionMatrix[9] = 0.0f;
    projectionMatrix[10] = -(far + near) / (far - near);
    projectionMatrix[11] = -2.0f * (far * near) / (far - near);

    projectionMatrix[12] = 0.0f;
    projectionMatrix[13] = 0.0f;
    projectionMatrix[14] = -1.0f;
    projectionMatrix[15] = 0.0f;

    float correction[16];
    setIdentityMatrix(correction);
    correction[10] = 0.5f;
    correction[11] = -0.5f;

    multMatrix(out, correction, projectionMatrix);
}

void ctr::gput::setTranslationMatrix(float* out, float x, float y, float z) {
    if(out == NULL) {
        return;
    }

    setIdentityMatrix(out);
    out[3] = x;
    out[7] = y;
    out[11] = z;
}

void ctr::gput::setRotationMatrixX(float* out, float rotation) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));

    out[0] = 1.0f;
    out[5] = std::cos(rotation);
    out[6] = std::sin(rotation);
    out[9] = -std::sin(rotation);
    out[10] = std::cos(rotation);
    out[15] = 1.0f;
}

void ctr::gput::setRotationMatrixY(float* out, float rotation) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));

    out[0] = std::cos(rotation);
    out[2] = std::sin(rotation);
    out[5] = 1.0f;
    out[8] = -std::sin(rotation);
    out[10] = std::cos(rotation);
    out[15] = 1.0f;
}

void ctr::gput::setRotationMatrixZ(float* out, float rotation) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));

    out[0] = std::cos(rotation);
    out[1] = std::sin(rotation);
    out[4] = -std::sin(rotation);
    out[5] = std::cos(rotation);
    out[10] = 1.0f;
    out[15] = 1.0f;
}

void ctr::gput::setScaleMatrix(float* matrix, float x, float y, float z) {
    matrix[0] *= x;
    matrix[4] *= x;
    matrix[8] *= x;
    matrix[12] *= x;

    matrix[1] *= y;
    matrix[5] *= y;
    matrix[9] *= y;
    matrix[13] *= y;

    matrix[2] *= z;
    matrix[6] *= z;
    matrix[10] *= z;
    matrix[14] *= z;
}

void ctr::gput::pushProjection() {
    float* old = new float[16];
    std::memcpy(old, projection, 16 * sizeof(float));
    projectionStack.push(old);
}

void ctr::gput::popProjection() {
    if(projectionStack.empty()) {
        return;
    }

    float* old = projectionStack.top();
    projectionStack.pop();
    setProjection(old);
    delete old;
}

float* ctr::gput::getProjection() {
    return projection;
}

void ctr::gput::setProjection(float* matrix) {
    if(matrix == NULL) {
        return;
    }

    std::memcpy(projection, matrix, 16 * sizeof(float));
    gpu::setUniform(defaultShader, gpu::SHADER_VERTEX, "projection", projection, 4);
}

void ctr::gput::setOrtho(float left, float right, float bottom, float top, float near, float far) {
    float orthoMatrix[16];
    setOrthoMatrix(orthoMatrix, left, right, bottom, top, near, far);
    setProjection(orthoMatrix);
}

void ctr::gput::setPerspective(float fovy, float aspect, float near, float far) {
    float perspectiveMatrix[16];
    setPerspectiveMatrix(perspectiveMatrix, fovy, aspect, near, far);
    setProjection(perspectiveMatrix);
}

void ctr::gput::pushModelView() {
    float* old = new float[16];
    memcpy(old, modelview, 16 * sizeof(float));
    modelviewStack.push(old);
}

void ctr::gput::popModelView() {
    if(modelviewStack.empty()) {
        return;
    }

    float* old = modelviewStack.top();
    modelviewStack.pop();
    setModelView(old);
    delete old;
}

float* ctr::gput::getModelView() {
    return modelview;
}

void ctr::gput::setModelView(float* matrix) {
    if(matrix == NULL) {
        return;
    }

    memcpy(modelview, matrix, 16 * sizeof(float));
    gpu::setUniform(defaultShader, gpu::SHADER_VERTEX, "modelview", modelview, 4);
}

void ctr::gput::translate(float x, float y, float z) {
    float translationMatrix[16];
    setTranslationMatrix(translationMatrix, x, y, z);

    float resultMatrix[16];
    multMatrix(resultMatrix, modelview, translationMatrix);
    setModelView(resultMatrix);
}

void ctr::gput::rotateX(float rotation) {
    float rotationMatrix[16];
    setRotationMatrixX(rotationMatrix, rotation);

    float resultMatrix[16];
    multMatrix(resultMatrix, modelview, rotationMatrix);
    setModelView(resultMatrix);
}

void ctr::gput::rotateY(float rotation) {
    float rotationMatrix[16];
    setRotationMatrixY(rotationMatrix, rotation);

    float resultMatrix[16];
    multMatrix(resultMatrix, modelview, rotationMatrix);
    setModelView(resultMatrix);
}

void ctr::gput::rotateZ(float rotation) {
    float rotationMatrix[16];
    setRotationMatrixZ(rotationMatrix, rotation);

    float resultMatrix[16];
    multMatrix(resultMatrix, modelview, rotationMatrix);
    setModelView(resultMatrix);
}

void ctr::gput::rotate(float x, float y, float z) {
    float tempMatrix[16];
    float tempMatrix2[16];
    float tempMatrix3[16];

    setRotationMatrixX(tempMatrix, x);
    setRotationMatrixY(tempMatrix2, y);
    multMatrix(tempMatrix3, tempMatrix, tempMatrix2);

    setRotationMatrixZ(tempMatrix2, z);
    multMatrix(tempMatrix, tempMatrix3, tempMatrix2);

    multMatrix(tempMatrix2, modelview, tempMatrix);
    setModelView(tempMatrix2);
}

void ctr::gput::scale(float x, float y, float z) {
    setScaleMatrix(modelview, x, y, z);
    setModelView(modelview);
}

void ctr::gput::setFont(void* image, u32 width, u32 height, u32 charWidth, u32 charHeight, gpu::PixelFormat format) {
    gpu::setTextureData(fontTexture, image, width, height, format, gpu::textureMinFilter(gpu::FILTER_NEAREST) | gpu::textureMagFilter(gpu::FILTER_NEAREST));
    fontWidth = width;
    fontHeight = height;
    fontCharWidth = charWidth;
    fontCharHeight = charHeight;
}

float ctr::gput::getStringWidth(const std::string str, float charWidth) {
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

    return (int) (longestLine * charWidth);
}

float ctr::gput::getStringHeight(const std::string str, float charHeight) {
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

    return (int) (lines * charHeight);
}

void ctr::gput::drawString(const std::string str, float x, float y, float charWidth, float charHeight, u8 red, u8 green, u8 blue, u8 alpha) {
    const u32 len = str.length();
    if(len == 0) {
        return;
    }

    static const float texCharWidth = (float) fontCharWidth / (float) fontWidth;
    static const float texCharHeight = (float) fontCharHeight / (float) fontHeight;
    static u32 charsX = fontWidth / fontCharWidth;
    static u32 charsY = fontHeight / fontCharHeight;

    const float r = (float) red / 255.0f;
    const float g = (float) green / 255.0f;
    const float b = (float) blue / 255.0f;
    const float a = (float) alpha / 255.0f;

    float* tempVboData;
    gpu::setVboDataInfo(stringVbo, len * 6, gpu::PRIM_TRIANGLES);
    gpu::getVboData(stringVbo, (void**) &tempVboData);

    float cx = x;
    float cy = y + getStringHeight(str, charHeight) - charHeight;
    for(u32 i = 0; i < len; i++) {
        char c = str[i];
        if(c == '\n') {
            std::memset(tempVboData + (i * 6 * 9), 0, 6 * 9 * sizeof(float));

            cx = x;
            cy -= charHeight;
            continue;
        }

        const float texX1 = (c % charsX) * texCharWidth;
        const float texY1 = 1.0f - ((c / charsY + 1) * texCharHeight);
        const float texX2 = texX1 + texCharWidth;
        const float texY2 = texY1 + texCharHeight;

        const float vboData[] = {
                cx, cy, -0.1f, texX1, texY1, r, g, b, a,
                cx + charWidth, cy, -0.1f, texX2, texY1, r, g, b, a,
                cx + charWidth, cy + charHeight, -0.1f, texX2, texY2, r, g, b, a,
                cx + charWidth, cy + charHeight, -0.1f, texX2, texY2, r, g, b, a,
                cx, cy + charHeight, -0.1f, texX1, texY2, r, g, b, a,
                cx, cy, -0.1f, texX1, texY1, r, g, b, a
        };

        std::memcpy(tempVboData + (i * 6 * 9), vboData, sizeof(vboData));
        cx += charWidth;
    }

    gpu::bindTexture(gpu::TEXUNIT0, fontTexture);
    gpu::drawVbo(stringVbo);

    // Flush the GPU command buffer so we can safely reuse the VBO.
    gpu::flushCommands();
}

void ctr::gput::takeScreenshot(bool top, bool bottom) {
    if(!top && !bottom) {
        return;
    }

    u32 width = top ? gpu::TOP_WIDTH : gpu::BOTTOM_WIDTH;
    u32 height = top && bottom ? gpu::TOP_HEIGHT + gpu::BOTTOM_HEIGHT : top ? gpu::TOP_HEIGHT : gpu::BOTTOM_HEIGHT;

    u32 headerSize = 0x36;
    u32 imageSize = width * height * 3;

    u8* header = new u8[headerSize]();

    *(u16*) &header[0x0] = 0x4D42;
    *(u32*) &header[0x2] = headerSize + imageSize;
    *(u32*) &header[0xA] = headerSize;
    *(u32*) &header[0xE] = 0x28;
    *(u32*) &header[0x12] = width;
    *(u32*) &header[0x16] = height;
    *(u32*) &header[0x1A] = 0x00180001;
    *(u32*) &header[0x22] = imageSize;

    u8* image = new u8[imageSize]();

    if(top && gfxGetScreenFormat(GFX_TOP) == GSP_BGR8_OES) {
        u8* topFb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        u32 xMod = (width - gpu::TOP_WIDTH) / 2;
        u32 yMod = 0;
        for(u32 x = 0; x < gpu::TOP_WIDTH; x++) {
            for(u32 y = 0; y < gpu::TOP_HEIGHT; y++) {
                u8* src = &topFb[((gpu::TOP_HEIGHT - 1 - y) + x * gpu::TOP_HEIGHT) * 3];
                u8* dst = &image[((height - 1 - (y + yMod)) * width + (x + xMod)) * 3];

                *(u16*) dst = *(u16*) src;
                dst[2] = src[2];
            }
        }
    }

    if(bottom && gfxGetScreenFormat(GFX_BOTTOM) == GSP_BGR8_OES) {
        u8* bottomFb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        u32 xMod = (width - gpu::BOTTOM_WIDTH) / 2;
        u32 yMod = top ? gpu::TOP_HEIGHT : 0;
        for(u32 x = 0; x < gpu::BOTTOM_WIDTH; x++) {
            for(u32 y = 0; y < gpu::BOTTOM_HEIGHT; y++) {
                u8* src = &bottomFb[((gpu::BOTTOM_HEIGHT - 1 - y) + x * gpu::BOTTOM_HEIGHT) * 3];
                u8* dst = &image[((height - 1 - (y + yMod)) * width + (x + xMod)) * 3];

                *(u16*) dst = *(u16*) src;
                dst[2] = src[2];
            }
        }
    }

    std::stringstream fileStream;
    fileStream << "/screenshot_" << time(NULL) << ".bmp";

    FILE* fd = fopen(fileStream.str().c_str(), "wb");
    if(fd) {
        fwrite(header, 1, headerSize, fd);
        fwrite(image, 1, imageSize, fd);
        fclose(fd);
    }

    delete header;
    delete image;
}