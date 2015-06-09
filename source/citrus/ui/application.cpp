#include "citrus/ui.hpp"
#include "citrus/core.hpp"
#include "citrus/gput.hpp"
#include "citrus/hid.hpp"

ctr::ui::Application::~Application() {
    if(this->topScreen != NULL) {
        this->topScreen->onRemoved();
        this->topScreen = NULL;
    }

    if(this->bottomScreen != NULL) {
        this->bottomScreen->onRemoved();
        this->bottomScreen = NULL;
    }
}

ctr::ui::Component* ctr::ui::Application::getTopScreen() {
    return this->topScreen;
}

void ctr::ui::Application::setTopScreen(ctr::ui::Component* component) {
    if(this->topScreen != NULL) {
        this->topScreen->onRemoved();
    }

    this->topScreen = component;
    if(this->topScreen != NULL) {
        this->topScreen->onAdded();
    }
}

ctr::ui::Component* ctr::ui::Application::getBottomScreen() {
    return this->bottomScreen;
}

void ctr::ui::Application::setBottomScreen(ctr::ui::Component* component) {
    if(this->bottomScreen != NULL) {
        this->bottomScreen->onRemoved();
    }

    this->bottomScreen = component;
    if(this->bottomScreen != NULL) {
        this->bottomScreen->onAdded();
    }
}

void ctr::ui::Application::start() {
    while(ctr::core::running() && (this->topScreen != NULL || this->bottomScreen != NULL)) {
        ctr::hid::poll();

        if(this->topScreen != NULL) {
            this->topScreen->onUpdate();
        }

        if(this->bottomScreen != NULL) {
            this->bottomScreen->onUpdate();
        }

        if(this->topScreen != NULL) {
            ctr::gpu::setViewport(gpu::SCREEN_TOP, 0, 0, TOP_WIDTH, TOP_HEIGHT);
            ctr::gput::setOrtho(0, TOP_WIDTH, 0, TOP_HEIGHT, -1, 1);
            ctr::gpu::clear();

            this->topScreen->onDraw(0, 0);

            ctr::gpu::flushCommands();
            ctr::gpu::flushBuffer();
        }

        if(this->bottomScreen != NULL) {
            ctr::gpu::setViewport(gpu::SCREEN_BOTTOM, 0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT);
            ctr::gput::setOrtho(0, BOTTOM_WIDTH, 0, BOTTOM_HEIGHT, -1, 1);
            ctr::gpu::clear();

            this->bottomScreen->onDraw(0, 0);

            ctr::gpu::flushCommands();
            ctr::gpu::flushBuffer();
        }

        ctr::gpu::swapBuffers(true);
    }
}