#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <fstream>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_wasm.h>
#include <libasr/codegen/wasm_assembler.h>
#include <libasr/pass/do_loops.h>
#include <libasr/pass/unused_functions.h>
#include <libasr/pass/pass_array_by_data.h>
#include <libasr/pass/print_arr.h>
#include <libasr/pass/intrinsic_function.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>

// #define SHOW_ASR

#ifdef SHOW_ASR
#include <lfortran/pickle.h>
#endif

namespace LCompilers {

namespace {

// This exception is used to abort the visitor pattern when an error occurs.
class CodeGenAbort {};

// Local exception that is only used in this file to exit the visitor
// pattern and caught later (not propagated outside)
class CodeGenError {
   public:
    diag::Diagnostic d;

   public:
    CodeGenError(const std::string &msg)
        : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen)} {}

    CodeGenError(const std::string &msg, const Location &loc)
        : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen,
                             {diag::Label("", {loc})})} {}
};

}  // namespace

// Platform dependent fast unique hash:
static uint64_t get_hash(ASR::asr_t *node) { return (uint64_t)node; }

struct SymbolFuncInfo {
    bool needs_declaration = true;
    bool intrinsic_function = false;
    uint32_t index = 0;
    uint32_t no_of_variables = 0;
    ASR::Variable_t *return_var = nullptr;
    Vec<ASR::Variable_t *> referenced_vars;
};

enum RT_FUNCS {
    print_i64 = 0,
    print_f64 = 1,
    add_c32 = 2,
    add_c64 = 3,
    sub_c32 = 4,
    sub_c64 = 5,
    mul_c32 = 6,
    mul_c64 = 7,
    abs_c32 = 9,
    abs_c64 = 10,
    rt_funcs_last = 11, // keep this as the last enumerator
};
const int NO_OF_RT_FUNCS = rt_funcs_last;

enum GLOBAL_VAR {
    cur_mem_loc = 0,
    tmp_reg_i32 = 1,
    tmp_reg_i64 = 2,
    tmp_reg_f32 = 3,
    tmp_reg_f64 = 4,
    global_vars_cnt = 5
};

enum IMPORT_FUNC {
    proc_exit = 0,
    fd_write = 1,
    import_funcs_cnt = 2
};

std::string import_fn_to_str(IMPORT_FUNC fn) {
    switch(fn) {
        case (IMPORT_FUNC::proc_exit): return "proc_exit";
        case (IMPORT_FUNC::fd_write): return "fd_write";
        default: throw CodeGenError("Unknown import function");
    }
}

class ASRToWASMVisitor : public ASR::BaseVisitor<ASRToWASMVisitor> {
   public:
    Allocator &m_al;
    diag::Diagnostics &diag;

    SymbolFuncInfo *cur_sym_info;
    uint32_t nesting_level;
    uint32_t cur_loop_nesting_level;
    bool is_prototype_only;
    bool is_local_vars_only;
    ASR::Function_t* main_func;

    Vec<uint8_t> m_type_section;
    Vec<uint8_t> m_import_section;
    Vec<uint8_t> m_func_section;
    Vec<uint8_t> m_memory_section;
    Vec<uint8_t> m_global_section;
    Vec<uint8_t> m_export_section;
    Vec<uint8_t> m_code_section;
    Vec<uint8_t> m_data_section;

    uint32_t no_of_types;
    uint32_t no_of_functions;
    uint32_t no_of_memories;
    uint32_t no_of_globals;
    uint32_t no_of_exports;
    uint32_t no_of_imports;
    uint32_t no_of_data_segments;
    uint32_t avail_mem_loc;
    uint32_t digits_mem_loc;

    uint32_t min_no_pages;
    uint32_t max_no_pages;

    std::map<uint64_t, uint32_t> m_var_idx_map;
    std::map<uint64_t, uint32_t> m_global_var_idx_map;
    std::map<uint64_t, SymbolFuncInfo *> m_func_name_idx_map;
    std::map<std::string, uint32_t> m_string_to_iov_loc_map;

    std::vector<uint32_t> m_compiler_globals;
    std::vector<uint32_t> m_import_func_idx_map;
    std::vector<void (LCompilers::ASRToWASMVisitor::*)(int)> m_rt_funcs_map;
    std::vector<int> m_rt_func_used_idx;

   public:
    ASRToWASMVisitor(Allocator &al, diag::Diagnostics &diagnostics)
        : m_al(al), diag(diagnostics) {
        is_prototype_only = false;
        is_local_vars_only = false;
        main_func = nullptr;
        nesting_level = 0;
        cur_loop_nesting_level = 0;
        no_of_types = 0;
        avail_mem_loc = 0;
        no_of_functions = 0;
        no_of_memories = 0;
        no_of_globals = 0;
        no_of_exports = 0;
        no_of_imports = 0;
        no_of_data_segments = 0;

        min_no_pages = 100;  // fixed 6.4 Mb memory currently
        max_no_pages = 100;  // fixed 6.4 Mb memory currently

        m_type_section.reserve(m_al, 1024 * 128);
        m_import_section.reserve(m_al, 1024 * 128);
        m_func_section.reserve(m_al, 1024 * 128);
        m_memory_section.reserve(m_al, 1024 * 128);
        m_global_section.reserve(m_al, 1024 * 128);
        m_export_section.reserve(m_al, 1024 * 128);
        m_code_section.reserve(m_al, 1024 * 128);
        m_data_section.reserve(m_al, 1024 * 128);

        m_compiler_globals.resize(global_vars_cnt);
        m_import_func_idx_map.resize(import_funcs_cnt);
        m_rt_funcs_map.resize(NO_OF_RT_FUNCS);
        m_rt_func_used_idx = std::vector<int>(NO_OF_RT_FUNCS, -1);
    }

    void get_wasm(Vec<uint8_t> &code) {
        code.reserve(m_al, 8U /* preamble size */ +
                               8U /* (section id + section size) */ *
                                   8U /* number of sections */
                               + m_type_section.size() +
                               m_import_section.size() + m_func_section.size() +
                               m_memory_section.size() + m_global_section.size() +
                               m_export_section.size() + m_code_section.size() +
                               m_data_section.size());

        wasm::emit_header(code, m_al);  // emit header and version
        wasm::encode_section(
            code, m_type_section, m_al, 1U,
            no_of_types);  // no_of_types indicates total (imported + defined)
                           // no of functions
        wasm::encode_section(code, m_import_section, m_al, 2U, no_of_imports);
        wasm::encode_section(code, m_func_section, m_al, 3U, no_of_functions);
        wasm::encode_section(code, m_memory_section, m_al, 5U, no_of_memories);
        wasm::encode_section(code, m_global_section, m_al, 6U, no_of_globals);
        wasm::encode_section(code, m_export_section, m_al, 7U, no_of_exports);
        wasm::encode_section(code, m_code_section, m_al, 10U, no_of_functions);
        wasm::encode_section(code, m_data_section, m_al, 11U,
                             no_of_data_segments);
    }

    void import_function(IMPORT_FUNC fn,
            std::vector<wasm::type> param_types,
            std::vector<wasm::type> result_types) {
        int func_idx = -1;
        emit_func_type(param_types, result_types, func_idx);
        m_import_func_idx_map[fn] = func_idx;

        wasm::emit_import_fn(m_import_section, m_al, "wasi_snapshot_preview1", import_fn_to_str(fn), func_idx);
        no_of_imports++;
    }

    void import_function2(ASR::Function_t* fn) {
        if (ASRUtils::get_FunctionType(fn)->m_abi != ASR::abiType::BindC) return;
        if (ASRUtils::get_FunctionType(fn)->m_deftype != ASR::deftypeType::Interface) return;
        if (ASRUtils::get_FunctionType(fn)->m_abi != ASR::abiType::BindC) return;
        if (ASRUtils::is_intrinsic_function2(fn)) return;

        wasm::emit_import_fn(m_import_section, m_al, "js", fn->m_name, no_of_types);
        no_of_imports++;
        emit_function_prototype(*fn);
    }

    void emit_imports(SymbolTable *global_scope) {
        using namespace wasm;

        avail_mem_loc += 4; /* initial 4 bytes to store return values of wasi funcs*/
        import_function(proc_exit, {i32}, {});
        import_function(fd_write, {i32, i32, i32, i32}, {i32});

        // In WASM: The indices of the imports precede the indices of other
        // definitions in the same index space. Therefore, declare the import
        // functions before defined functions
        for (auto &item : global_scope->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                ASR::Program_t *p = ASR::down_cast<ASR::Program_t>(item.second);
                for (auto &item : p->m_symtab->get_scope()) {
                    if (ASR::is_a<ASR::Function_t>(*item.second)) {
                        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(item.second);
                        import_function2(fn);
                    }
                }
            } else if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(item.second);
                import_function2(fn);
            }
        }
    }

    void emit_if_else(std::function<void()> test_cond, std::function<void()> if_block, std::function<void()> else_block) {
        test_cond();
        wasm::emit_b8(m_code_section, m_al, 0x04);  // emit if start
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type
        nesting_level++;
        if_block();
        wasm::emit_b8(m_code_section, m_al, 0x05);  // starting of else
        else_block();
        nesting_level--;
        wasm::emit_expr_end(m_code_section, m_al);  // emit if end
    }

    void emit_loop(std::function<void()> test_cond, std::function<void()> loop_block) {
        uint32_t prev_cur_loop_nesting_level = cur_loop_nesting_level;
        cur_loop_nesting_level = nesting_level;

        wasm::emit_b8(m_code_section, m_al, 0x03);  // emit loop start
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type

        nesting_level++;

        emit_if_else(test_cond, [&](){
            loop_block();
            // From WebAssembly Docs:
            // Unlike with other index spaces, indexing of labels is relative by
            // nesting depth, that is, label 0 refers to the innermost structured
            // control instruction enclosing the referring branch instruction, while
            // increasing indices refer to those farther out.
            wasm::emit_branch(m_code_section, m_al, nesting_level -
                cur_loop_nesting_level - 1);  // emit_branch and label the loop
        }, [&](){});

        nesting_level--;
        wasm::emit_expr_end(m_code_section, m_al);  // end loop
        cur_loop_nesting_level = prev_cur_loop_nesting_level;
    }

    void emit_func_type(std::vector<wasm::type> params, std::vector<wasm::type> results, int &func_idx) {
        wasm::emit_b8(m_type_section, m_al, 0x60);
        wasm::emit_u32(m_type_section, m_al, params.size()); // no of params
        for (auto param:params) {
            wasm::emit_b8(m_type_section, m_al, param);
        }
        wasm::emit_u32(m_type_section, m_al, results.size()); // no of results
        for (auto result:results) {
            wasm::emit_b8(m_type_section, m_al, result);
        }
        if (func_idx == -1) {
            func_idx = no_of_types++;
        }
    }

    void define_emit_func(
        std::vector<wasm::type> params,
        std::vector<wasm::type> results,
        std::vector<wasm::type> locals,
        std::string func_name,
        std::function<void()> func_body,
        int func_idx = -1) {

        emit_func_type(params, results, func_idx); // type declaration

        /*** Reference Function Prototype ***/
        wasm::emit_u32(m_func_section, m_al, func_idx);

        /*** Function Body Starts Here ***/
        uint32_t len_idx_code_section_func_size =
            wasm::emit_len_placeholder(m_code_section, m_al);

        wasm::emit_u32(m_code_section, m_al, locals.size());
        for (auto local:locals) {
            wasm::emit_u32(m_code_section, m_al, 1u); // count of local vars of this type
            wasm::emit_b8(m_code_section, m_al, local);
        }

        func_body();

        wasm::emit_b8(m_code_section, m_al, 0x0F);  // emit wasm return instruction
        wasm::emit_expr_end(m_code_section, m_al);
        wasm::fixup_len(m_code_section, m_al, len_idx_code_section_func_size);

        /*** Export the function ***/
        wasm::emit_export_fn(m_export_section, m_al, func_name, func_idx);  //  add function to export
        no_of_functions++;
        no_of_exports++;
    }

    void emit_print_int(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({i64}, {}, {i64, i64, i64, i64}, "print_i64", [&](){
            // locals 0 is given parameter
            // locals 1 is digits_cnt
            // locals 2 is divisor (in powers of 10)
            // locals 3 is loop counter (counts upto digits_cnt (which is decreasing))
            // locals 4 is extra copy of given parameter

            emit_if_else([&](){
                wasm::emit_local_get(m_code_section, m_al, 0);
                wasm::emit_i64_const(m_code_section, m_al, 0);
                wasm::emit_i64_eq(m_code_section, m_al);
            }, [&](){
                emit_call_fd_write(1, "0", 1, 0);
                wasm::emit_b8(m_code_section, m_al, 0x0F);  // emit wasm return instruction
            }, [&](){});

            emit_if_else([&](){
                wasm::emit_local_get(m_code_section, m_al, 0);
                wasm::emit_i64_const(m_code_section, m_al, 0);
                wasm::emit_i64_lt_s(m_code_section, m_al);
            }, [&](){
                emit_call_fd_write(1, "-", 1, 0);
                wasm::emit_local_get(m_code_section, m_al, 0);
                wasm::emit_i64_const(m_code_section, m_al, -1);
                wasm::emit_i64_mul(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 0);
            }, [&](){});

            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_set(m_code_section, m_al, 4);
            wasm::emit_i64_const(m_code_section, m_al, 0);
            wasm::emit_local_set(m_code_section, m_al, 1);

            emit_loop([&](){
                wasm::emit_local_get(m_code_section, m_al, 0);
                wasm::emit_i64_const(m_code_section, m_al, 0);
                wasm::emit_i64_gt_s(m_code_section, m_al);
            }, [&](){
                wasm::emit_local_get(m_code_section, m_al, 1);
                wasm::emit_i64_const(m_code_section, m_al, 1);
                wasm::emit_i64_add(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 1);
                wasm::emit_local_get(m_code_section, m_al, 0);
                wasm::emit_i64_const(m_code_section, m_al, 10);
                wasm::emit_i64_div_s(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 0);
            });

            emit_loop([&](){
                wasm::emit_local_get(m_code_section, m_al, 1);
                wasm::emit_i64_const(m_code_section, m_al, 0);
                wasm::emit_i64_gt_s(m_code_section, m_al);
            }, [&](){
                wasm::emit_local_get(m_code_section, m_al, 1);
                wasm::emit_i64_const(m_code_section, m_al, 1);
                wasm::emit_i64_sub(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 1);

                wasm::emit_i64_const(m_code_section, m_al, 1);
                wasm::emit_local_set(m_code_section, m_al, 2);
                wasm::emit_i64_const(m_code_section, m_al, 0);
                wasm::emit_local_set(m_code_section, m_al, 3);

                emit_loop([&](){
                    wasm::emit_local_get(m_code_section, m_al, 3);
                    wasm::emit_local_get(m_code_section, m_al, 1);
                    wasm::emit_i64_lt_s(m_code_section, m_al);
                }, [&](){
                    wasm::emit_local_get(m_code_section, m_al, 3);
                    wasm::emit_i64_const(m_code_section, m_al, 1);
                    wasm::emit_i64_add(m_code_section, m_al);
                    wasm::emit_local_set(m_code_section, m_al, 3);
                    wasm::emit_local_get(m_code_section, m_al, 2);
                    wasm::emit_i64_const(m_code_section, m_al, 10);
                    wasm::emit_i64_mul(m_code_section, m_al);
                    wasm::emit_local_set(m_code_section, m_al, 2);
                });


                wasm::emit_local_get(m_code_section, m_al, 4);
                wasm::emit_local_get(m_code_section, m_al, 2);
                wasm::emit_i64_div_s(m_code_section, m_al);
                wasm::emit_i64_const(m_code_section, m_al, 10);
                wasm::emit_i64_rem_s(m_code_section, m_al);

                /* The digit is on stack */
                wasm::emit_i64_const(m_code_section, m_al, 12 /* 4 + 4 + 4 (iov vec + str size)*/);
                wasm::emit_i64_mul(m_code_section, m_al);
                wasm::emit_i64_const(m_code_section, m_al, digits_mem_loc);
                wasm::emit_i64_add(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 0); // temporary save

                {
                    wasm::emit_i32_const(m_code_section, m_al, 1); // file type: 1 for stdout
                    wasm::emit_local_get(m_code_section, m_al, 0); // use stored digit
                    wasm::emit_i32_wrap_i64(m_code_section, m_al);
                    wasm::emit_i32_const(m_code_section, m_al, 1); // size of iov vector
                    wasm::emit_i32_const(m_code_section, m_al, 0); // mem_loction to return no. of bytes written
                    // call WASI fd_write
                    wasm::emit_call(m_code_section, m_al, m_import_func_idx_map[fd_write]);
                    wasm::emit_drop(m_code_section, m_al);
                }

            });
        }, fn_idx);
    }

    void emit_print_float(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f64}, {}, {i64, i64, i64}, "print_f64", [&](){
            emit_if_else([&](){
                wasm::emit_local_get(m_code_section, m_al, 0);
                wasm::emit_f64_const(m_code_section, m_al, 0);
                wasm::emit_f64_lt(m_code_section, m_al);
            }, [&](){
                emit_call_fd_write(1, "-", 1, 0);
                wasm::emit_local_get(m_code_section, m_al, 0);
                wasm::emit_f64_const(m_code_section, m_al, -1);
                wasm::emit_f64_mul(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 0);
            }, [&](){});

            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_i64_trunc_f64_s(m_code_section, m_al);
            wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_i64]);
            emit_call_fd_write(1, ".", 1, 0);

            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_i64_trunc_f64_s(m_code_section, m_al);
            wasm::emit_f64_convert_i64_s(m_code_section, m_al);
            wasm::emit_f64_sub(m_code_section, m_al);
            wasm::emit_f64_const(m_code_section, m_al, 1e8);
            wasm::emit_f64_mul(m_code_section, m_al);
            wasm::emit_i64_trunc_f64_s(m_code_section, m_al);
            wasm::emit_local_set(m_code_section, m_al, 2); /* save the current fractional part value */
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_local_set(m_code_section, m_al, 3); /* save the another copy */

            wasm::emit_i64_const(m_code_section, m_al, 0);
            wasm::emit_local_set(m_code_section, m_al, 1); // digits_cnt

            emit_loop([&](){
                wasm::emit_local_get(m_code_section, m_al, 2);
                wasm::emit_i64_const(m_code_section, m_al, 0);
                wasm::emit_i64_gt_s(m_code_section, m_al);
            }, [&](){
                wasm::emit_local_get(m_code_section, m_al, 1);
                wasm::emit_i64_const(m_code_section, m_al, 1);
                wasm::emit_i64_add(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 1);

                wasm::emit_local_get(m_code_section, m_al, 2);
                wasm::emit_f64_convert_i64_s(m_code_section, m_al);
                wasm::emit_i64_const(m_code_section, m_al, 10);
                wasm::emit_f64_convert_i64_s(m_code_section, m_al);
                wasm::emit_f64_div(m_code_section, m_al);
                wasm::emit_i64_trunc_f64_s(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 2);
            });

            emit_loop([&](){
                wasm::emit_local_get(m_code_section, m_al, 1);
                wasm::emit_i64_const(m_code_section, m_al, 8);
                wasm::emit_i64_lt_s(m_code_section, m_al);
            }, [&](){
                wasm::emit_local_get(m_code_section, m_al, 1);
                wasm::emit_i64_const(m_code_section, m_al, 1);
                wasm::emit_i64_add(m_code_section, m_al);
                wasm::emit_local_set(m_code_section, m_al, 1);

                emit_call_fd_write(1, "0", 1, 0);
            });

            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_i64]);
        }, fn_idx);
    }

    void emit_complex_add_32(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f32, f32, f32, f32}, {f32, f32}, {}, "add_c32", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f32_add(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f32_add(m_code_section, m_al);
        }, fn_idx);
    }

    void emit_complex_add_64(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f64, f64, f64, f64}, {f64, f64}, {}, "add_c64", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f64_add(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f64_add(m_code_section, m_al);
        }, fn_idx);
    }

    void emit_complex_sub_32(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f32, f32, f32, f32}, {f32, f32}, {}, "sub_c32", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f32_sub(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f32_sub(m_code_section, m_al);
        }, fn_idx);
    }

    void emit_complex_sub_64(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f64, f64, f64, f64}, {f64, f64}, {}, "sub_c64", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f64_sub(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f64_sub(m_code_section, m_al);
        }, fn_idx);
    }

    void emit_complex_mul_32(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f32, f32, f32, f32}, {f32, f32}, {}, "mul_c32", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f32_mul(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f32_mul(m_code_section, m_al);

            wasm::emit_f32_sub(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f32_mul(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f32_mul(m_code_section, m_al);

            wasm::emit_f32_add(m_code_section, m_al);
        }, fn_idx);
    }

    void emit_complex_mul_64(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f64, f64, f64, f64}, {f64, f64}, {}, "mul_c64", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f64_mul(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f64_mul(m_code_section, m_al);

            wasm::emit_f64_sub(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 3);
            wasm::emit_f64_mul(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 2);
            wasm::emit_f64_mul(m_code_section, m_al);

            wasm::emit_f64_add(m_code_section, m_al);
        }, fn_idx);
    }

    void emit_complex_abs_32(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f32, f32}, {f32}, {}, "abs_c32", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_f32_mul(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_f32_mul(m_code_section, m_al);

            wasm::emit_f32_add(m_code_section, m_al);
            wasm::emit_f32_sqrt(m_code_section, m_al);
        }, fn_idx);
    }

    void emit_complex_abs_64(int fn_idx = -1) {
        using namespace wasm;
        define_emit_func({f64, f64}, {f64}, {}, "abs_c64", [&](){
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_local_get(m_code_section, m_al, 0);
            wasm::emit_f64_mul(m_code_section, m_al);

            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_local_get(m_code_section, m_al, 1);
            wasm::emit_f64_mul(m_code_section, m_al);

            wasm::emit_f64_add(m_code_section, m_al);
            wasm::emit_f64_sqrt(m_code_section, m_al);
        }, fn_idx);
    }

    template <typename T>
    void declare_global_var(wasm::type var_type, GLOBAL_VAR name, T initial_value, bool isMutable) {
        m_global_section.push_back(m_al, var_type);
        m_global_section.push_back(m_al, isMutable);
        switch (var_type)
        {
            case wasm::type::i32: wasm::emit_i32_const(m_global_section, m_al, initial_value); break;
            case wasm::type::i64: wasm::emit_i64_const(m_global_section, m_al, initial_value); break;
            case wasm::type::f32: wasm::emit_f32_const(m_global_section, m_al, initial_value); break;
            case wasm::type::f64: wasm::emit_f64_const(m_global_section, m_al, initial_value); break;
            default: throw CodeGenError("declare_global_var: Unsupport var_type"); break;
        }
        wasm::emit_expr_end(m_global_section, m_al);  // end instructions
        m_compiler_globals[name] = no_of_globals;
        no_of_globals++;
    }

    void declare_global_var(ASR::Variable_t* v) {
        if (v->m_type->type == ASR::ttypeType::TypeParameter) {
            // Ignore type variables
            return;
        }
        m_global_var_idx_map[get_hash((ASR::asr_t *)v)] = no_of_globals;
        emit_var_type(m_global_section, v, no_of_globals, false);
        m_global_section.push_back(m_al, true); // mutable
        int kind = ASRUtils::extract_kind_from_ttype_t(v->m_type);
        switch (v->m_type->type){
            case ASR::ttypeType::Integer: {
                uint64_t init_val = 0;
                if (v->m_value && ASR::is_a<ASR::IntegerConstant_t>(*v->m_value)) {
                    init_val = ASR::down_cast<ASR::IntegerConstant_t>(v->m_value)->m_n;
                }
                switch (kind) {
                    case 4:
                        wasm::emit_i32_const(m_global_section, m_al, init_val);
                        break;
                    case 8:
                        wasm::emit_i64_const(m_global_section, m_al, init_val);
                        break;
                    default:
                        throw CodeGenError(
                            "Declare Global: Unsupported Integer kind");
                }
                break;
            }
            case ASR::ttypeType::Real: {
                double init_val = 0.0;
                if (v->m_value) {
                    init_val = ASR::down_cast<ASR::RealConstant_t>(v->m_value)->m_r;
                }
                switch (kind) {
                    case 4:
                        wasm::emit_f32_const(m_global_section, m_al, init_val);
                        break;
                    case 8:
                        wasm::emit_f64_const(m_global_section, m_al, init_val);
                        break;
                    default:
                        throw CodeGenError(
                            "Declare Global: Unsupported Real kind");
                }
                break;
            }
            case ASR::ttypeType::Logical: {
                bool init_val = false;
                if (v->m_value) {
                    init_val = ASR::down_cast<ASR::LogicalConstant_t>(v->m_value)->m_value;
                }
                switch (kind) {
                    case 4:
                        wasm::emit_i32_const(m_global_section, m_al, init_val);
                        break;
                    default:
                        throw CodeGenError(
                            "Declare Global: Unsupported Logical kind");
                }
                break;
            }
            case ASR::ttypeType::Character: {
                std::string init_val = "";
                if (v->m_value) {
                    init_val = ASR::down_cast<ASR::StringConstant_t>(v->m_value)->m_s;
                }
                emit_string(init_val);
                switch (kind) {
                    case 1:
                        wasm::emit_i32_const(m_global_section, m_al, m_string_to_iov_loc_map[init_val]);
                        break;
                    default:
                        throw CodeGenError(
                            "Declare Global: Unsupported Character kind");
                }
                break;
            }
            default: {
                diag.codegen_warning_label("Declare Global: Type "
                 + ASRUtils::type_to_str(v->m_type) + " not yet supported", {v->base.base.loc}, "");
                wasm::emit_i32_const(m_global_section, m_al, 0);
            }
        }
        wasm::emit_expr_end(m_global_section, m_al);  // end instructions
    }

    void declare_symbols(const ASR::TranslationUnit_t &x) {
        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order =
                ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_global_scope->get_scope().find(item) !=
                                x.m_global_scope->get_scope().end());
                ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                this->visit_symbol(*mod);
            }
        }

        // Process procedures first:
        declare_all_functions(*x.m_global_scope);

        // then the main program:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LCOMPILERS_ASSERT(x.n_items == 0);

        emit_imports(x.m_global_scope);

        wasm::emit_declare_mem(m_memory_section, m_al, min_no_pages, max_no_pages);
        no_of_memories++;
        wasm::emit_export_mem(m_export_section, m_al, "memory", 0 /* mem_idx */);
        no_of_exports++;

        declare_global_var(wasm::type::i32, cur_mem_loc, 0, true);
        declare_global_var(wasm::type::i32, tmp_reg_i32, 0, true);
        declare_global_var(wasm::type::i64, tmp_reg_i64, 0, true);
        declare_global_var(wasm::type::f32, tmp_reg_f32, 0, true);
        declare_global_var(wasm::type::f64, tmp_reg_f64, 0, true);

        emit_string(" ");
        emit_string("\n");
        emit_string("-");
        emit_string(".");
        emit_string("(");
        emit_string(")");
        emit_string(",");
        digits_mem_loc = avail_mem_loc;
        for (int i = 0; i < 10; i++) {
            emit_string(std::to_string(i));
        }

        m_rt_funcs_map[print_i64] = &ASRToWASMVisitor::emit_print_int;
        m_rt_funcs_map[print_f64] = &ASRToWASMVisitor::emit_print_float;
        m_rt_funcs_map[add_c32] = &ASRToWASMVisitor::emit_complex_add_32;
        m_rt_funcs_map[add_c64] = &ASRToWASMVisitor::emit_complex_add_64;
        m_rt_funcs_map[sub_c32] = &ASRToWASMVisitor::emit_complex_sub_32;
        m_rt_funcs_map[sub_c64] = &ASRToWASMVisitor::emit_complex_sub_64;
        m_rt_funcs_map[mul_c32] = &ASRToWASMVisitor::emit_complex_mul_32;
        m_rt_funcs_map[mul_c64] = &ASRToWASMVisitor::emit_complex_mul_64;
        m_rt_funcs_map[abs_c32] = &ASRToWASMVisitor::emit_complex_abs_32;
        m_rt_funcs_map[abs_c64] = &ASRToWASMVisitor::emit_complex_abs_64;

        {
            // Pre-declare all functions first, then generate code
            // Otherwise some function might not be found.
            is_prototype_only = true;
            declare_symbols(x);
            is_prototype_only = false;
        }
        declare_symbols(x);


        std::vector<std::pair<uint32_t, int>> ordered_rt_funcs_type_idx;
        for (int i = 0; i < NO_OF_RT_FUNCS; i++) {
            if (m_rt_func_used_idx[i] != -1) {
                ordered_rt_funcs_type_idx.push_back(std::make_pair(m_rt_func_used_idx[i], i));
            }
        }

        sort(ordered_rt_funcs_type_idx.begin(), ordered_rt_funcs_type_idx.end());

        for (auto rt_func:ordered_rt_funcs_type_idx) {
            (this->*m_rt_funcs_map[rt_func.second])(rt_func.first);
        }
    }

    void declare_all_functions(const SymbolTable &symtab) {
        for (auto &item : symtab.get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s =
                    ASR::down_cast<ASR::Function_t>(item.second);
                if (ASRUtils::get_FunctionType(s)->n_type_params == 0) {
                    this->visit_Function(*s);
                }
            }
        }
    }

    void declare_all_variables(const SymbolTable &symtab) {
        for (auto &item : symtab.get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *s = ASR::down_cast<ASR::Variable_t>(item.second);
                declare_global_var(s);
            }
        }
    }

    void visit_Module(const ASR::Module_t &x) {
        // Generate the bodies of functions and subroutines
        declare_all_functions(*x.m_symtab);

        if (is_prototype_only) {
            declare_all_variables(*x.m_symtab);
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate the bodies of functions and subroutines
        declare_all_functions(*x.m_symtab);

        // Generate main program code
        if (main_func == nullptr) {
            main_func = (ASR::Function_t *)ASRUtils::make_Function_t_util(
                m_al, x.base.base.loc, x.m_symtab, s2c(m_al, "_start"),
                nullptr, 0, nullptr, 0, x.m_body, x.n_body, nullptr,
                ASR::abiType::Source, ASR::accessType::Public,
                ASR::deftypeType::Implementation, nullptr, false, false, false, false, false,
                nullptr, 0, nullptr, 0, false, false, false);
        }
        this->visit_Function(*main_func);
    }

    void emit_var_type(Vec<uint8_t> &code, ASR::Variable_t *v, uint32_t &no_of_vars, bool emitCount) {
        bool is_array = ASRUtils::is_array(v->m_type);

        if (emitCount) {
            wasm::emit_u32(code, m_al, 1U);
        }

        if (ASRUtils::is_pointer(v->m_type)) {
            ASR::ttype_t *t2 =
                ASR::down_cast<ASR::Pointer_t>(v->m_type)->m_type;
            if (ASRUtils::is_integer(*t2)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(t2);
                diag.codegen_warning_label(
                    "Pointers are not currently supported", {v->base.base.loc},
                    "emitting integer for now");
                if (t->m_kind == 4) {
                    wasm::emit_b8(code, m_al, wasm::type::i32);
                } else if (t->m_kind == 8) {
                    wasm::emit_b8(code, m_al, wasm::type::i64);
                } else {
                    throw CodeGenError(
                        "Integers of kind 4 and 8 only supported");
                }
            } else {
                diag.codegen_error_label("Type number '" +
                                             std::to_string(v->m_type->type) +
                                             "' not supported",
                                         {v->base.base.loc}, "");
                throw CodeGenAbort();
            }
        } else {
            if (ASRUtils::is_integer(*v->m_type)) {
                ASR::Integer_t *v_int =
                    ASR::down_cast<ASR::Integer_t>(v->m_type);
                if (is_array) {
                    wasm::emit_b8(code, m_al, wasm::type::i32);
                } else {
                    if (v_int->m_kind == 4) {
                        wasm::emit_b8(code, m_al, wasm::type::i32);
                    } else if (v_int->m_kind == 8) {
                        wasm::emit_b8(code, m_al, wasm::type::i64);
                    } else {
                        throw CodeGenError(
                            "Integers of kind 4 and 8 only supported");
                    }
                }
            } else if (ASRUtils::is_real(*v->m_type)) {
                ASR::Real_t *v_float = ASR::down_cast<ASR::Real_t>(v->m_type);

                if (is_array) {
                    wasm::emit_b8(code, m_al, wasm::type::i32);
                } else {
                    if (v_float->m_kind == 4) {
                        wasm::emit_b8(code, m_al, wasm::type::f32);
                    } else if (v_float->m_kind == 8) {
                        wasm::emit_b8(code, m_al, wasm::type::f64);
                    } else {
                        throw CodeGenError(
                            "Floating Points of kind 4 and 8 only supported");
                    }
                }
            } else if (ASRUtils::is_logical(*v->m_type)) {
                ASR::Logical_t *v_logical =
                    ASR::down_cast<ASR::Logical_t>(v->m_type);

                if (is_array) {
                    wasm::emit_b8(code, m_al, wasm::type::i32);
                } else {
                    // All Logicals are represented as i32 in WASM
                    if (v_logical->m_kind == 4) {
                        wasm::emit_b8(code, m_al, wasm::type::i32);
                    } else {
                        throw CodeGenError("Logicals of kind 4 only supported");
                    }
                }
            } else if (ASRUtils::is_character(*v->m_type)) {
                ASR::Character_t *v_int =
                    ASR::down_cast<ASR::Character_t>(v->m_type);

                if (is_array) {
                    wasm::emit_b8(code, m_al, wasm::type::i32);
                } else {
                    if (v_int->m_kind == 1) {
                        /*  Character is stored as string in memory.
                            The variable points to this location in memory
                        */
                        wasm::emit_b8(code, m_al, wasm::type::i32);
                    } else {
                        throw CodeGenError(
                            "Characters of kind 1 only supported");
                    }
                }
            } else if (ASRUtils::is_complex(*v->m_type)) {
                ASR::Complex_t *v_comp =
                    ASR::down_cast<ASR::Complex_t>(v->m_type);

                if (is_array) {
                    wasm::emit_b8(code, m_al, wasm::type::i32);
                } else {
                    if (v_comp->m_kind == 4) {
                        wasm::emit_b8(code, m_al, wasm::type::f32); // real part
                        if (emitCount) {
                            wasm::emit_u32(code, m_al, 1U);
                        }
                        wasm::emit_b8(code, m_al, wasm::type::f32); // imag part
                    } else if (v_comp->m_kind == 8) {
                        wasm::emit_b8(code, m_al, wasm::type::f64); // real part
                        if (emitCount) {
                            wasm::emit_u32(code, m_al, 1U);
                        }
                        wasm::emit_b8(code, m_al, wasm::type::f64); // imag part
                    } else {
                        throw CodeGenError(
                            "Complex numbers of kind 4 and 8 only supported yet");
                    }
                    no_of_vars++;
                }
            } else {
                diag.codegen_warning_label("Unsupported variable type: " +
                        ASRUtils::type_to_str(v->m_type), {v->base.base.loc},
                        "Only integer, floats, logical and complex supported currently");
                wasm::emit_b8(code, m_al, wasm::type::i32);
            }
        }
        no_of_vars++;
    }

    bool isLocalVar(ASR::Variable_t *v) {
        return (v->m_intent == ASRUtils::intent_local ||
                v->m_intent == ASRUtils::intent_return_var);
    }

    void emit_local_vars(SymbolTable* symtab) {
        for (auto &item : symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v =
                    ASR::down_cast<ASR::Variable_t>(item.second);
                if (isLocalVar(v)) {
                    m_var_idx_map[get_hash((ASR::asr_t *)v)] = cur_sym_info->no_of_variables;
                    emit_var_type(m_code_section, v, cur_sym_info->no_of_variables, true);
                }
            }
        }
    }

    void emit_var_get(ASR::Variable_t *v) {
        uint64_t hash = get_hash((ASR::asr_t *)v);
        if (m_var_idx_map.find(hash) != m_var_idx_map.end()) {
            uint32_t var_idx = m_var_idx_map[hash];
            wasm::emit_local_get(m_code_section, m_al, var_idx);
            if (ASRUtils::is_complex(*v->m_type)) {
                // get the imaginary part
                wasm::emit_local_get(m_code_section, m_al, var_idx + 1u);
            }
        } else if (m_global_var_idx_map.find(hash) != m_global_var_idx_map.end()) {
            uint32_t var_idx = m_global_var_idx_map[hash];
            wasm::emit_global_get(m_code_section, m_al, var_idx);
            if (ASRUtils::is_complex(*v->m_type)) {
                // get the imaginary part
                wasm::emit_global_get(m_code_section, m_al, var_idx + 1u);
            }
        } else {
            throw CodeGenError("Variable " + std::string(v->m_name) + " not declared");
        }
    }

    void emit_var_set(ASR::Variable_t *v) {
        uint64_t hash = get_hash((ASR::asr_t *)v);
        if (m_var_idx_map.find(hash) != m_var_idx_map.end()) {
            uint32_t var_idx = m_var_idx_map[hash];
            if (ASRUtils::is_complex(*v->m_type)) {
                // set the imaginary part
                wasm::emit_local_set(m_code_section, m_al, var_idx + 1u);
            }
            wasm::emit_local_set(m_code_section, m_al, var_idx);
        } else if (m_global_var_idx_map.find(hash) != m_global_var_idx_map.end()) {
            uint32_t var_idx = m_global_var_idx_map[hash];
            if (ASRUtils::is_complex(*v->m_type)) {
                // set the imaginary part
                wasm::emit_global_set(m_code_section, m_al, var_idx + 1u);
            }
            wasm::emit_global_set(m_code_section, m_al, var_idx);
        } else {
            throw CodeGenError("Variable " + std::string(v->m_name) + " not declared");
        }
    }

    void initialize_local_vars(SymbolTable* symtab) {
        // initialize the value for local variables if initialization exists
        for (auto &item : symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v =
                    ASR::down_cast<ASR::Variable_t>(item.second);
                if (isLocalVar(v)) {
                    if (v->m_symbolic_value) {
                        this->visit_expr(*v->m_symbolic_value);
                        emit_var_set(v);
                    } else if (ASRUtils::is_array(v->m_type)) {
                        uint32_t kind =
                            ASRUtils::extract_kind_from_ttype_t(v->m_type);

                        Vec<uint32_t> array_dims;
                        get_array_dims(*v, array_dims);

                        uint32_t total_array_size = 1;
                        for (auto &dim : array_dims) {
                            total_array_size *= dim;
                        }

                        wasm::emit_i32_const(m_code_section, m_al,
                                             avail_mem_loc);
                        emit_var_set(v);
                        avail_mem_loc += kind * total_array_size;
                    }
                }
            }
        }
    }

    bool isRefVar(ASR::Variable_t* v) {
        return (v->m_intent == ASRUtils::intent_out ||
                v->m_intent == ASRUtils::intent_inout ||
                v->m_intent == ASRUtils::intent_unspecified);
    }

    void emit_function_prototype(const ASR::Function_t &x) {
        SymbolFuncInfo *s = new SymbolFuncInfo;

        /********************* New Type Declaration *********************/
        wasm::emit_b8(m_type_section, m_al, 0x60);

        /********************* Parameter Types List *********************/
        s->referenced_vars.reserve(m_al, x.n_args);
        uint32_t len_idx_type_section_param_types_list =
                wasm::emit_len_placeholder(m_type_section, m_al);
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::Variable_t *arg = ASRUtils::EXPR2VAR(x.m_args[i]);
            LCOMPILERS_ASSERT(ASRUtils::is_arg_dummy(arg->m_intent));
            m_var_idx_map[get_hash((ASR::asr_t *)arg)] = s->no_of_variables;
            emit_var_type(m_type_section, arg, s->no_of_variables, false);
            if (isRefVar(arg)) {
                s->referenced_vars.push_back(m_al, arg);
            }
        }
        wasm::fixup_len(m_type_section, m_al,
                            len_idx_type_section_param_types_list);

        /********************* Result Types List *********************/
        uint32_t len_idx_type_section_return_types_list = wasm::emit_len_placeholder(m_type_section, m_al);
        uint32_t no_of_return_vars = 0;
        if (x.m_return_var) {  // It is a function
            s->return_var = ASRUtils::EXPR2VAR(x.m_return_var);
            emit_var_type(m_type_section, s->return_var, no_of_return_vars, false);
        } else {  // It is a subroutine
            for (size_t i = 0; i < x.n_args; i++) {
                ASR::Variable_t *arg = ASRUtils::EXPR2VAR(x.m_args[i]);
                if (isRefVar(arg)) {
                    emit_var_type(m_type_section, arg, no_of_return_vars, false);
                }
            }
        }
        wasm::fixup_len(m_type_section, m_al, len_idx_type_section_return_types_list);

        /********************* Add Type to Map *********************/
        s->index = no_of_types++;
        m_func_name_idx_map[get_hash((ASR::asr_t *)&x)] =
            s;  // add function to map
    }

    template <typename T>
    void visit_BlockStatements(const T& x) {
        for (size_t i = 0; i < x.n_body; i++) {
            if (ASR::is_a<ASR::BlockCall_t>(*x.m_body[i])) {
                this->visit_stmt(*x.m_body[i]);
            }
        }
    }

    void emit_function_body(const ASR::Function_t &x) {
        LCOMPILERS_ASSERT(m_func_name_idx_map.find(get_hash((ASR::asr_t *)&x)) !=
                        m_func_name_idx_map.end());

        cur_sym_info = m_func_name_idx_map[get_hash((ASR::asr_t *)&x)];

        /********************* Reference Function Prototype
         * *********************/
        wasm::emit_u32(m_func_section, m_al, cur_sym_info->index);

        /********************* Function Body Starts Here *********************/
        uint32_t len_idx_code_section_func_size =
            wasm::emit_len_placeholder(m_code_section, m_al);

        {
            is_local_vars_only = true;
            int params_cnt = cur_sym_info->no_of_variables;
            /********************* Local Vars Types List *********************/
            uint32_t len_idx_code_section_local_vars_list =
                wasm::emit_len_placeholder(m_code_section, m_al);

            emit_local_vars(x.m_symtab);
            visit_BlockStatements(x);

            // fixup length of local vars list
            wasm::emit_u32_b32_idx(m_code_section, m_al,
                                len_idx_code_section_local_vars_list,
                                cur_sym_info->no_of_variables - params_cnt);
            is_local_vars_only = false;
        }

        initialize_local_vars(x.m_symtab);

        for (size_t i = 0; i < x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        if (strcmp(x.m_name, "_start") == 0) {
            wasm::emit_i32_const(m_code_section, m_al, 0 /* zero exit code */);
            wasm::emit_call(m_code_section, m_al, m_import_func_idx_map[proc_exit]);
        }

        if (x.n_body == 0 || !ASR::is_a<ASR::Return_t>(*x.m_body[x.n_body - 1])) {
            handle_return();
        }
        wasm::emit_expr_end(m_code_section, m_al);

        wasm::fixup_len(m_code_section, m_al, len_idx_code_section_func_size);

        /********************* Export the function *********************/
        wasm::emit_export_fn(m_export_section, m_al, x.m_name,
                             cur_sym_info->index);  //  add function to export
        no_of_functions++;
        no_of_exports++;
    }

    bool is_unsupported_function(const ASR::Function_t &x) {
        if (strcmp(x.m_name, "_start") == 0) return false;

        if (!x.n_body) {
            return true;
        }
        if (ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::BindC &&
            ASRUtils::get_FunctionType(x)->m_deftype == ASR::deftypeType::Interface) {
            if (ASRUtils::is_intrinsic_function2(&x)) {
                diag.codegen_warning_label(
                    "WASM: C Intrinsic Functions not yet supported",
                    {x.base.base.loc}, std::string(x.m_name));
            }
            return true;
        }
        for (size_t i = 0; i < x.n_body; i++) {
            if (x.m_body[i]->type == ASR::stmtType::SubroutineCall) {
                auto sub_call = (const ASR::SubroutineCall_t &)(*x.m_body[i]);
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
                    ASRUtils::symbol_get_past_external(sub_call.m_name));
                if (ASRUtils::get_FunctionType(s)->m_abi == ASR::abiType::BindC &&
                    ASRUtils::get_FunctionType(s)->m_deftype == ASR::deftypeType::Interface &&
                    ASRUtils::is_intrinsic_function2(s)) {
                    diag.codegen_warning_label(
                        "WASM: Calls to C Intrinsic Functions are not yet "
                        "supported",
                        {x.m_body[i]->base.loc},
                        "Function: calls " + std::string(s->m_name));
                    return true;
                }
            }
        }
        return false;
    }

    void visit_Function(const ASR::Function_t &x) {
        if (is_unsupported_function(x)) {
            return;
        }
        if (is_prototype_only) {
            emit_function_prototype(x);
            return;
        }
        emit_function_body(x);
    }

    void visit_BlockCall(const ASR::BlockCall_t &x) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Block_t>(*x.m_m));
        ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(x.m_m);
        if (is_local_vars_only) {
            emit_local_vars(block->m_symtab);
            visit_BlockStatements(*block);
        } else {
            initialize_local_vars(block->m_symtab);
            for (size_t i = 0; i < block->n_body; i++) {
                this->visit_stmt(*block->m_body[i]);
            }
        }
    }

    uint32_t emit_memory_store(ASR::expr_t *v) {
        auto ttype = ASRUtils::expr_type(v);
        auto kind = ASRUtils::extract_kind_from_ttype_t(ttype);
        switch (ttype->type) {
            case ASR::ttypeType::Integer: {
                switch (kind) {
                    case 4:
                        wasm::emit_i32_store(m_code_section, m_al,
                                             wasm::mem_align::b8, 0);
                        break;
                    case 8:
                        wasm::emit_i64_store(m_code_section, m_al,
                                             wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError(
                            "MemoryStore: Unsupported Integer kind");
                }
                break;
            }
            case ASR::ttypeType::Real: {
                switch (kind) {
                    case 4:
                        wasm::emit_f32_store(m_code_section, m_al,
                                             wasm::mem_align::b8, 0);
                        break;
                    case 8:
                        wasm::emit_f64_store(m_code_section, m_al,
                                             wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError(
                            "MemoryStore: Unsupported Real kind");
                }
                break;
            }
            case ASR::ttypeType::Logical: {
                switch (kind) {
                    case 4:
                        wasm::emit_i32_store(m_code_section, m_al,
                                             wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError(
                            "MemoryStore: Unsupported Logical kind");
                }
                break;
            }
            case ASR::ttypeType::Character: {
                switch (kind) {
                    case 4:
                        wasm::emit_i32_store(m_code_section, m_al,
                                             wasm::mem_align::b8, 0);
                        break;
                    case 8:
                        wasm::emit_i64_store(m_code_section, m_al,
                                             wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError(
                            "MemoryStore: Unsupported Character kind");
                }
                break;
            }
            default: {
                throw CodeGenError("MemoryStore: Type " +
                                   ASRUtils::type_to_str(ttype) +
                                   " not yet supported");
            }
        }
        return kind;
    }

    void emit_memory_load(ASR::expr_t *v) {
        auto ttype = ASRUtils::expr_type(v);
        auto kind = ASRUtils::extract_kind_from_ttype_t(ttype);
        switch (ttype->type) {
            case ASR::ttypeType::Integer: {
                switch (kind) {
                    case 4:
                        wasm::emit_i32_load(m_code_section, m_al,
                                            wasm::mem_align::b8, 0);
                        break;
                    case 8:
                        wasm::emit_i64_load(m_code_section, m_al,
                                            wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError(
                            "MemoryLoad: Unsupported Integer kind");
                }
                break;
            }
            case ASR::ttypeType::Real: {
                switch (kind) {
                    case 4:
                        wasm::emit_f32_load(m_code_section, m_al,
                                            wasm::mem_align::b8, 0);
                        break;
                    case 8:
                        wasm::emit_f64_load(m_code_section, m_al,
                                            wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError("MemoryLoad: Unsupported Real kind");
                }
                break;
            }
            case ASR::ttypeType::Logical: {
                switch (kind) {
                    case 4:
                        wasm::emit_i32_load(m_code_section, m_al,
                                            wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError(
                            "MemoryLoad: Unsupported Logical kind");
                }
                break;
            }
            case ASR::ttypeType::Character: {
                switch (kind) {
                    case 4:
                        wasm::emit_i32_load(m_code_section, m_al,
                                            wasm::mem_align::b8, 0);
                        break;
                    case 8:
                        wasm::emit_i64_load(m_code_section, m_al,
                                            wasm::mem_align::b8, 0);
                        break;
                    default:
                        throw CodeGenError(
                            "MemoryLoad: Unsupported Character kind");
                }
                break;
            }
            default: {
                throw CodeGenError("MemoryLoad: Type " +
                                   ASRUtils::type_to_str(ttype) +
                                   " not yet supported");
            }
        }
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        // this->visit_expr(*x.m_target);
        if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
            this->visit_expr(*x.m_value);
            ASR::Variable_t *asr_target = ASRUtils::EXPR2VAR(x.m_target);
            emit_var_set(asr_target);
        } else if (ASR::is_a<ASR::ArrayItem_t>(*x.m_target)) {
            emit_array_item_address_onto_stack(
                *(ASR::down_cast<ASR::ArrayItem_t>(x.m_target)));
            this->visit_expr(*x.m_value);
            emit_memory_store(x.m_value);
        } else {
            LCOMPILERS_ASSERT(false)
        }
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
        ASR::Integer_t *i = ASR::down_cast<ASR::Integer_t>(x.m_type);
        if (i->m_kind == 4) {
            switch (x.m_op) {
                case ASR::binopType::Add: {
                    wasm::emit_i32_add(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Sub: {
                    wasm::emit_i32_sub(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Mul: {
                    wasm::emit_i32_mul(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Div: {
                    wasm::emit_i32_div_s(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Pow: {
                    ASR::expr_t *val = ASRUtils::expr_value(x.m_right);
                    if (ASR::is_a<ASR::IntegerConstant_t>(*val)) {
                        ASR::IntegerConstant_t *c =
                            ASR::down_cast<ASR::IntegerConstant_t>(val);
                        if (c->m_n == 2) {
                            // drop the last stack item in the wasm stack
                            wasm::emit_drop(m_code_section, m_al);
                            this->visit_expr(*x.m_left);
                            wasm::emit_i32_mul(m_code_section, m_al);
                        } else {
                            throw CodeGenError(
                                "IntegerBinop kind 4: only x**2 implemented so "
                                "far for powers");
                        }
                    } else {
                        throw CodeGenError(
                            "IntegerBinop kind 4: only x**2 implemented so far "
                            "for powers");
                    }
                    break;
                };
                case ASR::binopType::BitAnd: {
                    wasm::emit_i32_and(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitOr: {
                    wasm::emit_i32_or(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitXor: {
                    wasm::emit_i32_xor(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitLShift: {
                    wasm::emit_i32_shl(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitRShift: {
                    wasm::emit_i32_shr_s(m_code_section, m_al);
                    break;
                };
                default: {
                    throw CodeGenError(
                        "ICE IntegerBinop kind 4: unknown operation");
                }
            }
        } else if (i->m_kind == 8) {
            switch (x.m_op) {
                case ASR::binopType::Add: {
                    wasm::emit_i64_add(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Sub: {
                    wasm::emit_i64_sub(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Mul: {
                    wasm::emit_i64_mul(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Div: {
                    wasm::emit_i64_div_s(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Pow: {
                    ASR::expr_t *val = ASRUtils::expr_value(x.m_right);
                    if (ASR::is_a<ASR::IntegerConstant_t>(*val)) {
                        ASR::IntegerConstant_t *c =
                            ASR::down_cast<ASR::IntegerConstant_t>(val);
                        if (c->m_n == 2) {
                            // drop the last stack item in the wasm stack
                            wasm::emit_drop(m_code_section, m_al);
                            this->visit_expr(*x.m_left);
                            wasm::emit_i64_mul(m_code_section, m_al);
                        } else {
                            throw CodeGenError(
                                "IntegerBinop kind 8: only x**2 implemented so "
                                "far for powers");
                        }
                    } else {
                        throw CodeGenError(
                            "IntegerBinop kind 8: only x**2 implemented so far "
                            "for powers");
                    }
                    break;
                };
                case ASR::binopType::BitAnd: {
                    wasm::emit_i64_and(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitOr: {
                    wasm::emit_i64_or(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitXor: {
                    wasm::emit_i64_xor(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitLShift: {
                    wasm::emit_i64_shl(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::BitRShift: {
                    wasm::emit_i64_shr_s(m_code_section, m_al);
                    break;
                };
                default: {
                    throw CodeGenError(
                        "ICE IntegerBinop kind 8: unknown operation");
                }
            }
        } else {
            throw CodeGenError("IntegerBinop: Integer kind not supported");
        }
    }

    void visit_IntegerBitNot(const ASR::IntegerBitNot_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_arg);
        ASR::Integer_t *i = ASR::down_cast<ASR::Integer_t>(x.m_type);
        // there is no direct bit-invert inst in wasm,
        // so xor-ing with -1 (sequence of 32/64 1s)
        if(i->m_kind == 4){
            wasm::emit_i32_const(m_code_section, m_al, -1);
            wasm::emit_i32_xor(m_code_section, m_al);
        }
        else if(i->m_kind == 8){
            wasm::emit_i64_const(m_code_section, m_al, -1LL);
            wasm::emit_i64_xor(m_code_section, m_al);
        }
        else{
            throw CodeGenError("IntegerBitNot: Only kind 4 and 8 supported");
        }
    }

    void visit_RealBinOp(const ASR::RealBinOp_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
        ASR::Real_t *f = ASR::down_cast<ASR::Real_t>(x.m_type);
        if (f->m_kind == 4) {
            switch (x.m_op) {
                case ASR::binopType::Add: {
                    wasm::emit_f32_add(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Sub: {
                    wasm::emit_f32_sub(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Mul: {
                    wasm::emit_f32_mul(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Div: {
                    wasm::emit_f32_div(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Pow: {
                    ASR::expr_t *val = ASRUtils::expr_value(x.m_right);
                    if (ASR::is_a<ASR::RealConstant_t>(*val)) {
                        ASR::RealConstant_t *c =
                            ASR::down_cast<ASR::RealConstant_t>(val);
                        if (c->m_r == 2.0) {
                            // drop the last stack item in the wasm stack
                            wasm::emit_drop(m_code_section, m_al);
                            this->visit_expr(*x.m_left);
                            wasm::emit_f32_mul(m_code_section, m_al);
                        } else {
                            throw CodeGenError(
                                "RealBinop: only x**2 implemented so far for "
                                "powers");
                        }
                    } else {
                        throw CodeGenError(
                            "RealBinop: only x**2 implemented so far for "
                            "powers");
                    }
                    break;
                };
                default: {
                    throw CodeGenError(
                        "ICE RealBinop kind 4: unknown operation");
                }
            }
        } else if (f->m_kind == 8) {
            switch (x.m_op) {
                case ASR::binopType::Add: {
                    wasm::emit_f64_add(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Sub: {
                    wasm::emit_f64_sub(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Mul: {
                    wasm::emit_f64_mul(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Div: {
                    wasm::emit_f64_div(m_code_section, m_al);
                    break;
                };
                case ASR::binopType::Pow: {
                    ASR::expr_t *val = ASRUtils::expr_value(x.m_right);
                    if (ASR::is_a<ASR::RealConstant_t>(*val)) {
                        ASR::RealConstant_t *c =
                            ASR::down_cast<ASR::RealConstant_t>(val);
                        if (c->m_r == 2.0) {
                            // drop the last stack item in the wasm stack
                            wasm::emit_drop(m_code_section, m_al);
                            this->visit_expr(*x.m_left);
                            wasm::emit_f64_mul(m_code_section, m_al);
                        } else {
                            throw CodeGenError(
                                "RealBinop: only x**2 implemented so far for "
                                "powers");
                        }
                    } else {
                        throw CodeGenError(
                            "RealBinop: only x**2 implemented so far for "
                            "powers");
                    }
                    break;
                };
                default: {
                    throw CodeGenError("ICE RealBinop: unknown operation");
                }
            }
        } else {
            throw CodeGenError("RealBinop: Real kind not supported");
        }
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t &x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
        LCOMPILERS_ASSERT(ASRUtils::is_complex(*x.m_type));
        int a_kind = ASR::down_cast<ASR::Complex_t>(ASRUtils::type_get_past_pointer(x.m_type))->m_kind;
        switch (x.m_op) {
            case ASR::binopType::Add: {
                if (a_kind == 4) {
                    if (m_rt_func_used_idx[add_c32] == -1) {
                        m_rt_func_used_idx[add_c32] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[add_c32]);
                } else {
                    if (m_rt_func_used_idx[add_c64] == -1) {
                        m_rt_func_used_idx[add_c64] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[add_c64]);
                }
                break;
            };
            case ASR::binopType::Sub: {
                if (a_kind == 4) {
                    if (m_rt_func_used_idx[sub_c32] == -1) {
                        m_rt_func_used_idx[sub_c32] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[sub_c32]);
                } else {
                    if (m_rt_func_used_idx[sub_c64] == -1) {
                        m_rt_func_used_idx[sub_c64] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[sub_c64]);
                }
                break;
            };
            case ASR::binopType::Mul: {
                if (a_kind == 4) {
                    if (m_rt_func_used_idx[mul_c32] == -1) {
                        m_rt_func_used_idx[mul_c32] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[mul_c32]);
                } else {
                    if (m_rt_func_used_idx[mul_c64] == -1) {
                        m_rt_func_used_idx[mul_c64] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[mul_c64]);
                }
                break;
            };
            default: {
                throw CodeGenError("ComplexBinOp: Binary operator '" + ASRUtils::binop_to_str_python(x.m_op) + "' not supported",
                    x.base.base.loc);
            }
        }
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        ASR::Integer_t *i = ASR::down_cast<ASR::Integer_t>(x.m_type);
        // there seems no direct unary-minus inst in wasm, so subtracting from 0
        if (i->m_kind == 4) {
            wasm::emit_i32_const(m_code_section, m_al, 0);
            this->visit_expr(*x.m_arg);
            wasm::emit_i32_sub(m_code_section, m_al);
        } else if (i->m_kind == 8) {
            wasm::emit_i64_const(m_code_section, m_al, 0LL);
            this->visit_expr(*x.m_arg);
            wasm::emit_i64_sub(m_code_section, m_al);
        } else {
            throw CodeGenError(
                "IntegerUnaryMinus: Only kind 4 and 8 supported");
        }
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        ASR::Real_t *f = ASR::down_cast<ASR::Real_t>(x.m_type);
        if (f->m_kind == 4) {
            this->visit_expr(*x.m_arg);
            wasm::emit_f32_neg(m_code_section, m_al);
        } else if (f->m_kind == 8) {
            this->visit_expr(*x.m_arg);
            wasm::emit_f64_neg(m_code_section, m_al);
        } else {
            throw CodeGenError("RealUnaryMinus: Only kind 4 and 8 supported");
        }
    }

    void visit_ComplexUnaryMinus(const ASR::ComplexUnaryMinus_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        ASR::Complex_t *f = ASR::down_cast<ASR::Complex_t>(x.m_type);
        if (f->m_kind == 4) {
            this->visit_expr(*x.m_arg);
            wasm::emit_f32_neg(m_code_section, m_al);
            wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f32]);
            wasm::emit_f32_neg(m_code_section, m_al);
            wasm::emit_global_get(m_code_section, m_al, m_compiler_globals[tmp_reg_f32]);
        } else if (f->m_kind == 8) {
            this->visit_expr(*x.m_arg);
            wasm::emit_f64_neg(m_code_section, m_al);
            wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
            wasm::emit_f64_neg(m_code_section, m_al);
            wasm::emit_global_get(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
        } else {
            throw CodeGenError("ComplexUnaryMinus: Only kind 4 and 8 supported");
        }
    }

    template <typename T>
    int get_kind_from_operands(const T &x) {
        ASR::ttype_t *left_ttype = ASRUtils::expr_type(x.m_left);
        int left_kind = ASRUtils::extract_kind_from_ttype_t(left_ttype);

        ASR::ttype_t *right_ttype = ASRUtils::expr_type(x.m_right);
        int right_kind = ASRUtils::extract_kind_from_ttype_t(right_ttype);

        if (left_kind != right_kind) {
            diag.codegen_error_label("Operand kinds do not match",
                                     {x.base.base.loc},
                                     "WASM Type Mismatch Error");
            throw CodeGenAbort();
        }

        return left_kind;
    }

    template <typename T>
    void handle_integer_compare(const T &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
        // int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        int a_kind = get_kind_from_operands(x);
        if (a_kind == 4) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq): {
                    wasm::emit_i32_eq(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Gt): {
                    wasm::emit_i32_gt_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::GtE): {
                    wasm::emit_i32_ge_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Lt): {
                    wasm::emit_i32_lt_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::LtE): {
                    wasm::emit_i32_le_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::NotEq): {
                    wasm::emit_i32_ne(m_code_section, m_al);
                    break;
                }
                default:
                    throw CodeGenError(
                        "handle_integer_compare: Kind 4: Unhandled switch "
                        "case");
            }
        } else if (a_kind == 8) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq): {
                    wasm::emit_i64_eq(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Gt): {
                    wasm::emit_i64_gt_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::GtE): {
                    wasm::emit_i64_ge_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Lt): {
                    wasm::emit_i64_lt_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::LtE): {
                    wasm::emit_i64_le_s(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::NotEq): {
                    wasm::emit_i64_ne(m_code_section, m_al);
                    break;
                }
                default:
                    throw CodeGenError(
                        "handle_integer_compare: Kind 8: Unhandled switch "
                        "case");
            }
        } else {
            throw CodeGenError("IntegerCompare: kind 4 and 8 supported only");
        }
    }

    void handle_real_compare(const ASR::RealCompare_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
        // int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        int a_kind = get_kind_from_operands(x);
        if (a_kind == 4) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq): {
                    wasm::emit_f32_eq(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Gt): {
                    wasm::emit_f32_gt(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::GtE): {
                    wasm::emit_f32_ge(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Lt): {
                    wasm::emit_f32_lt(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::LtE): {
                    wasm::emit_f32_le(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::NotEq): {
                    wasm::emit_f32_ne(m_code_section, m_al);
                    break;
                }
                default:
                    throw CodeGenError(
                        "handle_real_compare: Kind 4: Unhandled switch case");
            }
        } else if (a_kind == 8) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq): {
                    wasm::emit_f64_eq(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Gt): {
                    wasm::emit_f64_gt(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::GtE): {
                    wasm::emit_f64_ge(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::Lt): {
                    wasm::emit_f64_lt(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::LtE): {
                    wasm::emit_f64_le(m_code_section, m_al);
                    break;
                }
                case (ASR::cmpopType::NotEq): {
                    wasm::emit_f64_ne(m_code_section, m_al);
                    break;
                }
                default:
                    throw CodeGenError(
                        "handle_real_compare: Kind 8: Unhandled switch case");
            }
        } else {
            throw CodeGenError("RealCompare: kind 4 and 8 supported only");
        }
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        handle_integer_compare(x);
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
        handle_real_compare(x);
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t & /*x*/) {
        throw CodeGenError("Complex Types not yet supported");
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
        handle_integer_compare(x);
    }

    void visit_StringCompare(const ASR::StringCompare_t & /*x*/) {
        throw CodeGenError("String Types not yet supported");
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t &x) {
        if (x.m_value) {
            visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
        int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        if (a_kind == 4) {
            switch (x.m_op) {
                case (ASR::logicalbinopType::And): {
                    wasm::emit_i32_and(m_code_section, m_al);
                    break;
                }
                case (ASR::logicalbinopType::Or): {
                    wasm::emit_i32_or(m_code_section, m_al);
                    break;
                }
                case ASR::logicalbinopType::Xor: {
                    wasm::emit_i32_xor(m_code_section, m_al);
                    break;
                }
                case (ASR::logicalbinopType::NEqv): {
                    wasm::emit_i32_xor(m_code_section, m_al);
                    break;
                }
                case (ASR::logicalbinopType::Eqv): {
                    wasm::emit_i32_eq(m_code_section, m_al);
                    break;
                }
                default:
                    throw CodeGenError(
                        "LogicalBinOp: Kind 4: Unhandled switch case");
            }
        } else {
            throw CodeGenError("LogicalBinOp: kind 4 supported only");
        }
    }

    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_arg);
        int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        if (a_kind == 4) {
            wasm::emit_i32_eqz(m_code_section, m_al);
        } else if (a_kind == 8) {
            wasm::emit_i64_eqz(m_code_section, m_al);
        } else {
            throw CodeGenError("LogicalNot: kind 4 and 8 supported only");
        }
    }

    void visit_Var(const ASR::Var_t &x) {
        const ASR::symbol_t *s = ASRUtils::symbol_get_past_external(x.m_v);
        auto v = ASR::down_cast<ASR::Variable_t>(s);
        switch (v->m_type->type) {
            case ASR::ttypeType::Integer:
            case ASR::ttypeType::Logical:
            case ASR::ttypeType::Real:
            case ASR::ttypeType::Character:
            case ASR::ttypeType::Complex: {
                emit_var_get(v);
                break;
            }
            default:
                throw CodeGenError(
                    "Only Integer, Float, Bool, Character, Complex "
                    "variable types supported currently");
        }
    }

    void get_array_dims(const ASR::Variable_t &x, Vec<uint32_t> &dims) {
        ASR::dimension_t *m_dims;
        uint32_t n_dims =
            ASRUtils::extract_dimensions_from_ttype(x.m_type, m_dims);
        dims.reserve(m_al, n_dims);
        for (uint32_t i = 0; i < n_dims; i++) {
            ASR::expr_t *length_value =
                ASRUtils::expr_value(m_dims[i].m_length);
            uint64_t len_in_this_dim = -1;
            ASRUtils::extract_value(length_value, len_in_this_dim);
            dims.push_back(m_al, (uint32_t)len_in_this_dim);
        }
    }

    void emit_array_item_address_onto_stack(const ASR::ArrayItem_t &x) {
        this->visit_expr(*x.m_v);
        ASR::ttype_t *ttype = ASRUtils::expr_type(x.m_v);
        uint32_t kind = ASRUtils::extract_kind_from_ttype_t(ttype);
        ASR::dimension_t *m_dims;
        ASRUtils::extract_dimensions_from_ttype(ttype, m_dims);

        wasm::emit_i32_const(m_code_section, m_al, 0);
        for (uint32_t i = 0; i < x.n_args; i++) {
            if (x.m_args[i].m_right) {
                this->visit_expr(*x.m_args[i].m_right);
                this->visit_expr(*m_dims[i].m_start);
                wasm::emit_i32_sub(m_code_section, m_al);
                size_t jmin, jmax;

                if (x.m_storage_format == ASR::arraystorageType::ColMajor) {
                    // Column-major order
                    jmin = 0;
                    jmax = i;
                } else {
                    // Row-major order
                    jmin = i + 1;
                    jmax = x.n_args;
                }

                for (size_t j = jmin; j < jmax; j++) {
                    this->visit_expr(*m_dims[j].m_length);
                    wasm::emit_i32_mul(m_code_section, m_al);
                }

                wasm::emit_i32_add(m_code_section, m_al);
            } else {
                diag.codegen_warning_label("/* FIXME right index */",
                                           {x.base.base.loc}, "");
            }
        }
        wasm::emit_i32_const(m_code_section, m_al, kind);
        wasm::emit_i32_mul(m_code_section, m_al);
        wasm::emit_i32_add(m_code_section, m_al);
    }

    void visit_ArrayItem(const ASR::ArrayItem_t &x) {
        emit_array_item_address_onto_stack(x);
        emit_memory_load(x.m_v);
    }

    void visit_ArraySize(const ASR::ArraySize_t &x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            return;
        }
        ASR::dimension_t *m_dims;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(
            ASRUtils::expr_type(x.m_v), m_dims);
        if (x.m_dim) {
            int dim_idx = -1;
            ASRUtils::extract_value(ASRUtils::expr_value(x.m_dim), dim_idx);
            if (dim_idx == -1) {
                throw CodeGenError("Dimension index not available");
            }
            if (!m_dims[dim_idx - 1].m_length) {
                throw CodeGenError("Dimension length for index " +
                                   std::to_string(dim_idx) + " does not exist");
            }
            this->visit_expr(*(m_dims[dim_idx - 1].m_length));
        } else {
            if (!m_dims[0].m_length) {
                throw CodeGenError(
                    "Dimension length for index 0 does not exist");
            }
            this->visit_expr(*(m_dims[0].m_length));
            for (int i = 1; i < n_dims; i++) {
                this->visit_expr(*m_dims[i].m_length);
                wasm::emit_i32_mul(m_code_section, m_al);
            }
        }

        int kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        if (kind == 8) {
            wasm::emit_i64_extend_i32_s(m_code_section, m_al);
        }
    }

    void handle_return() {
        if (cur_sym_info->return_var) {
            emit_var_get(cur_sym_info->return_var);
        } else {
            for (auto return_var : cur_sym_info->referenced_vars) {
                emit_var_get(return_var);
            }
        }
        wasm::emit_b8(m_code_section, m_al, 0x0F); // wasm return
    }

    void visit_Return(const ASR::Return_t & /* x */) { handle_return(); }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        int64_t val = x.m_n;
        int a_kind = ((ASR::Integer_t *)(&(x.m_type->base)))->m_kind;
        switch (a_kind) {
            case 4: {
                wasm::emit_i32_const(m_code_section, m_al, val);
                break;
            }
            case 8: {
                wasm::emit_i64_const(m_code_section, m_al, val);
                break;
            }
            default: {
                throw CodeGenError(
                    "Constant Integer: Only kind 4 and 8 supported");
            }
        }
    }

    void visit_RealConstant(const ASR::RealConstant_t &x) {
        double val = x.m_r;
        int a_kind = ((ASR::Real_t *)(&(x.m_type->base)))->m_kind;
        switch (a_kind) {
            case 4: {
                wasm::emit_f32_const(m_code_section, m_al, val);
                break;
            }
            case 8: {
                wasm::emit_f64_const(m_code_section, m_al, val);
                break;
            }
            default: {
                throw CodeGenError(
                    "Constant Real: Only kind 4 and 8 supported");
            }
        }
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        bool val = x.m_value;
        int a_kind = ((ASR::Logical_t *)(&(x.m_type->base)))->m_kind;
        switch (a_kind) {
            case 4: {
                wasm::emit_i32_const(m_code_section, m_al, val);
                break;
            }
            default: {
                throw CodeGenError("Constant Logical: Only kind 4 supported");
            }
        }
    }

    void visit_ComplexConstructor(const ASR::ComplexConstructor_t &x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_re);
        this->visit_expr(*x.m_im);
    }

    void visit_ComplexConstant(const ASR::ComplexConstant_t &x) {
        int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        switch( a_kind ) {
            case 4: {
                wasm::emit_f32_const(m_code_section, m_al, x.m_re);
                wasm::emit_f32_const(m_code_section, m_al, x.m_im);
                break;
            }
            case 8: {
                wasm::emit_f64_const(m_code_section, m_al, x.m_re);
                wasm::emit_f64_const(m_code_section, m_al, x.m_im);
                break;
            }
            default: {
                throw CodeGenError("kind type is not supported");
            }
        }
    }

    std::string convert_int_to_bytes_string(int n) {
        uint8_t bytes[sizeof(n)];
        std::memcpy(&bytes, &n, sizeof(n));
        std::string result = "";
        for (size_t i = 0; i < sizeof(n); i++) {
            result += char(bytes[i]);
        }
        return result;
    }

    void align_str_by_4_bytes(std::string &s) {
        int n = s.length();
        if (n % 4 == 0) return;
        for (int i = 0; i < 4 - (n % 4); i++) {
            s += " ";
        }
    }

    void emit_string(std::string str) {
        if (m_string_to_iov_loc_map.find(str) != m_string_to_iov_loc_map.end()) {
            return;
        }

        // Todo: Add a check here if there is memory available to store the
        // given string

        m_string_to_iov_loc_map[str] = avail_mem_loc;

        uint32_t string_loc = avail_mem_loc + 8U /* IOV_SIZE */;
        std::string iov = convert_int_to_bytes_string(string_loc) + convert_int_to_bytes_string(str.length());
        wasm::emit_str_const(m_data_section, m_al, avail_mem_loc, iov);
        avail_mem_loc += iov.length();
        no_of_data_segments++;

        align_str_by_4_bytes(str);
        wasm::emit_str_const(m_data_section, m_al, avail_mem_loc, str);
        avail_mem_loc += str.length();
        no_of_data_segments++;
    }

    void visit_StringConstant(const ASR::StringConstant_t &x) {
        emit_string(x.m_s);
        wasm::emit_i32_const(m_code_section, m_al, m_string_to_iov_loc_map[x.m_s]);
    }

    void visit_ArrayConstant(const ASR::ArrayConstant_t &x) {
        // Todo: Add a check here if there is memory available to store the
        // given string
        uint32_t cur_mem_loc = avail_mem_loc;
        for (size_t i = 0; i < x.n_args; i++) {
            // emit memory location to store array element
            wasm::emit_i32_const(m_code_section, m_al, avail_mem_loc);

            this->visit_expr(*x.m_args[i]);
            int element_size_in_bytes = emit_memory_store(x.m_args[i]);
            avail_mem_loc += element_size_in_bytes;
        }
        // leave array location in memory on the stack
        wasm::emit_i32_const(m_code_section, m_al, cur_mem_loc);
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            return;
        }

        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(
            ASRUtils::symbol_get_past_external(x.m_name));

        for (size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i].m_value);
        }

        uint64_t hash = get_hash((ASR::asr_t *)fn);
        if (m_func_name_idx_map.find(hash) != m_func_name_idx_map.end()) {
            wasm::emit_call(m_code_section, m_al, m_func_name_idx_map[hash]->index);
        } else {
            if (strcmp(fn->m_name, "c_caimag") == 0) {
                LCOMPILERS_ASSERT(x.n_args == 1);
                wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f32]);
                wasm::emit_drop(m_code_section, m_al);
                wasm::emit_global_get(m_code_section, m_al, m_compiler_globals[tmp_reg_f32]);
            } else if (strcmp(fn->m_name, "c_zaimag") == 0) {
                wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
                wasm::emit_drop(m_code_section, m_al);
                wasm::emit_global_get(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
            } else {
                throw CodeGenError("FunctionCall: Function " + std::string(fn->m_name) + " not found");
            }
        }
    }

    void temp_value_set(ASR::expr_t* expr) {
        auto ttype = ASRUtils::expr_type(expr);
        auto kind = ASRUtils::extract_kind_from_ttype_t(ttype);
        GLOBAL_VAR global_var;
        switch (ttype->type) {
            case ASR::ttypeType::Integer: {
                switch (kind) {
                    case 4: global_var = tmp_reg_i32; break;
                    case 8: global_var = tmp_reg_i64; break;
                    default: throw CodeGenError(
                        "temp_value_set: Unsupported Integer kind");
                }
                break;
            }
            case ASR::ttypeType::Real: {
                switch (kind) {
                    case 4: global_var = tmp_reg_f32; break;
                    case 8: global_var = tmp_reg_f64; break;
                    default: throw CodeGenError(
                        "temp_value_set: Unsupported Real kind");
                }
                break;
            }
            case ASR::ttypeType::Logical: {
                switch (kind) {
                    case 4: global_var = tmp_reg_i32; break;
                    default: throw CodeGenError(
                        "temp_value_set: Unsupported Logical kind");
                }
                break;
            }
            case ASR::ttypeType::Character: {
                switch (kind) {
                    case 4: global_var = tmp_reg_i32; break;
                    case 8: global_var = tmp_reg_i64; break;
                    default: throw CodeGenError(
                        "temp_value_set: Unsupported Character kind");
                }
                break;
            }
            default: {
                throw CodeGenError("temp_value_set: Type " +
                                   ASRUtils::type_to_str(ttype) +
                                   " not yet supported");
            }
        }
        wasm::emit_global_set(m_code_section, m_al,
                            m_compiler_globals[global_var]);
    }

    void temp_value_get(ASR::expr_t* expr) {
        auto ttype = ASRUtils::expr_type(expr);
        auto kind = ASRUtils::extract_kind_from_ttype_t(ttype);
        GLOBAL_VAR global_var;
        switch (ttype->type) {
            case ASR::ttypeType::Integer: {
                switch (kind) {
                    case 4: global_var = tmp_reg_i32; break;
                    case 8: global_var = tmp_reg_i64; break;
                    default: throw CodeGenError(
                        "temp_value_get: Unsupported Integer kind");
                }
                break;
            }
            case ASR::ttypeType::Real: {
                switch (kind) {
                    case 4: global_var = tmp_reg_f32; break;
                    case 8: global_var = tmp_reg_f64; break;
                    default: throw CodeGenError(
                        "temp_value_get: Unsupported Real kind");
                }
                break;
            }
            case ASR::ttypeType::Logical: {
                switch (kind) {
                    case 4: global_var = tmp_reg_i32; break;
                    default: throw CodeGenError(
                        "temp_value_get: Unsupported Logical kind");
                }
                break;
            }
            case ASR::ttypeType::Character: {
                switch (kind) {
                    case 4: global_var = tmp_reg_i32; break;
                    case 8: global_var = tmp_reg_i64; break;
                    default: throw CodeGenError(
                        "temp_value_get: Unsupported Character kind");
                }
                break;
            }
            default: {
                throw CodeGenError("temp_value_get: Type " +
                                   ASRUtils::type_to_str(ttype) +
                                   " not yet supported");
            }
        }
        wasm::emit_global_get(m_code_section, m_al,
                            m_compiler_globals[global_var]);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
            ASRUtils::symbol_get_past_external(x.m_name));

        Vec<ASR::expr_t *> vars_passed_by_refs;
        vars_passed_by_refs.reserve(m_al, s->n_args);
        if (x.n_args == s->n_args) {
            for (size_t i = 0; i < x.n_args; i++) {
                ASR::Variable_t *arg = ASRUtils::EXPR2VAR(s->m_args[i]);
                if (arg->m_intent == ASRUtils::intent_out ||
                    arg->m_intent == ASRUtils::intent_inout ||
                    arg->m_intent == ASRUtils::intent_unspecified) {
                    vars_passed_by_refs.push_back(m_al, x.m_args[i].m_value);
                }
                visit_expr(*x.m_args[i].m_value);
            }
        } else {
            throw CodeGenError(
                "visitSubroutineCall: Number of arguments passed do not match "
                "the number of parameters");
        }

        uint64_t hash = get_hash((ASR::asr_t *)s);
        if (m_func_name_idx_map.find(hash) != m_func_name_idx_map.end()) {
            wasm::emit_call(m_code_section, m_al, m_func_name_idx_map[hash]->index);
        } else {
            throw CodeGenError("SubroutineCall: Function " + std::string(s->m_name) + " not found");
        }
        for (int i = (int)vars_passed_by_refs.size() - 1; i >= 0; i--) {
            ASR::expr_t* return_expr = vars_passed_by_refs[i];
            if (ASR::is_a<ASR::Var_t>(*return_expr)) {
                ASR::Variable_t* return_var = ASRUtils::EXPR2VAR(return_expr);
                emit_var_set(return_var);
            } else if (ASR::is_a<ASR::ArrayItem_t>(*return_expr)) {
                temp_value_set(return_expr);
                emit_array_item_address_onto_stack(*(ASR::down_cast<ASR::ArrayItem_t>(return_expr)));
                temp_value_get(return_expr);
                emit_memory_store(return_expr);
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    inline ASR::ttype_t *extract_ttype_t_from_expr(ASR::expr_t *expr) {
        return ASRUtils::expr_type(expr);
    }

    void extract_kinds(const ASR::Cast_t &x, int &arg_kind, int &dest_kind) {
        dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        ASR::ttype_t *curr_type = extract_ttype_t_from_expr(x.m_arg);
        LCOMPILERS_ASSERT(curr_type != nullptr)
        arg_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
    }

    void visit_Cast(const ASR::Cast_t &x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            return;
        }
        this->visit_expr(*x.m_arg);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 4) {
                        wasm::emit_f32_convert_i32_s(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 8) {
                        wasm::emit_f64_convert_i64_s(m_code_section, m_al);
                    } else if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_f64_convert_i32_s(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_f32_convert_i64_s(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::RealToInteger): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 4) {
                        wasm::emit_i32_trunc_f32_s(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 8) {
                        wasm::emit_i64_trunc_f64_s(m_code_section, m_al);
                    } else if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_i64_trunc_f32_s(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_i32_trunc_f64_s(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::RealToComplex): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind == dest_kind) {

                } else if (arg_kind == 4 && dest_kind == 8) {
                    wasm::emit_f64_promote_f32(m_code_section, m_al);
                } else if (arg_kind == 8 && dest_kind == 4) {
                    wasm::emit_f32_demote_f64(m_code_section, m_al);
                } else {
                    std::string msg = "RealToComplex: Conversion from " +
                                        std::to_string(arg_kind) + " to " +
                                        std::to_string(dest_kind) +
                                        " not implemented yet.";
                    throw CodeGenError(msg);
                }
                switch(dest_kind)
                {
                    case 4:
                        wasm::emit_f32_const(m_code_section, m_al, 0.0);
                        break;
                    case 8:
                        wasm::emit_f64_const(m_code_section, m_al, 0.0);
                        break;
                    default:
                        throw CodeGenError("RealToComplex: Only 32 and 64 bits real kinds are supported.");
                }
                break;
            }
            case (ASR::cast_kindType::IntegerToComplex): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 4) {
                        wasm::emit_f32_convert_i32_s(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 8) {
                        wasm::emit_f64_convert_i64_s(m_code_section, m_al);
                    } else if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_f64_convert_i32_s(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_f32_convert_i64_s(m_code_section, m_al);
                    } else {
                        std::string msg = "IntegerToComplex: Conversion from " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                switch(dest_kind)
                {
                    case 4:
                        wasm::emit_f32_const(m_code_section, m_al, 0.0);
                        break;
                    case 8:
                        wasm::emit_f64_const(m_code_section, m_al, 0.0);
                        break;
                    default:
                        throw CodeGenError("RealToComplex: Only 32 and 64 bits real kinds are supported.");
                }
                break;
            }
            case (ASR::cast_kindType::IntegerToLogical): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 4) {
                        wasm::emit_i32_eqz(m_code_section, m_al);
                        wasm::emit_i32_eqz(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_i64_eqz(m_code_section, m_al);
                        wasm::emit_i64_eqz(m_code_section, m_al);
                        wasm::emit_i32_wrap_i64(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from kinds " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not supported";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::RealToLogical): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 4) {
                        wasm::emit_f32_const(m_code_section, m_al, 0.0);
                        wasm::emit_f32_eq(m_code_section, m_al);
                        wasm::emit_i32_eqz(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_f64_const(m_code_section, m_al, 0.0);
                        wasm::emit_f64_eq(m_code_section, m_al);
                        wasm::emit_i64_eqz(m_code_section, m_al);
                        wasm::emit_i32_wrap_i64(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from kinds " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not supported";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::CharacterToLogical): {
                throw CodeGenError(R"""(STrings are not supported yet)""",
                                   x.base.base.loc);
                break;
            }
            case (ASR::cast_kindType::ComplexToLogical): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind == 4) {
                    if (m_rt_func_used_idx[abs_c32] == -1) {
                        m_rt_func_used_idx[abs_c32] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[abs_c32]);
                    wasm::emit_f32_const(m_code_section, m_al, 0.0);
                    wasm::emit_f32_gt(m_code_section, m_al);
                } else if (arg_kind == 8) {
                    if (m_rt_func_used_idx[abs_c64] == -1) {
                        m_rt_func_used_idx[abs_c64] = no_of_types++;
                    }
                    wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[abs_c64]);
                    wasm::emit_f64_const(m_code_section, m_al, 0.0);
                    wasm::emit_f64_gt(m_code_section, m_al);
                } else {
                    std::string msg = "ComplexToLogical: Conversion from kinds " +
                                        std::to_string(arg_kind) + " to " +
                                        std::to_string(dest_kind) +
                                        " not supported";
                    throw CodeGenError(msg);
                }
                break;
            }
            case (ASR::cast_kindType::LogicalToInteger): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_i64_extend_i32_s(m_code_section, m_al);
                    } else if (arg_kind == 4 && dest_kind == 4) {
                    } else {
                        std::string msg = "Conversion from kinds " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not supported";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::LogicalToReal): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 4) {
                        wasm::emit_f32_convert_i32_s(m_code_section, m_al);
                    } else if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_f64_convert_i32_s(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from kinds " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not supported";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::IntegerToInteger): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0 && arg_kind != dest_kind) {
                    if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_i64_extend_i32_s(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_i32_wrap_i64(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::RealToReal): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0 && arg_kind != dest_kind) {
                    if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_f64_promote_f32(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_f32_demote_f64(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::ComplexToComplex): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0 && arg_kind != dest_kind) {
                    if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_f64_promote_f32(m_code_section, m_al);
                        wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
                        wasm::emit_f64_promote_f32(m_code_section, m_al);
                        wasm::emit_global_get(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_f32_demote_f64(m_code_section, m_al);
                        wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f32]);
                        wasm::emit_f32_demote_f64(m_code_section, m_al);
                        wasm::emit_global_get(m_code_section, m_al, m_compiler_globals[tmp_reg_f32]);
                    } else {
                        std::string msg = "ComplexToComplex: Conversion from " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::ComplexToReal): {
                wasm::emit_drop(m_code_section, m_al); // drop imag part
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0 && arg_kind != dest_kind) {
                    if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_f64_promote_f32(m_code_section, m_al);
                    } else if (arg_kind == 8 && dest_kind == 4) {
                        wasm::emit_f32_demote_f64(m_code_section, m_al);
                    } else {
                        std::string msg = "ComplexToReal: Conversion from " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            default:
                throw CodeGenError("Cast kind not implemented");
        }
    }

    void visit_ComplexRe(const ASR::ComplexRe_t &x) {
        this->visit_expr(*x.m_arg);
        wasm::emit_drop(m_code_section, m_al);
    }

    void visit_ComplexIm(const ASR::ComplexIm_t &x) {
        this->visit_expr(*x.m_arg);

        int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x.m_arg));
        wasm::emit_global_set(m_code_section, m_al,
        (a_kind == 4) ? m_compiler_globals[tmp_reg_f32]
            : m_compiler_globals[tmp_reg_f64]);
        wasm::emit_drop(m_code_section, m_al);
        wasm::emit_global_get(m_code_section, m_al,
        (a_kind == 4) ? m_compiler_globals[tmp_reg_f32]
            : m_compiler_globals[tmp_reg_f64]);
    }

    void emit_call_fd_write(int filetype, const std::string &str, int iov_vec_len, int return_val_mem_loc) {
        wasm::emit_i32_const(m_code_section, m_al, filetype); // file type: 1 for stdout
        wasm::emit_i32_const(m_code_section, m_al, m_string_to_iov_loc_map[str]); // iov location
        wasm::emit_i32_const(m_code_section, m_al, iov_vec_len); // size of iov vector
        wasm::emit_i32_const(m_code_section, m_al, return_val_mem_loc); // mem_loction to return no. of bytes written
        // call WASI fd_write
        wasm::emit_call(m_code_section, m_al, m_import_func_idx_map[fd_write]);
        wasm::emit_drop(m_code_section, m_al);
    }

    template <typename T>
    void handle_print(const T &x) {
        for (size_t i = 0; i < x.n_values; i++) {
            if (i > 0) {
                if (x.m_separator) {
                    wasm::emit_i32_const(m_code_section, m_al, 1); // file type: 1 for stdout
                    this->visit_expr(*x.m_separator); // iov location
                    wasm::emit_i32_const(m_code_section, m_al, 1); // size of iov vector
                    wasm::emit_i32_const(m_code_section, m_al, 0); // mem_loction to return no. of bytes written

                    // call WASI fd_write
                    wasm::emit_call(m_code_section, m_al, m_import_func_idx_map[fd_write]);
                    wasm::emit_drop(m_code_section, m_al);
                } else {
                    emit_call_fd_write(1, " ", 1, 0);
                }
            }
            ASR::expr_t *v = x.m_values[i];
            ASR::ttype_t *t = ASRUtils::expr_type(v);
            int a_kind = ASRUtils::extract_kind_from_ttype_t(t);

            if (ASRUtils::is_integer(*t) || ASRUtils::is_logical(*t)) {
                if (m_rt_func_used_idx[print_i64] == -1) {
                    m_rt_func_used_idx[print_i64] = no_of_types++;
                }
                this->visit_expr(*x.m_values[i]);
                switch (a_kind) {
                    case 4: {
                        wasm::emit_i64_extend_i32_s(m_code_section, m_al);
                        wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_i64]);
                        break;
                    }
                    case 8: {
                        wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_i64]);
                        break;
                    }
                    default: {
                        throw CodeGenError(
                            R"""(Printing support is currently available only
                                            for 32, and 64 bit integer kinds.)""");
                    }
                }
            } else if (ASRUtils::is_real(*t)) {
                if (m_rt_func_used_idx[print_i64] == -1) {
                    m_rt_func_used_idx[print_i64] = no_of_types++;
                }
                if (m_rt_func_used_idx[print_f64] == -1) {
                    m_rt_func_used_idx[print_f64] = no_of_types++;
                }
                this->visit_expr(*x.m_values[i]);
                switch (a_kind) {
                    case 4: {
                        wasm::emit_f64_promote_f32(m_code_section, m_al);
                        wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_f64]);
                        break;
                    }
                    case 8: {
                        wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_f64]);
                        break;
                    }
                    default: {
                        throw CodeGenError(
                            R"""(Printing support is available only
                                            for 32, and 64 bit real kinds.)""");
                    }
                }
            } else if (t->type == ASR::ttypeType::Character) {
                wasm::emit_i32_const(m_code_section, m_al, 1); // file type: 1 for stdout
                this->visit_expr(*x.m_values[i]); // iov location
                wasm::emit_i32_const(m_code_section, m_al, 1); // size of iov vector
                wasm::emit_i32_const(m_code_section, m_al, 0); // mem_loction to return no. of bytes written

                // call WASI fd_write
                wasm::emit_call(m_code_section, m_al, m_import_func_idx_map[fd_write]);
                wasm::emit_drop(m_code_section, m_al);
            } else if (t->type == ASR::ttypeType::Complex) {
                if (m_rt_func_used_idx[print_i64] == -1) {
                    m_rt_func_used_idx[print_i64] = no_of_types++;
                }
                if (m_rt_func_used_idx[print_f64] == -1) {
                    m_rt_func_used_idx[print_f64] = no_of_types++;
                }
                emit_call_fd_write(1, "(", 1, 0);
                this->visit_expr(*x.m_values[i]);
                if (a_kind == 4) {
                    wasm::emit_f64_promote_f32(m_code_section, m_al);
                    wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
                    wasm::emit_f64_promote_f32(m_code_section, m_al);
                } else {
                    wasm::emit_global_set(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
                }
                wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_f64]);
                emit_call_fd_write(1, ",", 1, 0);
                wasm::emit_global_get(m_code_section, m_al, m_compiler_globals[tmp_reg_f64]);
                wasm::emit_call(m_code_section, m_al, m_rt_func_used_idx[print_f64]);
                emit_call_fd_write(1, ")", 1, 0);
            }
        }

        // print "\n" newline character
        if (x.m_end) {
            wasm::emit_i32_const(m_code_section, m_al, 1); // file type: 1 for stdout
            this->visit_expr(*x.m_end); // iov location
            wasm::emit_i32_const(m_code_section, m_al, 1); // size of iov vector
            wasm::emit_i32_const(m_code_section, m_al, 0); // mem_loction to return no. of bytes written

            // call WASI fd_write
            wasm::emit_call(m_code_section, m_al, m_import_func_idx_map[fd_write]);
            wasm::emit_drop(m_code_section, m_al);
        } else {
            emit_call_fd_write(1, "\n", 1, 0);
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        if (x.m_fmt != nullptr) {
            diag.codegen_warning_label(
                "format string in `print` is not implemented yet and it is "
                "currently treated as '*'",
                {x.m_fmt->base.loc}, "treated as '*'");
        }
        handle_print(x);
    }

    void visit_FileWrite(const ASR::FileWrite_t &x) {
        if (x.m_fmt != nullptr) {
            diag.codegen_warning_label(
                "format string in `print` is not implemented yet and it is "
                "currently treated as '*'",
                {x.m_fmt->base.loc}, "treated as '*'");
        }
        if (x.m_unit != nullptr) {
            diag.codegen_error_label("unit in write() is not implemented yet",
                                     {x.m_unit->base.loc}, "not implemented");
            throw CodeGenAbort();
        }
        handle_print(x);
    }

    void visit_FileRead(const ASR::FileRead_t &x) {
        if (x.m_fmt != nullptr) {
            diag.codegen_warning_label(
                "format string in read() is not implemented yet and it is "
                "currently treated as '*'",
                {x.m_fmt->base.loc}, "treated as '*'");
        }
        if (x.m_unit != nullptr) {
            diag.codegen_error_label("unit in read() is not implemented yet",
                                     {x.m_unit->base.loc}, "not implemented");
            throw CodeGenAbort();
        }
        diag.codegen_error_label(
            "The intrinsic function read() is not implemented yet in the LLVM "
            "backend",
            {x.base.base.loc}, "not implemented");
        throw CodeGenAbort();
    }

    void print_msg(std::string msg) {
        msg += "\n";
        emit_string(msg);
        emit_call_fd_write(1, msg, 1, 0);
    }

    void exit() {
        // exit_code would be on stack, so set this exit code using
        // proc_exit(). this exit code would be read by JavaScript glue code
        wasm::emit_call(m_code_section, m_al, m_import_func_idx_map[proc_exit]);
        wasm::emit_unreachable(m_code_section, m_al);  // raise trap/exception
    }

    void visit_ArrayBound(const ASR::ArrayBound_t& x) {
        ASR::ttype_t *ttype = ASRUtils::expr_type(x.m_v);
        uint32_t kind = ASRUtils::extract_kind_from_ttype_t(ttype);
        ASR::dimension_t *m_dims;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(ttype, m_dims);
        if (kind != 4) {
            throw CodeGenError("ArrayBound: Kind 4 only supported currently");
        }

        if (x.m_dim) {
            ASR::expr_t *val = ASRUtils::expr_value(x.m_dim);

            if (!ASR::is_a<ASR::IntegerConstant_t>(*val)) {
                throw CodeGenError("ArrayBound: Only constant dim values supported currently");
            }
            ASR::IntegerConstant_t *dimDir = ASR::down_cast<ASR::IntegerConstant_t>(val);
            if (x.m_bound == ASR::arrayboundType::LBound) {
                this->visit_expr(*m_dims[dimDir->m_n - 1].m_start);
            } else {
                this->visit_expr(*m_dims[dimDir->m_n - 1].m_start);
                this->visit_expr(*m_dims[dimDir->m_n - 1].m_length);
                wasm::emit_i32_add(m_code_section, m_al);
                wasm::emit_i32_const(m_code_section, m_al, 1);
                wasm::emit_i32_sub(m_code_section, m_al);
            }
        } else {
            if (x.m_bound == ASR::arrayboundType::LBound) {
                wasm::emit_i32_const(m_code_section, m_al, 1);
            } else {
                // emit the whole array size
                if (!m_dims[0].m_length) {
                    throw CodeGenError(
                        "ArrayBound: Dimension length for index 0 does not exist");
                }
                this->visit_expr(*(m_dims[0].m_length));
                for (int i = 1; i < n_dims; i++) {
                    this->visit_expr(*m_dims[i].m_length);
                    wasm::emit_i32_mul(m_code_section, m_al);
                }
            }
        }
    }

    void visit_Stop(const ASR::Stop_t &x) {
        print_msg("STOP");
        if (x.m_code &&
            ASRUtils::expr_type(x.m_code)->type == ASR::ttypeType::Integer) {
            this->visit_expr(*x.m_code);
        } else {
            wasm::emit_i32_const(m_code_section, m_al, 0);  // zero exit code
        }
        exit();
    }

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        print_msg("ERROR STOP");
        wasm::emit_i32_const(m_code_section, m_al, 1);  // non-zero exit code
        exit();
    }

    void visit_If(const ASR::If_t &x) {
        emit_if_else([&](){ this->visit_expr(*x.m_test); }, [&](){
            for (size_t i = 0; i < x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
        }, [&](){
            for (size_t i = 0; i < x.n_orelse; i++) {
                this->visit_stmt(*x.m_orelse[i]);
            }
        });
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        emit_loop([&](){ this->visit_expr(*x.m_test); }, [&](){
            for (size_t i = 0; i < x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
        });
    }

    void visit_Exit(const ASR::Exit_t & /* x */) {
        wasm::emit_branch(m_code_section, m_al,
                          nesting_level - cur_loop_nesting_level -
                              2U);  // branch to end of if
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
        wasm::emit_branch(
            m_code_section, m_al,
            nesting_level - cur_loop_nesting_level - 1U);  // branch to start of loop
    }

    void visit_Assert(const ASR::Assert_t &x) {
        this->visit_expr(*x.m_test);
        wasm::emit_b8(m_code_section, m_al, 0x04);  // emit if start
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type
        wasm::emit_b8(m_code_section, m_al, 0x05);  // starting of else
        if (x.m_msg) {
            std::string msg =
                ASR::down_cast<ASR::StringConstant_t>(x.m_msg)->m_s;
            print_msg("AssertionError: " + msg);
        } else {
            print_msg("AssertionError");
        }
        wasm::emit_i32_const(m_code_section, m_al, 1);  // non-zero exit code
        exit();
        wasm::emit_expr_end(m_code_section, m_al);  // emit if end
    }
};

Result<Vec<uint8_t>> asr_to_wasm_bytes_stream(ASR::TranslationUnit_t &asr,
                                              Allocator &al,
                                              diag::Diagnostics &diagnostics) {
    ASRToWASMVisitor v(al, diagnostics);
    Vec<uint8_t> wasm_bytes;

    LCompilers::PassOptions pass_options;
    pass_array_by_data(al, asr, pass_options);
    pass_replace_print_arr(al, asr, pass_options);
    pass_replace_do_loops(al, asr, pass_options);
    pass_replace_intrinsic_function(al, asr, pass_options);
    pass_options.always_run = true;
    pass_unused_functions(al, asr, pass_options);

#ifdef SHOW_ASR
    std::cout << LCompilers::LFortran::pickle(asr, false /* use colors */, true /* indent */,
                        true /* with_intrinsic_modules */)
              << std::endl;
#endif
    try {
        v.visit_asr((ASR::asr_t &)asr);
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    }

    v.get_wasm(wasm_bytes);

    return wasm_bytes;
}

Result<int> asr_to_wasm(ASR::TranslationUnit_t &asr, Allocator &al,
                        const std::string &filename, bool time_report,
                        diag::Diagnostics &diagnostics) {
    int time_visit_asr = 0;
    int time_save = 0;

    auto t1 = std::chrono::high_resolution_clock::now();
    Result<Vec<uint8_t>> wasm = asr_to_wasm_bytes_stream(asr, al, diagnostics);
    auto t2 = std::chrono::high_resolution_clock::now();
    time_visit_asr =
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    if (!wasm.ok) {
        return wasm.error;
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        wasm::save_bin(wasm.result, filename);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_save =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count();
    }

    if (time_report) {
        std::cout << "Codegen Time report:" << std::endl;
        std::cout << "ASR -> wasm: " << std::setw(5) << time_visit_asr
                  << std::endl;
        std::cout << "Save:       " << std::setw(5) << time_save << std::endl;
        int total = time_visit_asr + time_save;
        std::cout << "Total:      " << std::setw(5) << total << std::endl;
    }
    return 0;
}

}  // namespace LCompilers
