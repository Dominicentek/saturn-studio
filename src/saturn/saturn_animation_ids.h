#ifndef SaturnAnimationIds
#define SaturnAnimationIds

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <string>
#include <vector>
#include <map>
#include <functional>

#include <mario_animation_ids.h>
#include <model_ids.h>
#include <types.h>
#include "saturn_animations.h"

#include "actors/toad/anims/data.inc.c"
#include "actors/bowser/anims/data.inc.c"
#include "actors/peach/anims/data.inc.c"
#include "actors/penguin/anims/data.inc.c"
#include "actors/yoshi/anims/data.inc.c"
#include "actors/mips/anims/data.inc.c"
#include "actors/dorrie/anims/data.inc.c"
#include "actors/hoot/anims/data.inc.c"
#include "actors/butterfly/anims/data.inc.c"
#include "actors/blue_fish/anims/data.inc.c"
#include "actors/door/anims/data.inc.c"
#include "actors/goomba/anims/data.inc.c"
#include "actors/koopa/anims/data.inc.c"
#include "actors/flyguy/anims/data.inc.c"
#include "actors/piranha_plant/anims/data.inc.c"
#include "actors/bully/anims/data.inc.c"
#include "actors/chillychief/anims/data.inc.c"
#include "actors/lakitu_cameraman/anims/data.inc.c"
#include "actors/lakitu_enemy/anims/data.inc.c"
#include "actors/spiny/anims/data.inc.c"
#include "actors/bobomb/anims/data.inc.c"
#include "actors/king_bobomb/anims/data.inc.c"
#include "actors/whomp/anims/data.inc.c"
#include "actors/scuttlebug/anims/data.inc.c"
#include "actors/chuckya/anims/data.inc.c"
#include "actors/spindrift/anims/data.inc.c"
#include "actors/ukiki/anims/data.inc.c"
#include "actors/chain_chomp/anims/data.inc.c"
#include "actors/unagi/anims/data.inc.c"
#include "actors/bub/anims/data.inc.c"
#include "actors/sushi/anims/data.inc.c"
#include "actors/manta/anims/data.inc.c"
#include "actors/skeeter/anims/data.inc.c"
#include "actors/bookend/anims/data.inc.c"
#include "actors/mad_piano/anims/data.inc.c"
#include "actors/monty_mole/anims/data.inc.c"
#include "actors/moneybag/anims/data.inc.c"
#include "actors/swoop/anims/data.inc.c"
#include "actors/heave_ho/anims/data.inc.c"
#include "actors/amp/anims/data.inc.c"
#include "actors/klepto/anims/data.inc.c"

static struct Animation mario_anim_id(const void* addr) {
    struct Animation anim;
    load_animation(&anim, (uintptr_t)addr);
    return anim;
}

static struct Animation from_addr(const void* addr) {
    return *(struct Animation*)addr;
}

#define MODEL(id, name, anims) anims
#define MARIO_ANIM(name) { (void*)MARIO_ANIM_##name, mario_anim_id },
#define MODEL_ANIM(name, addr) { &addr, from_addr },
#define INITIAL_MARIO_ANIM(name) MARIO_ANIM(name)
#define INITIAL_MODEL_ANIM(name, addr) MODEL_ANIM(name, addr)
#define NO_ANIMS()
#define SWITCHES(switches...)
#define SWITCHES_ANIM(frames)
#define SWITCHES_EYES() SWITCHES("Eyes Open", "Eyes Closed")
#define ANIM_CATEGORY(label)
static std::vector<std::pair<const void*, std::function<struct Animation(const void*)>>> saturn_animation_data = {
#include "saturn_animation_def.h"
};

#undef MARIO_ANIM
#undef MODEL_ANIM
#define MARIO_ANIM(name) #name,
#define MODEL_ANIM(name, addr) name,

static std::vector<std::string> saturn_animation_names = {
#include "saturn_animation_def.h"
};

static int __saturn_anim_counter = 0;
static std::pair<int, int> __get_anim_counter_pair(int len) {
    std::pair<int, int> pair = {};
    pair.first  = __saturn_anim_counter;
    pair.second = __saturn_anim_counter += len;
    return pair;
}

#undef MODEL
#undef MARIO_ANIM
#undef MODEL_ANIM
#define MODEL(id, name, anims) { id, __get_anim_counter_pair(0 anims) },
#define MARIO_ANIM(name) +1
#define MODEL_ANIM(name, addr) +1
static std::map<int, std::pair<int, int>> saturn_animation_obj_ranges = {
#include "saturn_animation_def.h"
};

#undef MODEL
#undef MARIO_ANIM
#undef MODEL_ANIM
#define MODEL(id, name, anims) { id, name },
static std::map<int, std::string> saturn_object_names = {
#include "saturn_animation_def.h"
};

#undef MODEL
#define MODEL(id, name, anims) id,
static std::vector<int> saturn_iterable_obj_list = {
#include "saturn_animation_def.h"
};

#define INC2(x)
#define INC1(x) INC2(x)
#define INC     INC1(__COUNTER__)

#undef MODEL
#undef INITIAL_MARIO_ANIM
#undef INITIAL_MODEL_ANIM
#undef NO_ANIMS
#undef ANIM_CATEGORY
#define MODEL(id, name, anims) { id, { { "_", {anims } } } },
#define MARIO_ANIM(name) __COUNTER__,
#define MODEL_ANIM(name, addr) __COUNTER__,
#define INITIAL_MARIO_ANIM(name) __COUNTER__,
#define INITIAL_MODEL_ANIM(name, addr) __COUNTER__,
#define ANIM_CATEGORY(name) }}, { name, {
#define NO_ANIMS()
static std::map<int, std::map<std::string, std::vector<int>>> saturn_animation_categories = {
#include "saturn_animation_def.h"
};

static int __curr_count_1 = __COUNTER__;

#undef MODEL
#undef MARIO_ANIM
#undef MODEL_ANIM
#undef INITIAL_MARIO_ANIM
#undef INITIAL_MODEL_ANIM
#undef NO_ANIMS
#undef ANIM_CATEGORY
#define MODEL(id, name, anims) { id, anims },
#define MARIO_ANIM(name) INC
#define MODEL_ANIM(name, addr) INC
#define INITIAL_MARIO_ANIM(name) __COUNTER__ - __curr_count_1 - 1
#define INITIAL_MODEL_ANIM(name, addr) __COUNTER__ - __curr_count_1 - 1
#define NO_ANIMS() -1
#define ANIM_CATEGORY(name)
static std::map<int, int> saturn_obj_initial_anims = {
#include "saturn_animation_def.h"
};

#undef MODEL
#undef MARIO_ANIM
#undef MODEL_ANIM
#undef INITIAL_MARIO_ANIM
#undef INITIAL_MODEL_ANIM
#undef INC
#undef INC1
#undef INC2
#undef SWITCHES
#undef SWITCHES_ANIM
#undef NO_ANIMS

#define MODEL(id, name, anims) anims if (!curr_switches.empty()) switches.insert({ id, curr_switches }); curr_switches.clear();
#define MARIO_ANIM(name)
#define MODEL_ANIM(name, addr)
#define INITIAL_MARIO_ANIM(name)
#define INITIAL_MODEL_ANIM(name, addr)
#define SWITCHES(switches...) for (std::string entry : { switches }) { curr_switches.push_back(entry); }
#define SWITCHES_ANIM(frames) curr_switches.push_back("__ANIM_SWITCH_" + std::to_string(frames));
#define NO_ANIMS()
static std::map<int, std::vector<std::string>> __saturn_mk_switches() {
    std::map<int, std::vector<std::string>> switches = {};
    std::vector<std::string> curr_switches = {};
#include "saturn_animation_def.h"
    return switches;
}
#undef MODEL
#undef MARIO_ANIM
#undef MODEL_ANIM
#undef INITIAL_MARIO_ANIM
#undef INITIAL_MODEL_ANIM
#undef SWITCHES
#undef SWITCHES_EYES
#undef SWITCHES_ANIM
#undef NO_ANIMS

static std::map<int, std::vector<std::string>> saturn_obj_switches = __saturn_mk_switches();

#endif

#endif