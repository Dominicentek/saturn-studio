#ifndef SaturnImGuiFileBrowser
#define SaturnImGuiFileBrowser

#include <string>
#include "saturn/saturn.h"
#include <vector>

void saturn_file_browser_item(std::string item);
void saturn_file_browser_tree_node(std::string name);
void saturn_file_browser_tree_node_end();
void saturn_file_browser_scan_directory(fs::path path, bool recursive = true);
void saturn_file_browser_rescan_directory(fs::path path, bool recursive = true);
void saturn_file_browser_filter_extension(std::string extension);
void saturn_file_browser_filter_extensions(std::vector<std::string> extensions);
void saturn_file_browser_height(int height);
bool saturn_file_browser_show(std::string id);
bool saturn_file_browser_show_tree(std::string id);
fs_relative::path saturn_file_browser_get_selected();


#endif