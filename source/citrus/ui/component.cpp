#include "citrus/ui.hpp"
#include "citrus/gput.hpp"

int ctr::ui::Component::getX() {
    return this->x;
}

int ctr::ui::Component::getY() {
    return this->y;
}

u32 ctr::ui::Component::getWidth() {
    return this->width;
}

u32 ctr::ui::Component::getHeight() {
    return this->height;
}

bool ctr::ui::Component::isVisible() {
    return this->visible;
}

void ctr::ui::Component::setX(int x) {
    this->x = x;
}

void ctr::ui::Component::setY(int y) {
    this->y = y;
}

void ctr::ui::Component::setPosition(int x, int y) {
    this->x = x;
    this->y = y;
}

void ctr::ui::Component::setWidth(u32 width) {
    this->width = width;
    this->sizeChanged();
}

void ctr::ui::Component::setHeight(u32 height) {
    this->height = height;
    this->sizeChanged();
}

void ctr::ui::Component::setSize(u32 width, u32 height) {
    this->width = width;
    this->height = height;
    this->sizeChanged();
}

void ctr::ui::Component::setDimensions(int x, int y, u32 width, u32 height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->sizeChanged();
}

void ctr::ui::Component::setVisible(bool visible) {
    this->visible = visible;
}

std::map<const std::string, ctr::ui::Component*> ctr::ui::Component::getChildren() {
    return this->children;
}

ctr::ui::Component* ctr::ui::Component::getChild(const std::string name) {
    return this->children[name];
}

ctr::ui::Component* ctr::ui::Component::addChild(const std::string name, Component* component) {
    ctr::ui::Component* curr = this->children[name];
    if(curr != NULL) {
        curr->onRemoved();
    }

    this->children[name] = component;
    if(component != NULL) {
        component->onAdded();
    }

    return curr;
}

ctr::ui::Component* ctr::ui::Component::removeChild(const std::string name) {
    ctr::ui::Component* curr = this->children[name];
    if(curr != NULL) {
        curr->onRemoved();
    }

    this->children.erase(name);
    return curr;
}

void ctr::ui::Component::clearChildren() {
    for(std::map<const std::string, Component*>::iterator it = this->children.begin(); it != this->children.end(); it++) {
        it->second->onRemoved();
    }

    this->children.clear();
}

void ctr::ui::Component::onAdded() {
    this->clearChildren();
    this->added();
}

void ctr::ui::Component::onRemoved() {
    this->removed();
    this->clearChildren();
}

void ctr::ui::Component::onUpdate() {
    if(this->isVisible()) {
        this->update();

        for(std::map<const std::string, Component*>::iterator it = this->children.begin(); it != this->children.end(); it++) {
            it->second->onUpdate();
        }
    }
}

void ctr::ui::Component::onDraw(int parentX, int parentY) {
    if(this->isVisible()) {
        int baseX = parentX + this->getX();
        int baseY = parentY + this->getY();

        ctr::gput::pushModelView();
        ctr::gput::translate(this->getX(), this->getY(), 0);
        ctr::gpu::setScissorTest(ctr::gpu::SCISSOR_NORMAL, baseX, baseY, this->getWidth(), this->getHeight());

        this->draw();

        for(std::map<const std::string, Component*>::iterator it = this->children.begin(); it != this->children.end(); it++) {
            it->second->onDraw(baseX, baseY);
        }

        ctr::gput::popModelView();
    }
}