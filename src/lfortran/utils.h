#ifndef LFORTRAN_UTILS_H
#define LFORTRAN_UTILS_H

#include <string>
#include <lfortran/containers.h>

namespace LFortran {

enum Platform {
    Linux, macOS, Windows
};

Platform get_platform();

struct CompilerOptions {
    bool fixed_form = false;
    bool symtab_only = false;
    bool show_stacktrace = false;
    bool use_colors = true;
    Platform platform;

    CompilerOptions () : platform{get_platform()} {};
};


void get_executable_path(std::string &executable_path, int &dirname_length);
std::string get_runtime_library_dir();
std::string read_file(const std::string &filename);
bool present(Vec<char*> &v, const char* name);

} // LFortran

#endif // LFORTRAN_UTILS_H
