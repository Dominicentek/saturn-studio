#ifndef SaturnEmbeddedFilesystem_H
#define SaturnEmbeddedFilesystem_H

#include <vector>
#include "../saturn.h"

struct File {
    int type;
    char name[256];
    unsigned char* data;
    int data_length;
    std::vector<struct FileEntry> _1;
};

struct Folder {
    int type;
    char name[256];
    unsigned char* _1;
    int _2;
    std::vector<struct FileEntry> entries;
};

struct FileEntry {
    int type;
    char name[256];
    unsigned char* _1;
    int _2;
    std::vector<struct FileEntry> _3;
};

struct FileEntry saturn_embedded_filesystem_from_local_storage(fs::path path);
void saturn_embedded_filesystem_to_local_storage(struct FileEntry* entry, fs::path dest);
void saturn_embedded_filesystem_free(struct FileEntry* entry);

#endif