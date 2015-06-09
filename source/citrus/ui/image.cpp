#include "citrus/ui.hpp"

ctr::ui::Image::~Image() {
    if(this->vbo != 0) {
        ctr::gpu::freeVbo(this->vbo);
        this->vbo = 0;
    }
}

void ctr::ui::Image::setTexture(u32 texture) {
    this->texture = texture;
}

void ctr::ui::Image::setSubImage(float subX, float subY, float subWidth, float subHeight) {
    this->vboSubX = subX;
    this->vboSubY = subY;
    this->vboSubWidth = subWidth;
    this->vboSubHeight = subHeight;
    this->vboDirty = true;
}

void ctr::ui::Image::setColor(float r, float g, float b, float a) {
    this->vboR = r;
    this->vboG = g;
    this->vboB = b;
    this->vboA = a;
    this->vboDirty = true;
}

void ctr::ui::Image::sizeChanged() {
    this->vboDirty = true;
}

void ctr::ui::Image::draw() {
    if(this->vboDirty) {
        this->vboDirty = false;

        if(this->vbo == 0) {
            ctr::gpu::createVbo(&this->vbo);
            ctr::gpu::setVboAttributes(this->vbo, ATTRIBUTE(0, 3, ctr::gpu::ATTR_FLOAT) | ATTRIBUTE(1, 2, ctr::gpu::ATTR_FLOAT) | ATTRIBUTE(2, 4, ctr::gpu::ATTR_FLOAT), 3);
        }

        const float vboData[] = {
                0, 0, -0.1f, this->vboSubX, this->vboSubY, this->vboR, this->vboG, this->vboB, this->vboA,
                (float) this->getWidth(), 0, -0.1f, this->vboSubX + this->vboSubWidth, this->vboSubY, this->vboR, this->vboG, this->vboB, this->vboA,
                (float) this->getWidth(), (float) this->getHeight(), -0.1f, this->vboSubX + this->vboSubWidth, this->vboSubY + this->vboSubHeight, this->vboR, this->vboG, this->vboB, this->vboA,
                (float) this->getWidth(), (float) this->getHeight(), -0.1f, this->vboSubX + this->vboSubWidth, this->vboSubY + this->vboSubHeight, this->vboR, this->vboG, this->vboB, this->vboA,
                0, (float) this->getHeight(), -0.1f, this->vboSubX, this->vboSubY + this->vboSubHeight, this->vboR, this->vboG, this->vboB, this->vboA,
                0, 0, -0.1f, this->vboSubX, this->vboSubY, this->vboR, this->vboG, this->vboB, this->vboA
        };

        ctr::gpu::setVboData(this->vbo, vboData, 6, ctr::gpu::PRIM_TRIANGLES);
    }

    if(this->texture != 0 && this->vbo != 0) {
        ctr::gpu::bindTexture(ctr::gpu::TEXUNIT0, this->texture);
        ctr::gpu::drawVbo(this->vbo);
        ctr::gpu::bindTexture(ctr::gpu::TEXUNIT0, 0);
    }
}