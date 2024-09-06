#include "saturn_imgui_file_browser.h"

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "saturn/libs/imgui/imgui.h"
#include "saturn/imgui/icons/IconsForkAwesome.h"

class FileBrowserEntry {
private:
    std::string _name;
    bool _dir = false;
    std::vector<FileBrowserEntry> entries = {};
    FileBrowserEntry* _parent;
public:
    FileBrowserEntry(std::string name, bool dir, FileBrowserEntry* parent) {
        _name = name;
        _dir = dir;
        _parent = parent;
    }
    bool is_dir() { return _dir; }
    std::vector<FileBrowserEntry> dir() { return entries; }
    FileBrowserEntry* parent() { return _parent; }
    std::string name() { return _name; }
    void clear() { entries.clear(); }
    void add_file(std::string name) { entries.push_back(FileBrowserEntry(name, false, this)); }
    FileBrowserEntry* add_dir(std::string name) {
        entries.push_back(FileBrowserEntry(name, true, this));
        return &entries[entries.size() - 1];
    }
    void merge(FileBrowserEntry* entry) {
        for (FileBrowserEntry& e : entry->entries) {
            entries.push_back(e);
        }
    }
};

FileBrowserEntry root = FileBrowserEntry("root", true, nullptr);
FileBrowserEntry* curr = &root;
int browser_height = 150;
fs_relative::path selected_path;
std::vector<std::string> extension_filters = {};
std::map<std::string, char*> search_terms = {};
std::map<std::string, std::string> selected_paths = {};
std::map<fs::path, FileBrowserEntry*> scanned_paths = {};
fs::path last_scanned_path;

void saturn_file_browser_item(std::string item) {
    curr->add_file(item);
}

void saturn_file_browser_tree_node(std::string item) {
    curr = curr->add_dir(item);
}

void saturn_file_browser_tree_node_end() {
    curr = curr->parent();
}

void saturn_file_browser_scan_directory_internal(fs::path dir, bool recursive, FileBrowserEntry* currfolder) {
    std::vector<std::string> dirs = {};
    std::vector<std::string> files = {};
    if (!fs::exists(dir)) return;
    for (const auto& entry : fs::directory_iterator(dir)) {
        fs::path path = entry.path();
        if (fs::is_directory(path)) {
            if (!recursive) continue;
            dirs.push_back(path.filename().string());
            continue;
        }
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        bool allowed = false;
        if (extension_filters.empty()) allowed = true;
        else for (std::string extension : extension_filters) {
            if (ext == "." + extension) {
                allowed = true;
                break;
            }
        }
        if (!allowed) continue;
        files.push_back(path.filename().string());
    }
    auto stringcomp = [](std::string a, std::string b) {
        return a < b;
    };
    std::sort(dirs.begin(), dirs.end(), stringcomp);
    std::sort(files.begin(), files.end(), stringcomp);
    for (std::string dirname : dirs) {
        FileBrowserEntry* folder = currfolder->add_dir(dirname);
        saturn_file_browser_scan_directory_internal(dir / dirname, true, folder);
    }
    for (std::string filename : files) {
        currfolder->add_file(filename);
    }
}

void saturn_file_browser_scan_directory(fs::path dir, bool recursive) {
    last_scanned_path = dir;
    FileBrowserEntry* entry;
    if (scanned_paths.find(dir) != scanned_paths.end()) entry = scanned_paths[dir];
    else {
        entry = new FileBrowserEntry("root", true, nullptr);
        saturn_file_browser_scan_directory_internal(dir, recursive, entry);
        scanned_paths.insert({ dir, entry });
    }
    curr->merge(entry);
}

void saturn_file_browser_rescan_directory(fs::path dir, bool recursive) {
    if (scanned_paths.find(dir) != scanned_paths.end()) {
        free(scanned_paths[dir]);
        scanned_paths.erase(dir);
    }
    saturn_file_browser_scan_directory(dir, recursive);
}

void saturn_file_browser_filter_extensions(std::vector<std::string> extensions) {
    extension_filters = {};
    for (int i = 0; i < extensions.size(); i++) {
        std::string ext = std::string(extensions[i]);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        extension_filters.push_back(ext);
    }
}

void saturn_file_browser_filter_extension(std::string extension) {
    saturn_file_browser_filter_extensions({ extension });
}

void saturn_file_browser_height(int height) {
    browser_height = height;
}

void saturn_file_browser_clear() {
    root.clear();
    browser_height = 150;
    extension_filters = {};
}

bool saturn_file_browser_create_imgui(FileBrowserEntry dir, std::string path, std::string browser_id, bool do_search) {
    bool clicked = false;
    for (FileBrowserEntry& entry : dir.dir()) {
        if (entry.is_dir()) {
            if (ImGui::TreeNode(entry.name().c_str())) {
                clicked |= saturn_file_browser_create_imgui(entry, path + entry.name() + "/", browser_id, do_search);
                ImGui::TreePop();
            }
        }
        else {
            std::string filename = entry.name();
            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
            if (do_search) if (filename.find(search_terms[browser_id]) == std::string::npos) continue;
            std::string fullpath = path + entry.name();
            bool selected = selected_paths[browser_id] == fullpath;
            if (ImGui::Selectable(entry.name().c_str(), &selected)) {
                selected_paths[browser_id] = fullpath;
                selected_path = fullpath;
                clicked = true;
            }
        }
    }
    return clicked;
}

bool saturn_file_browser_show(std::string id) {
    selected_path = "";
    if (selected_paths.find(id) == selected_paths.end()) selected_paths.insert({ id, "" });
    if (search_terms.find(id) == search_terms.end()) {
        char* search = (char*)malloc(256);
        search[0] = 0;
        search_terms.insert({ id, search });
    }
    if (ImGui::Button(((ICON_FK_REFRESH "###refresh_file_browser_") + id).c_str())) {
        if (scanned_paths.find(last_scanned_path) != scanned_paths.end()) {
            free(scanned_paths[last_scanned_path]);
            scanned_paths.erase(last_scanned_path);
        }
    }
    ImGui::SameLine();
    ImGui::InputTextWithHint(("###searchbar_file_browser_" + id).c_str(), ICON_FK_SEARCH " Search...", search_terms[id], 256);
    ImGui::BeginChild(("###file_browser_" + id).c_str(), ImVec2(0, browser_height), true);
    bool result = saturn_file_browser_create_imgui(root, "", id, true);
    ImGui::EndChild();
    saturn_file_browser_clear();
    return result;
}

bool saturn_file_browser_show_tree(std::string id) {
    bool result = saturn_file_browser_create_imgui(root, "", id, false);
    saturn_file_browser_clear();
    return result;
}

fs_relative::path saturn_file_browser_get_selected() {
    return selected_path;
}