# Backup the original value of the requested library suffixes
set(_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
# Static libraries end with .a on Unix and .lib on Windows
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)

# Now use CMake's built-in ZLIB finder
find_package(ZLIB REQUIRED)

# Reset the library suffixes to the original value
set(CMAKE_FIND_LIBRARY_SUFFIXES ${_CMAKE_FIND_LIBRARY_SUFFIXES})
# Unset the temporary to not pollute the global namespace
unset(_CMAKE_FIND_LIBRARY_SUFFIXES)
