#ifndef SaturnActors
#define SaturnActors

#include "include/types.h"

#ifdef __cplusplus

#include "saturn/saturn.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_colors.h"

extern "C" {
#include "game/object_helpers.h"
#include "game/level_update.h"
#include "include/model_ids.h"
#include "include/object_fields.h"
#include "include/behavior_data.h"
}

struct InputRecordingFrame {
    float x, y, z;
    s16 angle;
    int animID;
    int animFrame;
    Vec3s torsoAngle;
};

class MarioActor {
public:
    float x = 0;
    float y = 0;
    float z = 0;
    float angle = 0;
    float xScale = 1;
    float yScale = 1;
    float zScale = 1;
    bool show_emblem = true;
    bool spinning = false;
    float spin_speed = 1;
    float head_rot_x = 0;
    float head_rot_y = 0;
    float head_rot_z = 0;
    int eye_state = 0;
    int cap_state = 0;
    int hand_state = 0;
    int powerup_state = 0;
    float alpha = 128;
    bool hidden = false;
    float shadow_scale = 1.f;
    int selected_model = -1;
    int cc_index = 0;
    bool cc_support = true;
    bool spark_support = true;
    bool custom_eyes = false;
    bool custom_bone = false;
    int custom_bone_iter = 0;
    float anim_state = 0;
    Vec3f scaler[3];
    int num_bones = 21;
    Vec3f bones[60];
    Model model = Model();
    ModelID obj_model;
    ColorCode colorcode;
    struct Animation anim;
    struct AnimationState animstate;
    std::vector<struct InputRecordingFrame> input_recording = {};
    float input_recording_frame = 0;
    Vec3s torsoAngle;
    bool playback_input = false;
    MarioActor* prev = nullptr;
    MarioActor* next = nullptr;
    struct Object* marioObj = nullptr;
    bool exists = true;
    char name[256];
    MarioActor();
};

extern MarioActor* gMarioActorList;
extern ModelID current_mario_model;

extern MarioActor* saturn_spawn_actor(float x, float y, float z);
extern MarioActor* saturn_add_actor(MarioActor& actor);
extern void saturn_remove_actor(int index);
extern MarioActor* saturn_get_actor(int index);
extern int saturn_actor_indexof(MarioActor* actor);

extern int recording_mario_actor;

extern "C" {
#endif
    int saturn_actor_sizeof();
    void saturn_clear_actors();
    void bhv_mario_actor_loop();
    void override_cc_color(int* r, int* g, int* b, int ccIndex, int marioIndex, int shadeIndex, float intensity, bool additive);
    void saturn_rotate_head(Vec3s rotation);
    void saturn_rotate_torso(Vec3s rotation);
    s16 saturn_actor_geo_switch(u8 item);
    void saturn_actor_get_scaler(Vec3f scale, int index);
    float saturn_actor_get_alpha();
    bool saturn_actor_has_custom_anim_extra();
    float saturn_actor_get_shadow_scale();
    bool saturn_actor_is_hidden();
    void saturn_actor_bone_override_begin();
    bool saturn_actor_bone_should_override();
    void saturn_actor_bone_iterate();
    void saturn_actor_bone_iterate_back();
    void saturn_actor_bone_do_override(Vec3s rotation);
    int saturn_actor_get_support_flags(int marioIndex);
    void saturn_actor_start_recording(int index);
    void saturn_actor_stop_recording();
    bool saturn_actor_is_recording_input();
    void saturn_actor_record_new_frame();
    struct Object* saturn_actor_get_object(int index);
    void saturn_actor_update_all();

    void saturn_actor_add_model_texture(char* id, char* data, int w, int h);
    char* saturn_actor_get_model_texture(char* id, int* w, int* h);
#ifdef __cplusplus
}
#endif

#define ACTOR_SWITCH_EYE     0
#define ACTOR_SWITCH_CAP     1
#define ACTOR_SWITCH_HAND    2
#define ACTOR_SWITCH_POWERUP 3

#endif