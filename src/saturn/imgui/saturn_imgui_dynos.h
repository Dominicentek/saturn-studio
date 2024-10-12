#ifndef SaturnImGuiDynOS
#define SaturnImGuiDynOS

#ifdef __cplusplus
#include <string>
#include <vector>

extern bool is_gameshark_open;

extern std::vector<std::string> choose_file_dialog(std::string windowTitle, std::vector<std::string> filetypes, bool multiselect);
extern std::string save_file_dialog(std::string windowTitle, std::vector<std::string> filetypes);
extern void open_file(std::string path);

extern "C" {
#endif
    //void apply_cc_from_editor(void);
    void sdynos_imgui_init(void);
    void sdynos_imgui_menu(int index);
    void saturn_show_crash_dialog();
#ifdef __cplusplus
}
#endif

#endif