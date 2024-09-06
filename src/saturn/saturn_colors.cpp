#include "saturn/saturn_colors.h"

#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <SDL2/SDL.h>

#include "saturn/saturn.h"
#include "saturn/imgui/saturn_imgui.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_actors.h"

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"

#include "pc/configfile.h"

extern "C" {
#include "game/camera.h"
#include "game/level_update.h"
#include "sm64.h"
}

#include "saturn/saturn_json.h"

#include <dirent.h>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <array>
#include "pc/fs/fs.h"

std::vector<std::string> color_code_list;
std::vector<std::string> model_color_code_list;

GameSharkCode current_color_code;

std::string HexifyColorTemplate(ColorTemplate &colorBodyPart) {
    char colour[64];
    ImFormatString(colour, IM_ARRAYSIZE(colour), "%02X%02X%02X%02X%02X%02X"
            , ImClamp((int)colorBodyPart.red[0], 0, 255)
            , ImClamp((int)colorBodyPart.green[0], 0, 255)
            , ImClamp((int)colorBodyPart.blue[0], 0, 255)
            , ImClamp((int)colorBodyPart.red[1], 0, 255)
            , ImClamp((int)colorBodyPart.green[1], 0, 255)
            , ImClamp((int)colorBodyPart.blue[1], 0, 255));
    std::string col = colour;
    return col;
}

ColorTemplate chromaColor                   {0,   0,   255, 255, 0,   0  };

std::string last_model_cc_address;

/* Global toggle for Saturn's color code compatibility. True by default */
bool support_color_codes = true;
/* Global toggle for Saturn's SPARK extension. False by default */
bool support_spark = true;

void PasteGameShark(std::string GameShark, ColorCode cc) {
    std::istringstream f(GameShark);
    std::string line;
        
    while (std::getline(f, line)) {
        if (line.rfind("81", 0) == 0) {
            int address = std::stoi(line.substr(2, 6), 0, 16);
            int value1 = std::stoi(line.substr(9, 2), 0, 16);
            int value2 = std::stoi(line.substr(11, 2), 0, 16);

            switch(address) {

                /* Address (80000000 + Offset)  RED     BLUE
                                                GREEN           */

                // Cap
                case 0x07EC40:  cc[CC_HAT].red[0] = value1;        cc[CC_HAT].green[0] = value2;          break;
                case 0x07EC42:  cc[CC_HAT].blue[0] = value1;                                              break;
                case 0x07EC38:  cc[CC_HAT].red[1] = value1;        cc[CC_HAT].green[1] = value2;          break;
                case 0x07EC3A:  cc[CC_HAT].blue[1] = value1;                                              break;
                // Overalls
                case 0x07EC28:  cc[CC_OVERALLS].red[0] = value1;   cc[CC_OVERALLS].green[0] = value2;     break;
                case 0x07EC2A:  cc[CC_OVERALLS].blue[0] = value1;                                         break;
                case 0x07EC20:  cc[CC_OVERALLS].red[1] = value1;   cc[CC_OVERALLS].green[1] = value2;     break;
                case 0x07EC22:  cc[CC_OVERALLS].blue[1] = value1;                                         break;
                // Gloves
                case 0x07EC58:  cc[CC_GLOVES].red[0] = value1;     cc[CC_GLOVES].green[0] = value2;       break;
                case 0x07EC5A:  cc[CC_GLOVES].blue[0] = value1;                                           break;
                case 0x07EC50:  cc[CC_GLOVES].red[1] = value1;     cc[CC_GLOVES].green[1] = value2;       break;
                case 0x07EC52:  cc[CC_GLOVES].blue[1] = value1;                                           break;
                // Shoes
                case 0x07EC70:  cc[CC_SHOES].red[0] = value1;      cc[CC_SHOES].green[0] = value2;        break;
                case 0x07EC72:  cc[CC_SHOES].blue[0] = value1;                                            break;
                case 0x07EC68:  cc[CC_SHOES].red[1] = value1;      cc[CC_SHOES].green[1] = value2;        break;
                case 0x07EC6A:  cc[CC_SHOES].blue[1] = value1;                                            break;
                // Skin
                case 0x07EC88:  cc[CC_SKIN].red[0] = value1;       cc[CC_SKIN].green[0] = value2;         break;
                case 0x07EC8A:  cc[CC_SKIN].blue[0] = value1;                                             break;
                case 0x07EC80:  cc[CC_SKIN].red[1] = value1;       cc[CC_SKIN].green[1] = value2;         break;
                case 0x07EC82:  cc[CC_SKIN].blue[1] = value1;                                             break;
                // Hair
                case 0x07ECA0:  cc[CC_HAIR].red[0] = value1;       cc[CC_HAIR].green[0] = value2;         break;
                case 0x07ECA2:  cc[CC_HAIR].blue[0] = value1;                                             break;
                case 0x07EC98:  cc[CC_HAIR].red[1] = value1;       cc[CC_HAIR].green[1] = value2;         break;
                case 0x07EC9A:  cc[CC_HAIR].blue[1] = value1;                                             break;

                // (Optional) SPARK Addresses

                // Shirt
                case 0x07ECB8:  cc[CC_SHIRT].red[0] = value1;            cc[CC_SHIRT].green[0] = value2;          break;
                case 0x07ECBA:  cc[CC_SHIRT].blue[0] = value1;                                                    break;
                case 0x07ECB0:  cc[CC_SHIRT].red[1] = value1;            cc[CC_SHIRT].green[1] = value2;          break;
                case 0x07ECB2:  cc[CC_SHIRT].blue[1] = value1;                                                    break;
                // Shoulders
                case 0x07ECD0:  cc[CC_SHOULDERS].red[0] = value1;        cc[CC_SHOULDERS].green[0] = value2;      break;
                case 0x07ECD2:  cc[CC_SHOULDERS].blue[0] = value1;                                                break;
                case 0x07ECC8:  cc[CC_SHOULDERS].red[1] = value1;        cc[CC_SHOULDERS].green[1] = value2;      break;
                case 0x07ECCA:  cc[CC_SHOULDERS].blue[1] = value1;                                                break;
                // Arms
                case 0x07ECE8:  cc[CC_ARMS].red[0] = value1;             cc[CC_ARMS].green[0] = value2;           break;
                case 0x07ECEA:  cc[CC_ARMS].blue[0] = value1;                                                     break;
                case 0x07ECE0:  cc[CC_ARMS].red[1] = value1;             cc[CC_ARMS].green[1] = value2;           break;
                case 0x07ECE2:  cc[CC_ARMS].blue[1] = value1;                                                     break;
                // Pelvis
                case 0x07ED00:  cc[CC_OVERALLS_BOTTOM].red[0] = value1;  cc[CC_OVERALLS_BOTTOM].green[0] = value2; break;
                case 0x07ED02:  cc[CC_OVERALLS_BOTTOM].blue[0] = value1;                                           break;
                case 0x07ECF8:  cc[CC_OVERALLS_BOTTOM].red[1] = value1;  cc[CC_OVERALLS_BOTTOM].green[1] = value2; break;
                case 0x07ECFA:  cc[CC_OVERALLS_BOTTOM].blue[1] = value1;                                           break;
                // Thighs
                case 0x07ED18:  cc[CC_LEG_TOP].red[0] = value1;          cc[CC_LEG_TOP].green[0] = value2;         break;
                case 0x07ED1A:  cc[CC_LEG_TOP].blue[0] = value1;                                                   break;
                case 0x07ED10:  cc[CC_LEG_TOP].red[1] = value1;          cc[CC_LEG_TOP].green[1] = value2;         break;
                case 0x07ED12:  cc[CC_LEG_TOP].blue[1] = value1;                                                   break;
                // Calves
                case 0x07ED30:  cc[CC_LEG_BOTTOM].red[0] = value1;       cc[CC_LEG_BOTTOM].green[0] = value2;      break;
                case 0x07ED32:  cc[CC_LEG_BOTTOM].blue[0] = value1;                                                break;
                case 0x07ED28:  cc[CC_LEG_BOTTOM].red[1] = value1;       cc[CC_LEG_BOTTOM].green[1] = value2;      break;
                case 0x07ED2A:  cc[CC_LEG_BOTTOM].blue[1] = value1;                                                break;
            }
        }
    }
}

GameSharkCode ParseGameShark(MarioActor* actor) {
    GameSharkCode gscode;
    std::string gameshark;

    std::string col1 = HexifyColorTemplate(actor->colorcode[CC_HAT]);
    std::string col2 = HexifyColorTemplate(actor->colorcode[CC_OVERALLS]);
    std::string col3 = HexifyColorTemplate(actor->colorcode[CC_GLOVES]);
    std::string col4 = HexifyColorTemplate(actor->colorcode[CC_SHOES]);
    std::string col5 = HexifyColorTemplate(actor->colorcode[CC_SKIN]);
    std::string col6 = HexifyColorTemplate(actor->colorcode[CC_HAIR]);

    gameshark += "8107EC40 " + col1.substr(0, 4) + "\n";
    gameshark += "8107EC42 " + col1.substr(4, 2) + "00\n";
    gameshark += "8107EC38 " + col1.substr(6, 4) + "\n";
    gameshark += "8107EC3A " + col1.substr(10, 2) + "00\n";
    gameshark += "8107EC28 " + col2.substr(0, 4) + "\n";
    gameshark += "8107EC2A " + col2.substr(4, 2) + "00\n";
    gameshark += "8107EC20 " + col2.substr(6, 4) + "\n";
    gameshark += "8107EC22 " + col2.substr(10, 2) + "00\n";
    gameshark += "8107EC58 " + col3.substr(0, 4) + "\n";
    gameshark += "8107EC5A " + col3.substr(4, 2) + "00\n";
    gameshark += "8107EC50 " + col3.substr(6, 4) + "\n";
    gameshark += "8107EC52 " + col3.substr(10, 2) + "00\n";
    gameshark += "8107EC70 " + col4.substr(0, 4) + "\n";
    gameshark += "8107EC72 " + col4.substr(4, 2) + "00\n";
    gameshark += "8107EC68 " + col4.substr(6, 4) + "\n";
    gameshark += "8107EC6A " + col4.substr(10, 2) + "00\n";
    gameshark += "8107EC88 " + col5.substr(0, 4) + "\n";
    gameshark += "8107EC8A " + col5.substr(4, 2) + "00\n";
    gameshark += "8107EC80 " + col5.substr(6, 4) + "\n";
    gameshark += "8107EC82 " + col5.substr(10, 2) + "00\n";
    gameshark += "8107ECA0 " + col6.substr(0, 4)+ "\n";
    gameshark += "8107ECA2 " + col6.substr(4, 2) + "00\n";
    gameshark += "8107EC98 " + col6.substr(6, 4)+ "\n";
    gameshark += "8107EC9A " + col6.substr(10, 2) + "00";

    if (actor->model.SparkSupport) {
        gameshark += "\n";

        std::string col7 = HexifyColorTemplate(actor->colorcode[CC_SHIRT]);
        std::string col8 = HexifyColorTemplate(actor->colorcode[CC_SHOULDERS]);
        std::string col9 = HexifyColorTemplate(actor->colorcode[CC_ARMS]);
        std::string col10 = HexifyColorTemplate(actor->colorcode[CC_OVERALLS_BOTTOM]);
        std::string col11 = HexifyColorTemplate(actor->colorcode[CC_LEG_TOP]);
        std::string col12 = HexifyColorTemplate(actor->colorcode[CC_LEG_BOTTOM]);

        gameshark += "8107ECB8 " + col7.substr(0, 4) + "\n";
        gameshark += "8107ECBA " + col7.substr(4, 2) + "00\n";
        gameshark += "8107ECB0 " + col7.substr(6, 4) + "\n";
        gameshark += "8107ECB2 " + col7.substr(10, 2) + "00\n";
        gameshark += "8107ECD0 " + col8.substr(0, 4) + "\n";
        gameshark += "8107ECD2 " + col8.substr(4, 2) + "00\n";
        gameshark += "8107ECC8 " + col8.substr(6, 4) + "\n";
        gameshark += "8107ECCA " + col8.substr(10, 2) + "00\n";
        gameshark += "8107ECE8 " + col9.substr(0, 4) + "\n";
        gameshark += "8107ECEA " + col9.substr(4, 2) + "00\n";
        gameshark += "8107ECE0 " + col9.substr(6, 4) + "\n";
        gameshark += "8107ECE2 " + col9.substr(10, 2) + "00\n";
        gameshark += "8107ED00 " + col10.substr(0, 4) + "\n";
        gameshark += "8107ED02 " + col10.substr(4, 2) + "00\n";
        gameshark += "8107ECF8 " + col10.substr(6, 4) + "\n";
        gameshark += "8107ECFA " + col10.substr(10, 2) + "00\n";
        gameshark += "8107ED18 " + col11.substr(0, 4) + "\n";
        gameshark += "8107ED1A " + col11.substr(4, 2) + "00\n";
        gameshark += "8107ED10 " + col11.substr(6, 4) + "\n";
        gameshark += "8107ED12 " + col11.substr(10, 2) + "00\n";
        gameshark += "8107ED30 " + col12.substr(0, 4) + "\n";
        gameshark += "8107ED32 " + col12.substr(4, 2) + "00\n";
        gameshark += "8107ED28 " + col12.substr(6, 4) + "\n";
        gameshark += "8107ED2A " + col12.substr(10, 2) + "00";
    }

    gscode.GameShark = gameshark;
    return gscode;
}

/*
    Applies a ColorCode to the game, overwriting the vanilla palette.
*/
void ApplyColorCode(GameSharkCode colorCode, MarioActor* actor) {
    current_color_code = colorCode;
    PasteGameShark(colorCode.GameShark, actor->colorcode);
}

std::vector<std::string> GetColorCodeList(std::string folderPath) {
    std::vector<std::string> cc_list;

    if (folderPath.find("dynos/packs") != std::string::npos) {
        if (fs::exists(folderPath + "/../default.gs")) {
            cc_list.push_back("../default.gs");
        }
        
        if (fs::exists(folderPath)) {
            for (const auto & entry : fs::directory_iterator(folderPath)) {
                fs::path path = entry.path();

                if (path.filename().string() != "Mario") {
                    if (path.extension().string() == ".gs")
                        cc_list.push_back(path.filename().string());
                }
            }
        }
    } else {
        if (fs::exists(folderPath)) {
            cc_list.push_back("Mario.gs");

            for (const auto & entry : fs::directory_iterator(folderPath)) {
                fs::path path = entry.path();
                
                if (fs::is_directory(path)) continue;
                if (path.filename().string() != "Mario") {
                    if (path.extension().string() == ".gs")
                        cc_list.push_back(path.filename().string());
                }
            }
        }
    }

    return cc_list;
}

GameSharkCode LoadGSFile(std::string fileName, std::string filePath, MarioActor* actor) {
    GameSharkCode colorCode;

    if (fileName == "../default.gs") {
        filePath = fs::path(filePath).parent_path().string();
        fileName = "default.gs";
    }

    if (fileName != "Mario.gs") {
        std::ifstream file(filePath + "/" + fileName, std::ios::in | std::ios::binary);
        if (file.good()) {
            std::cout << "Loaded CC file: " << filePath << "/" << fileName << std::endl;

            // Read GS File
            const std::size_t& size = fs::file_size(filePath + "/" + fileName);
            std::string content(size, '\0');
            file.read(content.data(), size);
            file.close();

            // i hate how nested this shit is
            int jsonBegin = content.find('{');
            if (jsonBegin != std::string::npos) {
                std::string json = content.substr(jsonBegin);
                content = content.substr(0, jsonBegin - 1);
                if (actor) {
                    Json::Value root;
                    std::stringstream stream = std::stringstream(json);
                    root << stream;
                    for (auto& entry : root.object()) {
                        if (entry.first == "enable_custom_eyes") {
                            actor->custom_eyes = entry.second.asBool();
                            continue;
                        }
                        // why the fuck is this not a map
                        for (auto& expr : actor->model.Expressions) {
                            if (expr.Name != entry.first) continue;
                            // kill me
                            for (int i = 0; i < expr.Textures.size(); i++) {
                                if (expr.Textures[i].FileName != entry.second.asString()) continue;
                                expr.CurrentIndex = i;
                            }
                        }
                    }
                }
            }

            // Write to CC
            colorCode.Name = fileName.substr(0, fileName.size() - 3);
            colorCode.GameShark = content;
            colorCode.IsModel = (filePath.find("dynos/packs") != std::string::npos);

        } else {
            // Load failed; Refresh active list
            std::cout << "Failed to load " << fileName.c_str() << std::endl;
            if (filePath.find("dynos/packs") != std::string::npos) model_color_code_list = GetColorCodeList(filePath);
            else color_code_list = GetColorCodeList(filePath);
        }
    }

    // Change conflicting file names
    if (colorCode.Name == "Mario" || colorCode.Name == "default")
        colorCode.Name = "Sample";

    return colorCode;
}

void SaveGSFile(GameSharkCode colorCode, std::string filePath, MarioActor* actor, bool expr_export) {
    // Change conflicting file names
    if (colorCode.Name == "Mario" || colorCode.Name == "default")
        colorCode.Name = "Sample";

    // Create "/colorcodes" directory if it doesn't already exist
    if (!fs::exists(filePath))
        fs::create_directory(filePath);

    std::ofstream file(filePath + "/" + colorCode.Name + ".gs");
    file << colorCode.GameShark;
    if (actor && expr_export) {
        file << "\n{\n";
        for (const auto& expr : actor->model.Expressions) {
            file << "   \"" << expr.Name << "\": \"" << expr.Textures[expr.CurrentIndex].FileName << "\",\n";
        }
        file << "   \"enable_custom_eyes\": " << (actor->custom_eyes ? "true" : "false") << "\n}";
    }
    file.close();
}

void DeleteGSFile(std::string filePath) {
    // Disallow paths that reach out of bounds
    if (filePath.find("/colorcodes/") == std::string::npos ||
        filePath.find("../") != std::string::npos)
            return;

    if (fs::exists(filePath))
        fs::remove(filePath);
}

void saturn_refresh_cc_count() {
    //cc_details = "" + std::to_string(model_color_code_list.size()) + " color code";
    //if (model_color_code_list.size() != 1) cc_details += "s";
}