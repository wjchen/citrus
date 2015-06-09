#pragma once

#include "citrus/types.hpp"
#include "citrus/gpu.hpp"

#include <map>

namespace ctr {
    namespace ui {
        class Font {
        public:
            Font(void* pixels, u32 width, u32 height, u32 charWidth, u32 charHeight, ctr::gpu::PixelFormat format, u32 params);
            ~Font();

            u32 getTexture();

            u32 getWidth();
            u32 getHeight();

            u32 getCharWidth();
            u32 getCharHeight();

            u32 getStringWidth(const std::string str);
            u32 getStringHeight(const std::string str);
        private:
            u32 texture = 0;

            u32 width = 0;
            u32 height = 0;
            u32 charWidth = 0;
            u32 charHeight = 0;
        };

        class Component {
        public:
            virtual ~Component() {}

            int getX();
            int getY();

            u32 getWidth();
            u32 getHeight();

            bool isVisible();

            void setX(int x);
            void setY(int y);
            void setPosition(int x, int y);

            void setWidth(u32 width);
            void setHeight(u32 height);
            void setSize(u32 width, u32 height);

            void setDimensions(int x, int y, u32 width, u32 height);

            void setVisible(bool visible);

            std::map<const std::string, Component*> getChildren();
            Component* getChild(const std::string name);
            ctr::ui::Component* addChild(const std::string name, Component* component);
            ctr::ui::Component* removeChild(const std::string name);
            void clearChildren();

            void onAdded();
            void onRemoved();
            void onUpdate();
            void onDraw(int parentX, int parentY);
        protected:
            virtual void added() {}
            virtual void removed() {}
            virtual void sizeChanged() {}
            virtual void update() {}
            virtual void draw() {}
        private:
            int x = 0;
            int y = 0;
            u32 width = 0;
            u32 height = 0;
            bool visible = true;
            std::map<const std::string, Component*> children;
        };

        class Application {
        public:
            ~Application();

            Component* getTopScreen();
            void setTopScreen(Component* component);

            Component* getBottomScreen();
            void setBottomScreen(Component* component);

            void start();
        private:
            Component* topScreen = NULL;
            Component* bottomScreen = NULL;
        };

        class Image : public Component {
        public:
            ~Image();

            void setTexture(u32 texture);
            void setSubImage(float subX, float subY, float subWidth, float subHeight);
            void setColor(float r, float g, float b, float a);
        protected:
            void sizeChanged();
            void draw();
        private:
            u32 texture = 0;
            u32 vbo = 0;

            bool vboDirty = true;
            float vboSubX = 0;
            float vboSubY = 0;
            float vboSubWidth = 1;
            float vboSubHeight = 1;
            float vboR = 1;
            float vboG = 1;
            float vboB = 1;
            float vboA = 1;
        };

        class Text : public Component {
        public:
            ~Text();

            void setFont(Font* font);
            void setText(const std::string text);
            void setColor(float r, float g, float b, float a);
        protected:
            void sizeChanged();
            void draw();
        private:
            u32 vbo = 0;

            bool vboDirty = false;
            Font* vboFont = NULL;
            std::string vboText = "";
            float vboR = 1;
            float vboG = 1;
            float vboB = 1;
            float vboA = 1;
        };

        Font* getDefaultFont();
    }
}