#ifndef LFORTRAN_UTILS_H
#define LFORTRAN_UTILS_H

#include <string>
#include <libasr/utils.h>

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

namespace LFortran {

void get_executable_path(std::string &executable_path, int &dirname_length);
std::string get_runtime_library_dir();
std::string get_runtime_library_header_dir();
bool is_directory(std::string path);
bool path_exits(std::string path);

} // LFortran

#endif // LFORTRAN_UTILS_H
