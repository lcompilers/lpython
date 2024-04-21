#ifndef LFORTRAN_UTILS_H
#define LFORTRAN_UTILS_H

#include <string>
#include <libasr/utils.h>

namespace LCompilers::LPython {

void get_executable_path(std::string &executable_path, int &dirname_length);
std::string get_runtime_library_dir();
std::string get_runtime_library_header_dir();
bool is_directory(std::string path);
bool path_exists(std::string path);

#ifdef HAVE_LFORTRAN_LLVM
struct DynamicLibrary {
    void *l;

    DynamicLibrary(): l(nullptr) {}
};

void open_cpython_library(DynamicLibrary &l);
void close_cpython_library(DynamicLibrary &l);
void open_symengine_library(DynamicLibrary &l);
void close_symengine_library(DynamicLibrary &l);
#endif

// Decodes the exit status code of the process (in Unix)
int32_t get_exit_status(int32_t err);
} // LFortran

#endif // LFORTRAN_UTILS_H
