#include <string>
#include <fstream>
#include <filesystem>
#include <zip.h>

#include "downloader.h"
#include "picojson.h"

#include "gui.inc.cpp"

#define EXIT_SUCCESS       0
#define EXIT_NO_ARGS       1
#define EXIT_INVALID_CMD   2
#define EXIT_INVALID_EXE   3
#define EXIT_NETWORK_ERROR 4
#define EXIT_LATEST_VER    5
#define EXIT_CAN_UPDATE    6

const char* exit_msgs[] = {
    "",

    "Saturn Studio Update Tool\n"
    " by Dominicentek\n"
    "\n"
    "Usage:\n"
    "  %1$s update <executable> - Update a Saturn Studio executable\n"
    "  %1$s check <executable>  - Check if it's the latest version\n",

    "Invalid command: %s\n",
    "Invalid or missing executable\n",
    "Network error\n",
    "Already up-to-date\n",
    "Update possible\n",
};

#define PREFIX "Version: "
#define PREFIX_SIZE (sizeof(PREFIX) - 1)

#define OWNER "Dominicentek"
#define REPO  "saturn-studio"
#ifdef WINDOWS
#define TARGET_OS "windows"
#define EXE ".exe"
#else
#define TARGET_OS "linux"
#define EXE
#endif

#define exit(code, ...) {                                 \
    printf(exit_msgs[code] __VA_OPT__(,) __VA_ARGS__, ""); \
    exit(code);                                             \
}

std::string extract_version(std::string filename) {
    if (!std::filesystem::exists(filename)) return "";
    std::ifstream stream = std::ifstream(filename);
    size_t size = std::filesystem::file_size(filename);
    char* data = (char*)malloc(size);
    stream.read(data, size);
    stream.close();
    for (size_t i = 0; i < size - PREFIX_SIZE; i++) {
        bool success = true;
        for (size_t j = 0; j < PREFIX_SIZE; j++) {
            if (data[i + j] != PREFIX[j]) {
                success = false;
                break;
            }
        }
        if (success) {
            i += PREFIX_SIZE;
            std::string version = data + i;
            if (version[version.size() - 1] == '\n') version = version.substr(0, version.size() - 1);
            free(data);
            return version;
        }
    }
    free(data);
    return "";
}

std::string get_latest_version() {
    Downloader downloader("https://api.github.com/repos/" OWNER "/" REPO "/releases/latest");
    downloader.download();
    if (downloader.status < 200 || downloader.status > 299) return "";
    picojson::value json;
    picojson::parse(json, downloader.data.data());
    return json.get("name").get<std::string>();
}

void unzip(char* inbuf, size_t insiz, char** outbuf, size_t* outsiz) {
    zip_error_t error;
    zip_stat_t file_stat;
    zip_source_t* zip_src = zip_source_buffer_create(inbuf, insiz, 0, &error);
    zip_t* zip_archive = zip_open_from_source(zip_src, 0, &error);
    zip_int64_t file_index = zip_name_locate(zip_archive, "saturn.us.f3dex2e" EXE, ZIP_FL_NOCASE);
    zip_file_t* zip_file = zip_fopen_index(zip_archive, file_index, 0);
    zip_stat_index(zip_archive, file_index, 0, &file_stat);
    *outsiz = file_stat.size;
    *outbuf = (char*)malloc(*outsiz);
    zip_fread(zip_file, *outbuf, *outsiz);
    zip_fclose(zip_file);
    zip_close(zip_archive);
}

int main(int argc, char** argv) {
    if (argc != 3) exit(EXIT_NO_ARGS, argv[0]);

    std::string version = extract_version(argv[2]);
    if (version.empty()) exit(EXIT_INVALID_EXE);

    if (argv[1] != std::string("update") &&
        argv[1] != std::string("check")
    ) exit(EXIT_INVALID_CMD, argv[1]);

    std::string latest_version = get_latest_version();
    if (latest_version.empty()) exit(EXIT_NETWORK_ERROR);

    printf("Current version: %s\nLatest version: %s\n", version.c_str(), latest_version.c_str());

    if (version == latest_version) exit(EXIT_LATEST_VER);
    if (argv[1] == std::string("check")) exit(EXIT_CAN_UPDATE);

    std::string url = "https://github.com/" OWNER "/" REPO "/releases/download/" + latest_version + "/" TARGET_OS ".zip";
    struct GUIHandle* handle = gui_init(version.c_str(), latest_version.c_str());
    Downloader downloader(url);
    downloader.progress([&](double now, double total){
        double ratio;
        if (total == 0) ratio = 0;
        else ratio = now / total;
        gui_update(handle, ratio);
    });
    downloader.download();
    gui_quit(handle);

    char* executable;
    size_t executable_size;
    unzip(downloader.data.data(), downloader.data.size(), &executable, &executable_size);
    
    std::ofstream stream = std::ofstream(argv[2]);
    stream.write(executable, executable_size);
    stream.close();

    free(executable);

    exit(EXIT_SUCCESS);
}