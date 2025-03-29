#ifndef LIBASR_UTILS_H
#define LIBASR_UTILS_H

#include <string>
#include <vector>
#include <filesystem>
#include <libasr/containers.h>

namespace LCompilers {

enum Platform {
    Linux,
    macOS_Intel,
    macOS_ARM,
    Windows,
    FreeBSD,
    OpenBSD,
};

std::string pf2s(Platform);
Platform get_platform();

std::string get_unique_ID();
int visualize_json(std::string &astr_data_json, LCompilers::Platform os);
std::string generate_visualize_html(std::string &astr_data_json);

struct PassOptions {
    std::filesystem::path mod_files_dir;
    std::vector<std::filesystem::path> include_dirs;

    int default_integer_kind = 4;

    std::string run_fun; // for global_stmts pass
    // TODO: Convert to std::filesystem::path (also change find_and_load_module())
    std::string runtime_library_dir;
    bool always_run = false; // for unused_functions pass
    bool inline_external_symbol_calls = true; // for inline_function_calls pass
    int64_t unroll_factor = 32; // for loop_unroll pass
    bool fast = false; // is fast flag enabled.
    bool verbose = false; // For developer debugging
    bool dump_all_passes = false; // For developer debugging
    bool dump_fortran = false; // For developer debugging
    bool pass_cumulative = false; // Apply passes cumulatively
    bool disable_main = false;
    bool use_loop_variable_after_loop = false;
    bool realloc_lhs = false;
    std::vector<int64_t> skip_optimization_func_instantiation;
    bool module_name_mangling = false;
    bool global_symbols_mangling = false;
    bool intrinsic_symbols_mangling = false;
    bool all_symbols_mangling = false;
    bool bindc_mangling = false;
    bool fortran_mangling = false;
    bool mangle_underscore = false;
    bool json = false;
    bool no_loc = false;
    bool visualize = false;
    bool tree = false;
    bool with_intrinsic_mods = false;
    bool c_mangling = false;
    bool openmp = false;
    bool enable_gpu_offloading = false;
    bool time_report = false;
    std::vector<std::string> vector_of_time_report;
};

struct CompilerOptions {
    std::vector<std::string> runtime_linker_paths;

    // TODO: Convert to std::filesystem::path (also change find_and_load_module())
    PassOptions po;

    bool fixed_form = false;
    bool interactive = false;
    bool c_preprocessor = false;
    std::vector<std::string> c_preprocessor_defines;
    bool prescan = true;
    bool disable_main = false;
    bool symtab_only = false;
    bool show_stacktrace = false;
    bool use_colors = true;
    bool indent = true;
    bool json = false;
    bool tree = false;
    bool visualize = false;
    bool fast = false;
    bool openmp = false;
    std::string openmp_lib_dir = "";
    bool lookup_name = false;
    bool rename_symbol = false;
    std::string line = "";
    std::string column = "";
    bool continue_compilation = false;
    bool semantics_only = false;
    bool generate_object_code = false;
    bool no_warnings = false;
    bool disable_style = false;
    bool logical_casting = false;
    bool no_error_banner = false;
    bool enable_bounds_checking = false;
    std::string error_format = "human";
    bool new_parser = false;
    bool implicit_typing = false;
    bool implicit_interface = false;
    bool implicit_argument_casting = false;
    bool print_leading_space = false;
    bool rtlib = false;
    bool use_loop_variable_after_loop = false;
    std::string target = "";
    std::string arg_o = "";
    bool emit_debug_info = false;
    bool emit_debug_line_column = false;
    bool enable_cpython = false;
    bool enable_symengine = false;
    bool link_numpy = false;
    bool run = false;
    bool legacy_array_sections = false;
    bool ignore_pragma = false;
    bool stack_arrays = false;
    bool wasm_html = false;
    bool time_report = false;
    std::string emcc_embed;
    std::vector<std::string> import_paths;
    Platform platform;

    CompilerOptions () : platform{get_platform()} {};
};

bool read_file(const std::string &filename, std::string &text);
bool present(Vec<char*> &v, const char* name);
bool present(char** const v, size_t n, const std::string name);
int initialize();

} // namespace LCompilers

#endif // LIBASR_UTILS_H
