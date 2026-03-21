#ifndef WINDOWS
#define WINDOWS

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdbool.h>

int windowsInit(lua_State* L);

// ↓ initialised lazily on first createWindow; torn down when the last window closes ↓
struct VanirGPU {
    WGPUInstance instance;
    WGPUAdapter  adapter;
    WGPUDevice   device;
    WGPUQueue    queue;
};

extern struct VanirGPU gpu;

// ↓ temp per-frame resources. created in selectRender, consumed in stopRender/update ↓
struct Frame {
    WGPUCommandEncoder    encoder;
    WGPURenderPassEncoder passEncoder;
    WGPUTextureView       view;
    WGPUTexture           texture;
    bool                  pending;
};

// ↓ records one logical draw call in submission order so later draws appear on top ↓
typedef enum { DRAW_TRIS, DRAW_LINES } DrawType;

struct DrawCmd {
    DrawType type;
    uint32_t firstVertex;
    uint32_t vertCount;
};

// ↓ per-window render state. Multiple pipelines can be created and swapped ↓
struct Pipeline {
    WGPURenderPipeline  pipeline;        // triangle list
    WGPURenderPipeline  pipelineLine;    // line list
    WGPUBuffer          uniformBuffer;   // { width_f, height_f }
    WGPUBindGroup       uniformBindGroup;
    WGPUBindGroupLayout uniformBindGroupLayout;

    // ↓ flat CPU vertex buffer ↓
    float    *verts;
    uint32_t  vertCount;
    uint32_t  vertCap;

    // ↓ draw command list ↓
    struct DrawCmd *cmds;
    uint32_t        cmdCount;
    uint32_t        cmdCap;

    // ↓ gpu vertex buffer ↓
    WGPUBuffer vertexBuffer;
    uint32_t   vertexBufferSize;

    WGPUBlendFactor blendSrc;
    WGPUBlendFactor blendDst;
};

// ↓ each window owns only its surface and surface format. ↓
struct glfwWindow {
    // ↓ what Lua sees via getSize() ↓
    int x, y, width, height, ref;

    // ↓ physical framebuffer size — used for gpu surface and viewport uniform ↓
    int fbWidth, fbHeight;
    const char *name;

    bool quit, hovering, focused, minimized;

    GLFWwindow       *window;
    WGPUSurface       surface;
    WGPUTextureFormat surfaceFormat;
    WGPUPresentMode   presentMode;

    struct color clearColor;

    struct Frame    frame;
    struct Pipeline *pipeline;
};

struct windowPool {
    struct glfwWindow **windows;
    int count;
};

// ↓ initialise the shared GPU context.
// ↓ takes ownership of probeInstance; probeSurface is used only for adapter 
// ↓ selection and must be released by the caller afterwards
bool vanirGPUInit(WGPUSurface firstSurface);
void vanirGPUDestroy(void);

void destroyPipeline(struct Pipeline *p);
void releaseFrame(struct glfwWindow *w);

struct Pipeline *buildPipelines(struct glfwWindow *w, WGPUBlendFactor blendSrc, WGPUBlendFactor blendDst);

extern struct glfwWindow *currentRenderWindow;

#endif