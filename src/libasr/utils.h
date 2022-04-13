#ifndef LIBASR_UTILS_H
#define LIBASR_UTILS_H

#include <string>
#include <libasr/containers.h>

namespace LFortran {

enum Platform {
    Linux,
    macOS_Intel,
    macOS_ARM,
    Windows,
    FreeBSD
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
    bool tree = false;
    bool fast = false;
    bool openmp = false;
    bool no_warnings = false;
    bool no_error_banner = false;
    bool new_parser = false;
    std::string target = "";
    Platform platform;

    CompilerOptions () : platform{get_platform()} {};
};


bool read_file(const std::string &filename, std::string &text);
bool present(Vec<char*> &v, const char* name);
int initialize();

} // LFortran

#endif // LIBASR_UTILS_H
