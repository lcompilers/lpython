#ifndef LCOMPILERS_UTILS_H
#define LCOMPILERS_UTILS_H

#include <string>
#include <libasr/utils.h>

namespace LCompilers {

void get_executable_path(std::string &executable_path, int &dirname_length);
std::string get_runtime_library_dir();
std::string get_runtime_library_header_dir();

} // namespace LCompilers

#endif // LCOMPILERS_UTILS_H
