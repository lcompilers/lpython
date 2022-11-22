#ifndef LPYTHON_UTILS_H
#define LPYTHON_UTILS_H

#include <string>
#include <libasr/utils.h>

namespace LCompilers::LPython {

bool is_directory(std::string path);
bool path_exists(std::string path);

// Decodes the exit status code of the process (in Unix)
int32_t get_exit_status(int32_t err);

std::string generate_visualize_html(std::string &astr_data_json);

} // LFortran

#endif // LPYTHON_UTILS_H
