#include "saturn/saturn_timelines.h"
#include "saturn/saturn.h"
#include "saturn/imgui/saturn_imgui.h"

#define DEFAULT 0
#define FORCE_WAIT 1
#define EVENT 2

#define SATURN_KFENTRY_BOOL(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_BOOL, KFBEH_FORCE_WAIT, name, 0, 1, mario } })
#define SATURN_KFENTRY_FLOAT(id, variable, values, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_FLOAT, KFBEH_DEFAULT, name, -3, values, mario } })
#define SATURN_KFENTRY_ANIM(id, name) timelineDataTable.insert({ id, { MARIO_ENTRY(animstate), KFTYPE_ANIM, KFBEH_FORCE_WAIT, name, 0, 2, true } })
#define SATURN_KFENTRY_EXPRESSION(id, name) timelineDataTable.insert({ id, { MARIO_ENTRY(model), KFTYPE_EXPRESSION, KFBEH_FORCE_WAIT, name, 0, 1, true } })
#define SATURN_KFENTRY_COLOR(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_COLOR, KFBEH_DEFAULT, name, 0, 6, mario } })
#define SATURN_KFENTRY_COLORF(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_COLORF, KFBEH_DEFAULT, name, -3, 3, mario } })
#define SATURN_KFENTRY_SWITCH(id, variable, name) timelineDataTable.insert({ id, { variable, KFTYPE_SWITCH, KFBEH_FORCE_WAIT, name, 0, 1, true } })

std::map<std::string, std::tuple<void*, KeyframeType, char, std::string, int, int, bool>> timelineDataTable = {};
std::map<std::string, std::vector<std::string>> kf_switch_names = {
    { "k_switch_eyes", { "Blinking", "Open", "Half", "Closed", "Left", "Right", "Up", "Down", "Dead" } },
    { "k_switch_hand", { "Fists", "Open", "Peace", "With Cap", "With Wing Cap", "Right Open" } },
    { "k_switch_cap", { "Cap On", "Cap Off", "Wing Cap" } },
    { "k_switch_powerup", { "Default", "Vanish", "Metal", "Metal & Vanish" } },
};

#define MARIO_ENTRY(var) (void*)offsetof(MarioActor, var)
#define ARR_ENTRY(arr, index) (void*)(offsetof(MarioActor, arr) + (index) * sizeof(*((MarioActor*)0)->arr))
#define CC_ENTRY(index) ARR_ENTRY(colorcode, index)
#define BONE_ENTRY(index) ARR_ENTRY(bones, index)
#define SCALER_ENTRY(index) ARR_ENTRY(scaler, index)
#define OBJ_BONE(index) SATURN_KFENTRY_FLOAT("k_objbone_" #index, BONE_ENTRY(index + 1), 3, "Bone " #index, true);

// { id, { variable_ptr, type, behavior, name, precision, num_values, is_mario } }
void saturn_fill_data_table() {
    SATURN_KFENTRY_BOOL("k_skybox_mode", &use_color_background, "Skybox Mode", false);
    SATURN_KFENTRY_BOOL("k_shadows", &enable_shadows, "Shadows", false);
    SATURN_KFENTRY_FLOAT("k_shade_x", &world_light_dir1, 1, "Shade X", false);
    SATURN_KFENTRY_FLOAT("k_shade_y", &world_light_dir2, 1, "Shade Y", false);
    SATURN_KFENTRY_FLOAT("k_shade_z", &world_light_dir3, 1, "Shade Z", false);
    SATURN_KFENTRY_FLOAT("k_shade_t", &world_light_dir4, 1, "Shade Tex", false);
    SATURN_KFENTRY_FLOAT("k_scale", MARIO_ENTRY(xScale), 3, "Scale", true);
    SATURN_KFENTRY_FLOAT("k_rh_scale", SCALER_ENTRY(0), 3, "Right Hand Scale", true);
    SATURN_KFENTRY_FLOAT("k_lh_scale", SCALER_ENTRY(1), 3, "Left Hand Scale", true);
    SATURN_KFENTRY_FLOAT("k_rf_scale", SCALER_ENTRY(2), 3, "Right Foot Scale", true);
    SATURN_KFENTRY_BOOL("k_v_cap_emblem", MARIO_ENTRY(show_emblem), "M Cap Emblem", true);
    SATURN_KFENTRY_FLOAT("k_angle", MARIO_ENTRY(angle), 1, "Mario Angle", true);
    SATURN_KFENTRY_FLOAT("k_mariopos_x", MARIO_ENTRY(x), 1, "Mario Pos X", true);
    SATURN_KFENTRY_FLOAT("k_mariopos_y", MARIO_ENTRY(y), 1, "Mario Pos Y", true);
    SATURN_KFENTRY_FLOAT("k_mariopos_z", MARIO_ENTRY(z), 1, "Mario Pos Z", true);
    SATURN_KFENTRY_FLOAT("k_shadow_scale", MARIO_ENTRY(shadow_scale), 1, "Shadow Scale", true);
    SATURN_KFENTRY_FLOAT("k_headrot_x", MARIO_ENTRY(head_rot_x), 1, "Head Rotation Yaw", true);
    SATURN_KFENTRY_FLOAT("k_headrot_y", MARIO_ENTRY(head_rot_y), 1, "Head Rotation Roll", true);
    SATURN_KFENTRY_FLOAT("k_headrot_z", MARIO_ENTRY(head_rot_z), 1, "Head Rotation Pitch", true);
    SATURN_KFENTRY_FLOAT("k_inputrec_frame", MARIO_ENTRY(input_recording_frame), 1, "Input Playback Frame", true);
    SATURN_KFENTRY_FLOAT("k_anim_frame", MARIO_ENTRY(anim_state), 1, "Anim Frame", true);
    SATURN_KFENTRY_SWITCH("k_anim_state", MARIO_ENTRY(anim_state), "Anim State");
    SATURN_KFENTRY_BOOL("k_inputrec_enable", MARIO_ENTRY(playback_input), "Input Playback Enable", true);
    SATURN_KFENTRY_BOOL("k_mario_hidden", MARIO_ENTRY(hidden), "Hidden", true);
    SATURN_KFENTRY_BOOL("k_hud", &configHUD, "HUD", false);
    SATURN_KFENTRY_BOOL("k_r_dls", &rainbow, "Rainbow", false);
    SATURN_KFENTRY_FLOAT("k_fov", &camera_fov, 1, "FOV", false);
    SATURN_KFENTRY_FLOAT("k_focus", &camera_focus, 1, "Follow", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pos0", &freezecamPos[0], 1, "Camera Pos X", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pos1", &freezecamPos[1], 1, "Camera Pos Y", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pos2", &freezecamPos[2], 1, "Camera Pos Z", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_yaw", &freezecamYaw, 1, "Camera Yaw", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pitch", &freezecamPitch, 1, "Camera Pitch", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_roll", &freezecamRoll, 1, "Camera Roll", false);
    SATURN_KFENTRY_FLOAT("k_gravity", &gravity, 1, "Gravity", false);
    SATURN_KFENTRY_FLOAT("k_mariostruct_x", gMarioState->pos + 0, 1, "Mario Struct X", false);
    SATURN_KFENTRY_FLOAT("k_mariostruct_y", gMarioState->pos + 1, 1, "Mario Struct Y", false);
    SATURN_KFENTRY_FLOAT("k_mariostruct_z", gMarioState->pos + 2, 1, "Mario Struct Z", false);
    SATURN_KFENTRY_FLOAT("k_mariostruct_angle", &gMarioState->fAngle, 1, "Mario Struct Angle", false);
    SATURN_KFENTRY_FLOAT("k_orthoscale", &ortho_settings.scale, 1, "Ortho Scale", false);
    SATURN_KFENTRY_FLOAT("k_orthoyaw", &ortho_settings.rotation_y, 1, "Ortho Yaw", false);
    SATURN_KFENTRY_FLOAT("k_orthopitch", &ortho_settings.rotation_x, 1, "Ortho Pitch", false);
    SATURN_KFENTRY_FLOAT("k_orthox", &ortho_settings.offset_x, 1, "Ortho Off X", false);
    SATURN_KFENTRY_FLOAT("k_orthoy", &ortho_settings.offset_y, 1, "Ortho Off Y", false);
    SATURN_KFENTRY_FLOAT("k_worldsim_frame", &world_simulation_curr_frame, 1, "Simulation Frame", false);
    SATURN_KFENTRY_COLORF("k_light_col", gLightingColor, "Light Color", false);
    SATURN_KFENTRY_COLOR("k_color", &chromaColor, "Skybox Color", false);
    SATURN_KFENTRY_COLOR("k_hat", CC_ENTRY(CC_HAT), "Hat", true);
    SATURN_KFENTRY_COLOR("k_overalls", CC_ENTRY(CC_OVERALLS), "Overalls", true);
    SATURN_KFENTRY_COLOR("k_gloves", CC_ENTRY(CC_GLOVES), "Gloves", true);
    SATURN_KFENTRY_COLOR("k_shoes", CC_ENTRY(CC_SHOES), "Shoes", true);
    SATURN_KFENTRY_COLOR("k_skin", CC_ENTRY(CC_SKIN), "Skin", true);
    SATURN_KFENTRY_COLOR("k_hair", CC_ENTRY(CC_HAIR), "Hair", true);
    SATURN_KFENTRY_COLOR("k_shirt", CC_ENTRY(CC_SHIRT), "Shirt", true);
    SATURN_KFENTRY_COLOR("k_shoulders", CC_ENTRY(CC_SHOULDERS), "Shoulders", true);
    SATURN_KFENTRY_COLOR("k_arms", CC_ENTRY(CC_ARMS), "Arms", true);
    SATURN_KFENTRY_COLOR("k_overalls_bottom", CC_ENTRY(CC_OVERALLS_BOTTOM), "Overalls (Bottom)", true);
    SATURN_KFENTRY_COLOR("k_leg_top", CC_ENTRY(CC_LEG_TOP), "Leg (Top)", true);
    SATURN_KFENTRY_COLOR("k_leg_bottom", CC_ENTRY(CC_LEG_BOTTOM), "Leg (Bottom)", true);
    SATURN_KFENTRY_ANIM("k_mario_anim", "Animation");
    SATURN_KFENTRY_FLOAT("k_mario_anim_frame", MARIO_ENTRY(animstate.frame), 1, "Anim Frame", true);
    SATURN_KFENTRY_EXPRESSION("k_mario_expr", "Expression");
    SATURN_KFENTRY_SWITCH("k_switch_eyes", MARIO_ENTRY(eye_state), "Eye Switch");
    SATURN_KFENTRY_SWITCH("k_switch_hand", MARIO_ENTRY(hand_state), "Hand Switch");
    SATURN_KFENTRY_SWITCH("k_switch_cap", MARIO_ENTRY(cap_state), "Cap Switch");
    SATURN_KFENTRY_SWITCH("k_switch_powerup", MARIO_ENTRY(powerup_state), "Powerup Switch");
    SATURN_KFENTRY_BOOL("k_customeyes", MARIO_ENTRY(custom_eyes), "Custom Eyes", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_t", BONE_ENTRY(0), 3, "Translation", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_1", BONE_ENTRY(1), 3, "Root", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_2", BONE_ENTRY(2), 3, "Body", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_3", BONE_ENTRY(3), 3, "Torso", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_4", BONE_ENTRY(4), 3, "Head", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_5", BONE_ENTRY(5), 3, "Left Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_6", BONE_ENTRY(6), 3, "Upper Left Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_7", BONE_ENTRY(7), 3, "Lower Left Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_8", BONE_ENTRY(9), 3, "Left Hand", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_9", BONE_ENTRY(9), 3, "Right Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_10", BONE_ENTRY(10), 3, "Upper Right Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_11", BONE_ENTRY(11), 3, "Lower Right Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_12", BONE_ENTRY(12), 3, "Right Hand", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_13", BONE_ENTRY(13), 3, "Left Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_14", BONE_ENTRY(14), 3, "Upper Left Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_15", BONE_ENTRY(15), 3, "Lower Left Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_16", BONE_ENTRY(16), 3, "Left Foot", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_17", BONE_ENTRY(17), 3, "Right Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_18", BONE_ENTRY(18), 3, "Upper Right Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_19", BONE_ENTRY(19), 3, "Lower Right Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_20", BONE_ENTRY(20), 3, "Right Foot", true);
    SATURN_KFENTRY_FLOAT("k_objbone_t", BONE_ENTRY(0), 3, "Translation", true);
    SATURN_KFENTRY_FLOAT("k_objbone_0", BONE_ENTRY(1), 3, "Root", true);
    OBJ_BONE(1);
    OBJ_BONE(2);
    OBJ_BONE(3);
    OBJ_BONE(4);
    OBJ_BONE(5);
    OBJ_BONE(6);
    OBJ_BONE(7);
    OBJ_BONE(8);
    OBJ_BONE(9);
    OBJ_BONE(10);
    OBJ_BONE(11);
    OBJ_BONE(12);
    OBJ_BONE(13);
    OBJ_BONE(14);
    OBJ_BONE(15);
    OBJ_BONE(16);
    OBJ_BONE(17);
    OBJ_BONE(18);
    OBJ_BONE(19);
    OBJ_BONE(20);
    OBJ_BONE(21);
    OBJ_BONE(22);
    OBJ_BONE(23);
    OBJ_BONE(24);
    OBJ_BONE(25);
    OBJ_BONE(26);
    OBJ_BONE(27);
    OBJ_BONE(28);
    OBJ_BONE(29);
    OBJ_BONE(30);
    OBJ_BONE(31);
    OBJ_BONE(32);
    OBJ_BONE(33);
    OBJ_BONE(34);
    OBJ_BONE(35);
    OBJ_BONE(36);
    OBJ_BONE(37);
    OBJ_BONE(38);
    OBJ_BONE(39);
    OBJ_BONE(40);
    OBJ_BONE(41);
    OBJ_BONE(42);
    OBJ_BONE(43);
    OBJ_BONE(44);
    OBJ_BONE(45);
    OBJ_BONE(46);
    OBJ_BONE(47);
    OBJ_BONE(48);
    OBJ_BONE(49);
    OBJ_BONE(50);
    OBJ_BONE(51);
    OBJ_BONE(52);
    OBJ_BONE(53);
    OBJ_BONE(54);
    OBJ_BONE(55);
    OBJ_BONE(56);
    OBJ_BONE(57);
    OBJ_BONE(58);
    OBJ_BONE(59);
    OBJ_BONE(60);
}