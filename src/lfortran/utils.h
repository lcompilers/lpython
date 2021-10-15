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
    bool c_preprocessor = false;
    std::vector<std::string> c_preprocessor_defines;
    bool prescan = true;
    bool symtab_only = false;
    bool show_stacktrace = false;
    bool use_colors = true;
    bool indent = false;
    bool fast = false;
    bool openmp = false;
    std::string target = "";
    Platform platform;

    CompilerOptions () : platform{get_platform()} {};
};


void get_executable_path(std::string &executable_path, int &dirname_length);
std::string get_runtime_library_dir();
bool read_file(const std::string &filename, std::string &text);
bool present(Vec<char*> &v, const char* name);

} // LFortran

#endif // LFORTRAN_UTILS_H
