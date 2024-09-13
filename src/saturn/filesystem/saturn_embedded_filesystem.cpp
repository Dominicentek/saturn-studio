#include "saturn_embedded_filesystem.h"

#include <fstream>
#include <iostream>
#include <string.h>

struct FileEntry saturn_embedded_filesystem_from_local_storage(fs::path path) {
    if (fs::is_directory(path)) {
        struct Folder folder;
        folder.type = 1;
        strncpy(folder.name, path.filename().string().c_str(), 255);
        folder.entries = {};
        for (auto entry : fs::directory_iterator(path)) {
            folder.entries.push_back(saturn_embedded_filesystem_from_local_storage(entry.path()));
        }
        return *(struct FileEntry*)&folder;
    }
    else {
        struct File file;
        file.type = 0;
        strncpy(file.name, path.filename().string().c_str(), 255);
        file.data_length = fs::file_size(path);
        file.data = (unsigned char*)malloc(file.data_length);
        std::ifstream stream = std::ifstream(path, std::ios::binary);
        stream.read((char*)file.data, file.data_length);
        stream.close();
        return *(struct FileEntry*)&file;
    }
}

void saturn_embedded_filesystem_to_local_storage(struct FileEntry* entry, fs::path dest) {
    if (entry->type == 0) {
        struct File* file = (struct File*)entry;
        fs::create_directories((dest / file->name).parent_path());
        std::ofstream stream = std::ofstream(dest / file->name, std::ios::binary);
        stream.write((char*)file->data, file->data_length);
        stream.close();
    }
    else {
        struct Folder* folder = (struct Folder*)entry;
        fs::create_directories(dest / folder->name);
        for (struct FileEntry e : folder->entries) {
            saturn_embedded_filesystem_to_local_storage(&e, dest / folder->name);
        }
    }
}

void saturn_embedded_filesystem_free(struct FileEntry* entry) {
    if (!entry) return;
    if (entry->type == 0) {
        free(((struct File*)entry)->data);
    }
    else {
        for (struct FileEntry e : ((struct Folder*)entry)->entries) {
            saturn_embedded_filesystem_free(&e);
        }
    }
}
