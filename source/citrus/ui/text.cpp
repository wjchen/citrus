#include "citrus/ui.hpp"

#include <cstring>

namespace ctr {
    namespace ui {
        extern Font* defaultFont;
    }
}

ctr::ui::Text::~Text() {
    if(this->vbo != 0) {
        ctr::gpu::freeVbo(this->vbo);
        this->vbo = 0;
    }
}

void ctr::ui::Text::setFont(Font* font) {
    this->vboFont = font;
    this->vboDirty = true;
}

void ctr::ui::Text::setText(const std::string text) {
    this->vboText = text;
    this->vboDirty = true;
}

void ctr::ui::Text::setColor(float r, float g, float b, float a) {
    this->vboR = r;
    this->vboG = g;
    this->vboB = b;
    this->vboA = a;
    this->vboDirty = true;
}

void ctr::ui::Text::sizeChanged() {
    this->vboDirty = true;
}

void ctr::ui::Text::draw() {
    if(this->vboDirty) {
        this->vboDirty = false;

        u32 len = this->vboText.length();
        if(this->vboFont == NULL || len == 0) {
            if(this->vbo != 0) {
                ctr::gpu::freeVbo(this->vbo);
                this->vbo = 0;
            }
        } else {
            u32 lines = 1;
            for(u32 i = 0; i < len; i++) {
                if(this->vboText[i] == '\n') {
                    lines++;
                }
            }

            const u32 charWidth = this->getWidth() / len;
            const u32 charHeight = this->getHeight() / lines;

            const float texCharWidth = (float) this->vboFont->getCharWidth() / (float) this->vboFont->getWidth();
            const float texCharHeight = (float) this->vboFont->getCharHeight() / (float) this->vboFont->getHeight();
            const u32 texCharsX = this->vboFont->getWidth() / this->vboFont->getCharWidth();
            const u32 texCharsY = this->vboFont->getHeight() / this->vboFont->getCharHeight();

            if(this->vbo == 0) {
                ctr::gpu::createVbo(&this->vbo);
                ctr::gpu::setVboAttributes(this->vbo, ATTRIBUTE(0, 3, ctr::gpu::ATTR_FLOAT) | ATTRIBUTE(1, 2, ctr::gpu::ATTR_FLOAT) | ATTRIBUTE(2, 4, ctr::gpu::ATTR_FLOAT), 3);
            }

            float* tempVboData;
            ctr::gpu::setVboDataInfo(this->vbo, len * 6, ctr::gpu::PRIM_TRIANGLES);
            ctr::gpu::getVboData(this->vbo, (void**) &tempVboData);

            u32 cx = 0;
            u32 cy = this->getHeight() - charHeight;
            for(u32 i = 0; i < len; i++) {
                char c = this->vboText[i];
                if(c == '\n') {
                    std::memset(tempVboData + (i * 6 * 9), 0, 6 * 9 * sizeof(float));

                    cx = 0;
                    cy -= charHeight;
                    continue;
                }

                const float texX1 = (c % texCharsX) * texCharWidth;
                const float texY1 = 1.0f - ((c / texCharsY + 1) * texCharHeight);
                const float texX2 = texX1 + texCharWidth;
                const float texY2 = texY1 + texCharHeight;

                const float vboData[] = {
                        (float) cx, (float) cy, -0.1f, texX1, texY1, this->vboR, this->vboG, this->vboB, this->vboA,
                        (float) cx + charWidth, (float) cy, -0.1f, texX2, texY1, this->vboR, this->vboG, this->vboB, this->vboA,
                        (float) cx + charWidth, (float) cy + charHeight, -0.1f, texX2, texY2, this->vboR, this->vboG, this->vboB, this->vboA,
                        (float) cx + charWidth, (float) cy + charHeight, -0.1f, texX2, texY2, this->vboR, this->vboG, this->vboB, this->vboA,
                        (float) cx, (float) cy + charHeight, -0.1f, texX1, texY2, this->vboR, this->vboG, this->vboB, this->vboA,
                        (float) cx, (float) cy, -0.1f, texX1, texY1, this->vboR, this->vboG, this->vboB, this->vboA
                };

                std::memcpy(tempVboData + (i * 6 * 9), vboData, sizeof(vboData));
                cx += charWidth;
            }
        }
    }

    if(this->vbo != 0) {
        ctr::gpu::bindTexture(ctr::gpu::TEXUNIT0, this->vboFont->getTexture());
        ctr::gpu::drawVbo(this->vbo);
        ctr::gpu::bindTexture(ctr::gpu::TEXUNIT0, 0);
    }
}