#include "saturn_imgui_dynos.h"

#include "saturn/imgui/saturn_imgui_settings.h"
#include "saturn/libs/portable-file-dialogs.h"

#include <algorithm>
#include <string>
#include <cstring>
#include <iostream>
#include "GL/glew.h"
#include "saturn/saturn_actors.h"
#include "saturn/saturn_animation_ids.h"
#include "types.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdlib>
#endif

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/libs/imgui/imgui-knobs.h"
#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn_imgui.h"
#include "saturn_imgui_cc_editor.h"
#include "saturn_imgui_expressions.h"
#include "saturn_imgui_machinima.h"
#include "pc/controller/controller_keyboard.h"
#include "data/dynos.cpp.h"
#include <SDL2/SDL.h>

#include "icons/IconsForkAwesome.h"

extern "C" {
#include "pc/gfx/gfx_pc.h"
#include "pc/configfile.h"
#include "pc/platform.h"
#include "game/mario.h"
#include "game/level_update.h"
#include <mario_animation_ids.h>
#include "pc/cheats.h"
#include "include/sm64.h"
#include "game/sound_init.h"
#include "engine/surface_collision.h"
}

Array<PackData *> &dynos_packs = DynOS_Gfx_GetPacks();

using namespace std;

int current_eye_state = 0;
int current_eye_id = 0;
string eye_name;
int current_mouth_id = 0;
string mouth_name;

int windowListSize = 325;

bool using_custom_eyes;

static char modelSearchTerm[128];

bool has_copy_mario;

bool last_model_had_model_eyes;

bool is_gameshark_open;

std::vector<std::string> choose_file_dialog(std::string windowTitle, std::vector<std::string> filetypes, bool multiselect) {
    return pfd::open_file(windowTitle, ".", filetypes, multiselect ? pfd::opt::multiselect : pfd::opt::none).result();
}

std::string save_file_dialog(std::string windowTitle, std::vector<std::string> filetypes) {
    return pfd::save_file(windowTitle, ".", filetypes, pfd::opt::none).result();
}

void open_file(std::string path) {
#if defined(_WIN32) // Windows
    ShellExecuteA(NULL, "open", ("\"" + path + "\"").c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__) // macOS
    system(("open \"" + path + "\"").c_str());
#else // Linux
    system(("xdg-open \"" + path + "\"").c_str());
#endif
}

void saturn_show_crash_dialog() {
    if (pfd::message("Saturn Studio",
        "Saturn Studio has crashed.\n"
        "\n"
        "Join the Saturn Discord server\n"
        "https://discord.gg/rGqREG2kYv\n"
        "to resolve the issue with the developers.\n"
        "\n"
        "Open the log file?",
        pfd::choice::yes_no, pfd::icon::warning
    ).result() == pfd::button::yes) {
        open_file(std::string(sys_user_path()) + "/latest.log");
    }
}

// UI

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

GLuint global_expression_preview = 0;
int global_expression_preview_width = 0;
int global_expression_preview_height = 0;
int global_sample_ratio;
string last_preview_name;

void expression_preview(const char* filename) {
    global_expression_preview = 0;
    bool ret = LoadTextureFromFile(filename, &global_expression_preview, &global_expression_preview_width, &global_expression_preview_height);
    IM_ASSERT(ret);
    global_sample_ratio = global_expression_preview_width / global_expression_preview_height;
    //if (global_expression_preview_height > 16 && global_expression_preview_width > 16) {
    //    global_sample_ratio = (256 / global_expression_preview_height);
    //}
}

void OpenModelSelector(MarioActor* actor) {
    CCChangeActor(actor);
    ImGui::Text("Model Packs");
    ImGui::SameLine(); imgui_bundled_help_marker(
        "DynOS v1.1 by PeachyPeach\n\nThese are DynOS model packs, used for live model loading.\nPlace packs in /dynos/packs.");

    // Model search when our list is 20 or more
    if (model_list.size() >= 20) {
        ImGui::InputTextWithHint("###model_search_text", ICON_FK_SEARCH " Search models...", modelSearchTerm, IM_ARRAYSIZE(modelSearchTerm), ImGuiInputTextFlags_AutoSelectAll);
    } else {
        // If our model list is reloaded, and we now have less than 20 packs, this can cause filter issues if not reset to nothing
        if (modelSearchTerm != "") strcpy(modelSearchTerm, "");
    }
    std::string modelSearchLower = modelSearchTerm;
    std::transform(modelSearchLower.begin(), modelSearchLower.end(), modelSearchLower.begin(),
        [](unsigned char c){ return std::tolower(c); });

    if (model_list.size() <= 0) {
        ImGui::TextDisabled("No model packs found.\nPlace model folders in\n/dynos/packs/.");
    } else {
        ImGui::BeginChild("###menu_model_selector", ImVec2(-FLT_MIN, 125), true);
        for (int i = 0; i < model_list.size(); i++) {
            if (model_list[i].Type != "mario") continue;
            Model model = model_list[i];
            if (model.Active) {
                bool is_selected = actor->selected_model == i;

                // If we're searching, only include CCs with the search keyword in the name
                // Also convert to lowercase
                std::string label_name_lower = model.SearchMeta();
                std::transform(label_name_lower.begin(), label_name_lower.end(), label_name_lower.begin(),
                        [](unsigned char c1){ return std::tolower(c1); });
                if (modelSearchLower != "") {
                    if (label_name_lower.find(modelSearchLower) == std::string::npos) {
                        continue;
                    }
                }

                std::string packLabelId = model.FolderName + "###s_model_pack_" + std::to_string(i);
                if (ImGui::Selectable(packLabelId.c_str(), &is_selected)) {
                    std::string timeline = saturn_keyframe_get_mario_timeline_id("k_mario_expr", saturn_actor_indexof(actor));
                    if (saturn_timeline_exists(timeline.c_str())) k_frame_keys.erase(timeline);
                    
                    // Select model
                    actor->model = model;
                    actor->selected_model = i;

                    // Load expressions
                    actor->model.Expressions.clear();
                    actor->model.Expressions = LoadExpressions(&actor->model, actor->model.FolderPath);

                    if (is_selected) {
                        std::cout << "Loaded " << model.Name << " by " << model.Author << std::endl;

                        // Load model CCs
                        RefreshColorCodeList();
                        if (is_selected && (current_color_code.IsDefaultColors() || last_model_cc_address == current_color_code.GameShark)) {
                            ResetColorCode(true);
                            last_model_cc_address = current_color_code.GameShark;
                        }
                    } else {
                        actor->model = Model();
                        actor->selected_model = -1;
                        
                        // Reset model CCs
                        model_color_code_list.clear();
                        RefreshColorCodeList();
                        if (last_model_cc_address == current_color_code.GameShark)
                            ResetColorCode(false);
                    }
                }
                std::string popupLabelId = "###menu_model_popup_" + std::to_string(i);
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    ImGui::OpenPopup(popupLabelId.c_str());
                if (ImGui::BeginPopup(popupLabelId.c_str())) {
                    // Right-click menu
                    if (model.Author.empty()) ImGui::Text(ICON_FK_FOLDER_O " %s/", model.FolderName.c_str());
                    else ImGui::Text(ICON_FK_FOLDER " %s/", model.FolderName.c_str());

                    imgui_bundled_tooltip(("/%s", model.FolderPath).c_str());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                        open_file(std::string(sys_exe_path()) + "/" + model.FolderPath + "/");

                    ImGui::SameLine(); ImGui::TextDisabled(" Pack #%i", model.DynOSId + 1);

                    std::vector<std::string> cc_list = GetColorCodeList(model.FolderPath + "/colorcodes");
                    has_open_any_model_cc = true;

                    if (model.HasColorCodeFolder() && cc_list.size() > 0) {
                        // List of model color codes
                        ImGui::BeginChild("###menu_model_ccs", ImVec2(-FLT_MIN, 75), true);
                        OpenModelCCSelector(model, cc_list, "");
                        ImGui::EndChild();
                    }

                    ImGui::Separator();
                    ImGui::TextDisabled("%i model pack(s)", model_list.size());
                    if (ImGui::Button(ICON_FK_DOWNLOAD " Refresh Packs###refresh_dynos_packs")) {
                        dynos_packs.Clear();
                        DynOS_Opt_Init();
                        std::vector<std::string> model_names = {};
                        MarioActor* actor = gMarioActorList;
                        while (actor) {
                            if (actor->selected_model == -1) model_names.push_back("");
                            else model_names.push_back(model_list[actor->selected_model].FolderName);
                            actor = actor->next;
                        }
                        model_list = GetModelList("dynos/packs");
                        actor = gMarioActorList;
                        int iter = 0;
                        while (actor) {
                            std::string name = model_names[iter++];
                            if (name != "") {
                                bool cannot_find = true;
                                for (int i = 0; i < model_list.size(); i++) {
                                    if (model_list[i].FolderName == name) {
                                        actor->selected_model = i;
                                        actor->model = model_list[i];
                                        cannot_find = false;
                                        break;
                                    }
                                }
                                if (cannot_find) {
                                    actor->selected_model = -1;
                                    actor->model = Model();
                                }
                            }
                            actor = actor->next;
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine(); imgui_bundled_help_marker("WARNING: Experimental - this will probably lag the game.");
                    ImGui::EndPopup();
                }

                if (!model.Author.empty()) {
                    ImGui::SameLine();
                    ImGui::TextDisabled(model.Author.c_str());
                }
            }
        }
        ImGui::EndChild();

        // Open DynOS Folder Button

        if (ImGui::Button(ICON_FK_FOLDER_OPEN_O " Open Packs Folder...###open_packs_folder"))
            open_file(std::string(sys_exe_path()) + "/dynos/packs/");
    }
}

void sdynos_imgui_init() {
    model_list = GetModelList("dynos/packs");
    RefreshColorCodeList();

    //model_details = "" + std::to_string(sDynosPacks.Count()) + " model pack";
    //if (sDynosPacks.Count() != 1) model_details += "s";
}

bool link_scaling = true;

void sdynos_imgui_menu(int index) {
    MarioActor* actor = saturn_get_actor(index);
    if (actor == nullptr) return;
    
    if (ImGui::MenuItem(ICON_FK_DOWNLOAD " Drop to Floor")) {
        actor->y = find_floor_height(actor->x, actor->y + 100, actor->z);
    }

    if (actor->obj_model == MODEL_MARIO) {
        if (ImGui::BeginMenu(ICON_FK_USER_CIRCLE " Edit Avatar###menu_edit_avatar")) {
            // Color Code Selection
            if (!actor->cc_support || !actor->model.ColorCodeSupport) ImGui::BeginDisabled();
                OpenCCSelector(actor);
                // Open File Dialog
                if (ImGui::Button(ICON_FK_FILE_TEXT_O " Open CC Folder...###open_cc_folder"))
                    open_file(std::string(sys_exe_path()) + "/dynos/colorcodes/");
            if (!actor->cc_support || !actor->model.ColorCodeSupport) ImGui::EndDisabled();

            // Model Selection
            OpenModelSelector(actor);

            ImGui::EndMenu();
        }

        // Color Code Editor
        if (ImGui::BeginMenu(ICON_FK_PAINT_BRUSH " Color Code Editor###menu_cc_editor", actor->cc_support & actor->model.ColorCodeSupport)) {
            OpenCCEditor(actor);
            ImGui::EndMenu();
        }
    }

    // Animation Mixtape
    if (actor->num_bones != 0) if (ImGui::BeginMenu(ICON_FK_FILM " Animation Mixtape###menu_anim_player")) {
        imgui_machinima_animation_player(actor);
        ImGui::EndMenu();
    }
    
    if (saturn_obj_switches.find(actor->obj_model) != saturn_obj_switches.end()) {
        if (saturn_obj_switches[actor->obj_model][0].rfind("__ANIM_SWITCH", 0) == 0) {
            int num_frames = std::stoi(saturn_obj_switches[actor->obj_model][0].substr(14));
            int frame = actor->anim_state;
            ImGui::SliderInt("Anim Frame", &frame, 0, num_frames - 1, "%d", ImGuiSliderFlags_AlwaysClamp);
            actor->anim_state = frame;
            saturn_keyframe_popout("k_anim_frame");
        }
        else {
            if (ImGui::BeginCombo("Anim State", saturn_obj_switches[actor->obj_model][actor->anim_state].c_str())) {
                for (int i = 0; i < saturn_obj_switches[actor->obj_model].size(); i++) {
                    bool selected = actor->anim_state == i;
                    if (ImGui::Selectable(saturn_obj_switches[actor->obj_model][i].c_str(), selected)) actor->anim_state = i;
                }
                ImGui::EndCombo();
            }
            saturn_keyframe_popout("k_anim_state");
        }
    }

    if (actor->obj_model == MODEL_MARIO) {
        if (ImGui::BeginMenu(ICON_FK_FILM " Input Recording")) {
            bool empty = actor->input_recording.empty();
            if (empty) ImGui::Text("No recording made");
            if (ImGui::Button("Record")) {
                set_mario_action(gMarioState, ACT_IDLE, 0);
                saturn_actor_start_recording(index);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Text("%s to stop", translate_bind_to_name(configKeyStopInpRec[0]));
            ImGui::EndDisabled();
            ImGui::Checkbox("Keep Angle", &inprec_keep_angle);
            ImGui::Separator();
            if (empty) ImGui::BeginDisabled();
            bool checked = !empty && actor->playback_input;
            if (ImGui::Checkbox("Playback", &checked)) actor->playback_input = !actor->playback_input;
            saturn_keyframe_popout("k_inputrec_enable");
            if (!empty && !actor->playback_input) ImGui::BeginDisabled();
            ImGui::SliderFloat("Frame", &actor->input_recording_frame, 0, actor->input_recording.size() - 1, "%.0f");
            if (!empty && !actor->playback_input) ImGui::EndDisabled();
            saturn_keyframe_popout("k_inputrec_frame");
            std::string timelineID = saturn_keyframe_get_mario_timeline_id("k_inputrec_frame", saturn_actor_indexof(actor));
            if (saturn_timeline_exists(timelineID.c_str()))
                saturn_keyframe_helper(timelineID, &actor->input_recording_frame, actor->input_recording.size() - 1);
            if (empty) ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        ImGui::Separator();
    }

    if (!actor->model.ColorCodeSupport) ImGui::BeginDisabled();
        ImGui::Checkbox("Color Code Support", &actor->cc_support);
        imgui_bundled_tooltip(
            "Toggles color code features.");

        if (!actor->cc_support) ImGui::BeginDisabled();
            if (actor->model.SparkSupport) {
                ImGui::Checkbox("CometSPARK Support", &actor->spark_support);
                imgui_bundled_tooltip(
                    "Toggles SPARK features, which provides supported models with extra color values.");
            }
        if (!actor->cc_support) ImGui::EndDisabled();
    if (!actor->model.ColorCodeSupport) ImGui::EndDisabled();

    // Misc. Avatar Settings
    if (ImGui::BeginMenu("Customize...###menu_misc")) {

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("Misc.###misc_child", ImVec2(275, 175), true, ImGuiWindowFlags_None);
        if (ImGui::BeginTabBar("###misc_tabbar", ImGuiTabBarFlags_None)) {

            if (actor->obj_model == MODEL_MARIO) if (ImGui::BeginTabItem("Switches###switches_scale")) {
                const char* eyes[] = { "Blinking", "Open", "Half", "Closed", "Left", "Right", "Up", "Down", "Dead" };
                ImGui::Combo("Eyes###eye_state", &actor->eye_state, eyes, IM_ARRAYSIZE(eyes));
                saturn_keyframe_popout("k_switch_eyes");
                const char* hands[] = { "Fists", "Open", "Peace", "With Cap", "With Wing Cap", "Right Open" };
                ImGui::Combo("Hand###hand_state", &actor->hand_state, hands, IM_ARRAYSIZE(hands));
                saturn_keyframe_popout("k_switch_hand");
                const char* caps[] = { "Cap On", "Cap Off", "Wing Cap" }; // unused "wing cap off" not included
                ImGui::Combo("Cap###cap_state", &actor->cap_state, caps, IM_ARRAYSIZE(caps));
                saturn_keyframe_popout("k_switch_cap");
                const char* powerups[] = { "Default", "Vanish", "Metal", "Metal & Vanish" };
                ImGui::Combo("Powerup###powerup_state", &actor->powerup_state, powerups, IM_ARRAYSIZE(powerups));
                saturn_keyframe_popout("k_switch_powerup");
                if (actor->powerup_state & 1) ImGui::SliderFloat("Alpha", &actor->alpha, 0, 255);
                if (AnyModelsEnabled(actor)) ImGui::BeginDisabled();
                ImGui::Checkbox("M Cap Emblem", &actor->show_emblem);
                imgui_bundled_tooltip("Enables the signature \"M\" logo on Mario's cap.");
                saturn_keyframe_popout("k_v_cap_emblem");
                if (AnyModelsEnabled(actor)) ImGui::EndDisabled();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Special###tab_special")) {
                if (link_scaling) {
                    if (ImGui::SliderFloat("Size###mscale_all", &actor->xScale, 0.f, 2.f)) {
                        actor->yScale = actor->xScale;
                        actor->zScale = actor->xScale;
                    }
                    saturn_keyframe_popout("k_scale");
                } else {
                    ImGui::SliderFloat("X###mscale_x", &actor->xScale, -2.f, 2.f);
                    ImGui::SliderFloat("Y###mscale_y", &actor->yScale, -2.f, 2.f);
                    ImGui::SliderFloat("Z###mscale_z", &actor->zScale, -2.f, 2.f);
                    saturn_keyframe_popout_next_line("k_scale");
                }
                ImGui::Checkbox("Link###link_mario_scale", &link_scaling);
                if (actor->xScale != 1.f || actor->yScale != 1.f || actor->zScale != 1.f) {
                    ImGui::SameLine(); if (ImGui::Button("Reset###reset_mscale")) {
                        actor->xScale = 1.f;
                        actor->yScale = 1.f;
                        actor->zScale = 1.f;
                    }
                }
                if (actor->obj_model == MODEL_MARIO) {
                    if (link_scaling) {
                        ImGui::SliderFloat("RH Size", &actor->scaler[0][0], -2, 2);
                        saturn_keyframe_popout("k_rh_scale");
                        ImGui::SliderFloat("LH Size", &actor->scaler[1][0], -2, 2);
                        saturn_keyframe_popout("k_lh_scale");
                        ImGui::SliderFloat("RF Size", &actor->scaler[2][0], -2, 2);
                        saturn_keyframe_popout("k_rf_scale");
                        vec3f_set(actor->scaler[0], actor->scaler[0][0], actor->scaler[0][0], actor->scaler[0][0]);
                        vec3f_set(actor->scaler[1], actor->scaler[1][0], actor->scaler[1][0], actor->scaler[1][0]);
                        vec3f_set(actor->scaler[2], actor->scaler[2][0], actor->scaler[2][0], actor->scaler[2][0]);
                    }
                    else {
                        ImGui::SliderFloat3("RH Size", actor->scaler[0], -2, 2);
                        saturn_keyframe_popout("k_rh_scale");
                        ImGui::SliderFloat3("LH Size", actor->scaler[1], -2, 2);
                        saturn_keyframe_popout("k_lh_scale");
                        ImGui::SliderFloat3("RF Size", actor->scaler[2], -2, 2);
                        saturn_keyframe_popout("k_rf_scale");
                    }
                }

                if (mario_exists) {
                    Vec3f actor_pos;
                    vec3f_set(actor_pos, actor->x, actor->y, actor->z);

                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::Text("Position");
                    ImGui::InputFloat3("###mario_set_pos", actor_pos);
                    if (ImGui::Button(ICON_FK_FILES_O " Copy###copy_mario")) {
                        vec3f_copy(stored_mario_pos, actor_pos);
                        vec3s_set(stored_mario_angle, 0, actor->angle, 0);
                        has_copy_mario = true;
                    } ImGui::SameLine();
                    if (!has_copy_mario) ImGui::BeginDisabled();
                    if (ImGui::Button(ICON_FK_CLIPBOARD " Paste###paste_mario")) {
                        if (has_copy_mario) {
                            vec3f_copy(actor_pos, stored_mario_pos);
                            actor->angle = stored_mario_angle[1];
                        }
                    }
                    if (!has_copy_mario) ImGui::EndDisabled();

                    actor->x = actor_pos[0];
                    actor->y = actor_pos[1];
                    actor->z = actor_pos[2];
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        if (ImGui::BeginTable("misc_table", 2)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (actor->obj_model == MODEL_MARIO) {
                ImGui::Checkbox("Dust Particles", &enable_dust_particles);
                imgui_bundled_tooltip("Displays dust particles when Mario moves.");
                ImGui::Checkbox("Torso Rotations", &enable_torso_rotation);
                imgui_bundled_tooltip("Tilts Mario's torso when he moves; Disable for a \"beta running\" effect.");
            }
            ImGui::Checkbox("Hidden", &actor->hidden);
            imgui_bundled_tooltip("Makes the Mario not visible in renders.");
            saturn_keyframe_popout("k_mario_hidden");
            ImGui::Dummy(ImVec2(0, 0)); ImGui::SameLine(25); ImGui::Text("Shadow Scale");
            ImGui::Dummy(ImVec2(0, 0)); ImGui::SameLine(25); ImGui::SliderFloat("###shadow_scale", &actor->shadow_scale, 0.f, 2.f);
            saturn_keyframe_popout("k_shadow_scale");
            ImGui::TableSetColumnIndex(1);
            float angle = actor->angle / 32768.0f * 180;

            ImGuiKnobs::Knob("Angle", &angle, -180.f, 180.f, 0.f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal);
            saturn_keyframe_popout("k_angle");

            ImGui::Checkbox("Spin###spin_angle", &actor->spinning);
            if (actor->spinning) {
                angle += actor->spin_speed * 5;
                ImGui::SliderFloat("Speed###spin,speed", &actor->spin_speed, -2.f, 2.f, "%.1f");
            }

            actor->angle = angle / 180 * 32768;

            ImGui::EndTable();
        }
        if (mario_exists && actor->obj_model == MODEL_MARIO) if (ImGui::BeginMenu("Head Rotations")) {
            ImGui::Text("C-Up Settings");
            if (ImGui::BeginTable("headrot_table", 3)) {
                float fake_yaw = actor->head_rot_x * 360.f / 65536;
                float fake_pitch = actor->head_rot_z * 360.f / 65536;
                float fake_roll = actor->head_rot_y * 360.f / 65536;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGuiKnobs::Knob("Yaw", &fake_yaw, configCUpLimit ? -120.0f : -180.0f, configCUpLimit ? 120.0f : 180.0f, 0.0f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal);
                ImGui::TableSetColumnIndex(1);
                ImGuiKnobs::Knob("Pitch", &fake_pitch, configCUpLimit ? -45.0f : -180.0f, configCUpLimit ? 80.0f : 180.0f, 0.0f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal);
                ImGui::TableSetColumnIndex(2);
                ImGuiKnobs::Knob("Roll", &fake_roll, configCUpLimit ? 0.f : -180.f, configCUpLimit ? 0.f : 180.f, 0.0f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal);
                actor->head_rot_x = fake_yaw * 65536 / 360;
                actor->head_rot_z = fake_pitch * 65536 / 360;
                actor->head_rot_y = fake_roll * 65536 / 360;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                saturn_keyframe_popout({ "k_headrot_x", "k_headrot_z", "k_headrot_y" });
                ImGui::EndTable();
            }
            ImGui::EndMenu();
        }

        ImGui::PopStyleVar();
        ImGui::EndMenu();
    }

    ImGui::Separator();

    ImGui::PushItemWidth(50);
    ImGui::DragFloat("###mariopos_x", &actor->x, 1.f, 0.f, 0.f, "X");
    ImGui::SameLine();
    ImGui::DragFloat("###mariopos_y", &actor->y, 1.f, 0.f, 0.f, "Y");
    ImGui::SameLine();
    ImGui::DragFloat("###mariopos_z", &actor->z, 1.f, 0.f, 0.f, "Z");
    ImGui::PopItemWidth();
    saturn_keyframe_popout({ "k_mariopos_x", "k_mariopos_y", "k_mariopos_z" });

    ImGui::Separator();

    // Model Metadata
    if (!actor->model.Author.empty()) {
        string metaLabelText = (ICON_FK_USER " " + actor->model.Name);
        string metaDataText = "v" + actor->model.Version;
        if (actor->model.Description != "")
            metaDataText = "v" + actor->model.Version + "\n" + actor->model.Description;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("###model_metadata", ImVec2(0, 45), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text(metaLabelText.c_str()); imgui_bundled_tooltip(metaDataText.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            open_file(std::string(sys_exe_path()) + "/" + actor->model.FolderPath + "/");
        ImGui::TextDisabled(("@ " + actor->model.Author).c_str());
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::Separator();
    }

    if (actor->model.CustomEyeSupport && actor->obj_model == MODEL_MARIO) {
        // Custom Eyes Checkbox
        ImGui::Checkbox("Custom Eyes", &actor->custom_eyes);
        saturn_keyframe_popout("k_customeyes");

        if ((actor->custom_eyes && actor->model.Expressions.size() > 0) || actor->model.Expressions.size() > 1)
            ImGui::Separator();

        // Expressions Selector
        OpenExpressionSelector(actor);

        // Keyframing
        if (actor->custom_eyes || actor->model.Expressions.size() > 0) saturn_keyframe_popout_next_line("k_mario_expr");
    }

    ImGui::Separator();
    ImGui::Text(ICON_FK_LINE_CHART " Timeline Controls");
    ImGui::PushItemWidth(100);
    ImGui::DragInt("Position", &k_current_frame, 0.35f, 0);
    ImGui::PopItemWidth();
    if (ImGui::Button("Switch to Mario tab")) {
        request_mario_tab = true;
    }
}