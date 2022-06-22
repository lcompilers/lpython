#include <string> 

#include "LPythonServer.hpp"

int main (int argc, char** argv) {
    (void)argc;
    std::string path;
    path = argv[1];
    LPythonServer().run(path);
}