#include <bin/tpl/whereami/whereami.h>

#include <lfortran/exception.h>
#include <lfortran/utils.h>
#include <lfortran/string_utils.h>

namespace LFortran {

void get_executable_path(std::string &executable_path, int &dirname_length)
{
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
        throw LFortranException("Cannot determine executable path.");
    }
}

std::string get_runtime_library_dir()
{
    char *env_p = std::getenv("LFORTRAN_RUNTIME_LIBRARY_DIR");
    if (env_p) return env_p;

    std::string path;
    int dirname_length;
    get_executable_path(path, dirname_length);
    std::string dirname = path.substr(0,dirname_length);
    if (endswith(dirname, "src/bin")) {
        // Development version
        return dirname + "/../runtime";
    } else {
        // Installed version
        return dirname + "/../share/lfortran/lib";
    }
}

}
