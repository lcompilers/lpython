find_path(RAPIDJSON_HEADER rapidjson/rapidjson.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON DEFAULT_MSG RAPIDJSON_HEADER)

add_library(p::rapidjson INTERFACE IMPORTED)
set_property(TARGET p::rapidjson PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${RAPIDJSON_HEADER})
