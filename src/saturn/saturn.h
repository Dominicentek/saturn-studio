#ifndef Saturn
#define Saturn

#include <stdio.h>
#include <PR/ultratypes.h>
#include <stdbool.h>
#include <mario_animation_ids.h>
#include <SDL2/SDL.h>
#include "types.h"
#include "saturn/saturn_animations.h"

#define MOUSEBTN_MASK_L (1 << 0)
#define MOUSEBTN_MASK_R (1 << 1)
#define MOUSEBTN_MASK_M (1 << 2)

extern bool mario_exists;

extern bool camera_frozen;
extern float camera_speed;
extern float camera_focus;
extern float camera_savestate_mult;
extern bool camera_fov_smooth;
extern bool is_camera_moving;

extern bool camera_view_enabled;
extern bool camera_view_moving;
extern bool camera_view_zooming;
extern bool camera_view_rotating;
extern int camera_view_move_x;
extern int camera_view_move_y;

extern int current_eye_state;

extern bool enable_head_rotations;
extern bool enable_shadows;
extern bool rainbow;
extern bool enable_dust_particles;
extern bool enable_torso_rotation;
extern bool enable_fog;
extern float run_speed;
extern bool can_fall_asleep;
extern int saturnModelState;
extern bool linkMarioScale;
extern bool is_spinning;
extern float spin_mult;

struct AnimationState {
    bool custom;
    int id;
    float frame;
    int length;
    int customanim_numindices;
    bool customanim_extra;
    int yTransform;
#ifdef __cplusplus
    std::vector<s16> customanim_indices;
    std::vector<s16> customanim_values;
    std::string customanim_name;
    std::string customanim_author;
#else
    u8 padding[32 * 2 + 24 * 2]; // std::vector has 32 bytes, std::string has 24 bytes
#endif
};

struct MouseState {
    int x;
    int y;
    int x_diff;
    int y_diff;
    int x_orig;
    int y_orig;
    int held;
    int pressed;
    int released;
    float scrollwheel;
    float dist_travelled;
    bool update_camera;
    bool focused_on_game;
};

extern bool using_chainer;
extern int chainer_index;
extern enum MarioAnimID selected_animation;
extern int current_anim_frame;
extern int current_anim_id;
extern int current_anim_length;
extern bool is_anim_playing;
extern bool is_anim_paused;
extern struct AnimationState current_animation;

extern float this_face_angle;

extern bool limit_fps;
extern bool has_discord_init;

extern unsigned int chromaKeyColorR;
extern unsigned int chromaKeyColorG;
extern unsigned int chromaKeyColorB;

extern u16 gChromaKeyColor;
extern u16 gChromaKeyBackground;

extern int mcam_timer;

extern SDL_Scancode saturn_key_to_scancode(unsigned int key[]);

extern bool autoChroma;
extern bool autoChromaLevel;
extern bool autoChromaObjects;

extern u8 activatedToads;

extern u8 godmode_temp_off;

extern f32 mario_headrot_yaw;
extern f32 mario_headrot_pitch;
extern f32 mario_headrot_speed;

extern bool keyframe_playing;
extern int k_previous_frame;

extern bool extract_thread_began;
extern bool extraction_finished;
extern float extraction_progress;

extern struct MouseState mouse_state;

extern struct Object* saturn_camera_object;

extern bool setting_mario_struct_pos;

extern struct Object (*world_simulation_data)[960];
extern int world_simulation_frames;
extern float world_simulation_curr_frame;
extern u16 world_simulation_seed;

extern bool simulating_world;

#ifdef __cplusplus
#include <string>
#include <vector>
#include <map>

enum InterpolationCurve {
    LINEAR,
    SLOW,
    FAST,
    SMOOTH,
    WAIT
};
enum KeyframeType {
    KFTYPE_FLOAT,
    KFTYPE_BOOL,
    KFTYPE_ANIM,
    KFTYPE_EXPRESSION,
    KFTYPE_COLOR,
    KFTYPE_COLORF,
    KFTYPE_SWITCH,
};

inline std::string curveNames[] = {
    "Linear",
    "Start Slow",
    "Start Fast",
    "Smooth",
    "Hold"
};

struct Keyframe {
    std::vector<float> value;
    InterpolationCurve curve;
    int position;
    std::string timelineID;
};

struct KeyframeTimeline {
    void* dest = nullptr;
    KeyframeType type;
    std::string name;
    int precision;
    int numValues;
    char behavior;
    int marioIndex;
};

#define KFBEH_DEFAULT 0
#define KFBEH_FORCE_WAIT 1

extern bool k_popout_open;
extern bool k_popout_focused;
extern float* active_key_float_value;
extern bool* active_key_bool_value;
extern s32 active_data_type;
extern int k_current_frame;
extern int k_curr_curve_type;

extern std::map<std::string, std::pair<KeyframeTimeline, std::vector<Keyframe>>> k_frame_keys;

extern int k_last_passed_index;
extern int k_distance_between;
extern int k_current_distance;
extern float k_static_increase_value;
extern int k_last_placed_frame;
extern bool k_loop;
extern bool k_animating_camera;

extern bool is_cc_editing;

extern int autosaveDelay;

extern Vec3f stored_mario_pos;
extern Vec3s stored_mario_angle;

extern bool inprec_keep_angle;

extern void saturn_clear_simulation();
extern void saturn_simulate(int);
extern void saturn_copy_camera(bool);
extern void saturn_paste_camera(void);
extern void* saturn_keyframe_get_timeline_ptr(KeyframeTimeline&);
extern bool saturn_keyframe_apply(std::string, int);
extern bool saturn_keyframe_matches(std::string, int);
extern void saturn_create_keyframe(std::string id, InterpolationCurve curve);
extern void saturn_place_keyframe(std::string id, int frame);

extern void schedule_animation();

extern "C" {
#endif
    void saturn_add_alloc_dl(Gfx* gfx);
    void saturn_free_alloc_dl();
    void saturn_update(void);
    void saturn_play_animation(enum MarioAnimID anim);
    void saturn_play_keyframe();
    int  saturn_keyframe_get_length();
    void saturn_print(const char*);
    const char* saturn_get_stage_name(int);
    bool saturn_do_load();
    void saturn_on_splash_finish();
    bool saturn_timeline_exists(const char*);
    bool saturn_begin_extract_rom_thread();
    int saturn_splash_screen_open();
#ifdef __cplusplus
}
#endif

#endif
