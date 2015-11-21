#include "citrus/gpu.hpp"
#include "internal.hpp"

#include <cstring>
#include <unordered_map>

#include <3ds.h>

#define COMMAND_BUFFER_SIZE 0x80000

#define TEX_ENV_COUNT 6
#define TEX_UNIT_COUNT 3

#define STATE_VIEWPORT (1 << 0)
#define STATE_DEPTH_MAP (1 << 1)
#define STATE_CULL (1 << 2)
#define STATE_STENCIL_TEST (1 << 3)
#define STATE_BLEND (1 << 4)
#define STATE_ALPHA_TEST (1 << 5)
#define STATE_DEPTH_TEST_AND_MASK (1 << 6)
#define STATE_ACTIVE_SHADER (1 << 7)
#define STATE_TEX_ENV (1 << 8)
#define STATE_TEXTURES (1 << 9)
#define STATE_SCISSOR_TEST (1 << 10)
#define STATE_ACTIVE_SHADER_UNIFORMS (1 << 11)
#define STATE_ACTIVE_SHADER_UNIFORM_BOOLS (1 << 12)

extern Handle gspEvents[GSPGPU_EVENT_MAX];

namespace ctr {
    namespace gpu {
        typedef struct {
            float* data;
            u32 elements;
        } Uniform;

        typedef struct {
            DVLB_s* dvlb;
            shaderProgram_s program;
            std::unordered_map<std::string, Uniform> uniforms[SHADER_GEOMETRY + 1];
            std::unordered_map<int, bool> uniformBools[SHADER_GEOMETRY + 1];
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
            u32 borderColor;
            TexturePlace place;
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

        static const PixelFormat fbFormatToGPU[] = {
                PIXEL_RGBA8,    // GSP_RGBA8_OES
                PIXEL_RGB8,     // GSP_BGR8_OES
                PIXEL_RGB565,   // GSP_RGB565_OES
                PIXEL_RGBA5551, // GSP_RGB5_A1_OES
                PIXEL_RGBA4     // GSP_RGBA4_OES
        };

        static aptHookCookie hookCookie;

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
        static int scissorX;
        static int scissorY;
        static u32 scissorWidth;
        static u32 scissorHeight;

        static float depthMapZScale;
        static float depthMapZOffset;

        static CullMode cullMode;

        static bool stencilEnable;
        static TestFunc stencilFunc;
        static u8 stencilRef;
        static u8 stencilInputMask;
        static u8 stencilWriteMask;
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

        static bool colorMaskRed;
        static bool colorMaskGreen;
        static bool colorMaskBlue;
        static bool colorMaskAlpha;
        static bool depthMask;

        static ShaderData* activeShader;

        static TexEnv currTexEnv[TEX_ENV_COUNT];

        static TextureData* activeTextures[TEX_UNIT_COUNT];
        static u32 enabledTextures;

        static bool allow3d;
        static ScreenSide screenSide;

        static u32* gpuCommandBuffer;
        static u32* gpuFrameBuffer;
        static u32* gpuDepthBuffer;

        void aptHook(APT_HookType hook, void* param);
        void updateState();
        void safeWait(GSPGPU_Event event);
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

    depthMapZScale = 0;
    depthMapZOffset = 1;

    cullMode = CULL_NONE;

    stencilEnable = false;
    stencilFunc = TEST_ALWAYS;
    stencilRef = 0;
    stencilInputMask = 0xFF;
    stencilWriteMask = 0;
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

    colorMaskRed = true;
    colorMaskGreen = true;
    colorMaskBlue = true;
    colorMaskAlpha = true;
    depthMask = true;

    activeShader = NULL;

    currTexEnv[0].rgbSources = gpu::texEnvSources(SOURCE_TEXTURE0, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
    currTexEnv[0].alphaSources = gpu::texEnvSources(SOURCE_TEXTURE0, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
    currTexEnv[0].rgbOperands = gpu::texEnvOperands(TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR);
    currTexEnv[0].alphaOperands = gpu::texEnvOperands(TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA);
    currTexEnv[0].rgbCombine = COMBINE_MODULATE;
    currTexEnv[0].alphaCombine = COMBINE_MODULATE;
    currTexEnv[0].constantColor = 0xFFFFFFFF;
    for(u8 env = 1; env < TEX_ENV_COUNT; env++) {
        currTexEnv[env].rgbSources = gpu::texEnvSources(SOURCE_PREVIOUS, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
        currTexEnv[env].alphaSources = gpu::texEnvSources(SOURCE_PREVIOUS, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
        currTexEnv[env].rgbOperands = gpu::texEnvOperands(TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR);
        currTexEnv[env].alphaOperands = gpu::texEnvOperands(TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA);
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

    gpuCommandBuffer = (u32*) linearAlloc(COMMAND_BUFFER_SIZE * sizeof(u32));
    if(gpuCommandBuffer == NULL) {
        return false;
    }

    gpuFrameBuffer = (u32*) vramAlloc(TOP_WIDTH * TOP_HEIGHT * sizeof(u32));
    if(gpuFrameBuffer == NULL) {
        linearFree(gpuCommandBuffer);
        gpuCommandBuffer = NULL;

        return false;
    }

    gpuDepthBuffer = (u32*) vramAlloc(TOP_WIDTH * TOP_HEIGHT * sizeof(u32));
    if(gpuDepthBuffer == NULL) {
        linearFree(gpuCommandBuffer);
        gpuCommandBuffer = NULL;

        vramFree(gpuFrameBuffer);
        gpuFrameBuffer = NULL;

        return false;
    }

    gfxInitDefault();
    gfxSet3D(true);

    GPUCMD_SetBuffer(gpuCommandBuffer, COMMAND_BUFFER_SIZE, 0);

    aptHook(&hookCookie, aptHook, NULL);

    clear();

    return true;
}

void ctr::gpu::exit()  {
    aptUnhook(&hookCookie);

    gfxExit();

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

void ctr::gpu::aptHook(APT_HookType hook, void* param) {
    if(hook == APTHOOK_ONRESTORE) {
        dirtyState = 0xFFFFFFFF;
        dirtyTexEnvs = 0xFFFFFFFF;
        dirtyTextures = 0xFFFFFFFF;
    }
}

void ctr::gpu::updateState()  {
    if(dirtyState & STATE_VIEWPORT) {
        u32 param[0x4] = {0};

        GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 0x00000001);
        GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 0x00000001);

        u32 dim2 = 0x01000000 | (((viewportWidth - 1) & 0xFFF) << 12) | (viewportHeight & 0xFFF);

        param[0x0] = osConvertVirtToPhys(gpuDepthBuffer) >> 3;
        param[0x1] = osConvertVirtToPhys(gpuFrameBuffer) >> 3;
        param[0x2] = dim2;
        GPUCMD_AddIncrementalWrites(GPUREG_DEPTHBUFFER_LOC, param, 0x00000003);

        GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_DIM2, dim2);
        GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_FORMAT, 0x00000003);
        GPUCMD_AddWrite(GPUREG_COLORBUFFER_FORMAT, 0x00000002);
        GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_BLOCK32, 0x00000000);

        param[0x0] = f32tof24((float) viewportHeight / 2.0f);
        param[0x1] = f32tof31(2.0f / (float) viewportHeight) << 1;
        param[0x2] = f32tof24((float) viewportWidth / 2.0f);
        param[0x3] = f32tof31(2.0f / (float) viewportWidth) << 1;
        GPUCMD_AddIncrementalWrites(GPUREG_VIEWPORT_WIDTH, param, 0x00000004);

        GPUCMD_AddWrite(GPUREG_VIEWPORT_XY, (viewportY << 16) | (viewportX & 0xFFFF));

        param[0x0] = 0x0000000F;
        param[0x1] = 0x0000000F;
        param[0x2] = 0x00000002;
        param[0x3] = 0x00000002;
        GPUCMD_AddIncrementalWrites(GPUREG_COLORBUFFER_READ, param, 0x00000004);
    }

    if(dirtyState & STATE_SCISSOR_TEST) {
        u32 screenWidth = viewportScreen == SCREEN_TOP ? TOP_WIDTH : BOTTOM_WIDTH;
        u32 screenHeight = viewportScreen == SCREEN_TOP ? TOP_HEIGHT : BOTTOM_HEIGHT;

        #define clamp(a, b, c) ((a) > (b) ? (a) < (c) ? (a) : (c) : (b))
        u32 left = (u32) clamp(scissorX, 0, (int) screenWidth);
        u32 bottom = (u32) clamp(scissorY, 0, (int) screenHeight);
        u32 right = (u32) clamp(scissorX + scissorWidth, 0, screenWidth);
        u32 top = (u32) clamp(scissorY + scissorHeight, 0, screenHeight);
        #undef clamp

        u32 param[0x3] = {0};
        param[0x0] = scissorMode;
        param[0x1] = ((screenWidth - right) << 16) | (bottom & 0xFFFF);
        param[0x2] = (((screenWidth - left) - 1) << 16) | ((top - 1) & 0xFFFF);
        GPUCMD_AddIncrementalWrites(GPUREG_SCISSORTEST_MODE, param, 0x00000003);
    }

    if(dirtyState & STATE_DEPTH_MAP) {
        GPUCMD_AddWrite(GPUREG_006D, 0x00000001);
        GPUCMD_AddWrite(GPUREG_DEPTHMAP_SCALE, f32tof24(depthMapZScale));
        GPUCMD_AddWrite(GPUREG_DEPTHMAP_OFFSET, f32tof24(depthMapZOffset));
    }

    if(dirtyState & STATE_CULL) {
        GPUCMD_AddWrite(GPUREG_FACECULLING_CONFIG, cullMode & 0x3);
    }

    if(dirtyState & STATE_STENCIL_TEST) {
        GPUCMD_AddWrite(GPUREG_STENCIL_TEST, (stencilEnable & 1) | ((stencilFunc & 7) << 4) | (stencilWriteMask << 8) | (stencilRef << 16) | (stencilInputMask << 24));
        GPUCMD_AddWrite(GPUREG_STENCIL_ACTION, stencilFail | (stencilZFail << 4) | (stencilZPass << 8));
    }

    if(dirtyState & STATE_BLEND) {
        GPUCMD_AddWrite(GPUREG_BLEND_COLOR, blendRed | (blendGreen << 8) | (blendBlue << 16) | (blendAlpha << 24));

        GPUCMD_AddWrite(GPUREG_BLEND_CONFIG, blendColorEquation | (blendAlphaEquation << 8) | (blendColorSrc << 16) | (blendColorDst << 20) | (blendAlphaSrc << 24) | (blendAlphaDst << 28));
        GPUCMD_AddMaskedWrite(GPUREG_BLEND_ENABLE, 0x2, 0x00000100);
    }

    if(dirtyState & STATE_ALPHA_TEST) {
        GPUCMD_AddWrite(GPUREG_ALPHATEST_CONFIG, (alphaEnable & 1) | ((alphaFunc & 7) << 4) | (alphaRef << 8));
    }

    if(dirtyState & STATE_DEPTH_TEST_AND_MASK) {
        u32 componentMask = ((u32) colorMaskRed * GPU_WRITE_RED) | ((u32) colorMaskGreen * GPU_WRITE_GREEN) | ((u32) colorMaskBlue * GPU_WRITE_BLUE) | ((u32) colorMaskAlpha * GPU_WRITE_ALPHA) | ((u32) depthMask * GPU_WRITE_DEPTH);
        GPUCMD_AddWrite(GPUREG_DEPTHTEST_CONFIG, (depthEnable & 1) | ((depthFunc & 7) << 4) | (componentMask << 8));
    }

    if((dirtyState & STATE_ACTIVE_SHADER) && activeShader != NULL && activeShader->dvlb != NULL) {
        shaderProgramUse(&activeShader->program);
    }

    if((dirtyState & STATE_ACTIVE_SHADER_UNIFORMS) && activeShader != NULL && activeShader->dvlb != NULL) {
        for(ShaderType type = SHADER_VERTEX; type <= SHADER_GEOMETRY; type = (ShaderType) (type + 1)) {
            shaderInstance_s* instance = type == SHADER_VERTEX ? activeShader->program.vertexShader : activeShader->program.geometryShader;
            if(instance != NULL) {
                for(std::unordered_map<std::string, Uniform>::iterator it = activeShader->uniforms[type].begin(); it != activeShader->uniforms[type].end(); it++) {
                    Result res = shaderInstanceGetUniformLocation(instance, (*it).first.c_str());
                    if(res >= 0) {
                        int regOffset = type == SHADER_GEOMETRY ? -0x30 : 0x0;

                        GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG + regOffset, 0x80000000 | res);
                        GPUCMD_AddWrites(GPUREG_VSH_FLOATUNIFORM_DATA + regOffset, (u32*) (*it).second.data, (*it).second.elements * 4);
                    }
                }
            }
        }
    }

    if((dirtyState & STATE_ACTIVE_SHADER_UNIFORM_BOOLS) && activeShader != NULL && activeShader->dvlb != NULL) {
        for(ShaderType type = SHADER_VERTEX; type <= SHADER_GEOMETRY; type = (ShaderType) (type + 1)) {
            shaderInstance_s* instance = type == SHADER_VERTEX ? activeShader->program.vertexShader : activeShader->program.geometryShader;
            if(instance != NULL) {
                for(std::unordered_map<int, bool>::iterator it = activeShader->uniformBools[type].begin(); it != activeShader->uniformBools[type].end(); it++) {
                    shaderInstanceSetBool(instance, (*it).first, (*it).second);
                }
            }
        }
    }

    if((dirtyState & STATE_TEX_ENV) && dirtyTexEnvs != 0) {
        for(u8 env = 0; env < TEX_ENV_COUNT; env++) {
            if(dirtyTexEnvs & (1 << env)) {
                u32 param[0x5] = {0};

                param[0x0] = (currTexEnv[env].alphaSources << 16) | (currTexEnv[env].rgbSources);
                param[0x1] = (currTexEnv[env].alphaOperands << 12) | (currTexEnv[env].rgbOperands);
                param[0x2] = (currTexEnv[env].alphaCombine << 16) | (currTexEnv[env].rgbCombine);
                param[0x3] = currTexEnv[env].constantColor;
                param[0x4] = 0x00000000;

                int tevBase = GPUREG_TEXENV0_SOURCE + (env * 0x8);
                if(env >= 4) {
                    tevBase += 0x10;
                }

                GPUCMD_AddIncrementalWrites(tevBase, param, 0x00000005);
            }
        }

        dirtyTexEnvs = 0;
    }

    if((dirtyState & STATE_TEXTURES) && dirtyTextures != 0) {
        for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
            TexUnit texUnit = (TexUnit) (1 << unit);
            if(dirtyTextures & texUnit) {
                TextureData* textureData = activeTextures[unit];
                if(textureData != NULL && textureData->data != NULL) {
                    u32 typeReg = 0;
                    u32 locReg = 0;
                    u32 dimReg = 0;
                    u32 paramReg = 0;
                    u32 borderColorReg = 0;
                    switch(texUnit) {
                        case TEXUNIT0:
                            typeReg = GPUREG_TEXUNIT0_TYPE;
                            locReg = GPUREG_TEXUNIT0_LOC;
                            dimReg = GPUREG_TEXUNIT0_DIM;
                            paramReg = GPUREG_TEXUNIT0_PARAM;
                            borderColorReg = GPUREG_TEXUNIT0_BORDER_COLOR;
                            break;
                        case TEXUNIT1:
                            typeReg = GPUREG_TEXUNIT1_TYPE;
                            locReg = GPUREG_TEXUNIT1_LOC;
                            dimReg = GPUREG_TEXUNIT1_DIM;
                            paramReg = GPUREG_TEXUNIT1_PARAM;
                            borderColorReg = GPUREG_TEXUNIT1_BORDER_COLOR;
                            break;
                        case TEXUNIT2:
                            typeReg = GPUREG_TEXUNIT2_TYPE;
                            locReg = GPUREG_TEXUNIT2_LOC;
                            dimReg = GPUREG_TEXUNIT2_DIM;
                            paramReg = GPUREG_TEXUNIT2_PARAM;
                            borderColorReg = GPUREG_TEXUNIT2_BORDER_COLOR;
                            break;
                    }

                    GPUCMD_AddWrite(typeReg, textureData->format);
                    GPUCMD_AddWrite(locReg, osConvertVirtToPhys(textureData->data) >> 3);
                    GPUCMD_AddWrite(dimReg, (textureData->width << 16) | textureData->height);
                    GPUCMD_AddWrite(paramReg, textureData->params);
                    GPUCMD_AddWrite(borderColorReg, textureData->borderColor);

                    enabledTextures |= texUnit;
                } else {
                    enabledTextures &= ~texUnit;
                }
            }
        }

        GPUCMD_AddMaskedWrite(GPUREG_006F, 0x2, enabledTextures << 8);
        GPUCMD_AddWrite(GPUREG_TEXUNIT_ENABLE, 0x00011000 | enabledTextures);

        dirtyTextures = 0;
    }

    dirtyState = 0;
}

void ctr::gpu::safeWait(GSPGPU_Event event)  {
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
    GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 0x00000001);
    GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 0x00000001);
    GPUCMD_AddWrite(GPUREG_0063, 0x00000001);

    GPUCMD_Finalize();
    GPUCMD_FlushAndRun();
    safeWait(GSPGPU_EVENT_P3D);

    GPUCMD_SetBufferOffset(0);
}

void ctr::gpu::flushBuffer()  {
    gfx3dSide_t side = allow3d && viewportScreen == SCREEN_TOP && screenSide == SIDE_RIGHT ? GFX_RIGHT : GFX_LEFT;
    PixelFormat screenFormat = fbFormatToGPU[gfxGetScreenFormat((gfxScreen_t) viewportScreen)];

    u16 fbWidth;
    u16 fbHeight;
    u32* fb = (u32*) gfxGetFramebuffer((gfxScreen_t) viewportScreen, side, &fbWidth, &fbHeight);

    GX_DisplayTransfer(gpuFrameBuffer, (viewportWidth << 16) | viewportHeight, fb, (fbHeight << 16) | fbWidth, GX_TRANSFER_OUT_FORMAT(screenFormat));
    safeWait(GSPGPU_EVENT_PPF);

    if(viewportScreen == SCREEN_TOP && !allow3d) {
        u16 fbWidthRight;
        u16 fbHeightRight;
        u32* fbRight = (u32*) gfxGetFramebuffer((gfxScreen_t) viewportScreen, GFX_RIGHT, &fbWidthRight, &fbHeightRight);

        GX_DisplayTransfer(gpuFrameBuffer, (viewportWidth << 16) | viewportHeight, fbRight, (fbHeightRight << 16) | fbWidthRight, GX_TRANSFER_OUT_FORMAT(screenFormat));
        safeWait(GSPGPU_EVENT_PPF);
    }
}

void ctr::gpu::swapBuffers(bool vblank)  {
    gfxSwapBuffersGpu();
    if(vblank) {
        safeWait(GSPGPU_EVENT_VBlank0);
    }
}

void ctr::gpu::clear()  {
    GX_MemoryFill(gpuFrameBuffer, clearColor, &gpuFrameBuffer[viewportWidth * viewportHeight], GX_FILL_32BIT_DEPTH | GX_FILL_TRIGGER, gpuDepthBuffer, clearDepth, &gpuDepthBuffer[viewportWidth * viewportHeight], GX_FILL_32BIT_DEPTH | GX_FILL_TRIGGER);
    safeWait(GSPGPU_EVENT_PSC0);
}

void ctr::gpu::dumpScreen(ctr::gpu::Screen screen, ctr::gpu::ScreenSide side, void** pixels, PixelFormat* format, u32* width, u32* height) {
    gfxScreen_t gfxScreen = screen == SCREEN_TOP ? GFX_TOP : GFX_BOTTOM;
    gfx3dSide_t gfxSide = side == SIDE_LEFT ? GFX_LEFT : GFX_RIGHT;
    PixelFormat fmt = fbFormatToGPU[gfxGetScreenFormat(gfxScreen)];
    u16 w = 0;
    u16 h = 0;
    u8* fb = gfxGetFramebuffer(gfxScreen, gfxSide, &h, &w);

    if(format != NULL) {
        *format = fmt;
    }

    if(width != NULL) {
        *width = w;
    }

    if(height != NULL) {
        *height = h;
    }

    if(pixels != NULL) {
        u32 bpp = bitsPerPixel(fmt);
        u8* px = new u8[w * h * bpp / 8];
        for(u32 x = 0; x < w; x++) {
            for(u32 y = 0; y < h; y++) {
                u8* src = &fb[((h - 1 - y) + x * h) * bpp / 8];
                u8* dst = &px[(y * w + x) * bpp / 8];

                *(u16*) dst = *(u16*) src;
                dst[2] = src[2];
            }
        }

        *pixels = px;
    }
}

void ctr::gpu::setClearColor(u8 red, u8 green, u8 blue, u8 alpha)  {
    clearColor = (u32) (((red & 0xFF) << 24) | ((green & 0xFF) << 16) | ((blue & 0xFF) << 8) | (alpha & 0xFF));
}

void ctr::gpu::setClearDepth(u32 depth)  {
    clearDepth = depth;
}

void ctr::gpu::setAllow3d(bool allow)  {
    allow3d = allow;
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

void ctr::gpu::setScissorTest(ScissorMode mode, int x, int y, u32 width, u32 height)  {
    scissorMode = mode;
    scissorX = x;
    scissorY = y;
    scissorWidth = width;
    scissorHeight = height;

    dirtyState |= STATE_SCISSOR_TEST;
}

void ctr::gpu::setDepthMap(float zScale, float zOffset)  {
    depthMapZScale = zScale;
    depthMapZOffset = zOffset;

    dirtyState |= STATE_DEPTH_MAP;
}

void ctr::gpu::setCullMode(CullMode mode)  {
    cullMode = mode;

    dirtyState |= STATE_CULL;
}

void ctr::gpu::setStencilTest(bool enable, TestFunc func, u8 ref, u8 inputMask, u8 writeMask)  {
    stencilEnable = enable;
    stencilFunc = func;
    stencilRef = ref;
    stencilInputMask = inputMask;
    stencilWriteMask = writeMask;

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
    colorMaskRed = red;
    colorMaskGreen = green;
    colorMaskBlue = blue;
    colorMaskAlpha = alpha;

    dirtyState |= STATE_DEPTH_TEST_AND_MASK;
}

void ctr::gpu::setDepthMask(bool depth)  {
    depthMask = depth;

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

    for(ShaderType type = SHADER_VERTEX; type <= SHADER_GEOMETRY; type = (ctr::gpu::ShaderType) (type + 1)) {
        for(std::unordered_map<std::string, Uniform>::iterator it = activeShader->uniforms[type].begin(); it != activeShader->uniforms[type].end(); it++) {
            delete (*it).second.data;
        }

        shdr->uniforms[type].clear();
        shdr->uniformBools[type].clear();
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

    dirtyState |= STATE_ACTIVE_SHADER | STATE_ACTIVE_SHADER_UNIFORMS | STATE_ACTIVE_SHADER_UNIFORM_BOOLS;
}

void ctr::gpu::getUniform(u32 shader, ShaderType type, const std::string name, float* data, u32 elements) {
    if(data == NULL || elements == 0) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    Uniform* uniform = &shdr->uniforms[type][name];
    u32 count = uniform->elements < elements ? uniform->elements : elements;
    for(u32 i = 0; i < count; i++) {
        data[i * 4 + 0] = uniform->data[i * 4 + 3];
        data[i * 4 + 1] = uniform->data[i * 4 + 2];
        data[i * 4 + 2] = uniform->data[i * 4 + 1];
        data[i * 4 + 3] = uniform->data[i * 4 + 0];
    }
}

void ctr::gpu::setUniform(u32 shader, ShaderType type, const std::string name, const float* data, u32 elements)  {
    if(data == NULL || elements == 0) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    float* fixedData = new float[elements * 4];
    for(u32 i = 0; i < elements; i++) {
        fixedData[i * 4 + 0] = data[i * 4 + 3];
        fixedData[i * 4 + 1] = data[i * 4 + 2];
        fixedData[i * 4 + 2] = data[i * 4 + 1];
        fixedData[i * 4 + 3] = data[i * 4 + 0];
    }

    Uniform uniform;
    uniform.data = fixedData;
    uniform.elements = elements;
    shdr->uniforms[type][name] = uniform;

    if(activeShader == shdr) {
        dirtyState |= STATE_ACTIVE_SHADER_UNIFORMS;
    }
}

void ctr::gpu::getUniformBool(u32 shader, ShaderType type, int id, bool* value)  {
    if(value == NULL) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    *value = shdr->uniformBools[type][id];
}

void ctr::gpu::setUniformBool(u32 shader, ShaderType type, int id, bool value)  {
    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    shdr->uniformBools[type][id] = value;

    if(activeShader == shdr) {
        dirtyState |= STATE_ACTIVE_SHADER_UNIFORM_BOOLS;
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
        AttributeType type = (ctr::gpu::AttributeType) (data & 3);
        vboData->bytesPerVertex += components * bitsPerAttribute(type) / 8;
    }
}

void ctr::gpu::drawVbo(u32 vbo)  {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL || vboData->data == NULL) {
        return;
    }

    updateState();

    u32 param[0x28] = {0};

    param[0x0] = osConvertVirtToPhys(vboData->data) >> 3;
    param[0x1] = (u32) (vboData->attributes & 0xFFFFFFFF);
    param[0x2] = ((vboData->attributeCount - 1) << 28) | ((vboData->attributeMask & 0xFFF) << 16) | (u32) ((vboData->attributes >> 32) & 0xFFFF);
    param[0x3] = 0;
    param[0x4] = (u32) (vboData->attributePermutations & 0xFFFFFFFF);
    param[0x5] = (vboData->attributeCount << 28) | ((vboData->bytesPerVertex & 0xFFF) << 16) | (u32) ((vboData->attributePermutations >> 32) & 0xFFFF);

    GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFERS_LOC, param, 0x00000027);

    GPUCMD_AddMaskedWrite(GPUREG_VSH_INPUTBUFFER_CONFIG, 0xB, 0xA0000000 | (vboData->attributeCount - 1));
    GPUCMD_AddWrite(GPUREG_0242, vboData->attributeCount - 1);

    u32 permutations[] = {(u32) (vboData->attributePermutations & 0xFFFFFFFF), (u32) ((vboData->attributePermutations >> 32) & 0xFFFF)};
    GPUCMD_AddIncrementalWrites(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW, permutations, 2);

    GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x2, vboData->primitive);
    GPUCMD_AddMaskedWrite(GPUREG_RESTART_PRIMITIVE, 0x2, 0x00000001);

    if(vboData->indices != NULL) {
        GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, 0x80000000 | ((u32) vboData->indices));
    } else {
        GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, 0x80000000);
    }

    GPUCMD_AddWrite(GPUREG_NUMVERTICES, vboData->numVertices);
    GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, 0);

    if(vboData->indices != NULL) {
        GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 0x2, 0x00000100);
    }

    GPUCMD_AddMaskedWrite(GPUREG_0253, 0x1, 0x00000001);
    GPUCMD_AddMaskedWrite(GPUREG_0245, 0x1, 0x00000000);

    if(vboData->indices != NULL) {
        GPUCMD_AddWrite(GPUREG_DRAWELEMENTS, 0x00000001);
    } else {
        GPUCMD_AddWrite(GPUREG_DRAWARRAYS, 0x00000001);
    }

    GPUCMD_AddMaskedWrite(GPUREG_0245, 0x1, 0x00000001);
    GPUCMD_AddWrite(GPUREG_0231, 0x00000001);
    GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 0x00000001);
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
        if(textureData->place == TEXTURE_PLACE_RAM) {
            linearFree(textureData->data);
        } else if(textureData->place == TEXTURE_PLACE_VRAM) {
            vramFree(textureData->data);
        }
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

void ctr::gpu::setTextureInfo(u32 texture, u32 width, u32 height, PixelFormat format, u32 params, TexturePlace place)  {
    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL || (textureData->data != NULL && width == textureData->width && height == textureData->height && format == textureData->format && params == textureData->params)) {
        return;
    }

    u32 size = (u32) (width * height * bitsPerPixel(format) / 8);
    if(textureData->data == NULL || textureData->size < size || textureData->place != place) {
        if(textureData->data != NULL) {
            if(textureData->place == TEXTURE_PLACE_RAM) {
                linearFree(textureData->data);
            } else if(textureData->place == TEXTURE_PLACE_VRAM) {
                vramFree(textureData->data);
            }
        }

        if(place == TEXTURE_PLACE_RAM) {
            textureData->data = linearMemAlign(size, 0x80);
        } else {
            textureData->data = vramMemAlign(size, 0x80);
        }

        if(textureData->data == NULL) {
            textureData->size = 0;
            return;
        }

        textureData->size = size;
        textureData->place = place;
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

void ctr::gpu::setTextureData(u32 texture, const void *data, u32 width, u32 height, PixelFormat format, u32 params, TexturePlace place)  {
    if(data == NULL) {
        return;
    }

    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        return;
    }

    setTextureInfo(texture, width, height, format, params, place);

    GSPGPU_FlushDataCache((u8*) data, (u32) (width * height * bitsPerPixel(format) / 8));
    GX_DisplayTransfer((u32*) data, (height << 16) | width, (u32*) textureData->data, (height << 16) | width, (u32) (GX_TRANSFER_OUT_TILED(true) | GX_TRANSFER_IN_FORMAT(format) | GX_TRANSFER_OUT_FORMAT(format)));
    safeWait(GSPGPU_EVENT_PPF);
}

void ctr::gpu::setTextureBorderColor(u32 texture, u8 red, u8 green, u8 blue, u8 alpha)  {
    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        return;
    }

    u32 color = (u32) (((red & 0xFF) << 24) | ((green & 0xFF) << 16) | ((blue & 0xFF) << 8) | (alpha & 0xFF));
    if(textureData->borderColor == color) {
        return;
    }

    textureData->borderColor = color;
    for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
        if(activeTextures[unit] == textureData) {
            dirtyState |= STATE_TEXTURES;
            dirtyTextures |= (1 << unit);
        }
    }
}

void ctr::gpu::bindTexture(TexUnit unit, u32 texture)  {
    u32 unitIndex = unit >> 1;
    if(activeTextures[unitIndex] != (TextureData*) texture) {
        activeTextures[unitIndex] = (TextureData*) texture;

        dirtyState |= STATE_TEXTURES;
        dirtyTextures |= (1 << unitIndex);
    }
}
