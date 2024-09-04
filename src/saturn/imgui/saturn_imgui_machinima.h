#ifndef SaturnImGuiMachinima
#define SaturnImGuiMachinima

#include "SDL2/SDL.h"
#include "include/types.h"

extern float gravity;
extern int time_freeze_state;
extern int current_sanim_id;

extern bool enabled_acts[];
extern s16 levelList[];

extern bool override_level;
extern struct GraphNode* override_level_geolayout;
extern Collision* override_level_collision;

extern void anim_play_button();
extern void saturn_create_object(int, const BehaviorScript*, float, float, float, s16, s16, s16, int);

#ifdef __cplusplus

#include "saturn/saturn_actors.h"
extern void imgui_machinima_animation_player(MarioActor* actor, bool sampling = false);

extern bool case_insensitive_contains(std::string base, std::string substr);

extern "C" {
#endif

    void warp_to_level(int, int, int);
    int get_saturn_level_id(int);
    void smachinima_imgui_init(void);
    void smachinima_imgui_controls(SDL_Event * event);
    Gfx* geo_do_override_level_geolayout(s32 callContext, struct GraphNode *node, UNUSED Mat4 *mtx);
    Gfx* geo_switch_override_model(s32 callContext, struct GraphNode *node, UNUSED Mat4 *mtx);

    void imgui_machinima_quick_options(void);
#ifdef __cplusplus
}
#endif

#endif