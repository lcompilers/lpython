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
#include <libasr/pass/global_stmts.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>

namespace LCompilers {

namespace {

    // Local exception that is only used in this file to exit the visitor
    // pattern and caught later (not propagated outside)
    class CodeGenError {
    public:
        diag::Diagnostic d;

    public:
        CodeGenError(const std::string &msg) : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen)} {}
    };

}  // namespace

class ASRToWASMVisitor : public ASR::BaseVisitor<ASRToWASMVisitor> {
   public:
    Allocator &m_al;
    ASR::Variable_t *return_var;

    Vec<uint8_t> m_preamble;
    Vec<uint8_t> m_type_section;
    Vec<uint8_t> m_func_section;
    Vec<uint8_t> m_export_section;
    Vec<uint8_t> m_code_section;

    uint32_t cur_func_idx;
    std::map<std::string, int32_t> m_var_name_idx_map;
    std::map<std::string, int32_t> m_func_name_idx_map;

   public:
    ASRToWASMVisitor(Allocator &al) : m_al{al} {
        cur_func_idx = 0;
        m_preamble.reserve(m_al, 1024 * 128);
        m_type_section.reserve(m_al, 1024 * 128);
        m_func_section.reserve(m_al, 1024 * 128);
        m_export_section.reserve(m_al, 1024 * 128);
        m_code_section.reserve(m_al, 1024 * 128);
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // the main program:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        wasm::emit_header(m_preamble, m_al); // emit header and version

        uint32_t len_idx_type_section = wasm::emit_len_placeholder(m_type_section, m_al);
        uint32_t len_idx_func_section = wasm::emit_len_placeholder(m_func_section, m_al);
        uint32_t len_idx_export_section = wasm::emit_len_placeholder(m_export_section, m_al);
        uint32_t len_idx_code_section = wasm::emit_len_placeholder(m_code_section, m_al);

        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                throw CodeGenError("Sub Routine not yet supported");
            }
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }

        wasm::emit_u32_b32_idx(m_type_section, len_idx_type_section, cur_func_idx);
        wasm::emit_u32_b32_idx(m_func_section, len_idx_func_section, cur_func_idx);
        wasm::emit_u32_b32_idx(m_export_section, len_idx_export_section, cur_func_idx);
        wasm::emit_u32_b32_idx(m_code_section, len_idx_code_section, cur_func_idx);
    }

    void visit_Function(const ASR::Function_t &x) {
        wasm::emit_b8(m_type_section, m_al, 0x60);  // type section

        uint32_t len_idx_type_section_param_types_list = wasm::emit_len_placeholder(m_type_section, m_al);
        int curIdx = 0;
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::Variable_t *arg = LCompilers::ASRUtils::EXPR2VAR(x.m_args[i]);
            LCOMPILERS_ASSERT(LCompilers::ASRUtils::is_arg_dummy(arg->m_intent));
            if (ASR::is_a<ASR::Integer_t>(*arg->m_type)) {
                // checking for array is currently omitted
                bool is_int32 = ASR::down_cast<ASR::Integer_t>(arg->m_type)->m_kind == 4;
                if (is_int32) {
                    wasm::emit_b8(m_type_section, m_al, 0x7F);  // i32
                } else {
                    wasm::emit_b8(m_type_section, m_al, 0x7E);  // i64
                }
                m_var_name_idx_map[arg->m_name] = curIdx++;
            } else {
                throw CodeGenError("Parameters other than integer not yet supported");
            }
        }
        wasm::fixup_len(m_type_section, len_idx_type_section_param_types_list);

        uint32_t len_idx_type_section_return_types_list = wasm::emit_len_placeholder(m_type_section, m_al);
        return_var = LCompilers::ASRUtils::EXPR2VAR(x.m_return_var);
        if (ASRUtils::is_integer(*return_var->m_type)) {
            bool is_int32 = ASR::down_cast<ASR::Integer_t>(return_var->m_type)->m_kind == 4;
            if (is_int32) {
                wasm::emit_b8(m_type_section, m_al, 0x7F);  // i32
            } else {
                wasm::emit_b8(m_type_section, m_al, 0x7E);  // i64
            }
        } else {
            throw CodeGenError("Return type not supported");
        }
        wasm::fixup_len(m_type_section, len_idx_type_section_return_types_list);

        uint32_t len_idx_code_section_func_size = wasm::emit_len_placeholder(m_code_section, m_al);
        uint32_t len_idx_code_section_local_vars_list = wasm::emit_len_placeholder(m_code_section, m_al);
        int local_vars_cnt = 0;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == LCompilers::ASRUtils::intent_local || v->m_intent == LCompilers::ASRUtils::intent_return_var) {
                    if (ASR::is_a<ASR::Integer_t>(*v->m_type)) {
                        wasm::emit_u32(m_code_section, m_al, 1);    // count of local vars of this type
                        bool is_int32 = ASR::down_cast<ASR::Integer_t>(v->m_type)->m_kind == 4;
                        if (is_int32) {
                            wasm::emit_b8(m_code_section, m_al, 0x7F);  // i32
                        } else {
                            wasm::emit_b8(m_code_section, m_al, 0x7E);  // i64
                        }
                        m_var_name_idx_map[v->m_name] = curIdx++;
                        local_vars_cnt++;
                    } else {
                        throw CodeGenError("Variables other than integer not yet supported");
                    }
                }
            }
        }
        // fixup length of local vars list
        wasm::emit_u32_b32_idx(m_code_section, len_idx_code_section_local_vars_list, local_vars_cnt);

        for (size_t i = 0; i < x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        wasm::emit_expr_end(m_code_section, m_al);
        wasm::fixup_len(m_code_section, len_idx_code_section_func_size);

        wasm::emit_u32(m_func_section, m_al, cur_func_idx);

        wasm::emit_export_fn(m_export_section, m_al, x.m_name, cur_func_idx);
        m_func_name_idx_map[x.m_name] = cur_func_idx++;
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        this->visit_expr(*x.m_value);
        // this->visit_expr(*x.m_target);
        if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
            ASR::Variable_t *asr_target = LCompilers::ASRUtils::EXPR2VAR(x.m_target);

            wasm::emit_set_local(m_code_section, m_al, m_var_name_idx_map[asr_target->m_name]);
        } else if (ASR::is_a<ASR::ArrayRef_t>(*x.m_target)) {
            throw CodeGenError("Assignment: Arrays not yet supported");
        } else {
            LCOMPILERS_ASSERT(false)
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        this->visit_expr(*x.m_left);

        this->visit_expr(*x.m_right);

        if (ASRUtils::is_integer(*x.m_type)) {
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
                        wasm::emit_i32_div(m_code_section, m_al);
                        break;
                    };
                    default:
                        throw CodeGenError("Binop: Pow Operation not yet implemented");
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
                        wasm::emit_i64_div(m_code_section, m_al);
                        break;
                    };
                    default:
                        throw CodeGenError("Binop: Pow Operation not yet implemented");
                }
            } else {
                throw CodeGenError("Binop: Integer kind not supported");
            }
        } else {
            throw CodeGenError("Binop: Only Integer type implemented");
        }
    }

    void visit_Var(const ASR::Var_t &x) {
        const ASR::symbol_t *s = ASRUtils::symbol_get_past_external(x.m_v);
        auto v = ASR::down_cast<ASR::Variable_t>(s);
        switch (v->m_type->type) {
            case ASR::ttypeType::Integer:
                // currently omitting check for 64 bit integers
                wasm::emit_get_local(m_code_section, m_al, m_var_name_idx_map[v->m_name]);
                break;

            default:
                throw CodeGenError("Variable type not supported");
        }
    }

    void visit_Return(const ASR::Return_t & /* x */) {
        wasm::emit_get_local(m_code_section, m_al, m_var_name_idx_map[return_var->m_name]);
        wasm::emit_b8(m_code_section, m_al, 0x0F);
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        int64_t val = x.m_n;
        int a_kind = ((ASR::Integer_t *)(&(x.m_type->base)))->m_kind;
        switch (a_kind) {
            case 4: {
                wasm::emit_i32_const(m_code_section, m_al, val);
                break;
            }
            default: {
                throw CodeGenError("Constant Integer: Only kind 4 currently supported");
            }
        }
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(
            LCompilers::ASRUtils::symbol_get_past_external(x.m_name));

        for (size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i].m_value);
        }

        wasm::emit_call(m_code_section, m_al, m_func_name_idx_map[fn->m_name]);
    }
};

Result<Vec<uint8_t>> asr_to_wasm_bytes_stream(ASR::TranslationUnit_t &asr, Allocator &al) {
    ASRToWASMVisitor v(al);
    Vec<uint8_t> wasm_bytes;

    pass_wrap_global_stmts_into_function(al, asr, "f");
    pass_replace_do_loops(al, asr);

    try {
        wasm::emit_u32(v.m_type_section, v.m_al, 1);
        wasm::emit_u32(v.m_func_section, v.m_al, 3);
        wasm::emit_u32(v.m_export_section, v.m_al, 7);
        wasm::emit_u32(v.m_code_section, v.m_al, 10);

        uint32_t len_idx_type_section = wasm::emit_len_placeholder(v.m_type_section, v.m_al);
        uint32_t len_idx_func_section = wasm::emit_len_placeholder(v.m_func_section, v.m_al);
        uint32_t len_idx_export_section = wasm::emit_len_placeholder(v.m_export_section, v.m_al);
        uint32_t len_idx_code_section = wasm::emit_len_placeholder(v.m_code_section, v.m_al);

        v.visit_asr((ASR::asr_t &)asr);

        wasm::fixup_len(v.m_type_section, len_idx_type_section);
        wasm::fixup_len(v.m_func_section, len_idx_func_section);
        wasm::fixup_len(v.m_export_section, len_idx_export_section);
        wasm::fixup_len(v.m_code_section, len_idx_code_section);

    } catch (const CodeGenError &e) {
        Error error;
        return error;
    }

    {
        wasm_bytes.reserve(al, v.m_preamble.size() + v.m_type_section.size() + v.m_func_section.size() + v.m_export_section.size() + v.m_code_section.size());
        for (auto &byte : v.m_preamble) {
            wasm_bytes.push_back(al, byte);
        }
        for (auto &byte : v.m_type_section) {
            wasm_bytes.push_back(al, byte);
        }
        for (auto &byte : v.m_func_section) {
            wasm_bytes.push_back(al, byte);
        }
        for (auto &byte : v.m_export_section) {
            wasm_bytes.push_back(al, byte);
        }
        for (auto &byte : v.m_code_section) {
            wasm_bytes.push_back(al, byte);
        }
    }

    return wasm_bytes;
}

Result<int> asr_to_wasm(ASR::TranslationUnit_t &asr, Allocator &al, const std::string &filename, bool time_report) {
    int time_visit_asr = 0;
    int time_save = 0;

    auto t1 = std::chrono::high_resolution_clock::now();
    Result<Vec<uint8_t>> wasm = asr_to_wasm_bytes_stream(asr, al);
    auto t2 = std::chrono::high_resolution_clock::now();
    time_visit_asr = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    if (!wasm.ok) {
        Error error;
        return error;
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        FILE *fp = fopen(filename.c_str(), "wb");
        fwrite(wasm.result.data(), sizeof(uint8_t), wasm.result.size(), fp);
        fclose(fp);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_save = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    if (time_report) {
        std::cout << "Codegen Time report:" << std::endl;
        std::cout << "ASR -> wasm: " << std::setw(5) << time_visit_asr << std::endl;
        std::cout << "Save:       " << std::setw(5) << time_save << std::endl;
        int total = time_visit_asr + time_save;
        std::cout << "Total:      " << std::setw(5) << total << std::endl;
    }
    return 0;
}

}  // namespace LCompilers
