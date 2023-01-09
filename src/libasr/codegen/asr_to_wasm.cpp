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
#include <libasr/exception.h>
#include <libasr/asr_utils.h>

// #define SHOW_ASR

#ifdef SHOW_ASR
#include <lfortran/pickle.h>
#endif

namespace LFortran {

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

struct ImportFunc {
    std::string name;
    std::vector<std::pair<ASR::ttypeType, uint32_t>> param_types, result_types;
};

struct SymbolFuncInfo {
    bool needs_declaration = true;
    bool intrinsic_function = false;
    uint32_t index = 0;
    uint32_t no_of_variables = 0;
    ASR::Variable_t *return_var = nullptr;
    Vec<ASR::Variable_t *> referenced_vars;
};

class ASRToWASMVisitor : public ASR::BaseVisitor<ASRToWASMVisitor> {
   public:
    Allocator &m_al;
    diag::Diagnostics &diag;

    bool intrinsic_module;
    SymbolTable *global_scope;
    Location global_scope_loc;
    SymbolFuncInfo *cur_sym_info;
    uint32_t nesting_level;
    uint32_t cur_loop_nesting_level;
    bool is_prototype_only;

    Vec<uint8_t> m_type_section;
    Vec<uint8_t> m_import_section;
    Vec<uint8_t> m_func_section;
    Vec<uint8_t> m_export_section;
    Vec<uint8_t> m_code_section;
    Vec<uint8_t> m_data_section;

    uint32_t no_of_types;
    uint32_t no_of_functions;
    uint32_t no_of_imports;
    uint32_t no_of_data_segments;
    uint32_t avail_mem_loc;

    uint32_t min_no_pages;
    uint32_t max_no_pages;

    std::map<uint64_t, uint32_t> m_var_name_idx_map;
    std::map<uint64_t, SymbolFuncInfo *> m_func_name_idx_map;
    std::map<std::string, ASR::asr_t *> m_import_func_asr_map;

   public:
    ASRToWASMVisitor(Allocator &al, diag::Diagnostics &diagnostics)
        : m_al(al), diag(diagnostics) {
        intrinsic_module = false;
        is_prototype_only = false;
        nesting_level = 0;
        cur_loop_nesting_level = 0;
        no_of_types = 0;
        avail_mem_loc = 0;
        no_of_functions = 0;
        no_of_imports = 0;
        no_of_data_segments = 0;

        min_no_pages = 100;  // fixed 6.4 Mb memory currently
        max_no_pages = 100;  // fixed 6.4 Mb memory currently

        m_type_section.reserve(m_al, 1024 * 128);
        m_import_section.reserve(m_al, 1024 * 128);
        m_func_section.reserve(m_al, 1024 * 128);
        m_export_section.reserve(m_al, 1024 * 128);
        m_code_section.reserve(m_al, 1024 * 128);
        m_data_section.reserve(m_al, 1024 * 128);
    }

    void get_wasm(Vec<uint8_t> &code) {
        code.reserve(m_al, 8U /* preamble size */ +
                               8U /* (section id + section size) */ *
                                   6U /* number of sections */
                               + m_type_section.size() +
                               m_import_section.size() + m_func_section.size() +
                               m_export_section.size() + m_code_section.size() +
                               m_data_section.size());

        wasm::emit_header(code, m_al);  // emit header and version
        wasm::encode_section(
            code, m_type_section, m_al, 1U,
            no_of_types);  // no_of_types indicates total (imported + defined)
                           // no of functions
        wasm::encode_section(code, m_import_section, m_al, 2U, no_of_imports);
        wasm::encode_section(code, m_func_section, m_al, 3U, no_of_functions);
        wasm::encode_section(code, m_export_section, m_al, 7U, no_of_functions);
        wasm::encode_section(code, m_code_section, m_al, 10U, no_of_functions);
        wasm::encode_section(code, m_data_section, m_al, 11U,
                             no_of_data_segments);
    }

    ASR::asr_t *get_import_func_var_type(
        std::pair<ASR::ttypeType, uint32_t> &type) {
        switch (type.first) {
            case ASR::ttypeType::Integer:
                return ASR::make_Integer_t(m_al, global_scope_loc, type.second,
                                           nullptr, 0);
            case ASR::ttypeType::Real:
                return ASR::make_Real_t(m_al, global_scope_loc, type.second,
                                        nullptr, 0);
            default:
                throw CodeGenError("Unsupported Type in Import Function");
        }
        return nullptr;
    }

    void import_function(ImportFunc &import_func) {
        Vec<ASR::expr_t *> params;
        params.reserve(m_al, import_func.param_types.size());
        uint32_t var_idx;
        for (var_idx = 0; var_idx < import_func.param_types.size(); var_idx++) {
            auto param = import_func.param_types[var_idx];
            auto type = get_import_func_var_type(param);
            auto variable = ASR::make_Variable_t(
                m_al, global_scope_loc, nullptr,
                s2c(m_al, std::to_string(var_idx)), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default,
                ASRUtils::TYPE(type), ASR::abiType::Source,
                ASR::accessType::Public, ASR::presenceType::Required, false);
            auto var = ASR::make_Var_t(m_al, global_scope_loc,
                                       ASR::down_cast<ASR::symbol_t>(variable));
            params.push_back(m_al, ASRUtils::EXPR(var));
        }

        auto func = ASR::make_Function_t(
            m_al, global_scope_loc, global_scope, s2c(m_al, import_func.name),
            nullptr, 0, params.data(), params.size(), nullptr, 0, nullptr,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::deftypeType::Implementation, nullptr, false, false, false, false, false,
            nullptr, 0, nullptr, 0, false);
        m_import_func_asr_map[import_func.name] = func;

        wasm::emit_import_fn(m_import_section, m_al, "js", import_func.name,
                             no_of_types);
        emit_function_prototype(*((ASR::Function_t *)func));
        no_of_imports++;
    }

    void emit_imports() {
        std::vector<ImportFunc> import_funcs = {
            {"print_i32", {{ASR::ttypeType::Integer, 4}}, {}},
            {"print_i64", {{ASR::ttypeType::Integer, 8}}, {}},
            {"print_f32", {{ASR::ttypeType::Real, 4}}, {}},
            {"print_f64", {{ASR::ttypeType::Real, 8}}, {}},
            {"print_str",
             {{ASR::ttypeType::Integer, 4}, {ASR::ttypeType::Integer, 4}},
             {}},
            {"flush_buf", {}, {}},
            {"set_exit_code", {{ASR::ttypeType::Integer, 4}}, {}}};


        for (auto ImportFunc : import_funcs) {
            import_function(ImportFunc);
        }

        // In WASM: The indices of the imports precede the indices of other
        // definitions in the same index space. Therefore, declare the import
        // functions before defined functions
        for (auto &item : global_scope->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                ASR::Program_t *p = ASR::down_cast<ASR::Program_t>(item.second);
                for (auto &item : p->m_symtab->get_scope()) {
                    if (ASR::is_a<ASR::Function_t>(*item.second)) {
                        ASR::Function_t *fn =
                            ASR::down_cast<ASR::Function_t>(item.second);
                        if (fn->m_abi == ASR::abiType::BindC &&
                            fn->m_deftype == ASR::deftypeType::Interface &&
                            !ASRUtils::is_intrinsic_function2(fn)) {
                            wasm::emit_import_fn(m_import_section, m_al, "js",
                                                 fn->m_name, no_of_types);
                            no_of_imports++;
                            emit_function_prototype(*fn);
                        }
                    }
                }
            } else if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *fn =
                    ASR::down_cast<ASR::Function_t>(item.second);
                if (fn->m_abi == ASR::abiType::BindC &&
                    fn->m_deftype == ASR::deftypeType::Interface &&
                    !ASRUtils::is_intrinsic_function2(fn)) {
                    wasm::emit_import_fn(m_import_section, m_al, "js",
                                            fn->m_name, no_of_types);
                    no_of_imports++;
                    emit_function_prototype(*fn);
                }
            }
        }

        wasm::emit_import_mem(m_import_section, m_al, "js", "memory",
                              min_no_pages, max_no_pages);
        no_of_imports++;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);

        global_scope = x.m_global_scope;
        global_scope_loc = x.base.base.loc;

        emit_imports();

        {
            // Pre-declare all functions first, then generate code
            // Otherwise some function might not be found.
            is_prototype_only = true;
            {
                // Process intrinsic modules in the right order
                std::vector<std::string> build_order =
                    ASRUtils::determine_module_dependencies(x);
                for (auto &item : build_order) {
                    LFORTRAN_ASSERT(x.m_global_scope->get_scope().find(item) !=
                                    x.m_global_scope->get_scope().end());
                    ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                    if (ASR::is_a<ASR::Module_t>(*mod)) {
                        ASR::Module_t *m =
                            ASR::down_cast<ASR::Module_t>(mod);
                        declare_all_functions(*(m->m_symtab));
                    }
                }

                // Process procedures first:
                declare_all_functions(*x.m_global_scope);

                // then the main program:
                for (auto &item : x.m_global_scope->get_scope()) {
                    if (ASR::is_a<ASR::Program_t>(*item.second)) {
                        ASR::Program_t *p =
                            ASR::down_cast<ASR::Program_t>(item.second);
                        declare_all_functions(*(p->m_symtab));
                    }
                }
            }
            is_prototype_only = false;
        }

        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order =
                ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LFORTRAN_ASSERT(x.m_global_scope->get_scope().find(item) !=
                                x.m_global_scope->get_scope().end());
                ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                this->visit_symbol(*mod);
            }
        }

        // Process procedures first:
        declare_all_functions(*x.m_global_scope);

        // // Then do all the modules in the right order
        // std::vector<std::string> build_order
        //     = LFortran::ASRUtils::determine_module_dependencies(x);
        // for (auto &item : build_order) {
        //     LFORTRAN_ASSERT(x.m_global_scope->get_scope().find(item)
        //         != x.m_global_scope->get_scope().end());
        //     if (!startswith(item, "lfortran_intrinsic")) {
        //         ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
        //         visit_symbol(*mod);
        //         // std::cout << "I am here -2: " << src << std::endl;
        //     }
        // }

        // then the main program:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }
    }

    void declare_all_functions(const SymbolTable &symtab) {
        for (auto &item : symtab.get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s =
                    ASR::down_cast<ASR::Function_t>(item.second);
                this->visit_Function(*s);
            }
        }
    }

    void visit_Module(const ASR::Module_t &x) {
        if (startswith(x.m_name, "lfortran_intrinsic_")) {
            intrinsic_module = true;
        } else {
            intrinsic_module = false;
        }

        // Generate the bodies of functions and subroutines
        declare_all_functions(*x.m_symtab);
        intrinsic_module = false;
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate the bodies of functions and subroutines
        declare_all_functions(*x.m_symtab);

        // Generate main program code
        auto main_func = ASR::make_Function_t(
            m_al, x.base.base.loc, x.m_symtab, s2c(m_al, "_lcompilers_main"),
            nullptr, 0, nullptr, 0, x.m_body, x.n_body, nullptr,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::deftypeType::Implementation, nullptr, false, false, false, false, false,
            nullptr, 0, nullptr, 0, false);
        emit_function_prototype(*((ASR::Function_t *)main_func));
        emit_function_body(*((ASR::Function_t *)main_func));
    }

    void emit_var_type(Vec<uint8_t> &code, ASR::Variable_t *v) {
        // bool use_ref = (v->m_intent == LFortran::ASRUtils::intent_out ||
        //                 v->m_intent == LFortran::ASRUtils::intent_inout);
        bool is_array = ASRUtils::is_array(v->m_type);

        if (ASRUtils::is_pointer(v->m_type)) {
            ASR::ttype_t *t2 =
                ASR::down_cast<ASR::Pointer_t>(v->m_type)->m_type;
            if (ASRUtils::is_integer(*t2)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(t2);
                // size_t size;
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
            } else {
                // throw CodeGenError("Param, Result, Var Types other than
                // integer, floating point and logical not yet supported");
                diag.codegen_warning_label("Unsupported variable type: " +
                                               ASRUtils::type_to_str(v->m_type),
                                           {v->base.base.loc}, "here");
            }
        }
    }

    template <typename T>
    void emit_local_vars(const T &x,
                         int var_idx /* starting index for local vars */) {
        /********************* Local Vars Types List *********************/
        uint32_t len_idx_code_section_local_vars_list =
            wasm::emit_len_placeholder(m_code_section, m_al);
        int local_vars_cnt = 0;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v =
                    ASR::down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == ASRUtils::intent_local ||
                    v->m_intent == ASRUtils::intent_return_var) {
                    wasm::emit_u32(m_code_section, m_al,
                                   1U);  // count of local vars of this type
                    emit_var_type(m_code_section,
                                  v);  // emit the type of this var
                    m_var_name_idx_map[get_hash((ASR::asr_t *)v)] = var_idx++;
                    local_vars_cnt++;
                }
            }
        }
        // fixup length of local vars list
        wasm::emit_u32_b32_idx(m_code_section, m_al,
                               len_idx_code_section_local_vars_list,
                               local_vars_cnt);

        // initialize the value for local variables if initialization exists
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v =
                    ASR::down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == ASRUtils::intent_local ||
                    v->m_intent == ASRUtils::intent_return_var) {
                    if (v->m_symbolic_value) {
                        this->visit_expr(*v->m_symbolic_value);
                        // Todo: Checking for Array is currently omitted
                        LFORTRAN_ASSERT(m_var_name_idx_map.find(
                                            get_hash((ASR::asr_t *)v)) !=
                                        m_var_name_idx_map.end())
                        wasm::emit_set_local(
                            m_code_section, m_al,
                            m_var_name_idx_map[get_hash((ASR::asr_t *)v)]);
                    } else if (ASRUtils::is_array(v->m_type)) {
                        uint32_t kind =
                            ASRUtils::extract_kind_from_ttype_t(v->m_type);

                        Vec<uint32_t> array_dims;
                        get_array_dims(*v, array_dims);

                        uint32_t total_array_size = 1;
                        for (auto &dim : array_dims) {
                            total_array_size *= dim;
                        }

                        LFORTRAN_ASSERT(m_var_name_idx_map.find(
                                            get_hash((ASR::asr_t *)v)) !=
                                        m_var_name_idx_map.end());
                        wasm::emit_i32_const(m_code_section, m_al,
                                             avail_mem_loc);
                        wasm::emit_set_local(
                            m_code_section, m_al,
                            m_var_name_idx_map[get_hash((ASR::asr_t *)v)]);
                        avail_mem_loc += kind * total_array_size;
                    }
                }
            }
        }
    }

    void emit_function_prototype(const ASR::Function_t &x) {
        SymbolFuncInfo *s = new SymbolFuncInfo;

        /********************* New Type Declaration *********************/
        wasm::emit_b8(m_type_section, m_al, 0x60);

        /********************* Parameter Types List *********************/
        s->referenced_vars.reserve(m_al, x.n_args);
        wasm::emit_u32(m_type_section, m_al, x.n_args);
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::Variable_t *arg = ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(ASRUtils::is_arg_dummy(arg->m_intent));
            emit_var_type(m_type_section, arg);
            m_var_name_idx_map[get_hash((ASR::asr_t *)arg)] =
                s->no_of_variables++;
            if (arg->m_intent == ASR::intentType::Out ||
                arg->m_intent == ASR::intentType::InOut ||
                arg->m_intent == ASR::intentType::Unspecified) {
                s->referenced_vars.push_back(m_al, arg);
            }
        }

        /********************* Result Types List *********************/
        if (x.m_return_var) {  // It is a function
            wasm::emit_u32(m_type_section, m_al,
                           1U);  // there is just one return variable
            s->return_var = ASRUtils::EXPR2VAR(x.m_return_var);
            emit_var_type(m_type_section, s->return_var);
        } else {  // It is a subroutine
            uint32_t len_idx_type_section_return_types_list =
                wasm::emit_len_placeholder(m_type_section, m_al);
            for (size_t i = 0; i < x.n_args; i++) {
                ASR::Variable_t *arg = ASRUtils::EXPR2VAR(x.m_args[i]);
                if (arg->m_intent == ASR::intentType::Out ||
                    arg->m_intent == ASR::intentType::InOut ||
                    arg->m_intent == ASR::intentType::Unspecified) {
                    emit_var_type(m_type_section, arg);
                }
            }
            wasm::fixup_len(m_type_section, m_al,
                            len_idx_type_section_return_types_list);
        }

        /********************* Add Type to Map *********************/
        s->index = no_of_types++;
        m_func_name_idx_map[get_hash((ASR::asr_t *)&x)] =
            s;  // add function to map
    }

    void emit_function_body(const ASR::Function_t &x) {
        LFORTRAN_ASSERT(m_func_name_idx_map.find(get_hash((ASR::asr_t *)&x)) !=
                        m_func_name_idx_map.end());

        cur_sym_info = m_func_name_idx_map[get_hash((ASR::asr_t *)&x)];

        /********************* Reference Function Prototype
         * *********************/
        wasm::emit_u32(m_func_section, m_al, cur_sym_info->index);

        /********************* Function Body Starts Here *********************/
        uint32_t len_idx_code_section_func_size =
            wasm::emit_len_placeholder(m_code_section, m_al);

        emit_local_vars(x, cur_sym_info->no_of_variables);
        for (size_t i = 0; i < x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        if (strcmp(x.m_name, "_lcompilers_main") == 0) {
            wasm::emit_i32_const(m_code_section, m_al, 0 /* zero exit code */);
            wasm::emit_call(
                m_code_section, m_al,
                m_func_name_idx_map[get_hash(
                                        m_import_func_asr_map["set_exit_code"])]
                    ->index);
        }
        if ((x.n_body == 0) ||
            ((x.n_body > 0) &&
             !ASR::is_a<ASR::Return_t>(*x.m_body[x.n_body - 1]))) {
            handle_return();
        }
        wasm::emit_expr_end(m_code_section, m_al);

        wasm::fixup_len(m_code_section, m_al, len_idx_code_section_func_size);

        /********************* Export the function *********************/
        wasm::emit_export_fn(m_export_section, m_al, x.m_name,
                             cur_sym_info->index);  //  add function to export
        no_of_functions++;
    }

    bool is_unsupported_function(const ASR::Function_t &x) {
        if (!x.n_body) {
            return true;
        }
        if (x.m_abi == ASR::abiType::BindC &&
            x.m_deftype == ASR::deftypeType::Interface) {
            if (ASRUtils::is_intrinsic_function2(&x)) {
                diag.codegen_warning_label(
                    "WASM: C Intrinsic Functions not yet spported",
                    {x.base.base.loc}, std::string(x.m_name));
            }
            return true;
        }
        for (size_t i = 0; i < x.n_body; i++) {
            if (x.m_body[i]->type == ASR::stmtType::SubroutineCall) {
                auto sub_call = (const ASR::SubroutineCall_t &)(*x.m_body[i]);
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
                    ASRUtils::symbol_get_past_external(sub_call.m_name));
                if (s->m_abi == ASR::abiType::BindC &&
                    s->m_deftype == ASR::deftypeType::Interface &&
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
            LFORTRAN_ASSERT(
                m_var_name_idx_map.find(get_hash((ASR::asr_t *)asr_target)) !=
                m_var_name_idx_map.end());
            wasm::emit_set_local(
                m_code_section, m_al,
                m_var_name_idx_map[get_hash((ASR::asr_t *)asr_target)]);
        } else if (ASR::is_a<ASR::ArrayItem_t>(*x.m_target)) {
            emit_array_item_address_onto_stack(
                *(ASR::down_cast<ASR::ArrayItem_t>(x.m_target)));
            this->visit_expr(*x.m_value);
            emit_memory_store(x.m_value);
        } else {
            LFORTRAN_ASSERT(false)
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
                default: {
                    throw CodeGenError(
                        "ICE IntegerBinop kind 8: unknown operation");
                }
            }
        } else {
            throw CodeGenError("IntegerBinop: Integer kind not supported");
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
            case ASR::ttypeType::Character: {
                LFORTRAN_ASSERT(
                    m_var_name_idx_map.find(get_hash((ASR::asr_t *)v)) !=
                    m_var_name_idx_map.end());
                wasm::emit_get_local(
                    m_code_section, m_al,
                    m_var_name_idx_map[get_hash((ASR::asr_t *)v)]);
                break;
            }

            default:
                throw CodeGenError(
                    "Only Integer and Float Variable types currently "
                    "supported");
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

    // following function is useful for printing debug statements from
    // webassembly
    void print_wasm_debug_statement(std::string message, bool endline = true) {
        static int debug_mem_space = 10000 + avail_mem_loc;
        uint32_t avail_mem_loc_copy = avail_mem_loc;
        avail_mem_loc = debug_mem_space;
        emit_string(message);
        avail_mem_loc = avail_mem_loc_copy;  // restore avail_mem_loc
        // push string length on function stack
        wasm::emit_i32_const(m_code_section, m_al, message.length());

        // call JavaScript print_str
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(m_import_func_asr_map["print_str"])]
                ->index);

        if (endline) {
            // call JavaScript flush_buf
            wasm::emit_call(
                m_code_section, m_al,
                m_func_name_idx_map[get_hash(
                                        m_import_func_asr_map["flush_buf"])]
                    ->index);
        }
    }

    // following function is useful for debugging webassembly memory
    // it prints the value present at a given location in memory
    void print_mem_loc_value(uint32_t mem_loc) {
        print_wasm_debug_statement("Memory Location =", false);
        wasm::emit_i32_const(m_code_section, m_al, mem_loc);
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(m_import_func_asr_map["print_i32"])]
                ->index);
        print_wasm_debug_statement(", value=", false);
        wasm::emit_i32_const(m_code_section, m_al, mem_loc);
        wasm::emit_i32_load(m_code_section, m_al, wasm::mem_align::b8, 0);
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(m_import_func_asr_map["print_i32"])]
                ->index);
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(m_import_func_asr_map["flush_buf"])]
                ->index);
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
            LFORTRAN_ASSERT(m_var_name_idx_map.find(get_hash(
                                (ASR::asr_t *)cur_sym_info->return_var)) !=
                            m_var_name_idx_map.end());
            wasm::emit_get_local(m_code_section, m_al,
                                 m_var_name_idx_map[get_hash(
                                     (ASR::asr_t *)cur_sym_info->return_var)]);
        } else {
            for (auto return_var : cur_sym_info->referenced_vars) {
                wasm::emit_get_local(
                    m_code_section, m_al,
                    m_var_name_idx_map[get_hash((ASR::asr_t *)(return_var))]);
            }
        }
        wasm::emit_b8(m_code_section, m_al,
                      0x0F);  // emit wasm return instruction
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

    void emit_string(std::string str) {
        // Todo: Add a check here if there is memory available to store the
        // given string
        wasm::emit_str_const(m_data_section, m_al, avail_mem_loc, str);
        // Add string location in memory onto stack
        wasm::emit_i32_const(m_code_section, m_al, avail_mem_loc);
        avail_mem_loc += str.length();
        no_of_data_segments++;
    }

    void visit_StringConstant(const ASR::StringConstant_t &x) {
        emit_string(x.m_s);
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

        LFORTRAN_ASSERT(m_func_name_idx_map.find(get_hash((ASR::asr_t *)fn)) !=
                        m_func_name_idx_map.end())
        wasm::emit_call(m_code_section, m_al,
                        m_func_name_idx_map[get_hash((ASR::asr_t *)fn)]->index);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
            ASRUtils::symbol_get_past_external(x.m_name));
        // TODO: use a mapping with a hash(s) instead:
        // std::string sym_name = s->m_name;
        // if (sym_name == "exit") {
        //     sym_name = "_xx_lcompilers_changed_exit_xx";
        // }

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

        LFORTRAN_ASSERT(m_func_name_idx_map.find(get_hash((ASR::asr_t *)s)) !=
                        m_func_name_idx_map.end())
        wasm::emit_call(m_code_section, m_al,
                        m_func_name_idx_map[get_hash((ASR::asr_t *)s)]->index);
        for (auto return_expr : vars_passed_by_refs) {
            if (ASR::is_a<ASR::Var_t>(*return_expr)) {
                auto return_var = ASRUtils::EXPR2VAR(return_expr);
                LFORTRAN_ASSERT(
                m_var_name_idx_map.find(get_hash((ASR::asr_t *)return_var)) !=
                m_var_name_idx_map.end());
                wasm::emit_set_local(
                    m_code_section, m_al,
                    m_var_name_idx_map[get_hash((ASR::asr_t *)return_var)]);
            } else if (ASR::is_a<ASR::ArrayItem_t>(*return_expr)) {
                // emit_memory_store(ASRUtils::EXPR(return_var));

                throw CodeGenError(
                    "Passing array elements as arguments (with intent out, "
                    "inout, unspecified) to Subroutines is not yet supported");
            } else {
                LFORTRAN_ASSERT(false);
            }
        }
    }

    inline ASR::ttype_t *extract_ttype_t_from_expr(ASR::expr_t *expr) {
        return ASRUtils::expr_type(expr);
    }

    void extract_kinds(const ASR::Cast_t &x, int &arg_kind, int &dest_kind) {
        dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        ASR::ttype_t *curr_type = extract_ttype_t_from_expr(x.m_arg);
        LFORTRAN_ASSERT(curr_type != nullptr)
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
                throw CodeGenError("Complex types are not supported yet.");
                break;
            }
            case (ASR::cast_kindType::IntegerToComplex): {
                throw CodeGenError("Complex types are not supported yet.");
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
                throw CodeGenError("Complex types are not supported yet.");
                break;
            }
            case (ASR::cast_kindType::LogicalToInteger): {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if (arg_kind > 0 && dest_kind > 0) {
                    if (arg_kind == 4 && dest_kind == 8) {
                        wasm::emit_i64_extend_i32_s(m_code_section, m_al);
                    } else {
                        std::string msg = "Conversion from kinds " +
                                          std::to_string(arg_kind) + " to " +
                                          std::to_string(dest_kind) +
                                          " not supported";
                        throw CodeGenError(msg);
                    }
                }
                break;
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
                throw CodeGenError("Complex types are not supported yet.");
                break;
            }
            case (ASR::cast_kindType::ComplexToReal): {
                throw CodeGenError("Complex types are not supported yet.");
                break;
            }
            default:
                throw CodeGenError("Cast kind not implemented");
        }
    }

    template <typename T>
    void handle_print(const T &x) {
        for (size_t i = 0; i < x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            ASR::expr_t *v = x.m_values[i];
            ASR::ttype_t *t = ASRUtils::expr_type(v);
            int a_kind = ASRUtils::extract_kind_from_ttype_t(t);

            if (ASRUtils::is_integer(*t) || ASRUtils::is_logical(*t)) {
                switch (a_kind) {
                    case 4: {
                        // the value is already on stack. call JavaScript
                        // print_i32
                        wasm::emit_call(
                            m_code_section, m_al,
                            m_func_name_idx_map
                                [get_hash(m_import_func_asr_map["print_i32"])]
                                    ->index);
                        break;
                    }
                    case 8: {
                        // the value is already on stack. call JavaScript
                        // print_i64
                        wasm::emit_call(
                            m_code_section, m_al,
                            m_func_name_idx_map
                                [get_hash(m_import_func_asr_map["print_i64"])]
                                    ->index);
                        break;
                    }
                    default: {
                        throw CodeGenError(
                            R"""(Printing support is currently available only
                                            for 32, and 64 bit integer kinds.)""");
                    }
                }
            } else if (ASRUtils::is_real(*t)) {
                switch (a_kind) {
                    case 4: {
                        // the value is already on stack. call JavaScript
                        // print_f32
                        wasm::emit_call(
                            m_code_section, m_al,
                            m_func_name_idx_map
                                [get_hash(m_import_func_asr_map["print_f32"])]
                                    ->index);
                        break;
                    }
                    case 8: {
                        // the value is already on stack. call JavaScript
                        // print_f64
                        wasm::emit_call(
                            m_code_section, m_al,
                            m_func_name_idx_map
                                [get_hash(m_import_func_asr_map["print_f64"])]
                                    ->index);
                        break;
                    }
                    default: {
                        throw CodeGenError(
                            R"""(Printing support is available only
                                            for 32, and 64 bit real kinds.)""");
                    }
                }
            } else if (t->type == ASR::ttypeType::Character) {
                // the string location is already on function stack

                // now, push the string length onto stack
                wasm::emit_i32_const(
                    m_code_section, m_al,
                    ASR::down_cast<ASR::Character_t>(t)->m_len);

                // call JavaScript print_str
                wasm::emit_call(
                    m_code_section, m_al,
                    m_func_name_idx_map[get_hash(
                                            m_import_func_asr_map["print_str"])]
                        ->index);
            }
        }

        // call JavaScript flush_buf
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(m_import_func_asr_map["flush_buf"])]
                ->index);
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
        emit_string(msg);
        // push string length on function stack
        wasm::emit_i32_const(m_code_section, m_al, msg.length());
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(m_import_func_asr_map["print_str"])]
                ->index);
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(m_import_func_asr_map["flush_buf"])]
                ->index);
    }

    void exit() {
        // exit_code would be on stack, so set this exit code using
        // set_exit_code(). this exit code would be read by JavaScript glue code
        wasm::emit_call(
            m_code_section, m_al,
            m_func_name_idx_map[get_hash(
                                    m_import_func_asr_map["set_exit_code"])]
                ->index);
        wasm::emit_unreachable(m_code_section, m_al);  // raise trap/exception
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
        this->visit_expr(*x.m_test);
        wasm::emit_b8(m_code_section, m_al, 0x04);  // emit if start
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type
        nesting_level++;
        for (size_t i = 0; i < x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        wasm::emit_b8(m_code_section, m_al, 0x05);  // starting of else
        for (size_t i = 0; i < x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
        }
        nesting_level--;
        wasm::emit_expr_end(m_code_section, m_al);  // emit if end
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        uint32_t prev_cur_loop_nesting_level = cur_loop_nesting_level;
        cur_loop_nesting_level = nesting_level;

        wasm::emit_b8(m_code_section, m_al, 0x03);  // emit loop start
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type

        nesting_level++;

        this->visit_expr(*x.m_test);  // emit test condition

        wasm::emit_b8(m_code_section, m_al, 0x04);  // emit if
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type

        for (size_t i = 0; i < x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        // From WebAssembly Docs:
        // Unlike with other index spaces, indexing of labels is relative by
        // nesting depth, that is, label 0 refers to the innermost structured
        // control instruction enclosing the referring branch instruction, while
        // increasing indices refer to those farther out.

        wasm::emit_branch(
            m_code_section, m_al,
            nesting_level -
                cur_loop_nesting_level);  // emit_branch and label the loop
        wasm::emit_b8(m_code_section, m_al, 0x05);  // starting of else
        wasm::emit_expr_end(m_code_section, m_al);  // end if

        nesting_level--;
        wasm::emit_expr_end(m_code_section, m_al);  // end loop
        cur_loop_nesting_level = prev_cur_loop_nesting_level;
    }

    void visit_Exit(const ASR::Exit_t & /* x */) {
        wasm::emit_branch(m_code_section, m_al,
                          nesting_level - cur_loop_nesting_level -
                              1U);  // branch to end of if
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
        wasm::emit_branch(
            m_code_section, m_al,
            nesting_level - cur_loop_nesting_level);  // branch to start of loop
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
    pass_replace_do_loops(al, asr, pass_options);
    pass_array_by_data(al, asr, pass_options);
    pass_options.always_run = true;
    pass_unused_functions(al, asr, pass_options);

#ifdef SHOW_ASR
    std::cout << pickle(asr, true /* use colors */, true /* indent */,
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

}  // namespace LFortran
