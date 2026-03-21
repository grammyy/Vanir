#include <webgpu/webgpu.h>
#include <stdlib.h>
#include <math.h>

#include "../vanir.h"
#include "../modules/windows.h"

extern struct VanirGPU gpu;

#include "../modules/render.h"

#define VERTEX_STRIDE 7              // x y z  r g b a
#define VERTEX_BYTES  (VERTEX_STRIDE * sizeof(float))

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/* ↓ batch helpers ↓ */
/* ↓ all draw calls within a frame accumulate vertices into flat array, cmd list, and at stopRender(), flushBatches() uploads everything in one buffer and walks cmd list ↓ */
/* ↓ reserves double in size each time to amortize reallocs ↓ */
static bool vertReserve(struct Pipeline *p, uint32_t extra) {
    uint32_t needed = p->vertCount + extra;
    
    if (needed <= p->vertCap) 
        return true;

    uint32_t newCap = p->vertCap == 0 ? 256 : p->vertCap;
    
    while (newCap < needed) 
        newCap *= 2;

    float *buf = realloc(p->verts, newCap * VERTEX_BYTES);
    
    if (!buf) {
        vanir_log("[ERROR] vertReserve: realloc failed"); 
        
        return false; 
    }
    
    p->verts = buf;
    p->vertCap = newCap;
    
    return true;
}

static bool cmdReserve(struct Pipeline *p, uint32_t extra) {
    uint32_t needed = p->cmdCount + extra;
    
    if (needed <= p->cmdCap) 
        return true;

    uint32_t newCap = p->cmdCap == 0 ? 32 : p->cmdCap;
    
    while (newCap < needed) 
        newCap *= 2;

    struct DrawCmd *buf = realloc(p->cmds, newCap * sizeof(struct DrawCmd));
    
    if (!buf) {
        vanir_log("[ERROR] cmdReserve: realloc failed"); 
        
        return false;
    }
    
    p->cmds = buf;
    p->cmdCap = newCap;
    
    return true;
}

// Push one vertex into the flat buffer.
static void pushVert(struct Pipeline *p, float x, float y, float z, float r, float g, float b, float a) {
    float *dst = p->verts + p->vertCount * VERTEX_STRIDE;
    
    dst[0]=x; dst[1]=y; dst[2]=z; dst[3]=r; dst[4]=g; dst[5]=b; dst[6]=a;
    
    p->vertCount++;
}

/* ↓ record a draw command for the vertices just pushed; if same type and contiguous then extend ↓ */
static void pushCmd(struct Pipeline *p, DrawType type, uint32_t vertCount) {
    if (p->cmdCount > 0) {
        struct DrawCmd *last = &p->cmds[p->cmdCount - 1];
        
        if (last->type == type &&
            last->firstVertex + last->vertCount == p->vertCount - vertCount) {
            last->vertCount += vertCount;
            return;
        }
    }

    if (!cmdReserve(p, 1)) 
        return;

    p->cmds[p->cmdCount++] = (struct DrawCmd){
        .type        = type,
        .firstVertex = p->vertCount - vertCount,
        .vertCount   = vertCount
    };
}
/* ↑ batch helpers ↑ */

/* ↓ gpu flush, called by stopRender in render.c ↓ */
void flushBatches(struct glfwWindow *w) {
    struct Pipeline *p = w->pipeline;
    
    if (!p || p->vertCount == 0 || p->cmdCount == 0) 
        return;

    uint32_t needed = p->vertCount * VERTEX_BYTES;

    if (!p->vertexBuffer || needed > p->vertexBufferSize) {
        if (p->vertexBuffer) 
            wgpuBufferRelease(p->vertexBuffer);

        WGPUBufferDescriptor desc = {
            .usage            = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
            .size             = needed,
            .mappedAtCreation = false
        };

        p->vertexBuffer     = wgpuDeviceCreateBuffer(gpu.device, &desc);
        p->vertexBufferSize = needed;
    }

    wgpuQueueWriteBuffer(gpu.queue, p->vertexBuffer, 0, p->verts, needed);

    /* ↓ bind buffer once, and walk cmd in order; switches pipeline on type ↓ */
    wgpuRenderPassEncoderSetVertexBuffer(w->frame.passEncoder, 0, p->vertexBuffer, 0, needed);

    DrawType currentType = (DrawType)-1;

    for (uint32_t i = 0; i < p->cmdCount; ++i) {
        struct DrawCmd *cmd = &p->cmds[i];

        if (cmd->type != currentType) {
            WGPURenderPipeline pipe = (cmd->type == DRAW_TRIS) ? p->pipeline : p->pipelineLine;
            
            wgpuRenderPassEncoderSetPipeline(w->frame.passEncoder, pipe);
            
            if (p->uniformBindGroup)
                wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 0, p->uniformBindGroup, 0, NULL);
            
                currentType = cmd->type;
        }

        wgpuRenderPassEncoderDraw(w->frame.passEncoder, cmd->vertCount, 1, cmd->firstVertex, 0);
    }

    /* ↓ reset batch counter for next frame, allocations are kept ↓ */
    p->vertCount = 0;
    p->cmdCount  = 0;
}

/* ↓ per vertex color callback helper ↓*/
static void callColorCb(lua_State *L, int cbIdx, int i) {
    lua_pushvalue(L, cbIdx);
    lua_pushinteger(L, i);
    lua_call(L, 1, 0);
}

/* ↓ precompute all point positions along circumference, avoids repeat sin/cos ↓ */
static void circlePoints(float cx, float cy, float radius, int segments, float *out_x, float *out_y) {
    float theta = (2.0f * (float)M_PI) / (float)segments;
    float cosT = cosf(theta), sinT = sinf(theta);
    float px = radius, py = 0;

    for (int i = 0; i <= segments; ++i) {
        out_x[i] = cx + px;
        out_y[i] = cy + py;

        float nx = px * cosT - py * sinT;
        float ny = px * sinT + py * cosT;

        px = nx; py = ny;
    }
}

/* lua draw functions ↓↓↓ lua draw functions  */
/* ↓ all functions follow the same pattern; validate frame -> reserver space in cpu buffer -> push vertices -> record draw cmd ↓ */
int drawLine(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;
    
    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;
    
    struct Pipeline *p = w->pipeline;
    
    if (!p) 
        return 0;

    float x1 = lua_tonumber(L, 1), y1 = lua_tonumber(L, 2);
    float x2 = lua_tonumber(L, 3), y2 = lua_tonumber(L, 4);
    /* ↑ webgpu does not support variable line width, make new function later ↑ */

    if (!vertReserve(p, 2)) 
        return 0;
    
    struct color c; 
    getGlobalColor(L, &c);
    
    pushVert(p, x1, y1, 0, c.r, c.g, c.b, c.a);
    pushVert(p, x2, y2, 0, c.r, c.g, c.b, c.a);
    pushCmd(p, DRAW_LINES, 2);
    
    return 0;
}

int drawRect(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;
    
    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;
    
    struct Pipeline *p = w->pipeline;
    
    if (!p) 
        return 0;

    float x  = lua_tonumber(L, 1), y  = lua_tonumber(L, 2);
    float bw = lua_tonumber(L, 3), bh = lua_tonumber(L, 4);

    if (!vertReserve(p, 6)) 
        return 0;
    
    struct color c; 
    getGlobalColor(L, &c);
    
    pushVert(p, x,      y,      0, c.r, c.g, c.b, c.a);
    pushVert(p, x + bw, y,      0, c.r, c.g, c.b, c.a);
    pushVert(p, x + bw, y + bh, 0, c.r, c.g, c.b, c.a);
    pushVert(p, x,      y,      0, c.r, c.g, c.b, c.a);
    pushVert(p, x + bw, y + bh, 0, c.r, c.g, c.b, c.a);
    pushVert(p, x,      y + bh, 0, c.r, c.g, c.b, c.a);
    pushCmd(p, DRAW_TRIS, 6);
    
    return 0;
}

int drawCircle(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;
    
    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;
    
    struct Pipeline *p = w->pipeline;
    
    if (!p) 
        return 0;

    float x = lua_tonumber(L, 1), y = lua_tonumber(L, 2), radius = lua_tonumber(L, 3);
    int segs = (int)luaL_optinteger(L, 4, 32);
    bool hasCb = lua_isfunction(L, 5);
    
    if (segs < 2) segs = 2;

    float *px = alloca((segs + 1) * sizeof(float));
    float *py = alloca((segs + 1) * sizeof(float));
    
    circlePoints(x, y, radius, segs, px, py);

    if (!vertReserve(p, (uint32_t)(segs * 2))) 
        return 0;
    
    struct color c;
    
    for (int i = 0; i < segs; ++i) {
        if (hasCb) 
            callColorCb(L, 5, i);

        getGlobalColor(L, &c);
        pushVert(p, px[i],   py[i],   0, c.r, c.g, c.b, c.a);

        if (hasCb) 
            callColorCb(L, 5, i + 1);

        getGlobalColor(L, &c);
        pushVert(p, px[i+1], py[i+1], 0, c.r, c.g, c.b, c.a);
    }
    
    pushCmd(p, DRAW_LINES, (uint32_t)(segs * 2));
    
    return 0;
}

/* ↓ center vertex gets average color of two surrounding edge ↓ */
int drawFilledCircle(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;
    
    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;
    
    struct Pipeline *p = w->pipeline;
    
    if (!p) 
        return 0;

    float x = lua_tonumber(L, 1), y = lua_tonumber(L, 2), radius = lua_tonumber(L, 3);
    int segs = (int)luaL_optinteger(L, 4, 32);
    bool hasCb = lua_isfunction(L, 5);
    
    if (segs < 2) segs = 2;

    float *px = alloca((segs + 1) * sizeof(float));
    float *py = alloca((segs + 1) * sizeof(float));
    
    circlePoints(x, y, radius, segs, px, py);

    if (!vertReserve(p, (uint32_t)(segs * 3))) 
        return 0;

    if (!hasCb) {
        struct color c; 
        getGlobalColor(L, &c);
        
        for (int i = 0; i < segs; ++i) {
            pushVert(p, x,       y,       0, c.r, c.g, c.b, c.a);
            pushVert(p, px[i],   py[i],   0, c.r, c.g, c.b, c.a);
            pushVert(p, px[i+1], py[i+1], 0, c.r, c.g, c.b, c.a);
        }
    } else {
        // Pre-sample one color per edge vertex (0..segs-1), then assemble.
        // This ensures each color is fetched exactly once and the center
        // can blend smoothly between its two neighbours.
        struct color *ec = alloca((segs + 1) * sizeof(struct color));
        
        for (int i = 0; i < segs; ++i) {
            callColorCb(L, 5, i);
            getGlobalColor(L, &ec[i]);
        }
        
        ec[segs] = ec[0]; // close the loop

        for (int i = 0; i < segs; ++i) {
            // Center gets the average of the two surrounding edge colors.
            struct color cc = {
                (ec[i].r + ec[i+1].r) * 0.5f,
                (ec[i].g + ec[i+1].g) * 0.5f,
                (ec[i].b + ec[i+1].b) * 0.5f,
                (ec[i].a + ec[i+1].a) * 0.5f,
            };
            
            pushVert(p, x,       y,       0, cc.r,     cc.g,     cc.b,     cc.a);
            pushVert(p, px[i],   py[i],   0, ec[i].r,  ec[i].g,  ec[i].b,  ec[i].a);
            pushVert(p, px[i+1], py[i+1], 0, ec[i+1].r,ec[i+1].g,ec[i+1].b,ec[i+1].a);
        }
    }
    
    pushCmd(p, DRAW_TRIS, (uint32_t)(segs * 3));
    
    return 0;
}

/* ↓ pre-samples all colors before fan loop so vertex 0's color is only fetched once ↓ */
int drawPoly(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;
    
    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;
    
    struct Pipeline *p = w->pipeline;
    
    if (!p) 
        return 0;

    int n = (int)lua_rawlen(L, 1);
    bool hasCb = lua_isfunction(L, 2);

    if (n < 3) 
        return 0;

    float *vx = alloca(n * sizeof(float));
    float *vy = alloca(n * sizeof(float));
    float *vz = alloca(n * sizeof(float));
    
    for (int i = 0; i < n; ++i) {
        lua_rawgeti(L, 1, i + 1);
        lua_rawgeti(L, -1, 1); vx[i] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); vy[i] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 3); vz[i] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_pop(L, 1);
    }

    if (!vertReserve(p, (uint32_t)((n - 2) * 3))) 
        return 0;

    if (!hasCb) {
        struct color c; 
        getGlobalColor(L, &c);
        
        for (int i = 1; i < n - 1; ++i) {
            pushVert(p, vx[0],   vy[0],   vz[0],   c.r, c.g, c.b, c.a);
            pushVert(p, vx[i],   vy[i],   vz[i],   c.r, c.g, c.b, c.a);
            pushVert(p, vx[i+1], vy[i+1], vz[i+1], c.r, c.g, c.b, c.a);
        }
    } else {
        /* ↓ pre-sampling happens here ↓ */
        struct color *vc = alloca(n * sizeof(struct color));
        
        for (int i = 0; i < n; ++i) {
            callColorCb(L, 2, i + 1); // Lua 1-indexed
            getGlobalColor(L, &vc[i]);
        }
        
        for (int i = 1; i < n - 1; ++i) {
            pushVert(p, vx[0],   vy[0],   vz[0],   vc[0].r,   vc[0].g,   vc[0].b,   vc[0].a);
            pushVert(p, vx[i],   vy[i],   vz[i],   vc[i].r,   vc[i].g,   vc[i].b,   vc[i].a);
            pushVert(p, vx[i+1], vy[i+1], vz[i+1], vc[i+1].r, vc[i+1].g, vc[i+1].b, vc[i+1].a);
        }
    }
    
    pushCmd(p, DRAW_TRIS, (uint32_t)((n - 2) * 3));
    
    return 0;
}

int drawVertex(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;
    
    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;
    
    struct Pipeline *p = w->pipeline;
    
    if (!p) 
        return 0;

    float x = lua_tonumber(L, 1), y = lua_tonumber(L, 2), z = lua_tonumber(L, 3);
    
    if (!vertReserve(p, 1)) 
        return 0;
    
    struct color c; 
    getGlobalColor(L, &c);
    
    pushVert(p, x, y, z, c.r, c.g, c.b, c.a);
    pushCmd(p, DRAW_TRIS, 1);
    
    return 0;
}
/* lua draw functions ↑↑↑ lua draw functions  */
