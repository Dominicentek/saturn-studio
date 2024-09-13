#include "saturn_actors.h"
#include "behavior_data.h"
#include "game/object_helpers.h"
#include "mario_animation_ids.h"
#include "saturn/saturn.h"
#include "saturn/saturn_animation_ids.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "sm64.h"

extern "C" {
#include "include/object_fields.h"
#include "game/object_list_processor.h"
#include "game/level_update.h"
#include "engine/math_util.h"
#include "game/memory.h"
#include "game/mario.h"
#include "engine/behavior_script.h"
#include "game/spawn_object.h"
}

#define o gCurrentObject

MarioActor* gMarioActorList = nullptr;
ModelID current_mario_model = MODEL_MARIO;

MarioActor::MarioActor() {
    PasteGameShark(GameSharkCode().GameShark, colorcode);
    marioObj = (struct Object*)malloc(sizeof(struct Object));
    memset(marioObj, 0, sizeof(struct Object));
    geo_reset_object_node(&marioObj->header.gfx);
    initialize_object(marioObj);
    geo_obj_init((struct GraphNodeObject*)&marioObj->header.gfx, gLoadedGraphNodes[MODEL_MARIO], gVec3fZero, gVec3sZero);
    marioObj->behavior = marioObj->curBhvCommand = bhvMarioActor;
    marioObj->header.gfx.node.flags |= GRAPH_RENDER_ACTIVE | GRAPH_RENDER_HAS_ANIMATION;
    marioObj->header.gfx.unk18 = gCurrAreaIndex;
    scaler[0][0] = scaler[0][1] = scaler[0][2] =
    scaler[1][0] = scaler[1][1] = scaler[1][2] =
    scaler[2][0] = scaler[2][1] = scaler[2][2] = 1;
    struct Animation anim;
    load_animation(&anim, MARIO_ANIM_A_POSE);
    saturn_sample_animation(this, &anim, 0);
}

void delete_mario_actor_timelines(int index) {
    std::vector<std::string> ids = {};
    for (auto& timeline : k_frame_keys) {
        if (timeline.second.first.marioIndex != index) continue;
        ids.push_back(timeline.first);
    }
    for (std::string id : ids) {
        k_frame_keys.erase(id);
    }
}

MarioActor* saturn_spawn_actor(float x, float y, float z) {
    MarioActor actor;
    actor.x = x;
    actor.y = y;
    actor.z = z;
    auto range = saturn_animation_obj_ranges[current_mario_model];
    actor.animstate.custom = false;
    actor.animstate.id = current_mario_model == MODEL_MARIO ? MARIO_ANIM_A_POSE : range.first;
    actor.animstate.frame = 0;
    actor.animstate.yTransform = current_mario_model == MODEL_MARIO ? 0xBD : 0x00;
    actor.obj_model = current_mario_model;
    if (range.first >= saturn_animation_data.size()) {
        actor.num_bones = 0;
        return saturn_add_actor(actor);
    }
    auto anim = saturn_animation_data[range.first];
    actor.num_bones = range.second - range.first == 0 ? 0 : anim.second(anim.first).unk0A + 1;
    if (actor.num_bones != 0 && saturn_obj_initial_anims.find(current_mario_model) != saturn_obj_initial_anims.end()) {
        actor.animstate.id = saturn_obj_initial_anims[current_mario_model];
    }
    return saturn_add_actor(actor);
}

MarioActor* saturn_add_new_actor(MarioActor& actor) {
    MarioActor* new_actor = new MarioActor(actor);
    new_actor->marioObj->oMarioActorIndex = saturn_actor_sizeof();
    if (gMarioActorList) {
        MarioActor* prev = gMarioActorList;
        while (prev->next) prev = prev->next;
        prev->next = new_actor;
        new_actor->prev = prev;
    }
    else gMarioActorList = new_actor;
    return new_actor;
}

MarioActor* saturn_try_replace_actor(MarioActor& actor) {
    MarioActor* curr = gMarioActorList;
    int i = 0;
    while (curr) {
        if (!curr->exists) break;
        curr = curr->next;
        i++;
    }
    if (!curr) return nullptr;
    MarioActor* new_actor = new MarioActor(actor);
    new_actor->marioObj->oMarioActorIndex = i;
    MarioActor* prev = curr->prev;
    MarioActor* next = curr->next;
    delete curr;
    new_actor->prev = prev;
    new_actor->next = next;
    if (next) next->prev = new_actor;
    if (prev) prev->next = new_actor;
    else gMarioActorList = new_actor;
    return new_actor;
}

MarioActor* saturn_add_actor(MarioActor& actor) {
    MarioActor* new_actor = saturn_try_replace_actor(actor);
    if (new_actor) return new_actor;
    return saturn_add_new_actor(actor);
}

void saturn_remove_actor(int index) {
    MarioActor* actorptr = gMarioActorList;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return;
        actorptr = actorptr->next;
    }
    if (!actorptr) return;
    if (!actorptr->exists) return;
    actorptr->exists = false;
    delete_mario_actor_timelines(index);
    actorptr->marioObj->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
    free(actorptr->marioObj);
}

MarioActor* saturn_get_actor(int index) {
    MarioActor* actorptr = gMarioActorList;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return nullptr;
        actorptr = actorptr->next;
    }
    if (!actorptr) return nullptr;
    if (!actorptr->exists) return nullptr;
    return actorptr;
}

int saturn_actor_indexof(MarioActor* actor) {
    MarioActor* actorptr = gMarioActorList;
    int idx = 0;
    while (actorptr) {
        if (actorptr == actor) return idx;
        actorptr = actorptr->next;
        idx++;
    }
    return idx;
}

int saturn_actor_sizeof() {
    return saturn_actor_indexof(nullptr);
}

void saturn_clear_actors() {
    MarioActor* actor = gMarioActorList;
    int i = 0;
    while (actor) {
        delete_mario_actor_timelines(i++);
        actor->marioObj->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
        obj_mark_for_deletion(actor->marioObj);
        actor->exists = false;
        actor = actor->next;
    }
}

int recording_mario_actor = -1;
InputRecordingFrame latest_recording_frame;

#define wrap(x, n) (((x) % (n) + (n)) % (n))

void bhv_mario_actor_loop() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (!actor) return;
    vec3f_set(o->header.gfx.scale, actor->xScale, actor->yScale, actor->zScale);
    if (recording_mario_actor == o->oMarioActorIndex || actor->playback_input) {
        InputRecordingFrame frame;
        if (actor->playback_input) frame = actor->input_recording[actor->input_recording_frame];
        else frame = latest_recording_frame;
        vec3s_copy(actor->torsoAngle, frame.torsoAngle);
        o->oPosX = frame.x;
        o->oPosY = frame.y;
        o->oPosZ = frame.z;
        o->oFaceAngleYaw = frame.angle;
        load_animation(&actor->anim, frame.animID);
        o->header.gfx.unk38.animID = frame.animID;
        o->header.gfx.unk38.curAnim = &actor->anim;
        o->header.gfx.unk38.curAnim->flags = 4; // prevent the anim to get a mind on its own
        o->header.gfx.unk38.animYTrans = actor->animstate.yTransform;
        o->header.gfx.unk38.animFrame = frame.animFrame;
    }
    else {
        vec3s_set(actor->torsoAngle, 0, 0, 0);
        o->oPosX = actor->x;
        o->oPosY = actor->y;
        o->oPosZ = actor->z;
        o->oFaceAngleYaw = actor->angle;
        if (actor->num_bones != 0) {
            o->header.gfx.unk38.curAnim = &actor->anim;
            if (actor->animstate.custom) {
                o->header.gfx.unk38.curAnim->flags = 4;
                o->header.gfx.unk38.curAnim->unk02 = 0;
                o->header.gfx.unk38.curAnim->unk04 = 0;
                o->header.gfx.unk38.curAnim->unk06 = 0;
                o->header.gfx.unk38.curAnim->unk08 = (s16)actor->animstate.length;
                o->header.gfx.unk38.curAnim->unk0A = actor->animstate.customanim_indices.size() / 6 - 1;
                o->header.gfx.unk38.curAnim->values = actor->animstate.customanim_values.data();
                o->header.gfx.unk38.curAnim->index = (const u16*)actor->animstate.customanim_indices.data();
                o->header.gfx.unk38.curAnim->length = (s16)actor->animstate.length;
            }
            else {
                auto anim = saturn_animation_data[actor->animstate.id];
                actor->anim = anim.second(anim.first);
                o->header.gfx.unk38.animID = actor->animstate.id;
                o->header.gfx.unk38.curAnim->flags = 4; // prevent the anim to get a mind on its own
                actor->animstate.length = o->header.gfx.unk38.curAnim->unk08;
            }
            o->header.gfx.unk38.animYTrans = actor->animstate.yTransform;
            o->header.gfx.unk38.animFrame = wrap((int)actor->animstate.frame, actor->animstate.length);
        }
    }
    o->oOpacity = 0xFF;
    o->oAnimState = actor->anim_state;
    cur_obj_set_model(actor->obj_model);
}

ColorCode default_cc;
bool inited_default_cc = false;

void override_cc_color(int* r, int* g, int* b, int ccIndex, int marioIndex, int shadeIndex, float intensity, bool additive) {
    if (!inited_default_cc) {
        inited_default_cc = true;
        PasteGameShark(DEFAULT_COLOR_CODE, default_cc);
    }
    ColorCode cc;
    MarioActor* actor = saturn_get_actor(marioIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) {
        if (o->behavior == bhvMario) memcpy(cc, default_cc, sizeof(ColorCode));
        else return; 
    }
    else memcpy(cc, actor->colorcode, sizeof(ColorCode));
    *r = (*r * additive) + intensity * cc[ccIndex].red[shadeIndex];
    *g = (*g * additive) + intensity * cc[ccIndex].green[shadeIndex];
    *b = (*b * additive) + intensity * cc[ccIndex].blue[shadeIndex];
}

void saturn_rotate_head(Vec3s rotation) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor != nullptr) {
        vec3s_set(rotation,
            actor->head_rot_x,
            actor->head_rot_y,
            actor->head_rot_z
        );
        return;
    }
    vec3s_set(rotation, 0, 0, 0);
}

void saturn_rotate_torso(Vec3s rotation) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor != nullptr) {
        vec3s_copy(rotation, actor->torsoAngle);
        return;
    }
    if (recording_mario_actor == o->oMarioActorIndex) vec3s_copy(rotation, gMarioState->marioBodyState->torsoAngle);
    else vec3s_set(rotation, 0, 0, 0);
}

s16 saturn_actor_geo_switch(u8 item) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return 0;
    switch (item) {
        case ACTOR_SWITCH_EYE:
            return actor->custom_eyes ? 4 : actor->eye_state;
            break;
        case ACTOR_SWITCH_CAP:
            return actor->cap_state;
            break;
        case ACTOR_SWITCH_HAND:
            return actor->hand_state;
            break;
        case ACTOR_SWITCH_POWERUP:
            return actor->powerup_state;
            break;
    }
    return 0;
}

void saturn_actor_get_scaler(Vec3f scale, int index) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) vec3f_set(scale, 1, 1, 1);
    else vec3f_copy(scale, actor->scaler[index]);
}

float saturn_actor_get_alpha() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return 255;
    if (actor->powerup_state & 1) return actor->alpha;
    return 255;
}

bool saturn_actor_has_custom_anim_extra() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return false;
    return actor->animstate.custom && actor->animstate.customanim_extra;
}

int saturn_actor_get_support_flags(int marioIndex) {
    if (o->behavior == bhvMario) return 1;
    MarioActor* actor = saturn_get_actor(marioIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return 0;
    return (actor->spark_support << 1) | actor->cc_support;
}

float saturn_actor_get_shadow_scale() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return 1.f;
    return actor->shadow_scale;
}

bool saturn_actor_is_hidden() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return false;
    return actor->hidden;
}

void saturn_actor_bone_override_begin() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    actor->custom_bone_iter = 0;
}

bool saturn_actor_bone_should_override() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return false;
    return actor->custom_bone;
}

void saturn_actor_bone_iterate() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    actor->custom_bone_iter++;
}

void saturn_actor_bone_iterate_back() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    actor->custom_bone_iter--;
}

void saturn_actor_bone_do_override(Vec3s rotation) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    float multiplier = actor->custom_bone_iter == 0 ? 1 : (65536 / 360.f);
    vec3s_set(rotation,
        actor->bones[actor->custom_bone_iter][0] * multiplier,
        actor->bones[actor->custom_bone_iter][1] * multiplier,
        actor->bones[actor->custom_bone_iter][2] * multiplier
    );
}

Vec3f stored_struct_pos;

void saturn_actor_start_recording(int index) {
    MarioActor* actor = saturn_get_actor(index);
    if (actor == nullptr) return;
    vec3f_copy(stored_struct_pos, gMarioState->pos);
    recording_mario_actor = index;
    actor->input_recording.clear();
    actor->input_recording_frame = 0;
    actor->playback_input = false;
    gMarioState->pos[0] = actor->x;
    gMarioState->pos[1] = actor->y;
    gMarioState->pos[2] = actor->z;
    gMarioState->vel[0] = 0;
    gMarioState->vel[1] = 0;
    gMarioState->vel[2] = 0;
    gMarioState->faceAngle[1] = actor->angle;
}

void saturn_actor_stop_recording() {
    vec3f_copy(gMarioState->pos, stored_struct_pos);
    recording_mario_actor = -1;
}

bool saturn_actor_is_recording_input() {
    if (recording_mario_actor == -1) return false;
    return !!saturn_get_actor(recording_mario_actor);
}

void saturn_actor_record_new_frame() {
    if (!saturn_actor_is_recording_input()) return;
    MarioActor* actor = saturn_get_actor(recording_mario_actor);
    if (!actor) return;
    InputRecordingFrame frame;
    frame.x = gMarioState->pos[0];
    frame.y = gMarioState->pos[1];
    frame.z = gMarioState->pos[2];
    frame.angle = gMarioState->marioObj->header.gfx.angle[1];
    frame.animID = gMarioState->marioObj->header.gfx.unk38.animID;
    frame.animFrame = gMarioState->marioObj->header.gfx.unk38.animFrame;
    vec3s_copy(frame.torsoAngle, gMarioState->marioBodyState->torsoAngle);
    if (frame.animFrame < 0) frame.animFrame = 0;
    actor->input_recording.push_back(frame);
    latest_recording_frame = frame;
}

struct Object* saturn_actor_get_object(int index) {
    MarioActor* actor = saturn_get_actor(index);
    if (!actor) return nullptr;
    if (!actor->exists) return nullptr;
    return actor->marioObj;
}

void saturn_actor_update_all() {
    MarioActor* actor = gMarioActorList;
    while (actor) {
        if (!actor->exists) {
            actor = actor->next;
            continue;
        }
        gCurrentObject = actor->marioObj;
        gCurrentObject->header.gfx.node.flags |= GRAPH_RENDER_HAS_ANIMATION;
        cur_obj_update();
        actor = actor->next;
    }
}

struct ModelTexture {
    struct ModelTexture* next;
    char* id;
    char* data;
    int w, h;
};
struct ModelTexture* gModelTextureList;

void saturn_actor_add_model_texture(char* id, char* data, int w, int h) {
    struct ModelTexture* curr = gModelTextureList;
    while (curr) {
        if (strcmp(curr->id, id) == 0) return;
        curr = curr->next;
    }
    struct ModelTexture* tex = (struct ModelTexture*)malloc(sizeof(struct ModelTexture));
    int texid_len = strlen(id) + 1;
    char* texid = (char*)malloc(texid_len);
    char* texdata = (char*)malloc(w * h * 4);
    memcpy(texid, id, texid_len);
    memcpy(texdata, data, w * h * 4);
    tex->w = w;
    tex->h = h;
    tex->id = texid;
    tex->data = texdata;
    tex->next = nullptr;
    if (gModelTextureList) {
        struct ModelTexture* prev = gModelTextureList;
        while (prev->next) prev = prev->next;
        prev->next = tex;
    }
    else gModelTextureList = tex;
}

char* saturn_actor_get_model_texture(char* id, int* w, int* h) {
    struct ModelTexture* curr = gModelTextureList;
    while (curr) {
        if (strcmp(curr->id, id) == 0) {
            *w = curr->w;
            *h = curr->h;
            return curr->data;
        }
        curr = curr->next;
    }
    return nullptr;
}