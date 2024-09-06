#include "saturn/filesystem/saturn_animfile.h"

#include "saturn/filesystem/saturn_format.h"
#include <vector>

extern "C" {
#include "pc/platform.h"
}

const int curr_ver = 2;

std::vector<int> favorite_anims = {};

bool saturn_favorite_anim_handler(SaturnFormatStream* stream, int version) {
    favorite_anims.push_back(saturn_format_read_int32(stream));
    return true;
}

bool saturn_favorite_anim_data_handler(SaturnFormatStream* stream, int version) {
    int count = saturn_format_read_int32(stream);
    for (int i = 0; i < count; i++) {
        saturn_favorite_anim_handler(stream, version);
    }
    return true;
}

void saturn_load_favorite_anims() {
    char anim_favorites_path[SYS_MAX_PATH] = "";
    strncat(anim_favorites_path,  sys_user_path(), SYS_MAX_PATH - 1);
    strncat(anim_favorites_path, "/dynos/anim_favorites.bin", SYS_MAX_PATH - 1);

    favorite_anims.clear();
    saturn_format_input(anim_favorites_path, "STFA", {
        { "DATA", saturn_favorite_anim_data_handler }, // legacy format
        { "FVAN", saturn_favorite_anim_handler }
    });
}

void saturn_save_favorite_anims() {
    char anim_favorites_path[SYS_MAX_PATH] = "";
    strncat(anim_favorites_path,  sys_user_path(), SYS_MAX_PATH - 1);
    strncat(anim_favorites_path, "/dynos/anim_favorites.bin", SYS_MAX_PATH - 1);

    SaturnFormatStream stream = saturn_format_output("STFA", curr_ver);
    for (int anim : favorite_anims) {
        saturn_format_new_section(&stream, "FVAN");
        saturn_format_write_int32(&stream, anim);
        saturn_format_close_section(&stream);
    }
    saturn_format_write(anim_favorites_path, &stream);
}