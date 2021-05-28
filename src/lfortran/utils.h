#ifndef LFORTRAN_UTILS_H
#define LFORTRAN_UTILS_H

#include <string>

namespace LFortran {

void get_executable_path(std::string &executable_path, int &dirname_length);
std::string get_runtime_library_dir();
std::string read_file(const std::string &filename);

} // LFortran

#endif // LFORTRAN_UTILS_H
