cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lfort C)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug
        CACHE STRING "Build type (Debug, Release)" FORCE)
endif ()
if (NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR
        CMAKE_BUILD_TYPE STREQUAL "Release"))
    message("${CMAKE_BUILD_TYPE}")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be one of: Debug, Release (current value: '${CMAKE_BUILD_TYPE}')")
endif ()

enable_testing()

message("\n")
message("Configuration results")
message("---------------------")
message("C compiler      : ${CMAKE_C_COMPILER}")
message("Build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("C compiler flags      : ${CMAKE_C_FLAGS_DEBUG}")
else ()
    message("C compiler flags      : ${CMAKE_C_FLAGS_RELEASE}")
endif ()
message("Installation prefix: ${CMAKE_INSTALL_PREFIX}")


set(SRC
    lfort_intrinsics.c
)
add_library(lfortran SHARED ${SRC})
add_library(lfortran_static STATIC ${SRC})
install(TARGETS lfortran lfortran_static
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)
