#include "pc/configfile.h"
#include "pc/gfx/gfx_pc.h"

extern int videores[2];

Gfx* geo_camera_scale(s32 callContext, struct GraphNode *node, UNUSED Mat4 *c) {
    struct GraphNodeScale* scaleNode = (struct GraphNodeScale*)node->next;
    float h = camera_fov / 50.f;
    vec3f_set(scaleNode->scale, h * ((f32)videores[0] / (f32)videores[1]), h, 1.f);
    return NULL;
}

void bhv_camera_update() {}