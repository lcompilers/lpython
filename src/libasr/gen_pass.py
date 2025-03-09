passes = [
        "array_struct_temporary",
        "replace_arr_slice",
        "replace_openmp",
        "replace_function_call_in_declaration",
        "replace_array_passed_in_function_call",
        "replace_array_op",
        "replace_class_constructor",
        "dead_code_removal",
        "replace_div_to_mul",
        "replace_do_loops",
        "replace_flip_sign",
        "replace_fma",
        "replace_for_all",
        "while_else",
        "wrap_global_stmts",
        "replace_implied_do_loops",
        "replace_init_expr",
        "inline_function_calls",
        "replace_symbolic",
        "replace_intrinsic_function",
        "loop_unroll",
        "loop_vectorise",
        "nested_vars",
        "replace_param_to_const",
        "array_by_data",
        "compare",
        "list_expr",
        "replace_print_arr",
        "replace_print_list_tuple",
        "replace_print_struct_type",
        "replace_select_case",
        "replace_sign_from_value",
        "create_subroutine_from_function",
        "transform_optional_argument_functions",
        "unused_functions",
        "update_array_dim_intrinsic_calls",
        "replace_where",
        "unique_symbols",
        "insert_deallocate",
        "promote_allocatable_to_nonallocatable",
        "replace_with_compile_time_values"
]



for name in passes:
    print(f"Processing: {name}")
    name_up = name.upper()
    header = rf"""#ifndef LIBASR_PASS_{name_up}_H
#define LIBASR_PASS_{name_up}_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {{

    void pass_{name}(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

}} // namespace LCompilers

#endif // LIBASR_PASS_{name_up}_H
"""
    header_filename = f"pass/{name}.h"
    f = open(header_filename, "w")
    f.write(header)
