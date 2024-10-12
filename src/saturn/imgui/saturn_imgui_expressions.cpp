#include "saturn_imgui_dynos.h"

#include "saturn/libs/portable-file-dialogs.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include "GL/glew.h"

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/libs/imgui/imgui-knobs.h"
#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_actors.h"
#include "saturn_imgui.h"
#include "saturn/imgui/saturn_imgui_file_browser.h"
#include "saturn/imgui/saturn_imgui_dynos.h"
#include "data/dynos.cpp.h"
#include <SDL2/SDL.h>

#include "icons/IconsForkAwesome.h"

extern "C" {
#include "pc/gfx/gfx_pc.h"
#include "pc/configfile.h"
}

GLuint active_expression_preview = 0;
int preview_width = 0;
int preview_height = 0;
int preview_sample_ratio = 0;
std::string last_preview_path;

/* Updates the expression preview window with a PNG texture path */
bool UpdateExpressionPreview(std::string TexturePath) {
    // Should we update?
    // Only update if our active preview must be changed
    if (active_expression_preview != 0 && last_preview_path == TexturePath)
        return true;

    const char* filename = TexturePath.c_str();
    last_preview_path = TexturePath;

    // Load from file
    unsigned char* image_data = stbi_load(filename, &preview_width, &preview_height, NULL, 4);
    if (image_data == NULL)
        return false;

    preview_sample_ratio = preview_width / preview_height;

    // Create a OpenGL texture identifier
    active_expression_preview = 0;
    glGenTextures(1, &active_expression_preview);
    glBindTexture(GL_TEXTURE_2D, active_expression_preview);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, preview_width, preview_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    return true;
}

/* Shows an expression preview with a given texture */
void ShowExpressionPreview(TexturePath texture) {
    if (!configEditorExpressionPreviews) return;

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (UpdateExpressionPreview(texture.FilePath)) {
            ImGui::Image((void*)(intptr_t)active_expression_preview, ImVec2(preview_sample_ratio * 37, 37));
        }
        ImGui::EndTooltip();
    }
}

/* Shows a context menu for an expression's texture */
void ShowTextureContextMenu(Expression* expression, TexturePath texture, int id) {
    std::string popupLabelId = "###menu_texture_popup_" + texture.FileName + "_" + std::to_string(id);

    // Show expression previews
    if (ImGui::IsItemHovered()) {
        ShowExpressionPreview(texture);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) ImGui::OpenPopup(popupLabelId.c_str());
    }

    // Context Menu
    if (ImGui::BeginPopup(popupLabelId.c_str())) {
        // Mini expression preview
        if (configEditorExpressionPreviews && UpdateExpressionPreview(texture.FilePath)) {
            ImGui::Image((void*)(intptr_t)active_expression_preview, ImVec2(preview_sample_ratio * 14, 14));
            ImGui::SameLine();
        }
        // Label
        if (ImGui::Selectable(("%s/", texture.FileName.c_str()), false)) {
            open_file(std::string(sys_exe_path()) + "/" + texture.ParentPath() + "/");
            ImGui::CloseCurrentPopup();
        }
        imgui_bundled_tooltip(("/%s", texture.FilePath).c_str());

        ImGui::Separator();
        ImGui::TextDisabled("%i texture(s)", expression->Textures.size());
        // Refresh Textures
        if (ImGui::Button(ICON_FK_REFRESH " Refresh All###refresh_textures")) {
            expression->Textures = LoadExpressionTextures(expression->FolderPath, *expression);
            // We attempt to keep each currently-selected texture
            // If our index is out of bounds (e.g. PNG was deleted) reset it
            for (int i = 0; i < expression->Textures.size(); i++) {
                if (expression->CurrentIndex > i || expression->CurrentIndex < i)
                    expression->CurrentIndex = 0;
            }
            gfx_precache_textures();
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

/* Creates a nested expression selection menu for a model; Contains dropdown OR checkbox widgets */
void OpenExpressionSelector(MarioActor* actor) {
    if (!actor) return;
    if (actor->custom_eyes && actor->model.Expressions.size() > 0 && actor->model.CustomEyeSupport) {
        // Eye Selector
        saturn_file_browser_filter_extension("png");
        saturn_file_browser_scan_directory(actor->model.Expressions[0].FolderPath);
        saturn_file_browser_height(150);
        if (saturn_file_browser_show("eyes")) {
            for (int i = 0; i < actor->model.Expressions[0].Textures.size(); i++) {
                std::filesystem::path path = actor->model.Expressions[0].Textures[i].FilePath;
                std::filesystem::path base = actor->model.Expressions[0].FolderPath;
                if (std::filesystem::relative(path, base) == saturn_file_browser_get_selected()) {
                    actor->model.Expressions[0].CurrentIndex = i;
                    break;
                }
            }
        }
    }

    if (!actor->model.Active) return;
    if (actor->model.Expressions.size() > 1) {
        // Other Expressions
        if (actor->model.Expressions.size() > 8) ImGui::BeginChild(("###menu_exp_model"), ImVec2(200, 190), false);
        for (int i = 0; i < actor->model.Expressions.size(); i++) {
            Expression expression = actor->model.Expressions[i];
            if (expression.Name == "eyes") continue;

            std::string label_name = "###exp_label_" + expression.Name;
            ImGui::Text(expression.Name.c_str());
            ImGui::SameLine(75);
            ImGui::PushItemWidth(120);

            // Use checkbox
            if (expression.Textures.size() == 2 && (
                expression.Textures[0].FileName.find("default") != std::string::npos ||
                expression.Textures[1].FileName.find("default") != std::string::npos )) {
                    // Check which index "default.png" is (0 or 1) to determine default value
                    int select_index = (expression.Textures[0].FileName.find("default") != std::string::npos) ? 0 : 1;
                    int deselect_index = (expression.Textures[0].FileName.find("default") != std::string::npos) ? 1 : 0;
                    bool is_selected = (expression.CurrentIndex == select_index);

                    if (ImGui::Checkbox(label_name.c_str(), &is_selected)) {
                        if (is_selected) actor->model.Expressions[i].CurrentIndex = select_index;
                        else actor->model.Expressions[i].CurrentIndex = deselect_index;
                    }
                    // Popout showing the checkbox's actively displayed value
                    // This is technically the opposite of the dropdowns and selectables, which shows the value-that-will-be, not current
                    ShowTextureContextMenu(&actor->model.Expressions[i], actor->model.Expressions[i].Textures[expression.CurrentIndex], i);
            } else {
                // Use dropdown
                std::string defaultLabel = ((expression.Textures.size() > 0) ? expression.Textures[expression.CurrentIndex].FileName : expression.Name) + "###combo_" + expression.Name.c_str();
                if (ImGui::BeginCombo(label_name.c_str(), defaultLabel.c_str(), ImGuiComboFlags_None)) {
                    saturn_file_browser_filter_extension("png");
                    saturn_file_browser_scan_directory(actor->model.Expressions[i].FolderPath);
                    if (saturn_file_browser_show_tree("expr_" + std::to_string(i))) {
                        for (int j = 0; j < actor->model.Expressions[i].Textures.size(); j++) {
                            std::filesystem::path path = actor->model.Expressions[i].Textures[j].FilePath;
                            std::filesystem::path base = actor->model.Expressions[i].FolderPath;
                            if (std::filesystem::relative(path, base) == saturn_file_browser_get_selected()) {
                                actor->model.Expressions[i].CurrentIndex = j;
                                break;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
                
                if (ImGui::IsItemHovered()) {
                    ShowExpressionPreview(actor->model.Expressions[i].Textures[expression.CurrentIndex]);
                }
            }
            ImGui::PopItemWidth();
        }

        if (actor->model.Expressions.size() > 8) ImGui::EndChild();
    }
}