#include "saturn_animations.h"

#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <SDL2/SDL.h>

#include "saturn/saturn.h"
#include "saturn/imgui/saturn_imgui.h"

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"

#include "saturn/saturn_animation_ids.h"

#include "pc/configfile.h"

extern "C" {
#include "game/camera.h"
#include "game/level_update.h"
#include "sm64.h"
}

using namespace std;
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
namespace fs = std::filesystem;
#include "pc/fs/fs.h"

#include "saturn/saturn_json.h"
#include "saturn/saturn_actors.h"

typedef struct {
    std::string name;
    std::string author;
    std::vector<s16> values;
    std::vector<s16> indices;
    bool extra;
    int length;
} CustomAnim;

std::vector<string> canim_array;
std::map<std::string, CustomAnim> canims;
std::string current_anim_dir_path;
std::vector<std::string> previous_anim_paths;

std::string chainer_name;

extern "C" {
#include "game/mario.h"
}

#define gMarioAnims mario_animation_data
#include "assets/mario_anim_data.c"
#undef gMarioAnims

void saturn_load_anim_folder(string path, int* index) {
    canim_array.clear();

    // If anim folder is misplaced
    if (!fs::exists("dynos/anims/"))
        return;

    // Go back a subfolder
    if (path == "../") {
        // Only go back if the previous directory actually exists
        if (previous_anim_paths.size() < 1 || !fs::exists(previous_anim_paths[previous_anim_paths.size() - 2])) {
            path = "";
            current_anim_dir_path = "dynos/anims/";
            previous_anim_paths.clear();
        } else {
            current_anim_dir_path = previous_anim_paths[previous_anim_paths.size() - 2];
            previous_anim_paths.pop_back();
        }
    }
    if (path == "") current_anim_dir_path = "dynos/anims/";

    // only update current path if folder exists
    if (fs::is_directory(current_anim_dir_path + path) && path != "../") {
        previous_anim_paths.push_back(current_anim_dir_path + path);
        current_anim_dir_path = current_anim_dir_path + path;
    }

    if (current_anim_dir_path != "dynos/anims/") {
        canim_array.push_back("../");
    }

    for (const auto & entry : fs::directory_iterator(current_anim_dir_path)) {
        fs::path path = entry.path();

        if (path.extension().string() == ".json") {
            string filename = path.filename().string().substr(0, path.filename().string().size() - 5);
            if (::isdigit(filename.back()) && filename.find("_") != string::npos) {
                // Ignore
            } else {
                canim_array.push_back(path.filename().string());
            }
        }
        if (fs::is_directory(entry.path())) {
            canim_array.push_back(entry.path().stem().string() + "/");
        }
    }

    for (int j = 0; j < canim_array.size(); j++) {
        if (canim_array[j].find("/") == string::npos) {
            *index = j;
            break;
        }
    }
}

string current_canim_name;
string current_canim_author;
bool current_canim_looping;
int current_canim_length;
int current_canim_nodes;
std::vector<s16> current_canim_values;
std::vector<u16> current_canim_indices;
bool current_canim_has_extra;

void run_hex_array(Json::Value array, std::vector<s16>* dest) {
    string even_one, odd_one;
    for (int i = 0; i < array.size(); i++) {
        if (i % 2 == 0) {
            // Run on even
            even_one = array[i].asString();
        } else {
            // Run on odd
            odd_one = array[i].asString();

            int out = std::stoi(even_one, 0, 16) * 256 + std::stoi(odd_one, 0, 16);
            dest->push_back(out);
        }
    }
}

std::tuple<int, std::vector<s16>, std::vector<s16>> read_bone_data(Json::Value root) {
    std::vector<s16> values = {};
    std::vector<s16> indices = {};
    int length = root["length"].asInt();
    current_canim_nodes = root["nodes"].asInt();
    run_hex_array(root["values"], (std::vector<s16>*)&values);
    run_hex_array(root["indices"], (std::vector<s16>*)&indices);
    return { length, values, indices };
}

void load_cached_mcomp_animation(MarioActor* actor, string cache_key) {
    CustomAnim anim = canims[cache_key];
    actor->animstate.customanim_name = anim.name;
    actor->animstate.customanim_author = anim.author;
    actor->animstate.customanim_extra = anim.extra;
    actor->animstate.customanim_values = anim.values;
    actor->animstate.customanim_indices = anim.indices;
    actor->animstate.length = anim.length;
}

void saturn_read_mcomp_animation(MarioActor* actor, string anim_path) {
    if (canims.find(anim_path) != canims.end()) {
        load_cached_mcomp_animation(actor, anim_path);
        return;
    }
    // Load the json file
    std::ifstream file(current_anim_dir_path + anim_path);
    if (!file.good()) {
        std::cout << "failed to load anim " << anim_path << std::endl;
        actor->animstate.id = MARIO_ANIM_A_POSE;
        actor->animstate.frame = 0;
        actor->animstate.custom = false;
        return;
    }

    // Check if we should enable chainer
    // This is only the case if we have a followup animation
    // i.e. specialist.json, specialist_1.json
    /*if (!using_chainer) {
        std::ifstream file_c(current_anim_dir_path + json_path + "_1.json");
        if (file_c.good() && chainer_index == 0) {
            using_chainer = true;
            chainer_name = json_path;
            // Chainer only works with looping off
            //is_anim_looped = false;
        }
    } else {
        // Check if we're at the end of our chain
        std::ifstream file_c(current_anim_dir_path + chainer_name + "_" + std::to_string(chainer_index) + ".json");
        if (!file_c.good()) {
            //if (is_anim_looped) {
                // Looping restarts from the beginning
            //    chainer_index = 0;
            //    saturn_read_mcomp_animation(chainer_name);
            //    saturn_play_animation(MARIO_ANIM_A_POSE);
            //    saturn_play_custom_animation();
            //    return;
            //}
            using_chainer = false;
            chainer_index = 0;
            is_anim_playing = false;
            is_anim_paused = false;
            return;
        }
    }*/

    CustomAnim anim;
    std::filesystem::path path = anim_path;
    if (path.extension().string() == ".panim") {
        printf("reading as panim\n");
        int length = std::filesystem::file_size(path);
        unsigned char* data = (unsigned char*)malloc(length);
        file.read((char*)data, length);
        char name[33], author[33];
        memcpy(name, data + 0x00, 32);
        memcpy(author, data + 0x20, 32);
        name[32] = 0; // in case the string has 32 chars
        author[32] = 0;
        anim.name = name;
        anim.author = author;
        anim.length = (int)data[0x41] + (int)data[0x42] * 0x100;
        int ptr = 0x43;
        std::vector<s16>* curr_array;
        while (ptr < length) {
            if (strcmp((char*)data + ptr, "values")  == 0) {
                curr_array = &anim.values;
                ptr += 6;
                continue;
            }
            if (strcmp((char*)data + ptr, "indices") == 0) {
                curr_array = &anim.indices;
                ptr += 7;
                continue;
            }
            curr_array->push_back((int)data[ptr] * 256 + (int)data[ptr + 1]);
            ptr += 2;
        }
        free(data);
        file.close();
    }
    else {
        printf("reading as json\n");
        Json::Value root;
        root << file;
        anim.name = root["name"].asString();
        anim.author = root["author"].asString();
        if (root.isMember("extra_bone")) {
            if (root["extra_bone"].asString() == "true") anim.extra = true;
            if (root["extra_bone"].asString() == "false") anim.extra = false;
        } else { anim.extra = false; }
        // A mess
        if (root["looping"].asString() == "true") current_canim_looping = true;
        if (root["looping"].asString() == "false") current_canim_looping = false;
        auto [ length, values, indices ] = read_bone_data(root);
        anim.length = length;
        anim.values = values;
        anim.indices = indices;
    }

    canims.insert({ anim_path, anim });
    load_cached_mcomp_animation(actor, anim_path);
}

void saturn_play_custom_animation() {
    gMarioState->animation->targetAnim->flags = 0;
    gMarioState->animation->targetAnim->unk02 = 0;
    gMarioState->animation->targetAnim->unk04 = 0;
    gMarioState->animation->targetAnim->unk06 = 0;
    gMarioState->animation->targetAnim->unk08 = (s16)current_canim_length;
    gMarioState->animation->targetAnim->unk0A = current_canim_indices.size() / 6 - 1;
    gMarioState->animation->targetAnim->values = current_canim_values.data();
    gMarioState->animation->targetAnim->index = current_canim_indices.data();
    gMarioState->animation->targetAnim->length = 0;
    gMarioState->marioObj->header.gfx.unk38.curAnim = gMarioState->animation->targetAnim;
}

void saturn_run_chainer() {
    // todo
    /*if (is_anim_playing && current_animation.custom) {
        if (is_anim_past_frame(gMarioState, (int)gMarioState->marioObj->header.gfx.unk38.curAnim->unk08) || is_anim_at_end(gMarioState)) {
            // Check if our next animation exists
            std::ifstream file_c1(current_anim_dir_path + chainer_name + "_" + std::to_string(chainer_index) + ".json");
            string test = current_anim_dir_path + chainer_name + "_" + std::to_string(chainer_index) + ".json";
            std::cout << test << std::endl;
            if (file_c1.good()) {
                saturn_read_mcomp_animation(chainer_name + "_" + std::to_string(chainer_index));
                saturn_play_animation(MARIO_ANIM_A_POSE);
                saturn_play_custom_animation();
            } else {
                if (current_animation.loop) {
                    // Looping restarts from the beginning
                    is_anim_playing = false;
                    using_chainer = false;
                    chainer_index = 0;
                    saturn_read_mcomp_animation(chainer_name);
                    saturn_play_animation(MARIO_ANIM_A_POSE);
                    saturn_play_custom_animation();
                } else {
                    using_chainer = false;
                    chainer_index = 0;
                    is_anim_playing = false;
                    is_anim_paused = false;
                }
            }
        }
    }*/
}

void load_animation(struct Animation* out, int index) {
    struct Animation* anim = (struct Animation*)((u8*)&mario_animation_data + mario_animation_data.entries[index].offset);
    out->flags = anim->flags;
    out->length = anim->length;
    out->unk02 = anim->unk02;
    out->unk04 = anim->unk04;
    out->unk06 = anim->unk06;
    out->unk08 = anim->unk08;
    out->unk0A = anim->unk0A;
    out->values = (const s16*)((u8*)anim + (uintptr_t)anim->values);
    out->index  = (const u16*)((u8*)anim + (uintptr_t)anim->index );
}

void saturn_sample_animation(MarioActor* actor, struct Animation* anim, int frame) {
    const u16* curindex = anim->index;
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 3; j++) {
            int valindex = 0;
            if (frame < curindex[0]) valindex = curindex[1] + frame;
            else valindex = curindex[1] + curindex[0] - 1;
            curindex += 2;
            actor->bones[i][j] = (float)(anim->values[valindex]) * (i == 0 ? 1 : 360.f / 65536.f);
        }
    }
}
