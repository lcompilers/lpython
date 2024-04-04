#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#endif

#include <fstream>

#include <bin/tpl/whereami/whereami.h>

#include <libasr/exception.h>
#include <lpython/utils.h>
#include <libasr/string_utils.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
    #include <unistd.h>
#endif

#ifdef _WIN32
    #define stat _stat
    #if !defined S_ISDIR
        #define S_ISDIR(m) (((m) & _S_IFDIR) == _S_IFDIR)
    #endif
#endif

namespace LCompilers::LPython {

void get_executable_path(std::string &executable_path, int &dirname_length)
{
#ifdef HAVE_WHEREAMI
    int length;

    length = wai_getExecutablePath(NULL, 0, &dirname_length);
    if (length > 0) {
        std::string path(length+1, '\0');
        wai_getExecutablePath(&path[0], length, &dirname_length);
        executable_path = path;
        if (executable_path[executable_path.size()-1] == '\0') {
            executable_path = executable_path.substr(0,executable_path.size()-1);
        }
    } else {
        throw LCompilersException("Cannot determine executable path.");
    }
#else
    executable_path = "src/bin/lpython.js";
    dirname_length = 7;
#endif
}

std::string get_runtime_library_dir()
{
#ifdef HAVE_BUILD_TO_WASM
    return "asset_dir";
#endif
    char *env_p = std::getenv("LFORTRAN_RUNTIME_LIBRARY_DIR");
    if (env_p) return env_p;

    std::string path;
    int dirname_length;
    get_executable_path(path, dirname_length);
    std::string dirname = path.substr(0,dirname_length);
    if (   endswith(dirname, "src/bin")
        || endswith(dirname, "src\\bin")
        || endswith(dirname, "SRC\\BIN")) {
        // Development version
        return dirname + "/../runtime";
    } else if (endswith(dirname, "src/lpython/tests") ||
               endswith(to_lower(dirname), "src\\lpython\\tests")) {
        // CTest Tests
        return dirname + "/../../runtime";
    } else {
        // Installed version
        return dirname + "/../share/lpython/lib";
    }
}

std::string get_runtime_library_header_dir()
{
    char *env_p = std::getenv("LFORTRAN_RUNTIME_LIBRARY_HEADER_DIR");
    if (env_p) return env_p;

    // The header file is in src/libasr/runtime for development, but in impure
    // in installed version
    std::string path;
    int dirname_length;
    get_executable_path(path, dirname_length);
    std::string dirname = path.substr(0,dirname_length);
    if (   endswith(dirname, "src/bin")
        || endswith(dirname, "src\\bin")
        || endswith(dirname, "SRC\\BIN")) {
        // Development version
        return dirname + "/../libasr/runtime";
    } else if (endswith(dirname, "src/lpython/tests") ||
               endswith(to_lower(dirname), "src\\lpython\\tests")) {
        // CTest Tests
        return dirname + "/../../libasr/runtime";
    } else {
        // Installed version
        return dirname + "/../share/lpython/lib/impure";
    }

    return path;
}

bool is_directory(std::string path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        if (S_ISDIR(buffer.st_mode)) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool path_exists(std::string path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return true;
    } else {
        return false;
    }
}

#ifdef HAVE_LFORTRAN_LLVM

void open_cpython_library(DynamicLibrary &l) {
    std::string conda_prefix = std::getenv("CONDA_PREFIX");
#if defined (__linux__)
    l.l = dlopen((conda_prefix + "/lib/libpython3.so").c_str(), RTLD_DEEPBIND | RTLD_GLOBAL | RTLD_NOW);
#elif defined (__APPLE__)
    l.l = dlopen((conda_prefix + "/lib/libpython3.dylib").c_str(), RTLD_GLOBAL | RTLD_NOW);
#else
    l.l = LoadLibrary((conda_prefix + "\\python3.dll").c_str());
#endif

    if (l.l == nullptr)
        throw "Could not open CPython library";
}

void close_cpython_library(DynamicLibrary &l) {
#if (defined (__linux__)) or (defined (__APPLE__))
    dlclose(l.l);
    l.l = nullptr;
#else
    FreeLibrary(l.l);
    l.l = nullptr;
#endif
}

void open_symengine_library(DynamicLibrary &l) {
    std::string conda_prefix = std::getenv("CONDA_PREFIX");
#if defined (__linux__)
    l.l = dlopen((conda_prefix + "/lib/libsymengine.so").c_str(), RTLD_DEEPBIND | RTLD_GLOBAL | RTLD_NOW);
#elif defined (__APPLE__)
    l.l = dlopen((conda_prefix + "/lib/libsymengine.dylib").c_str(), RTLD_GLOBAL | RTLD_NOW);
#else
    l.l = LoadLibrary((conda_prefix + "\\Library\\bin\\symengine-0.11.dll").c_str());
#endif

    if (l.l == nullptr)
        throw "Could not open SymEngine library";
}

void close_symengine_library(DynamicLibrary &l) {
#if (defined (__linux__)) or (defined (__APPLE__))
    dlclose(l.l);
    l.l = nullptr;
#else
    FreeLibrary(l.l);
    l.l = nullptr;
#endif
}

#endif

// Decodes the exit status code of the process (in Unix)
// See `WEXITSTATUS` for more information.
// https://stackoverflow.com/a/27117435/15913193
// https://linux.die.net/man/3/system
int32_t get_exit_status(int32_t err) {
    return (((err) >> 8) & 0x000000ff);
}
}
