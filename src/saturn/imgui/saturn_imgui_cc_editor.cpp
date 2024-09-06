#include "saturn_imgui_cc_editor.h"

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/libs/imgui/imgui-knobs.h"
#include "saturn/saturn.h"
#include "saturn/saturn_actors.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/imgui/saturn_imgui.h"
#include "saturn/imgui/saturn_imgui_dynos.h"
#include "icons/IconsForkAwesome.h"

// UI Variables; Used by specifically ImGui

int uiCcListId;
static char uiCcLabelName[128] = "Sample";
static char uiGameShark[1024 * 16] =    "8107EC40 FF00\n8107EC42 0000\n8107EC38 7F00\n8107EC3A 0000\n"
                                        "8107EC28 0000\n8107EC2A FF00\n8107EC20 0000\n8107EC22 7F00\n"
                                        "8107EC58 FFFF\n8107EC5A FF00\n8107EC50 7F7F\n8107EC52 7F00\n"
                                        "8107EC70 721C\n8107EC72 0E00\n8107EC68 390E\n8107EC6A 0700\n"
                                        "8107EC88 FEC1\n8107EC8A 7900\n8107EC80 7F60\n8107EC82 3C00\n"
                                        "8107ECA0 7306\n8107ECA2 0000\n8107EC98 3903\n8107EC9A 0000";
#define CREATE_COLOR(r, g, b) {                                  \
    ImVec4(r / 255.f,       g / 255.f,       b / 255.f,       1), \
    ImVec4(r / 255.f / 2.f, g / 255.f / 2.f, b / 255.f / 2.f, 1)   \
}
ImVec4 uiColors[][2] = {
    CREATE_COLOR(255, 0, 0),     // hat
    CREATE_COLOR(0, 0, 255),     // overalls
    CREATE_COLOR(255, 255, 255), // gloves
    CREATE_COLOR(114, 28, 14),   // shoes
    CREATE_COLOR(254, 193, 121), // skin
    CREATE_COLOR(155, 6, 0),     // hair
    CREATE_COLOR(255, 255, 0),   // shirt
    CREATE_COLOR(0, 255, 255),   // shoulders
    CREATE_COLOR(0, 255, 127),   // arms
    CREATE_COLOR(255, 0, 255),   // overalls bottom
    CREATE_COLOR(255, 0, 127),   // leg top
    CREATE_COLOR(127, 0, 255),   // leg bottom
};

static char ccSearchTerm[128];
bool has_open_any_model_cc;

bool auto_shading;

MarioActor* current_actor = nullptr;

/*
Update the "defaultColor" presets with our CC Editor colors.
These values are directly used in gfx_pc.c to overwrite incoming lights.
*/
void UpdatePaletteFromEditor() {
    for (int i = 0; i < 12; i++) {
        for (int shade = 0; shade < 2; shade++) {
            current_actor->colorcode[i].red  [shade] = uiColors[i][shade].x * 255;
            current_actor->colorcode[i].green[shade] = uiColors[i][shade].y * 255;
            current_actor->colorcode[i].blue [shade] = uiColors[i][shade].z * 255;
        }
    }

    current_color_code = ParseGameShark(current_actor);
    strcpy(uiGameShark, current_color_code.GameShark.c_str());
    current_color_code.Name = uiCcLabelName;

    std::replace(current_color_code.Name.begin(), current_color_code.Name.end(), '.', '-');
    std::replace(current_color_code.Name.begin(), current_color_code.Name.end(), '/', '-');
    std::replace(current_color_code.Name.begin(), current_color_code.Name.end(), '\\', '-');
} 

/*
Update our CC Editor colors with our "defaultColor" values.
This should be called when loading a CC, to insert our new colors into the editor.
*/
void UpdateEditorFromPalette() {
    for (int i = 0; i < 12; i++) {
        for (int shade = 0; shade < 2; shade++) {
            uiColors[i][shade].x = current_actor->colorcode[i].red  [shade] / 255.f;
            uiColors[i][shade].y = current_actor->colorcode[i].green[shade] / 255.f;
            uiColors[i][shade].z = current_actor->colorcode[i].blue [shade] / 255.f;
        }
    }

    current_color_code = ParseGameShark(current_actor);
    strcpy(uiGameShark, current_color_code.GameShark.c_str());
    strcpy(uiCcLabelName, current_color_code.Name.c_str());
}

void SparkilizeEditor() {
    for (int i = 0; i < 2; i++) {
        uiColors[CC_SHIRT          ][i] = uiColors[CC_HAT     ][i];
        uiColors[CC_SHOULDERS      ][i] = uiColors[CC_HAT     ][i];
        uiColors[CC_ARMS           ][i] = uiColors[CC_HAT     ][i];
        uiColors[CC_OVERALLS_BOTTOM][i] = uiColors[CC_OVERALLS][i];
        uiColors[CC_LEG_TOP        ][i] = uiColors[CC_OVERALLS][i];
        uiColors[CC_LEG_BOTTOM     ][i] = uiColors[CC_OVERALLS][i];
    }
    UpdatePaletteFromEditor();
}

// Util Stuff

void ResetColorCode(bool usingModel) {
    uiCcListId = 1;
    // Use Mario's default palette (class init)
    current_color_code = GameSharkCode();
    ApplyColorCode(current_color_code, current_actor);

    // Model CCs will attempt to select their first color code instead
    if (usingModel && model_color_code_list.size() > 0 && current_actor->model.HasColorCodeFolder()) {
        uiCcListId = -1;
        current_color_code = LoadGSFile(model_color_code_list[0], current_actor->model.FolderPath + "/colorcodes", usingModel ? current_actor : nullptr);
        ApplyColorCode(current_color_code, current_actor);
        if (current_color_code.Name == "default")
            current_color_code.Name = "Sample";
        last_model_cc_address = current_color_code.GameShark;
    } else if (!usingModel)
        // Also reset the current CC labels
        current_actor->model.Colors = Model().Colors;

    UpdateEditorFromPalette();
}

void RefreshColorCodeList() {
    if (current_actor) {
        if (current_actor->model.HasColorCodeFolder()) {
            model_color_code_list.clear();
            model_color_code_list = GetColorCodeList(current_actor->model.FolderPath + "/colorcodes");
        }
    }
    else {
        color_code_list.clear();
        color_code_list = GetColorCodeList(std::string(sys_user_path()) + "/dynos/colorcodes");
    }
}

// UI Segments

void OpenModelCCSelector(Model model, std::vector<std::string> list, std::string ccSearchLower) {
    for (int n = 0; n < list.size(); n++) {
        const bool is_selected = (uiCcListId == ((n + 1) * -1) && model.FolderPath == current_actor->model.FolderPath);
        std::string label_name = list[n].substr(0, list[n].size() - 3);

        // If we're searching, only include CCs with the search keyword in the name
        // Also convert to lowercase
        std::string label_name_lower = label_name;
        std::transform(label_name_lower.begin(), label_name_lower.end(), label_name_lower.begin(),
                [](unsigned char c1){ return std::tolower(c1); });
        if (ccSearchLower != "") {
            if (label_name_lower.find(ccSearchLower) == std::string::npos) {
                continue;
            }
        }

        // Selectable
        if (ImGui::Selectable((ICON_FK_USER " " + label_name).c_str(), is_selected)) {
            if (model.FolderPath == current_actor->model.FolderPath)
                uiCcListId = (n + 1) * -1;
            else uiCcListId = 0;

            // Overwrite current color code
            current_color_code = LoadGSFile(list[n], model.FolderPath + "/colorcodes", current_actor);
            ApplyColorCode(current_color_code, current_actor);
            if (label_name_lower == "default" || label_name_lower != "../default") {
                label_name = "Sample";
                //ResetColorCode(current_model.HasColorCodeFolder());
            }
            UpdateEditorFromPalette();
            last_model_cc_address = uiGameShark;
        }

        // Right-Click Menu
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Text("%s.gs", label_name.c_str());
            imgui_bundled_tooltip((model.FolderPath + "/colorcodes/" + label_name + ".gs").c_str());

            // Delete GS File
            if (label_name_lower != "default" && label_name_lower != "../default") {
                if (ImGui::Button(ICON_FK_TRASH " Delete File"))
                    ImGui::OpenPopup("###delete_m_gs_file");
                if (ImGui::BeginPopup("###delete_m_gs_file")) {

                    ImGui::Text("Are you sure you want to delete %s?", label_name.c_str());
                    if (ImGui::Button("Yes")) {
                        DeleteGSFile(model.FolderPath + "/colorcodes/" + label_name + ".gs");
                        RefreshColorCodeList();
                        ImGui::CloseCurrentPopup();
                    } ImGui::SameLine();
                    if (ImGui::Button("No")) {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }
            
            ImGui::Separator();
            ImGui::TextDisabled("%i color code(s)", list.size());
            if (ImGui::Button(ICON_FK_UNDO " Refresh")) {
                RefreshColorCodeList();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void OpenCCSelector(MarioActor* actor) {
    std::string colorcodes_dir_path = std::string(sys_user_path()) + "/dynos/colorcodes/";

    ImGui::Text("Color Codes");
    ImGui::SameLine(); imgui_bundled_help_marker(
        "These are GameShark color codes, which overwrite Mario's lights. Place GS files in dynos/colorcodes.");

    // If we have 18 or more color codes, a search bar is added
    if (color_code_list.size() >= 18) {
        ImGui::InputTextWithHint("###cc_search_text", ICON_FK_SEARCH " Search color codes...", ccSearchTerm, IM_ARRAYSIZE(ccSearchTerm), ImGuiInputTextFlags_AutoSelectAll);
    } else {
        // If our CC list is reloaded, and we now have less than 18 files, this can cause filter issues if not reset to nothing
        if (ccSearchTerm != "") strcpy(ccSearchTerm, "");
    }
    std::string ccSearchLower = ccSearchTerm;
    std::transform(ccSearchLower.begin(), ccSearchLower.end(), ccSearchLower.begin(),
        [](unsigned char c){ return std::tolower(c); });

    // UI list
    ImGui::BeginChild("###menu_cc_selector", ImVec2(-FLT_MIN, 100), true);

    //if (!ImGui::IsPopupOpen(0u, ImGuiPopupFlags_AnyPopupId)) {
        if (model_color_code_list.size() > 0 && current_actor->model.HasColorCodeFolder()) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode((current_actor->model.FolderName + "###model_cc_header").c_str())) {
                OpenModelCCSelector(current_actor->model, model_color_code_list, ccSearchLower);
                ImGui::TreePop();
            }
        }
    //}

    for (int n = 0; n < color_code_list.size(); n++) {

        const bool is_selected = (actor->cc_index == n + 1);
        std::string label_name = color_code_list[n].substr(0, color_code_list[n].size() - 3);

        // If we're searching, only include CCs with the search keyword in the name
        // Also convert to lowercase
        std::string label_name_lower = label_name;
        std::transform(label_name_lower.begin(), label_name_lower.end(), label_name_lower.begin(),
                [](unsigned char c1){ return std::tolower(c1); });
        if (ccSearchLower != "") {
            if (label_name_lower.find(ccSearchLower) == std::string::npos) {
                continue;
            }
        }

        // Selectable
        if (ImGui::Selectable(label_name.c_str(), is_selected)) {
            actor->cc_index = n + 1;

            // Overwrite current color code
            current_color_code = LoadGSFile(color_code_list[n], colorcodes_dir_path, current_actor);
            ApplyColorCode(current_color_code, current_actor);
            if (label_name_lower == "mario") {
                label_name = "Sample";
                current_color_code = GameSharkCode();
            }
            PasteGameShark(current_color_code.GameShark, actor->colorcode);

            UpdateEditorFromPalette();

        }
        // Right-Click Menu
        if (ImGui::BeginPopupContextItem()) {
            if (label_name_lower != "mario") {
                ImGui::Text("%s.gs", label_name.c_str());
                imgui_bundled_tooltip(("/dynos/colorcodes/" + label_name + ".gs").c_str());
                ImGui::Separator();

                // Delete GS File
                if (ImGui::Button(ICON_FK_TRASH " Delete File"))
                    ImGui::OpenPopup("###delete_v_gs_file");
                if (ImGui::BeginPopup("###delete_v_gs_file")) {

                    ImGui::Text("Are you sure you want to delete %s?", label_name.c_str());
                    if (ImGui::Button("Yes")) {
                        DeleteGSFile(colorcodes_dir_path + label_name + ".gs");
                        RefreshColorCodeList();
                        ImGui::CloseCurrentPopup();
                    } ImGui::SameLine();
                    if (ImGui::Button("No")) {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                ImGui::Separator();
            }

            ImGui::TextDisabled("%i color code(s)", color_code_list.size());
            if (ImGui::Button(ICON_FK_UNDO " Refresh")) {
                RefreshColorCodeList();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
}

void ColorPartBox(std::string name, const char* mainName, const char* shadeName, ImVec4* color, std::string id, std::string kf_id) {
    if (name == "") return;

    saturn_keyframe_popout_next_line(kf_id);
    ImGui::SameLine();

    // MAIN Color Picker
    ImGui::ColorEdit4(mainName, (float*)&color[0], ImGuiColorEditFlags_NoAlpha |
                                                    ImGuiColorEditFlags_InputRGB |
                                                    ImGuiColorEditFlags_Uint8 |
                                                    ImGuiColorEditFlags_NoLabel |
                                                    ImGuiColorEditFlags_NoOptions |
                                                    ImGuiColorEditFlags_NoInputs);
    if (ImGui::IsItemHovered()) {
        UpdatePaletteFromEditor();
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) ImGui::OpenPopup(id.c_str());
    }
    if (ImGui::BeginPopup(id.c_str())) {
        // Misc. Options

        // Multiply shade by 2
        if (ImGui::Selectable("×2")) {
            color[0].x = color[1].x * 2;
            color[0].y = color[1].y * 2;
            color[0].z = color[1].z * 2;
            UpdatePaletteFromEditor();
        }
        // Reset
        if (ImGui::Selectable(ICON_FK_UNDO " Reset")) {
            if (mainName == "Hat, Main")                PasteGameShark("8107EC40 FF00\n8107EC42 0000", current_actor->colorcode);
            if (mainName == "Overalls, Main")           PasteGameShark("8107EC28 0000\n8107EC2A FF00", current_actor->colorcode);
            if (mainName == "Gloves, Main")             PasteGameShark("8107EC58 FFFF\n8107EC5A FF00", current_actor->colorcode);
            if (mainName == "Shoes, Main")              PasteGameShark("8107EC70 721C\n8107EC72 0E00", current_actor->colorcode);
            if (mainName == "Skin, Main")               PasteGameShark("8107EC88 FEC1\n8107EC8A 7900", current_actor->colorcode);
            if (mainName == "Hair, Main")               PasteGameShark("8107ECA0 7306\n8107ECA2 0000", current_actor->colorcode);
            if (mainName == "Shirt, Main")              PasteGameShark("8107ECB8 FFFF\n8107ECBA 0000", current_actor->colorcode);
            if (mainName == "Shoulders, Main")          PasteGameShark("8107ECD0 00FF\n8107ECD2 FF00", current_actor->colorcode);
            if (mainName == "Arms, Main")               PasteGameShark("8107ECE8 00FF\n8107ECEA 7F00", current_actor->colorcode);
            if (mainName == "Overalls (Bottom), Main")  PasteGameShark("8107ED00 FF00\n8107ED02 FF00", current_actor->colorcode);
            if (mainName == "Leg (Top), Main")          PasteGameShark("8107ED18 FF00\n8107ED1A 7F00", current_actor->colorcode);
            if (mainName == "Leg (Bottom), Main")       PasteGameShark("8107ED30 7F00\n8107ED32 FF00", current_actor->colorcode);
            UpdateEditorFromPalette();
        }
        // Randomize
        if (ImGui::Selectable("Randomize")) {
            color[0].x = (rand() % 255) / 255.0f;
            color[0].y = (rand() % 255) / 255.0f;
            color[0].z = (rand() % 255) / 255.0f;
            UpdatePaletteFromEditor();
        }

        ImGui::EndPopup();
    } ImGui::SameLine();

    // SHADE Color Picker
    ImGui::ColorEdit4(shadeName, (float*)&color[1],   ImGuiColorEditFlags_NoAlpha |
                                                            ImGuiColorEditFlags_InputRGB |
                                                            ImGuiColorEditFlags_Uint8 |
                                                            ImGuiColorEditFlags_NoLabel |
                                                            ImGuiColorEditFlags_NoOptions |
                                                            ImGuiColorEditFlags_NoInputs);

    if (ImGui::IsItemHovered()) {
        UpdatePaletteFromEditor();
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) ImGui::OpenPopup((id + "1").c_str());
    }
    if (ImGui::BeginPopup((id + "1").c_str())) {
        // Misc. Options

        // Divide main by 2
        if (ImGui::Selectable("÷2")) {
            color[1].x = floor(color[0].x / 2.0f * 255.0f) / 255.0f;
            color[1].y = floor(color[0].y / 2.0f * 255.0f) / 255.0f;
            color[1].z = floor(color[0].z / 2.0f * 255.0f) / 255.0f;
            UpdatePaletteFromEditor();
        }
        // Reset
        if (ImGui::Selectable(ICON_FK_UNDO " Reset")) {
            if (shadeName == "Hat, Shade")                PasteGameShark("8107EC38 7F00\n8107EC3A 0000", current_actor->colorcode);
            if (shadeName == "Overalls, Shade")           PasteGameShark("8107EC20 0000\n8107EC22 7F00", current_actor->colorcode);
            if (shadeName == "Gloves, Shade")             PasteGameShark("8107EC50 7F7F\n8107EC52 7F00", current_actor->colorcode);
            if (shadeName == "Shoes, Shade")              PasteGameShark("8107EC68 390E\n8107EC6A 0700", current_actor->colorcode);
            if (shadeName == "Skin, Shade")               PasteGameShark("8107EC80 7F60\n8107EC82 3C00", current_actor->colorcode);
            if (shadeName == "Hair, Shade")               PasteGameShark("8107EC98 3903\n8107EC9A 0000", current_actor->colorcode);
            if (shadeName == "Shirt, Shade")              PasteGameShark("8107ECB0 7F7F\n8107ECB2 0000", current_actor->colorcode);
            if (shadeName == "Shoulders, Shade")          PasteGameShark("8107ECC8 007F\n8107ECCA 7F00", current_actor->colorcode);
            if (shadeName == "Arms, Shade")               PasteGameShark("8107ECE0 007F\n8107ECE2 4000", current_actor->colorcode);
            if (shadeName == "Overalls (Bottom), Shade")  PasteGameShark("8107ECF8 7F00\n8107ECFA 7F00", current_actor->colorcode);
            if (shadeName == "Leg (Top), Shade")          PasteGameShark("8107ED10 7F00\n8107ED12 4000", current_actor->colorcode);
            if (shadeName == "Leg (Bottom), Shade")       PasteGameShark("8107ED28 4000\n8107ED2A 7F00", current_actor->colorcode);
            UpdateEditorFromPalette();
        }
        // Randomize
        if (ImGui::Selectable("Randomize")) {
            color[1].x = (rand() % 127) / 255.0f;
            color[1].y = (rand() % 127) / 255.0f;
            color[1].z = (rand() % 127) / 255.0f;
            UpdatePaletteFromEditor();
        }

        ImGui::EndPopup();
    } ImGui::SameLine();

    ImGui::Text(name.c_str());

    if (ImGui::IsPopupOpen(ImGui::GetID((std::string(mainName)  + "picker").c_str()), ImGuiPopupFlags_None) && auto_shading) {
        color[1].x = color[0].x / 2;
        color[1].y = color[0].y / 2;
        color[1].z = color[0].z / 2;
        UpdatePaletteFromEditor();
    }
    if (ImGui::IsPopupOpen(ImGui::GetID((std::string(shadeName) + "picker").c_str()), ImGuiPopupFlags_None) && auto_shading) {
        color[0].x = color[1].x * 2;
        color[0].y = color[1].y * 2;
        color[0].z = color[1].z * 2;
        UpdatePaletteFromEditor();
    }

    if (ImGui::IsPopupOpen(ImGui::GetID((std::string( mainName) + "picker").c_str()), ImGuiPopupFlags_None) ||
        ImGui::IsPopupOpen(ImGui::GetID((std::string(shadeName) + "picker").c_str()), ImGuiPopupFlags_None)) {
            UpdatePaletteFromEditor();
    }
}

bool expr_export = true;

void OpenCCEditor(MarioActor* actor) {
    CCChangeActor(actor);
    ImGui::PushItemWidth(100);
    ImGui::InputText(".gs", uiCcLabelName, IM_ARRAYSIZE(uiCcLabelName));
    ImGui::PopItemWidth();

    ImGui::SameLine(150);

    if (ImGui::Button(ICON_FK_FILE_TEXT " File###save_cc_to_file")) {
        UpdatePaletteFromEditor();
        SaveGSFile(current_color_code, std::string(sys_user_path()) + "/dynos/colorcodes", nullptr, false);
        RefreshColorCodeList();
    }
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::SameLine(150);
    if (actor->model.ColorCodeSupport && AnyModelsEnabled(actor)) {
        if (ImGui::Button(ICON_FK_FOLDER_OPEN_O " Model###save_cc_to_model")) {
            UpdatePaletteFromEditor();
            SaveGSFile(current_color_code, actor->model.FolderPath + "/colorcodes", actor, expr_export);
            RefreshColorCodeList();
        }
        ImGui::Checkbox("Export Expressions", &expr_export);
        imgui_bundled_tooltip("Includes expression information with the model color code.");
    }

    ImGui::Dummy(ImVec2(0, 5));

    if (ImGui::Button(ICON_FK_UNDO " Reset Colors"))
        ResetColorCode(actor->model.HasColorCodeFolder());

    ImGui::SameLine(); if (ImGui::SmallButton(ICON_FK_RANDOM "###randomize_all")) {
        for (int i = 0; i < 12; i++) {
            uiColors[i][0].x = (rand() % 255) / 255.f;
            uiColors[i][0].y = (rand() % 255) / 255.f;
            uiColors[i][0].z = (rand() % 255) / 255.f;
            uiColors[i][1].x = (rand() % 127) / 255.f;
            uiColors[i][1].y = (rand() % 127) / 255.f;
            uiColors[i][1].z = (rand() % 127) / 255.f;
        }
        UpdatePaletteFromEditor();
    } imgui_bundled_tooltip("Randomizes all color values; WARNING: This will overwrite your active color code.");

    // Editor Tabs

    // Visual CC Editor
    ImGui::Checkbox("Auto 1/2", &auto_shading);
    imgui_bundled_tooltip("Automatically adjusts the shading for their respecive body parts.");
    if (ImGui::BeginTabBar("###dynos_tabbar", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Editor")) {

            ColorPartBox(actor->model.Colors.Hat.c_str(), "Hat, Main", "Hat, Shade", uiColors[CC_HAT], "1/2###hat_half", "k_hat");
            ColorPartBox(actor->model.Colors.Overalls.c_str(), "Overalls, Main", "Overalls, Shade", uiColors[CC_OVERALLS], "1/2###overalls_half", "k_overalls");
            ColorPartBox(actor->model.Colors.Gloves.c_str(), "Gloves, Main", "Gloves, Shade", uiColors[CC_GLOVES], "1/2###gloves_half", "k_gloves");
            ColorPartBox(actor->model.Colors.Shoes.c_str(), "Shoes, Main", "Shoes, Shade", uiColors[CC_SHOES], "1/2###shoes_half", "k_shoes");
            ColorPartBox(actor->model.Colors.Skin.c_str(), "Skin, Main", "Skin, Shade", uiColors[CC_SKIN], "1/2###skin_half", "k_skin");
            ColorPartBox(actor->model.Colors.Hair.c_str(), "Hair, Main", "Hair, Shade", uiColors[CC_HAIR], "1/2###hair_half", "k_hair");

            if (actor->model.SparkSupport) {
                if (!actor->spark_support) ImGui::BeginDisabled();
                    // SPARKILIZE
                    if (ImGui::SmallButton("v SPARKILIZE v###cc_editor_sparkilize")) {
                        SparkilizeEditor();
                    } ImGui::SameLine(); imgui_bundled_help_marker("Automatically converts a regular CC to a SPARK CC; WARNING: This will overwrite your active color code.");

                    ColorPartBox(actor->model.Colors.Shirt.c_str(), "Shirt, Main", "Shirt, Shade", uiColors[CC_SHIRT], "1/2###shirt_half", "k_shirt");
                    ColorPartBox(actor->model.Colors.Shoulders.c_str(), "Shoulders, Main", "Shoulders, Shade", uiColors[CC_SHOULDERS], "1/2###shoulders_half", "k_shoulders");
                    ColorPartBox(actor->model.Colors.Arms.c_str(), "Arms, Main", "Arms, Shade", uiColors[CC_ARMS], "1/2###arms_half", "k_arms");
                    ColorPartBox(actor->model.Colors.Pelvis.c_str(), "Overalls (Bottom), Main", "Overalls (Bottom), Shade", uiColors[CC_OVERALLS_BOTTOM], "1/2###overalls_bottom_half", "k_overalls_bottom");
                    ColorPartBox(actor->model.Colors.Thighs.c_str(), "Leg (Top), Main", "Leg (Top), Shade", uiColors[CC_LEG_TOP], "1/2###leg_top_half", "k_leg_top");
                    ColorPartBox(actor->model.Colors.Calves.c_str(), "Leg (Bottom), Main", "Leg (Bottom), Shade", uiColors[CC_LEG_BOTTOM], "1/2###leg_bottom_half", "k_leg_bottom");
                if (!actor->spark_support) ImGui::EndDisabled();
            }
            
            ImGui::EndTabItem();
        }
        // GameShark Editor
        if (ImGui::BeginTabItem("GameShark")) {

            ImGui::InputTextMultiline("###gameshark_box", uiGameShark, IM_ARRAYSIZE(uiGameShark), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 25),    ImGuiInputTextFlags_CharsUppercase |
                                                                                                                                                        ImGuiInputTextFlags_AutoSelectAll);

            if (ImGui::Button(ICON_FK_CLIPBOARD " Apply GS Code")) {
                //PasteGameShark(std::string(uiGameShark));
                PasteGameShark(uiGameShark, actor->colorcode);
                UpdateEditorFromPalette();
            } ImGui::SameLine(); imgui_bundled_help_marker(
                "Copy/paste a GameShark color code from here!");                                                                                                                 

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void CCChangeActor(MarioActor* actor) {
    if (current_actor == actor) return;
    current_actor = actor;
    UpdateEditorFromPalette();
}