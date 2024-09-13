#ifndef SaturnTextures
#define SaturnTextures

#include <stdio.h>
#include <stdbool.h>


#ifdef __cplusplus
#include <string>
#include <vector>
#include <iostream>

#include "saturn.h"

extern "C" {
#include "pc/platform.h"
}
class Model;

class TexturePath {
public:
    std::string FileName = "";
    std::string FilePath = "";
    /* Relative path from res/gfx, as used by EXTERNAL_DATA */
    // For AppImages etc, I changed FilePath to be an absolute path, so now
    // this has to be properly a conversion from absolute to relative path
    std::string GetRelativePath() {
        // return "../../" + this->FilePath;//.substr(0, this->FilePath.size() - 4);
        return fs_relative::relative(fs_relative::path(this->FilePath),
               fs_relative::path(std::string(sys_user_path()) + "/res/gfx")).generic_string();
    }
    /* Parent directory, used for subfolders */
    std::string ParentPath() {
        return FilePath.substr(0, this->FilePath.length() - this->FileName.length());
    }
    std::string DynosPath() {
        return FilePath.substr(std::string("dynos/eyes/").length());
    }
    
    bool IsModelTexture = false;
};

class Expression {
private:
    std::string ReplaceKey() {
        return "saturn_" + this->Name;
    }
public:
    std::string Name = "";
    std::string FolderPath = "";
    std::vector<TexturePath> Textures = {};
    /* The index of the current selected texture */
    int CurrentIndex = 0;

    /* Returns true if a given path contains a prefix (saturn_) followed by the expression's replacement keywords
       For example, "/eye" can be "saturn_eye" or "saturn_eyes" */
    bool PathHasReplaceKey(std::string path, std::string prefix) {
        // We also append a _ or . to the end, so "saturn_eyebrow" doesn't get read as "saturn_eye"
        return (path.find(prefix + this->Name + "_") != std::string::npos ||
                path.find(prefix + this->Name + "_s") != std::string::npos ||
                path.find(prefix + this->Name.substr(0, this->Name.size() - 1) + "_") != std::string::npos);
    }
};

extern bool custom_eyes_enabled;

extern Expression VanillaEyes;
extern void LoadEyesFolder(Model*);

std::vector<TexturePath> LoadExpressionTextures(std::string, Expression);
std::vector<Expression> LoadExpressions(Model*, std::string);

void saturn_copy_file(std::string from, std::string to);
void saturn_delete_file(std::string file);
std::size_t number_of_files_in_directory(fs::path path);

extern bool show_vmario_emblem;

extern "C" {
#endif
    const void* saturn_bind_texture(const void*);
#ifdef __cplusplus
}
#endif

#endif