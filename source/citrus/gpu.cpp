#include "citrus/gpu.hpp"
#include "internal.hpp"

#include <cstring>

#include <3ds.h>

extern Handle gspEvents[GSPEVENT_MAX];

namespace ctr {
    namespace gpu {
        typedef struct {
            DVLB_s* dvlb;
            shaderProgram_s program;
        } ShaderData;

        typedef struct {
            void* data;
            u32 size;
            u32 numVertices;
            u32 bytesPerVertex;
            Primitive primitive;

            void* indices;
            u32 indicesSize;

            u64 attributes;
            u8 attributeCount;
            u16 attributeMask;
            u64 attributePermutations;
        } VboData;

        typedef struct {
            void* data;
            u32 width;
            u32 height;
            u32 size;
            PixelFormat format;
            u32 params;
        } TextureData;

        typedef struct {
            u16 rgbSources;
            u16 alphaSources;
            u16 rgbOperands;
            u16 alphaOperands;
            CombineFunc rgbCombine;
            CombineFunc alphaCombine;
            u32 constantColor;
        } TexEnv;

        static u32 bytesPerAttrFormat[] = {
                1, // ATTR_BYTE
                1, // ATTR_UNSIGNED_BYTE
                2, // ATTR_SHORT
                4, // ATTR_FLOAT
        };

        static u32 nibblesPerPixelFormat[] = {
                8, // RGBA8
                6, // RGB8
                4, // RGBA5551
                4, // RGB565
                4, // RGBA4
                4, // LA8
                4, // HILO8
                2, // L8
                2, // A8
                2, // LA4
                1, // L4
                1, // A4
                1, // ETC1
                2, // ETC1A4
        };

        static PixelFormat fbFormatToGPU[] = {
                PIXEL_RGBA8,    // GSP_RGBA8_OES
                PIXEL_RGB8,     // GSP_BGR8_OES
                PIXEL_RGB565,   // GSP_RGB565_OES
                PIXEL_RGBA5551, // GSP_RGB5_A1_OES
                PIXEL_RGBA4     // GSP_RGBA4_OES
        };

        static const u32 GPU_COMMAND_BUFFER_SIZE = 0x80000;

        static const u32 TEX_ENV_COUNT = 6;
        static const u32 TEX_UNIT_COUNT = 3;

        static const u32 STATE_VIEWPORT = (1 << 0);
        static const u32 STATE_DEPTH_MAP = (1 << 1);
        static const u32 STATE_CULL = (1 << 2);
        static const u32 STATE_STENCIL_TEST = (1 << 3);
        static const u32 STATE_BLEND = (1 << 4);
        static const u32 STATE_ALPHA_TEST = (1 << 5);
        static const u32 STATE_DEPTH_TEST_AND_MASK = (1 << 6);
        static const u32 STATE_ACTIVE_SHADER = (1 << 7);
        static const u32 STATE_TEX_ENV = (1 << 8);
        static const u32 STATE_TEXTURES = (1 << 9);
        static const u32 STATE_SCISSOR_TEST = (1 << 10);

        static u32 dirtyState;
        static u32 dirtyTexEnvs;
        static u32 dirtyTextures;

        static u32 clearColor;
        static u32 clearDepth;

        static Screen viewportScreen;
        static u32 viewportX;
        static u32 viewportY;
        static u32 viewportWidth;
        static u32 viewportHeight;

        static ScissorMode scissorMode;
        static u32 scissorX;
        static u32 scissorY;
        static u32 scissorWidth;
        static u32 scissorHeight;

        static float depthNear;
        static float depthFar;

        static CullMode cullMode;

        static bool stencilEnable;
        static TestFunc stencilFunc;
        static u8 stencilRef;
        static u8 stencilMask;
        static u8 stencilReplace;
        static StencilOp stencilFail;
        static StencilOp stencilZFail;
        static StencilOp stencilZPass;

        static u8 blendRed;
        static u8 blendGreen;
        static u8 blendBlue;
        static u8 blendAlpha;
        static BlendEquation blendColorEquation;
        static BlendEquation blendAlphaEquation;
        static BlendFactor blendColorSrc;
        static BlendFactor blendColorDst;
        static BlendFactor blendAlphaSrc;
        static BlendFactor blendAlphaDst;

        static bool alphaEnable;
        static TestFunc alphaFunc;
        static u8 alphaRef;

        static bool depthEnable;
        static TestFunc depthFunc;

        static u32 componentMask;

        static ShaderData* activeShader;

        static TexEnv currTexEnv[TEX_ENV_COUNT];

        static TextureData* activeTextures[TEX_UNIT_COUNT];
        static u32 enabledTextures;

        static bool allow3d;
        static ScreenSide screenSide;

        static u32* gpuCommandBuffer;
        static u32* gpuFrameBuffer;
        static u32* gpuDepthBuffer;

        void updateState();
        void safeWait(GSP_Event event);
    }
}

bool ctr::gpu::init()  {
    dirtyState = 0xFFFFFFFF;
    dirtyTexEnvs = 0xFFFFFFFF;
    dirtyTextures = 0xFFFFFFFF;

    clearColor = 0;
    clearDepth = 0;

    viewportScreen = SCREEN_TOP;
    viewportX = 0;
    viewportY = 0;
    viewportWidth = TOP_WIDTH;
    viewportHeight = TOP_HEIGHT;

    scissorMode = SCISSOR_DISABLE;
    scissorX = 0;
    scissorY = 0;
    scissorWidth = TOP_WIDTH;
    scissorHeight = TOP_HEIGHT;

    depthNear = 0;
    depthFar = 1;

    cullMode = CULL_NONE;

    stencilEnable = false;
    stencilFunc = TEST_ALWAYS;
    stencilRef = 0;
    stencilMask = 0xFF;
    stencilReplace = 0;
    stencilFail = STENCIL_OP_KEEP;
    stencilZFail = STENCIL_OP_KEEP;
    stencilZPass = STENCIL_OP_KEEP;

    blendRed = 0;
    blendGreen = 0;
    blendBlue = 0;
    blendAlpha = 0;
    blendColorEquation = BLEND_ADD;
    blendAlphaEquation = BLEND_ADD;
    blendColorSrc = FACTOR_SRC_ALPHA;
    blendColorDst = FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAlphaSrc = FACTOR_SRC_ALPHA;
    blendAlphaDst = FACTOR_ONE_MINUS_SRC_ALPHA;

    alphaEnable = false;
    alphaFunc = TEST_ALWAYS;
    alphaRef = 0;

    depthEnable = false;
    depthFunc = TEST_GREATER;

    componentMask = GPU_WRITE_ALL;

    activeShader = NULL;

    currTexEnv[0].rgbSources = TEXENV_SOURCES(SOURCE_TEXTURE0, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
    currTexEnv[0].alphaSources = TEXENV_SOURCES(SOURCE_TEXTURE0, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
    currTexEnv[0].rgbOperands = TEXENV_OPERANDS(TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR);
    currTexEnv[0].alphaOperands = TEXENV_OPERANDS(TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA);
    currTexEnv[0].rgbCombine = COMBINE_MODULATE;
    currTexEnv[0].alphaCombine = COMBINE_MODULATE;
    currTexEnv[0].constantColor = 0xFFFFFFFF;
    for(u8 env = 1; env < TEX_ENV_COUNT; env++) {
        currTexEnv[env].rgbSources = TEXENV_SOURCES(SOURCE_PREVIOUS, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
        currTexEnv[env].alphaSources = TEXENV_SOURCES(SOURCE_PREVIOUS, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
        currTexEnv[env].rgbOperands = TEXENV_OPERANDS(TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR);
        currTexEnv[env].alphaOperands = TEXENV_OPERANDS(TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA);
        currTexEnv[env].rgbCombine = COMBINE_REPLACE;
        currTexEnv[env].alphaCombine = COMBINE_REPLACE;
        currTexEnv[env].constantColor = 0xFFFFFFFF;
    }

    for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
        activeTextures[unit] = NULL;
    }

    enabledTextures = 0;

    allow3d = false;
    screenSide = SIDE_LEFT;

    gpuCommandBuffer = (u32*) linearAlloc(GPU_COMMAND_BUFFER_SIZE * sizeof(u32));
    if(gpuCommandBuffer == NULL) {
        return false;
    }

    gpuFrameBuffer = (u32*) vramAlloc(TOP_WIDTH * TOP_HEIGHT * sizeof(u32));
    if(gpuFrameBuffer == NULL) {
        linearFree(gpuCommandBuffer);
        return false;
    }

    gpuDepthBuffer = (u32*) vramAlloc(TOP_WIDTH * TOP_HEIGHT * sizeof(u32));
    if(gpuDepthBuffer == NULL) {
        linearFree(gpuCommandBuffer);
        vramFree(gpuFrameBuffer);
        return false;
    }

    gfxInitDefault();
    gfxSet3D(true);

    GPU_Init(NULL);
    GPU_Reset(NULL, gpuCommandBuffer, GPU_COMMAND_BUFFER_SIZE);

    clear();

    return true;
}

void ctr::gpu::exit()  {
    if(gpuCommandBuffer != NULL) {
        linearFree(gpuCommandBuffer);
        gpuCommandBuffer = NULL;
    }

    if(gpuFrameBuffer != NULL) {
        vramFree(gpuFrameBuffer);
        gpuFrameBuffer = NULL;
    }

    if(gpuDepthBuffer != NULL) {
        vramFree(gpuDepthBuffer);
        gpuDepthBuffer = NULL;
    }
}

void ctr::gpu::updateState()  {
    u32 dirtyUpdate = dirtyState;
    dirtyState = 0;

    if(dirtyUpdate & STATE_VIEWPORT) {
        GPU_SetViewport((u32*) osConvertVirtToPhys((u32) gpuDepthBuffer), (u32*) osConvertVirtToPhys((u32) gpuFrameBuffer), viewportX, viewportY, viewportHeight, viewportWidth);
    }

    if(dirtyUpdate & STATE_SCISSOR_TEST) {
        GPU_SetScissorTest((GPU_SCISSORMODE) scissorMode, scissorX, scissorY, scissorHeight, scissorWidth);
    }

    if(dirtyUpdate & STATE_DEPTH_MAP) {
        GPU_DepthMap(depthNear, depthFar);
    }

    if(dirtyUpdate & STATE_CULL) {
        GPU_SetFaceCulling((GPU_CULLMODE) cullMode);
    }

    if(dirtyUpdate & STATE_STENCIL_TEST) {
        GPU_SetStencilTest(stencilEnable, (GPU_TESTFUNC) stencilFunc, stencilRef, stencilMask, stencilReplace);
        GPU_SetStencilOp((GPU_STENCILOP) stencilFail, (GPU_STENCILOP) stencilZFail, (GPU_STENCILOP) stencilZPass);
    }

    if(dirtyUpdate & STATE_BLEND) {
        GPU_SetBlendingColor(blendRed, blendGreen, blendBlue, blendAlpha);
        GPU_SetAlphaBlending((GPU_BLENDEQUATION) blendColorEquation, (GPU_BLENDEQUATION) blendAlphaEquation, (GPU_BLENDFACTOR) blendColorSrc, (GPU_BLENDFACTOR) blendColorDst, (GPU_BLENDFACTOR) blendAlphaSrc, (GPU_BLENDFACTOR) blendAlphaDst);
    }

    if(dirtyUpdate & STATE_ALPHA_TEST) {
        GPU_SetAlphaTest(alphaEnable, (GPU_TESTFUNC) alphaFunc, alphaRef);
    }

    if(dirtyUpdate & STATE_DEPTH_TEST_AND_MASK) {
        GPU_SetDepthTestAndWriteMask(depthEnable, (GPU_TESTFUNC) depthFunc, (GPU_WRITEMASK) componentMask);
    }

    if((dirtyUpdate & STATE_ACTIVE_SHADER) && activeShader != NULL && activeShader->dvlb != NULL) {
        shaderProgramUse(&activeShader->program);
    }

    if((dirtyUpdate & STATE_TEX_ENV) && dirtyTexEnvs != 0) {
        for(u8 env = 0; env < TEX_ENV_COUNT; env++) {
            if(dirtyTexEnvs & (1 << env)) {
                GPU_SetTexEnv(env, currTexEnv[env].rgbSources, currTexEnv[env].alphaSources, currTexEnv[env].rgbOperands, currTexEnv[env].alphaOperands, (GPU_COMBINEFUNC) currTexEnv[env].rgbCombine, (GPU_COMBINEFUNC) currTexEnv[env].alphaCombine, currTexEnv[env].constantColor);
            }
        }

        dirtyTexEnvs = 0;
    }

    if((dirtyUpdate & STATE_TEXTURES) && dirtyTextures != 0) {
        for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
            TexUnit texUnit = (TexUnit) (1 << unit);
            if(dirtyTextures & texUnit) {
                TextureData* textureData = activeTextures[unit];
                if(textureData != NULL && textureData->data != NULL) {
                    GPU_SetTexture((GPU_TEXUNIT) texUnit, (u32*) osConvertVirtToPhys((u32) textureData->data), (u16) textureData->width, (u16) textureData->height, textureData->params, (GPU_TEXCOLOR) textureData->format);
                    enabledTextures |= texUnit;
                } else {
                    enabledTextures &= ~texUnit;
                }
            }
        }

        GPU_SetTextureEnable((GPU_TEXUNIT) enabledTextures);
        dirtyTextures = 0;
    }
}

void ctr::gpu::safeWait(GSP_Event event)  {
    Handle eventHandle = gspEvents[event];
    if(!svcWaitSynchronization(eventHandle, 40 * 1000 * 1000)) {
        svcClearEvent(eventHandle);
    }
}

void* ctr::gpu::galloc(u32 size)  {
    return linearAlloc(size);
}

void ctr::gpu::gfree(void* mem)  {
    linearFree(mem);
}

void ctr::gpu::flushCommands()  {
    GPU_FinishDrawing();
    GPUCMD_Finalize();
    GPUCMD_FlushAndRun(NULL);
    safeWait(GSPEVENT_P3D);

    GPUCMD_SetBufferOffset(0);
}

void ctr::gpu::flushBuffer()  {
    gfx3dSide_t side = allow3d && viewportScreen == SCREEN_TOP && screenSide == SIDE_RIGHT ? GFX_RIGHT : GFX_LEFT;
    PixelFormat screenFormat = fbFormatToGPU[gfxGetScreenFormat((gfxScreen_t) viewportScreen)];

    u16 fbWidth;
    u16 fbHeight;
    u32* fb = (u32*) gfxGetFramebuffer((gfxScreen_t) viewportScreen, side, &fbWidth, &fbHeight);

    GX_SetDisplayTransfer(NULL, gpuFrameBuffer, (viewportWidth << 16) | viewportHeight, fb, (fbHeight << 16) | fbWidth, GX_TRANSFER_OUT_FORMAT(screenFormat));
    safeWait(GSPEVENT_PPF);

    if(viewportScreen == SCREEN_TOP && !allow3d) {
        u16 fbWidthRight;
        u16 fbHeightRight;
        u32* fbRight = (u32*) gfxGetFramebuffer((gfxScreen_t) viewportScreen, GFX_RIGHT, &fbWidthRight, &fbHeightRight);

        GX_SetDisplayTransfer(NULL, gpuFrameBuffer, (viewportWidth << 16) | viewportHeight, fbRight, (fbHeightRight << 16) | fbWidthRight, GX_TRANSFER_OUT_FORMAT(screenFormat));
        safeWait(GSPEVENT_PPF);
    }
}

void ctr::gpu::swapBuffers(bool vblank)  {
    gfxSwapBuffersGpu();
    if(vblank) {
        safeWait(GSPEVENT_VBlank0);
    }
}

void ctr::gpu::clear()  {
    GX_SetMemoryFill(NULL, gpuFrameBuffer, clearColor, &gpuFrameBuffer[viewportWidth * viewportHeight], 0x201, gpuDepthBuffer, clearDepth, &gpuDepthBuffer[viewportWidth * viewportHeight], 0x201);
    safeWait(GSPEVENT_PSC0);
}

void ctr::gpu::setClearColor(u8 red, u8 green, u8 blue, u8 alpha)  {
    clearColor = (u32) (((red & 0xFF) << 24) | ((green & 0xFF) << 16) | ((blue & 0xFF) << 8) | (alpha & 0xFF));
}

void ctr::gpu::setClearDepth(u32 depth)  {
    clearDepth = depth;
}

void ctr::gpu::setAllow3d(bool allow3d)  {
    allow3d = allow3d;
}

void ctr::gpu::setScreenSide(ScreenSide side)  {
    screenSide = side;
}

void ctr::gpu::getViewportWidth(u32* out)  {
    if(out == NULL) {
        return;
    }

    *out = viewportWidth;
}

void ctr::gpu::getViewportHeight(u32* out)  {
    if(out == NULL) {
        return;
    }

    *out = viewportHeight;
}

void ctr::gpu::setViewport(Screen screen, u32 x, u32 y, u32 width, u32 height)  {
    viewportScreen = screen;
    viewportX = x;
    viewportY = y;
    viewportWidth = width;
    viewportHeight = height;

    dirtyState |= STATE_VIEWPORT;
}

void ctr::gpu::setScissorTest(ScissorMode mode, u32 x, u32 y, u32 width, u32 height)  {
    scissorMode = mode;
    scissorX = x;
    scissorY = y;
    scissorWidth = width;
    scissorHeight = height;

    dirtyState |= STATE_SCISSOR_TEST;
}

void ctr::gpu::setDepthMap(float near, float far)  {
    depthNear = near;
    depthFar = far;

    dirtyState |= STATE_DEPTH_MAP;
}

void ctr::gpu::setCullMode(CullMode mode)  {
    cullMode = mode;

    dirtyState |= STATE_CULL;
}

void ctr::gpu::setStencilTest(bool enable, TestFunc func, u8 ref, u8 mask, u8 replace)  {
    stencilEnable = enable;
    stencilFunc = func;
    stencilRef = ref;
    stencilMask = mask;
    stencilReplace = replace;

    dirtyState |= STATE_STENCIL_TEST;
}

void ctr::gpu::setStencilOp(StencilOp fail, StencilOp zfail, StencilOp zpass)  {
    stencilFail = fail;
    stencilZFail = zfail;
    stencilZPass = zpass;

    dirtyState |= STATE_STENCIL_TEST;
}

void ctr::gpu::setBlendColor(u8 red, u8 green, u8 blue, u8 alpha)  {
    blendRed = red;
    blendGreen = green;
    blendBlue = blue;
    blendAlpha = alpha;

    dirtyState |= STATE_BLEND;
}

void ctr::gpu::setBlendFunc(BlendEquation colorEquation, BlendEquation alphaEquation, BlendFactor colorSrc, BlendFactor colorDst, BlendFactor alphaSrc, BlendFactor alphaDst)  {
    blendColorEquation = colorEquation;
    blendAlphaEquation = alphaEquation;
    blendColorSrc = colorSrc;
    blendColorDst = colorDst;
    blendAlphaSrc = alphaSrc;
    blendAlphaDst = alphaDst;

    dirtyState |= STATE_BLEND;
}

void ctr::gpu::setAlphaTest(bool enable, TestFunc func, u8 ref)  {
    alphaEnable = enable;
    alphaFunc = func;
    alphaRef = ref;

    dirtyState |= STATE_ALPHA_TEST;
}

void ctr::gpu::setDepthTest(bool enable, TestFunc func)  {
    depthEnable = enable;
    depthFunc = func;

    dirtyState |= STATE_DEPTH_TEST_AND_MASK;
}

void ctr::gpu::setColorMask(bool red, bool green, bool blue, bool alpha)  {
    componentMask = red ? componentMask | GPU_WRITE_RED : componentMask & ~GPU_WRITE_RED;
    componentMask = green ? componentMask | GPU_WRITE_GREEN : componentMask & ~GPU_WRITE_GREEN;
    componentMask = blue ? componentMask | GPU_WRITE_BLUE : componentMask & ~GPU_WRITE_BLUE;
    componentMask = alpha ? componentMask | GPU_WRITE_ALPHA : componentMask & ~GPU_WRITE_ALPHA;

    dirtyState |= STATE_DEPTH_TEST_AND_MASK;
}

void ctr::gpu::setDepthMask(bool depth)  {
    componentMask = depth ? componentMask | GPU_WRITE_DEPTH : componentMask & ~GPU_WRITE_DEPTH;

    dirtyState |= STATE_DEPTH_TEST_AND_MASK;
}

void ctr::gpu::createShader(u32* shader)  {
    if(shader == NULL) {
        return;
    }

    *shader = (u32) new ShaderData();
}

void ctr::gpu::freeShader(u32 shader)  {
    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL) {
        return;
    }

    if(shdr->dvlb != NULL) {
        shaderProgramFree(&shdr->program);
        DVLB_Free(shdr->dvlb);
    }

    delete shdr;
}

void ctr::gpu::loadShader(u32 shader, const void* data, u32 size, u8 geometryStride)  {
    if(data == NULL) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL) {
        return;
    }

    if(shdr->dvlb != NULL) {
        shaderProgramFree(&shdr->program);
        DVLB_Free(shdr->dvlb);
    }

    shdr->dvlb = DVLB_ParseFile((u32*) data, size);
    shaderProgramInit(&shdr->program);
    if(shdr->dvlb->numDVLE > 0) {
        shaderProgramSetVsh(&shdr->program, &shdr->dvlb->DVLE[0]);
        if(shdr->dvlb->numDVLE > 1) {
            shaderProgramSetGsh(&shdr->program, &shdr->dvlb->DVLE[1], geometryStride);
        }
    }
}

void ctr::gpu::useShader(u32 shader)  {
    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    activeShader = shdr;

    dirtyState |= STATE_ACTIVE_SHADER;
}

void ctr::gpu::getUniformBool(u32 shader, ShaderType type, int id, bool* value)  {
    if(value == NULL) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    shaderInstance_s* instance = type == SHADER_VERTEX ? shdr->program.vertexShader : shdr->program.geometryShader;
    if(instance != NULL) {
        shaderInstanceGetBool(instance, id, value);
    }
}

void ctr::gpu::setUniformBool(u32 shader, ShaderType type, int id, bool value)  {
    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    shaderInstance_s* instance = type == SHADER_VERTEX ? shdr->program.vertexShader : shdr->program.geometryShader;
    if(instance != NULL) {
        shaderInstanceSetBool(instance, id, value);
    }
}

void ctr::gpu::setUniform(u32 shader, ShaderType type, const char *name, const float *data, u32 elements)  {
    if(name == NULL || data == NULL) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    float fixedData[elements * 4];
    for(u32 i = 0; i < elements; i++) {
        fixedData[i * 4 + 0] = data[i * 4 + 3];
        fixedData[i * 4 + 1] = data[i * 4 + 2];
        fixedData[i * 4 + 2] = data[i * 4 + 1];
        fixedData[i * 4 + 3] = data[i * 4 + 0];
    }

    shaderInstance_s* instance = type == SHADER_VERTEX ? shdr->program.vertexShader : shdr->program.geometryShader;
    if(instance != NULL) {
        Result res = shaderInstanceGetUniformLocation(instance, name);
        if(res >= 0) {
            GPU_SetFloatUniform((GPU_SHADER_TYPE) type, (u32) res, (u32*) fixedData, elements);
        }
    }
}

void ctr::gpu::createVbo(u32* vbo)  {
    if(vbo == NULL) {
        return;
    }

    *vbo = (u32) new VboData();
}

void ctr::gpu::freeVbo(u32 vbo)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    if(vboData->data != NULL) {
        linearFree(vboData->data);
    }

    if(vboData->indices != NULL) {
        linearFree(vboData->indices);
    }

    delete vboData;
}

void ctr::gpu::getVboData(u32 vbo, void** out)  {
    if(out == NULL) {
        return;
    }

    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        *out = NULL;
        return;
    }

    *out = vboData->data;
}

void ctr::gpu::setVboDataInfo(u32 vbo, u32 numVertices, Primitive primitive)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    u32 size = numVertices * vboData->bytesPerVertex;
    if(size != 0 && (vboData->data == NULL || vboData->size < size)) {
        if(vboData->data != NULL) {
            linearFree(vboData->data);
        }

        vboData->data = linearMemAlign(size, 0x80);
        if(vboData->data == NULL) {
            vboData->size = 0;
            return;
        }

        vboData->size = size;
    }

    vboData->numVertices = numVertices;
    vboData->primitive = primitive;
}

void ctr::gpu::setVboData(u32 vbo, const void *data, u32 numVertices, Primitive primitive)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    setVboDataInfo(vbo, numVertices, primitive);
    if(data == NULL) {
        return;
    }

    u32 size = numVertices * vboData->bytesPerVertex;
    if(size > 0) {
        std::memcpy(vboData->data, data, size);
    }
}

void ctr::gpu::getVboIndices(u32 vbo, void** out)  {
    if(out == NULL) {
        return;
    }

    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        *out = NULL;
        return;
    }

    *out = vboData->indices;
}

void ctr::gpu::setVboIndicesInfo(u32 vbo, u32 size)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    if(size == 0) {
        if(vboData->indices != NULL) {
            linearFree(vboData->indices);
            vboData->indices = NULL;
            vboData->indicesSize = 0;
        }

        return;
    }

    if(vboData->indices == NULL || vboData->indicesSize < size) {
        if(vboData->indices != NULL) {
            linearFree(vboData->indices);
        }

        vboData->indices = linearMemAlign(size, 0x80);
        if(vboData->indices == NULL) {
            vboData->indicesSize = 0;
            return;
        }

        vboData->indicesSize = size;
    }
}

void ctr::gpu::setVboIndices(u32 vbo, const void *data, u32 size)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    setVboIndicesInfo(vbo, data != NULL ? size : 0);
    if(data == NULL || size == 0) {
        return;
    }

    std::memcpy(vboData->indices, data, size);
}

void ctr::gpu::setVboAttributes(u32 vbo, u64 attributes, u8 attributeCount)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    vboData->attributes = attributes;
    vboData->attributeCount = attributeCount;
    vboData->attributeMask = 0xFFC;
    vboData->attributePermutations = 0;
    vboData->bytesPerVertex = 0;
    for(u32 i = 0; i < vboData->attributeCount; i++) {
        vboData->attributePermutations |= i << (i * 4);

        u8 data = (u8) ((attributes >> (i * 4)) & 0xF);
        u8 components = (u8) (((data >> 2) & 3) + 1);
        AttributeType type = (AttributeType) (data & 3);
        vboData->bytesPerVertex += components * bytesPerAttrFormat[type];
    }
}

void ctr::gpu::drawVbo(u32 vbo)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL || vboData->data == NULL) {
        return;
    }

    updateState();

    static u32 attributeBufferOffset = 0;
    GPU_SetAttributeBuffers(vboData->attributeCount, (u32*) osConvertVirtToPhys((u32) vboData->data), vboData->attributes, vboData->attributeMask, vboData->attributePermutations, 1, &attributeBufferOffset, &vboData->attributePermutations, &vboData->attributeCount);
    if(vboData->indices != NULL) {
        GPU_DrawElements((GPU_Primitive_t) vboData->primitive, (u32*) vboData->indices, vboData->numVertices);
    } else {
        GPU_DrawArray((GPU_Primitive_t) vboData->primitive, vboData->numVertices);
    }
}

void ctr::gpu::setTexEnv(u32 env, u16 rgbSources, u16 alphaSources, u16 rgbOperands, u16 alphaOperands, CombineFunc rgbCombine, CombineFunc alphaCombine, u32 constantColor)  {
    if(env >= TEX_ENV_COUNT) {
        return;
    }

    currTexEnv[env].rgbSources = rgbSources;
    currTexEnv[env].alphaSources = alphaSources;
    currTexEnv[env].rgbOperands = rgbOperands;
    currTexEnv[env].alphaOperands = alphaOperands;
    currTexEnv[env].rgbCombine = rgbCombine;
    currTexEnv[env].alphaCombine = alphaCombine;
    currTexEnv[env].constantColor = constantColor;

    dirtyState |= STATE_TEX_ENV;
    dirtyTexEnvs |= (1 << env);
}

void ctr::gpu::createTexture(u32* texture)  {
    if(texture == NULL) {
        return;
    }

    *texture = (u32) new TextureData();
}

void ctr::gpu::freeTexture(u32 texture)  {
    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        return;
    }

    if(textureData->data != NULL) {
        linearFree(textureData->data);
    }

    delete textureData;
}

void ctr::gpu::getTextureData(u32 texture, void** out)  {
    if(out == NULL) {
        return;
    }

    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        *out = NULL;
        return;
    }

    *out = textureData->data;
}

void ctr::gpu::setTextureInfo(u32 texture, u32 width, u32 height, PixelFormat format, u32 params)  {
    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL || (textureData->data != NULL && width == textureData->width && height == textureData->height && format == textureData->format && params == textureData->params)) {
        return;
    }

    u32 size = (u32) (width * height * nibblesPerPixelFormat[format] / 2);
    if(textureData->data == NULL || textureData->size < size) {
        if(textureData->data != NULL) {
            linearFree(textureData->data);
        }

        textureData->data = linearMemAlign(size, 0x80);
        if(textureData->data == NULL) {
            textureData->size = 0;
            return;
        }

        textureData->size = size;
    }

    textureData->width = width;
    textureData->height = height;
    textureData->format = format;
    textureData->params = params;

    for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
        if(activeTextures[unit] == textureData) {
            dirtyState |= STATE_TEXTURES;
            dirtyTextures |= (1 << unit);
        }
    }
}

void ctr::gpu::setTextureData(u32 texture, const void *data, u32 width, u32 height, PixelFormat format, u32 params)  {
    if(data == NULL) {
        return;
    }

    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        return;
    }

    setTextureInfo(texture, width, height, format, params);

    GSPGPU_FlushDataCache(NULL, (u8*) data, (u32) (width * height * nibblesPerPixelFormat[format] / 2));
    GX_SetDisplayTransfer(NULL, (u32*) data, (height << 16) | width, (u32*) textureData->data, (height << 16) | width, (u32) (GX_TRANSFER_OUT_TILED(true) | GX_TRANSFER_IN_FORMAT(format) | GX_TRANSFER_OUT_FORMAT(format)));
    safeWait(GSPEVENT_PPF);
}

void ctr::gpu::bindTexture(TexUnit unit, u32 texture)  {
    u32 unitIndex = unit >> 1;
    if(activeTextures[unitIndex] != (TextureData*) texture) {
        activeTextures[unitIndex] = (TextureData*) texture;

        dirtyState |= STATE_TEXTURES;
        dirtyTextures |= (1 << unitIndex);
    }
}