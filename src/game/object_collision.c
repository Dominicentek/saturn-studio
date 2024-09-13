#include <PR/ultratypes.h>

#include "sm64.h"
#include "debug.h"
#include "interaction.h"
#include "mario.h"
#include "object_list_processor.h"
#include "spawn_object.h"
#include "object_helpers.h"
#include "saturn/saturn_actors.h"

struct Object *debug_print_obj_collision(struct Object *a) {
    struct Object *sp24;
    UNUSED s32 unused;
    s32 i;

    for (i = 0; i < a->numCollidedObjs; i++) {
        print_debug_top_down_objectinfo("ON", 0);
        sp24 = a->collidedObjs[i];
        if (sp24 != gMarioObject) {
            return sp24;
        }
    }
    return NULL;
}

int detect_object_hitbox_overlap(struct Object *a, struct Object *b) {
    f32 sp3C = a->oPosY - a->hitboxDownOffset;
    f32 sp38 = b->oPosY - b->hitboxDownOffset;
    f32 dx = a->oPosX - b->oPosX;
    UNUSED f32 sp30 = sp3C - sp38;
    f32 dz = a->oPosZ - b->oPosZ;
    f32 collisionRadius = a->hitboxRadius + b->hitboxRadius;
    f32 distance = sqrtf(dx * dx + dz * dz);

    if (collisionRadius > distance) {
        f32 sp20 = a->hitboxHeight + sp3C;
        f32 sp1C = b->hitboxHeight + sp38;

        if (sp3C > sp1C) {
            return 0;
        }
        if (sp20 < sp38) {
            return 0;
        }
        if (a->numCollidedObjs >= 4) {
            return 0;
        }
        if (b->numCollidedObjs >= 4) {
            return 0;
        }
        a->collidedObjs[a->numCollidedObjs] = b;
        b->collidedObjs[b->numCollidedObjs] = a;
        a->collidedObjInteractTypes |= b->oInteractType;
        b->collidedObjInteractTypes |= a->oInteractType;
        a->numCollidedObjs++;
        b->numCollidedObjs++;
        return 1;
    }

    //! no return value
}

int detect_object_hurtbox_overlap(struct Object *a, struct Object *b) {
    f32 sp3C = a->oPosY - a->hitboxDownOffset;
    f32 sp38 = b->oPosY - b->hitboxDownOffset;
    f32 sp34 = a->oPosX - b->oPosX;
    UNUSED f32 sp30 = sp3C - sp38;
    f32 sp2C = a->oPosZ - b->oPosZ;
    f32 sp28 = a->hurtboxRadius + b->hurtboxRadius;
    f32 sp24 = sqrtf(sp34 * sp34 + sp2C * sp2C);

    if (a == gMarioObject) {
        b->oInteractionSubtype |= INT_SUBTYPE_DELAY_INVINCIBILITY;
    }

    if (sp28 > sp24) {
        f32 sp20 = a->hitboxHeight + sp3C;
        f32 sp1C = b->hurtboxHeight + sp38;

        if (sp3C > sp1C) {
            return 0;
        }
        if (sp20 < sp38) {
            return 0;
        }
        if (a == gMarioObject) {
            b->oInteractionSubtype &= ~INT_SUBTYPE_DELAY_INVINCIBILITY;
        }
        return 1;
    }

    //! no return value
}

void check_collision(struct Object* a, struct Object* b) {
    if (a->oIntangibleTimer + b->oIntangibleTimer != 0) return;
    if (detect_object_hitbox_overlap(a, b) && b->hurtboxRadius != 0.0f) {
        detect_object_hurtbox_overlap(a, b);
    }
}

void check_player_object_collision(void) {
    for (int i = 0; i < OBJECT_POOL_CAPACITY; i++) {
        if (gObjectPool[i].activeFlags == ACTIVE_FLAG_DEACTIVATED) continue;
        if (
            (gObjectPool[i].objList != OBJ_LIST_POLELIKE) &&
            (gObjectPool[i].objList != OBJ_LIST_PUSHABLE) &&
            (gObjectPool[i].objList != OBJ_LIST_GENACTOR) &&
            (gObjectPool[i].objList != OBJ_LIST_LEVEL) &&
            (gObjectPool[i].objList != OBJ_LIST_SURFACE) &&
            (gObjectPool[i].objList != OBJ_LIST_DESTRUCTIVE)
        ) continue;
        check_collision(gMarioObject, &gObjectPool[i]);
    }
}

void check_pushable_object_collision(void) {
    for (int i = 0; i < OBJECT_POOL_CAPACITY; i++) {
        if (gObjectPool[i].activeFlags == ACTIVE_FLAG_DEACTIVATED) continue;
        if (gObjectPool[i].objList != OBJ_LIST_PUSHABLE) continue;
        for (int j = 0; j < OBJECT_POOL_CAPACITY; j++) {
            if (i == j) continue;
            if (gObjectPool[i].activeFlags == ACTIVE_FLAG_DEACTIVATED) continue;
            if (gObjectPool[i].objList != OBJ_LIST_PUSHABLE) continue;
            check_collision(gObjectPool + i, gObjectPool + j);
        }
    }
}

void check_destructive_object_collision(void) {
    for (int i = 0; i < OBJECT_POOL_CAPACITY; i++) {
        if (gObjectPool[i].activeFlags == ACTIVE_FLAG_DEACTIVATED) continue;
        if (gObjectPool[i].objList != OBJ_LIST_DESTRUCTIVE) continue;
        for (int j = 0; j < OBJECT_POOL_CAPACITY; j++) {
            if (i == j) continue;
            if (gObjectPool[i].activeFlags == ACTIVE_FLAG_DEACTIVATED) continue;
            if (
                (gObjectPool[i].objList != OBJ_LIST_GENACTOR) &&
                (gObjectPool[i].objList != OBJ_LIST_PUSHABLE) &&
                (gObjectPool[i].objList != OBJ_LIST_SURFACE)
            ) continue;
            check_collision(gObjectPool + i, gObjectPool + j);
        }
    }
}

void detect_object_collisions(void) {
    for (int i = 0; i < OBJECT_POOL_CAPACITY; i++) {
        if (gObjectPool[i].activeFlags == ACTIVE_FLAG_DEACTIVATED) continue;
        if (
            (gObjectPool[i].objList != OBJ_LIST_POLELIKE) &&
            (gObjectPool[i].objList != OBJ_LIST_PLAYER) &&
            (gObjectPool[i].objList != OBJ_LIST_PUSHABLE) &&
            (gObjectPool[i].objList != OBJ_LIST_GENACTOR) &&
            (gObjectPool[i].objList != OBJ_LIST_LEVEL) &&
            (gObjectPool[i].objList != OBJ_LIST_SURFACE) &&
            (gObjectPool[i].objList != OBJ_LIST_DESTRUCTIVE)
        ) continue;
        gObjectPool[i].numCollidedObjs = 0;
        gObjectPool[i].collidedObjInteractTypes = 0;
        if (gObjectPool[i].oIntangibleTimer > 0) {
            gObjectPool[i].oIntangibleTimer--;
        }
    }
    check_player_object_collision();
    check_destructive_object_collision();
    check_pushable_object_collision();
}

float distance_to_line(float px, float py, float x1, float y1, float x2, float y2) {
    float length = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
    if (length == 0) return sqrtf((px - x1) * (px - x1) + (py - y1) * (py - y1));

    float t = ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / length;
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    float closestX = x1 + t * (x2 - x1);
    float closestY = y1 + t * (y2 - y1);

    float distance = sqrtf((px - closestX) * (px - closestX) + (py - closestY) * (py - closestY));

    return distance;
}

u8 line_intersect(float x1a, float y1a, float x2a, float y2a, float x1b, float y1b, float x2b, float y2b) {
    float dx1 = x2a - x1a;
    float dy1 = y2a - y1a;
    float dx2 = x2b - x1b;
    float dy2 = y2b - y1b;

    float determinant = dx1 * dy2 - dx2 * dy1;

    if (absf(determinant) < 1e-6) return 0;

    float t1 = ((x1b - x1a) * dy2 - (y1b - y1a) * dx2) / determinant;
    float t2 = ((x1b - x1a) * dy1 - (y1b - y1a) * dx1) / determinant;

    return t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1;
}

u8 lineseg_intersects_circle(float x1, float y1, float x2, float y2, float x, float y, float r) {
    return distance_to_line(x, y, x1, y1, x2, y2) <= r;
}

u8 lineseg_intersects_rect(float x1, float y1, float x2, float y2, float x, float y, float w, float h) {
    return line_intersect(x1, y1, x2, y2, x,     y,     x + w, y    ) ||
           line_intersect(x1, y1, x2, y2, x,     y,     x,     y + h) ||
           line_intersect(x1, y1, x2, y2, x,     y + h, x + w, y + h) ||
           line_intersect(x1, y1, x2, y2, x + w, y    , x + w, y + h);
}


u8 lineseg_intersects_cylinder(Vec3f a, Vec3f b, Vec3f p, float r, float h) {
    u8 x = lineseg_intersects_circle(a[0], a[2], b[0], b[2], p[0],     p[2], r       );
    u8 y = lineseg_intersects_rect  (a[0], a[1], b[0], b[1], p[0] - r, p[1], r * 2, h);
    u8 z = lineseg_intersects_rect  (a[2], a[1], b[2], b[1], p[2] - r, p[1], r * 2, h);
    return x && y && z;
}

struct Object* get_mario_actor_from_ray(Vec3f from, Vec3f to) {
    struct Object* intersect = NULL;
    float intersect_distance = 0x10000;
    for (int i = 0; i < saturn_actor_sizeof(); i++) {
        struct Object* obj = saturn_actor_get_object(i);
        if (!obj) continue;
        Vec3f pos;
        pos[0] = obj->oPosX;
        pos[1] = obj->oPosY;
        pos[2] = obj->oPosZ;
        if (lineseg_intersects_cylinder(from, to, pos, 37, 160)) {
            float dist = sqrtf(
                (pos[0] - from[0]) * (pos[0] - from[0]) +
                (pos[1] - from[1]) * (pos[1] - from[1]) +
                (pos[2] - from[2]) * (pos[2] - from[2])
            );
            if ((intersect && intersect_distance > dist) || !intersect) {
                intersect_distance = dist;
                intersect = obj;
            }
        }
        obj = (struct Object*)obj->header.next;
    }
    return intersect;
}
