#include "../vanir.h"
#include "../modules/windows.h"

#include <webgpu/webgpu.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

extern struct VanirGPU gpu;

#include "../modules/render.h"
#include "render.h"
#include "shader.h"
#include "textures.h"

extern struct Shader *activeShader;

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

/* ↓ push one vertex into the flat buffer; applies the active matrix transform ↓ */
static void pushVert(struct Pipeline *p, float x, float y, float z, float r, float g, float b, float a) {
    applyActiveMatrix(&x, &y);

    float *dst = p->verts + p->vertCount * VERTEX_STRIDE;
    dst[0]=x; 
    dst[1]=y; 
    dst[2]=z; 
    dst[3]=r; 
    dst[4]=g; 
    dst[5]=b; 
    dst[6]=a;
    
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
            .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
            .size = needed,
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
            WGPURenderPipeline pipe = NULL;

            if (activeShader) {
                pipe = (cmd->type == DRAW_TRIS)
                    ? activeShader->pipeline
                    : activeShader->pipelineLine;
            } else {
                pipe = (cmd->type == DRAW_TRIS)
                    ? p->pipeline
                    : p->pipelineLine;
            }

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
    getGlobalColor(&c);
    
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
    getGlobalColor(&c);
    
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

        getGlobalColor(&c);
        pushVert(p, px[i],   py[i],   0, c.r, c.g, c.b, c.a);

        if (hasCb) 
            callColorCb(L, 5, i + 1);

        getGlobalColor(&c);
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
        getGlobalColor(&c);
        
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
            getGlobalColor(&ec[i]);
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
        getGlobalColor(&c);
        
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
            getGlobalColor(&vc[i]);
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
    getGlobalColor(&c);
    
    pushVert(p, x, y, z, c.r, c.g, c.b, c.a);
    pushCmd(p, DRAW_TRIS, 1);
    
    return 0;
}
/* render.drawRectOutline(x, y, w, h [, thickness]) */
/* thickness <= 1: four line pairs. thickness > 1: four filled rects. */
int drawRectOutline(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;

    struct Pipeline *p = w->pipeline;

    if (!p) 
        return 0;

    float x = (float)lua_tonumber(L, 1), y = (float)lua_tonumber(L, 2);
    float bw = (float)lua_tonumber(L, 3), bh = (float)lua_tonumber(L, 4);
    float t = lua_isnoneornil(L, 5) ? 1.0f : (float)lua_tonumber(L, 5);

    struct color c;
    getGlobalColor(&c);

    if (t <= 1.0f) {
        if (!vertReserve(p, 8)) 
            return 0;

        pushVert(p, x,      y,      0, c.r,c.g,c.b,c.a);
        pushVert(p, x+bw,   y,      0, c.r,c.g,c.b,c.a);
        pushVert(p, x+bw,   y,      0, c.r,c.g,c.b,c.a);
        pushVert(p, x+bw,   y+bh,   0, c.r,c.g,c.b,c.a);
        pushVert(p, x+bw,   y+bh,   0, c.r,c.g,c.b,c.a);
        pushVert(p, x,      y+bh,   0, c.r,c.g,c.b,c.a);
        pushVert(p, x,      y+bh,   0, c.r,c.g,c.b,c.a);
        pushVert(p, x,      y,      0, c.r,c.g,c.b,c.a);
        pushCmd(p, DRAW_LINES, 8);
    } else {
        /* thick outline: four filled border rects */
        if (!vertReserve(p, 24)) 
            return 0;

        /* top */
        pushVert(p, x,      y,      0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y,      0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+t,    0, c.r,c.g,c.b,c.a);
        pushVert(p, x,      y,      0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+t,    0, c.r,c.g,c.b,c.a); pushVert(p, x,      y+t,    0, c.r,c.g,c.b,c.a);
        
        /* bottom */
        pushVert(p, x,      y+bh-t, 0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+bh-t, 0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+bh,   0, c.r,c.g,c.b,c.a);
        pushVert(p, x,      y+bh-t, 0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+bh,   0, c.r,c.g,c.b,c.a); pushVert(p, x,      y+bh,   0, c.r,c.g,c.b,c.a);
        
        /* left */
        pushVert(p, x,      y+t,    0, c.r,c.g,c.b,c.a); pushVert(p, x+t,    y+t,    0, c.r,c.g,c.b,c.a); pushVert(p, x+t,    y+bh-t, 0, c.r,c.g,c.b,c.a);
        pushVert(p, x,      y+t,    0, c.r,c.g,c.b,c.a); pushVert(p, x+t,    y+bh-t, 0, c.r,c.g,c.b,c.a); pushVert(p, x,      y+bh-t, 0, c.r,c.g,c.b,c.a);
        
        /* right */
        pushVert(p, x+bw-t, y+t,    0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+t,    0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+bh-t, 0, c.r,c.g,c.b,c.a);
        pushVert(p, x+bw-t, y+t,    0, c.r,c.g,c.b,c.a); pushVert(p, x+bw,   y+bh-t, 0, c.r,c.g,c.b,c.a); pushVert(p, x+bw-t, y+bh-t, 0, c.r,c.g,c.b,c.a);
        pushCmd(p, DRAW_TRIS, 24);
    }

    return 0;
}

/* render.drawTriangle(x1, y1, x2, y2, x3, y3) */
int drawTriangle(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;

    struct Pipeline *p = w->pipeline;

    if (!p) 
        return 0;

    float x1 = (float)lua_tonumber(L, 1), y1 = (float)lua_tonumber(L, 2);
    float x2 = (float)lua_tonumber(L, 3), y2 = (float)lua_tonumber(L, 4);
    float x3 = (float)lua_tonumber(L, 5), y3 = (float)lua_tonumber(L, 6);

    if (!vertReserve(p, 3)) 
        return 0;

    struct color c;
    getGlobalColor(&c);

    pushVert(p, x1, y1, 0, c.r,c.g,c.b,c.a);
    pushVert(p, x2, y2, 0, c.r,c.g,c.b,c.a);
    pushVert(p, x3, y3, 0, c.r,c.g,c.b,c.a);
    pushCmd(p, DRAW_TRIS, 3);

    return 0;
}

/* ↓ push a filled arc as a triangle fan from (cx,cy); used by rounded box corners ↓ */
static void pushArc(struct Pipeline *p, float cx, float cy, float r, float aStart, float aEnd, int segs, float cr, float cg, float cb, float ca) {
    float step = (aEnd - aStart) / (float)segs;

    for (int i = 0; i < segs; ++i) {
        float a0 = aStart + step * (float)i;
        float a1 = a0 + step;

        pushVert(p, cx, cy, 0, cr,cg,cb,ca);
        pushVert(p, cx + cosf(a0) * r, cy + sinf(a0) * r, 0, cr,cg,cb,ca);
        pushVert(p, cx + cosf(a1) * r, cy + sinf(a1) * r, 0, cr,cg,cb,ca);
    }
}

/* ↓ internal: draw a rounded rect, per-corner rounding flags ↓ */
static int drawRoundedBoxInternal(lua_State *L, float r, float x, float y, float bw, float bh, bool tl, bool tr, bool bl, bool br) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;

    struct Pipeline *p = w->pipeline;

    if (!p) 
        return 0;

    struct color c;
    getGlobalColor(&c);

    float cr = c.r, cg = c.g, cb = c.b, ca = c.a;

    /* ↓ clamp radius so it never exceeds half the shorter side ↓ */
    float maxR = (bw < bh ? bw : bh) * 0.5f;
    
    if (r > maxR) 
        r = maxR;
    
    if (r < 0.0f) 
        r = 0.0f;

    int segs = (int)(r * 0.5f);
    
    if (segs < 4) 
        segs = 4;

    /* ↓ 3 body rects (center + left strip + right strip) = 18 verts ↓ */
    if (!vertReserve(p, 18)) return 0;

    /* center rect */
    pushVert(p, x+r,    y,      0, cr,cg,cb,ca); pushVert(p, x+bw-r, y,      0, cr,cg,cb,ca); pushVert(p, x+bw-r, y+bh,   0, cr,cg,cb,ca);
    pushVert(p, x+r,    y,      0, cr,cg,cb,ca); pushVert(p, x+bw-r, y+bh,   0, cr,cg,cb,ca); pushVert(p, x+r,    y+bh,   0, cr,cg,cb,ca);
    
    /* left strip */
    pushVert(p, x,      y+r,    0, cr,cg,cb,ca); pushVert(p, x+r,    y+r,    0, cr,cg,cb,ca); pushVert(p, x+r,    y+bh-r, 0, cr,cg,cb,ca);
    pushVert(p, x,      y+r,    0, cr,cg,cb,ca); pushVert(p, x+r,    y+bh-r, 0, cr,cg,cb,ca); pushVert(p, x,      y+bh-r, 0, cr,cg,cb,ca);
    
    /* right strip */
    pushVert(p, x+bw-r, y+r,    0, cr,cg,cb,ca); pushVert(p, x+bw,   y+r,    0, cr,cg,cb,ca); pushVert(p, x+bw,   y+bh-r, 0, cr,cg,cb,ca);
    pushVert(p, x+bw-r, y+r,    0, cr,cg,cb,ca); pushVert(p, x+bw,   y+bh-r, 0, cr,cg,cb,ca); pushVert(p, x+bw-r, y+bh-r, 0, cr,cg,cb,ca);
    pushCmd(p, DRAW_TRIS, 18);

    /* ↓ four corners: rounded arc or square fill depending on flag ↓ */
    int arcVerts = segs * 3;
    int cornerVerts = tl ? arcVerts : 6;
    
    if (!vertReserve(p, (uint32_t)cornerVerts)) 
        return 0;
    
    if (tl) {
        pushArc(p, x+r,    y+r,    r, (float)M_PI,       (float)M_PI*1.5f, segs, cr,cg,cb,ca);
    } else {
        pushVert(p,x,   y,      0,cr,cg,cb,ca); pushVert(p,x+r, y,      0,cr,cg,cb,ca); pushVert(p,x+r, y+r, 0,cr,cg,cb,ca);
        pushVert(p,x,   y,      0,cr,cg,cb,ca); pushVert(p,x+r, y+r,    0,cr,cg,cb,ca); pushVert(p,x,   y+r, 0,cr,cg,cb,ca);
    }

    pushCmd(p, DRAW_TRIS, (uint32_t)cornerVerts);

    cornerVerts = tr ? arcVerts : 6;

    if (!vertReserve(p, (uint32_t)cornerVerts)) 
        return 0;

    if (tr) {
        pushArc(p, x+bw-r, y+r,    r, (float)M_PI*1.5f,  (float)M_PI*2.0f, segs, cr,cg,cb,ca);
    } else {
        pushVert(p,x+bw-r, y,      0,cr,cg,cb,ca); pushVert(p,x+bw,   y,      0,cr,cg,cb,ca); pushVert(p,x+bw,   y+r, 0,cr,cg,cb,ca);
        pushVert(p,x+bw-r, y,      0,cr,cg,cb,ca); pushVert(p,x+bw,   y+r,    0,cr,cg,cb,ca); pushVert(p,x+bw-r, y+r, 0,cr,cg,cb,ca);
    }

    pushCmd(p, DRAW_TRIS, (uint32_t)cornerVerts);

    cornerVerts = br ? arcVerts : 6;

    if (!vertReserve(p, (uint32_t)cornerVerts)) 
        return 0;

    if (br) {
        pushArc(p, x+bw-r, y+bh-r, r, 0.0f,             (float)M_PI*0.5f, segs, cr,cg,cb,ca);
    } else {
        pushVert(p,x+bw-r, y+bh-r, 0,cr,cg,cb,ca); pushVert(p,x+bw,   y+bh-r, 0,cr,cg,cb,ca); pushVert(p,x+bw,   y+bh, 0,cr,cg,cb,ca);
        pushVert(p,x+bw-r, y+bh-r, 0,cr,cg,cb,ca); pushVert(p,x+bw,   y+bh,   0,cr,cg,cb,ca); pushVert(p,x+bw-r, y+bh, 0,cr,cg,cb,ca);
    }

    pushCmd(p, DRAW_TRIS, (uint32_t)cornerVerts);

    cornerVerts = bl ? arcVerts : 6;

    if (!vertReserve(p, (uint32_t)cornerVerts)) 
        return 0;

    if (bl) {
        pushArc(p, x+r,    y+bh-r, r, (float)M_PI*0.5f,  (float)M_PI,      segs, cr,cg,cb,ca);
    } else {
        pushVert(p,x,   y+bh-r, 0,cr,cg,cb,ca); pushVert(p,x+r, y+bh-r, 0,cr,cg,cb,ca); pushVert(p,x+r, y+bh, 0,cr,cg,cb,ca);
        pushVert(p,x,   y+bh-r, 0,cr,cg,cb,ca); pushVert(p,x+r, y+bh,   0,cr,cg,cb,ca); pushVert(p,x,   y+bh, 0,cr,cg,cb,ca);
    }

    pushCmd(p, DRAW_TRIS, (uint32_t)cornerVerts);

    return 0;
}

/* render.drawRoundedBox(r, x, y, w, h) */
int drawRoundedBox(lua_State *L) {
    float r  = (float)lua_tonumber(L, 1);
    float x  = (float)lua_tonumber(L, 2);
    float y  = (float)lua_tonumber(L, 3);
    float bw = (float)lua_tonumber(L, 4);
    float bh = (float)lua_tonumber(L, 5);

    return drawRoundedBoxInternal(L, r, x, y, bw, bh, true, true, true, true);
}

/* render.drawRoundedBoxEx(r, x, y, w, h [, tl, tr, bl, br]) */
/* Per-corner rounding: pass false to get a square corner instead */
int drawRoundedBoxEx(lua_State *L) {
    float r  = (float)lua_tonumber(L, 1);
    float x  = (float)lua_tonumber(L, 2);
    float y  = (float)lua_tonumber(L, 3);
    float bw = (float)lua_tonumber(L, 4);
    float bh = (float)lua_tonumber(L, 5);

    bool tl = lua_isnoneornil(L, 6) ? true : (bool)lua_toboolean(L, 6);
    bool tr = lua_isnoneornil(L, 7) ? true : (bool)lua_toboolean(L, 7);
    bool bl = lua_isnoneornil(L, 8) ? true : (bool)lua_toboolean(L, 8);
    bool br = lua_isnoneornil(L, 9) ? true : (bool)lua_toboolean(L, 9);

    return drawRoundedBoxInternal(L, r, x, y, bw, bh, tl, tr, bl, br);
}

/* render.drawTexturedRectUV(x, y, w, h, startU, startV, endU, endV) */
/* Draws the active texture stretched to the given rect with explicit UV coords */
int drawTexturedRectUV(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder || !w->frame.passEncoder) return 0;

    if (!activeTexture) return 0;

    float x  = (float)lua_tonumber(L, 1), y  = (float)lua_tonumber(L, 2);
    float bw = (float)lua_tonumber(L, 3), bh = (float)lua_tonumber(L, 4);
    float u0 = (float)lua_tonumber(L, 5), v0 = (float)lua_tonumber(L, 6);
    float u1 = (float)lua_tonumber(L, 7), v1 = (float)lua_tonumber(L, 8);

    drawTexturedQuadImmediate(w, activeTexture, x, y, bw, bh, u0, v0, u1, v1);

    return 0;
}

/* render.drawTexturedTriangleUV(vert1, vert2, vert3) */
/* Each vert is a table {x, y [, u, v]}. UV is accepted but currently ignored — */
/* the vertex format (x y z r g b a) has no UV channel; draws a flat-colored tri. */
int drawTexturedTriangleUV(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder || !w->frame.passEncoder) 
        return 0;

    struct Pipeline *p = w->pipeline;

    if (!p) 
        return 0;

    float vx[3], vy[3];

    for (int i = 0; i < 3; ++i) {
        luaL_checktype(L, i + 1, LUA_TTABLE);
        lua_rawgeti(L, i + 1, 1); vx[i] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, i + 1, 2); vy[i] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    }

    if (!vertReserve(p, 3)) 
        return 0;

    struct color c;
    getGlobalColor(&c);

    pushVert(p, vx[0], vy[0], 0, c.r,c.g,c.b,c.a);
    pushVert(p, vx[1], vy[1], 0, c.r,c.g,c.b,c.a);
    pushVert(p, vx[2], vy[2], 0, c.r,c.g,c.b,c.a);
    pushCmd(p, DRAW_TRIS, 3);

    return 0;
}

/* render.drawPixelsRGB(w, h, dataR, dataG, dataB) */
/* Each data table is a flat 1-indexed array of integers 0-255, length w*h. */
/* Uploads as a scratch texture and draws it at (0, 0) at native size. */
int drawPixelsRGB(lua_State *L) {
    struct glfwWindow *win = currentRenderWindow;

    if (!win || !win->frame.encoder || !win->frame.passEncoder) 
        return 0;

    int pw = (int)luaL_checkinteger(L, 1);
    int ph = (int)luaL_checkinteger(L, 2);

    if (pw <= 0 || ph <= 0) 
        return 0;

    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    luaL_checktype(L, 5, LUA_TTABLE);

    int n = pw * ph;
    uint8_t *pixels = (uint8_t *)malloc((size_t)(4 * n));

    if (!pixels) 
        return 0;

    for (int i = 0; i < n; ++i) {
        lua_rawgeti(L, 3, i + 1); uint8_t r = (uint8_t)lua_tointeger(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 4, i + 1); uint8_t g = (uint8_t)lua_tointeger(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 5, i + 1); uint8_t b = (uint8_t)lua_tointeger(L, -1); lua_pop(L, 1);

        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = 255;
    }

    /* ↓ upload and draw as a scratch texture ↓ */
    struct Texture *tex = textureUpload("__drawPixelsRGB_scratch__", pixels, (uint32_t)pw, (uint32_t)ph);
    
    free(pixels);

    if (!tex) 
        return 0;

    drawTexturedQuadImmediate(win, tex, 0, 0, (float)pw, (float)ph, 0.0f, 0.0f, 1.0f, 1.0f);
    textureRelease(tex);

    return 0;
}

/* render.drawPixelsSubrectRGB(dstX, dstY, srcX, srcY, srcW, srcH, subrectW, subrectH, dataR, dataG, dataB) */
/* Draws a subrect of a pixel buffer at (dstX, dstY). */
int drawPixelsSubrectRGB(lua_State *L) {
    struct glfwWindow *win = currentRenderWindow;

    if (!win || !win->frame.encoder || !win->frame.passEncoder) 
        return 0;

    float dstX = (float)luaL_checknumber(L, 1);
    float dstY = (float)luaL_checknumber(L, 2);
    float srcX = (float)luaL_checknumber(L, 3);
    float srcY = (float)luaL_checknumber(L, 4);

    /* srcW / srcH unused here — subrectW/H define the full buffer size */
    int srW = (int)luaL_checkinteger(L, 7);
    int srH = (int)luaL_checkinteger(L, 8);

    if (srW <= 0 || srH <= 0) return 0;

    luaL_checktype(L, 9,  LUA_TTABLE);
    luaL_checktype(L, 10, LUA_TTABLE);
    luaL_checktype(L, 11, LUA_TTABLE);

    int n = srW * srH;
    uint8_t *pixels = (uint8_t *)malloc((size_t)(4 * n));

    if (!pixels) 
        return 0;

    for (int i = 0; i < n; ++i) {
        lua_rawgeti(L, 9,  i + 1); uint8_t r = (uint8_t)lua_tointeger(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 10, i + 1); uint8_t g = (uint8_t)lua_tointeger(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 11, i + 1); uint8_t b = (uint8_t)lua_tointeger(L, -1); lua_pop(L, 1);

        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = 255;
    }

    /* ↓ compute UV subrect within the uploaded texture ↓ */
    float srcWf   = (float)luaL_checknumber(L, 5);
    float srcHf   = (float)luaL_checknumber(L, 6);
    float u0 = srcX / srcWf;
    float v0 = srcY / srcHf;
    float u1 = (srcX + (float)srW) / srcWf;
    float v1 = (srcY + (float)srH) / srcHf;

    struct Texture *tex = textureUpload("__drawPixelsSubrectRGB_scratch__", pixels, (uint32_t)srW, (uint32_t)srH);
    
    free(pixels);

    if (!tex) 
        return 0;

    drawTexturedQuadImmediate(win, tex, dstX, dstY, (float)srW, (float)srH, u0, v0, u1, v1);
    textureRelease(tex);

    return 0;
}
/* lua draw functions ↑↑↑ lua draw functions  */