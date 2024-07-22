#include <iostream>
#include <fstream>
#include <string>

#include <lpython/python_evaluator.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <lpython/python_ast.h>
#include <lpython/pickle.h>
#include <lpython/parser/parser.h>
#include <libasr/codegen/asr_to_cpp.h>
#include <libasr/exception.h>
#include <libasr/asr.h>
#include <libasr/asr_scopes.h>
#include <libasr/pickle.h>

#ifdef HAVE_LFORTRAN_LLVM
#include <libasr/codegen/evaluator.h>
#include <libasr/codegen/asr_to_llvm.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DataLayout.h>
#else
namespace LCompilers {
    class LLVMEvaluator {};
}
#endif

namespace LCompilers {


/* ------------------------------------------------------------------------- */
// PythonEvaluator

PythonCompiler::PythonCompiler(CompilerOptions compiler_options)
    :
    compiler_options{compiler_options},
    al{1024*1024},
#ifdef HAVE_LFORTRAN_LLVM
    e{std::make_unique<LLVMEvaluator>()},
#endif
    eval_count{1},
    symbol_table{nullptr},
    global_underscore_name{""}
{
}

PythonCompiler::~PythonCompiler() = default;

Result<PythonCompiler::EvalResult> PythonCompiler::evaluate2(const std::string &code) {
    LocationManager lm;
    LCompilers::PassManager lpm;
    lpm.use_default_passes();
    {
        LCompilers::LocationManager::FileLocations fl;
        fl.in_filename = "input";
        std::ofstream out("input");
        out << code;
        lm.files.push_back(fl);
        lm.init_simple(code);
        lm.file_ends.push_back(code.size());
    }
    diag::Diagnostics diagnostics;
    return evaluate(code, false, lm, lpm, diagnostics);
}

Result<PythonCompiler::EvalResult> PythonCompiler::evaluate(
#ifdef HAVE_LFORTRAN_LLVM
            const std::string &code_orig, bool verbose, LocationManager &lm,
            LCompilers::PassManager& pass_manager, diag::Diagnostics &diagnostics
#else
            const std::string &/*code_orig*/, bool /*verbose*/,
                LocationManager &/*lm*/, LCompilers::PassManager& /*pass_manager*/,
                diag::Diagnostics &/*diagnostics*/
#endif
            )
{
#ifdef HAVE_LFORTRAN_LLVM
    EvalResult result;
    result.type = EvalResult::none;

    // Src -> AST
    Result<LCompilers::LPython::AST::ast_t*> res = get_ast2(code_orig, diagnostics);
    LCompilers::LPython::AST::ast_t* ast;
    if (res.ok) {
        ast = res.result;
    } else {
        return res.error;
    }

    if (verbose) {
        result.ast = LCompilers::LPython::pickle_python(*ast, true, true);
    }

    // AST -> ASR
    Result<ASR::TranslationUnit_t*> res2 = get_asr3(*ast, diagnostics, lm, true);
    ASR::TranslationUnit_t* asr;
    if (res2.ok) {
        asr = res2.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res2.error;
    }

    if (verbose) {
        result.asr = pickle(*asr, true, true, true);
    }

    // ASR -> LLVM
    std::string module_name = "__main__";
    run_fn = module_name + "global_stmts_" + std::to_string(eval_count) + "__";

    Result<std::unique_ptr<LLVMModule>> res3 = get_llvm3(*asr,
        pass_manager, diagnostics, lm.files.back().in_filename);
    std::unique_ptr<LCompilers::LLVMModule> m;
    if (res3.ok) {
        m = std::move(res3.result);
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res3.error;
    }

    if (verbose) {
        result.llvm_ir = m->str();
    }

    ASR::symbol_t *global_underscore_symbol = ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))
                                    ->m_symtab->get_symbol("_" + run_fn);
    if (global_underscore_symbol) {
        global_underscore_name = "_" + run_fn;
    }

    bool call_run_fn = false;
    std::string return_type = m->get_return_type(run_fn);
    if (return_type != "none") {
      call_run_fn = true;
    }

    bool struct_to_print = false;
    ASR::symbol_t *global_underscore_sym = ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))
                                    ->m_symtab->get_symbol("_" + run_fn);
    if ((return_type == "void") && (global_underscore_sym)) {
        // we compute the offsets of the struct's attribute here
        // we will be using it later in aggregate_type_to_string to print the struct

        // we compute the offsets here instead of computing it in aggregate_type_to_string
        // because once we call `e->add_module`, internally LLVM may deallocate all the
        // type info after compiling the IR into machine code

        llvm::GlobalVariable *g = m->get_global("_" + run_fn);
        LCOMPILERS_ASSERT(g)
        llvm::Type *llvm_type = g->getValueType();
        if (llvm_type->isStructTy()) {
            struct_to_print = true;
            compute_offsets(llvm_type, global_underscore_sym, result);
        }
    }

    e->add_module(std::move(m));
    if (call_run_fn) {
        if (return_type == "integer1ptr") {
            ASR::symbol_t *fn = ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))
                                    ->m_symtab->get_symbol(run_fn);
            LCOMPILERS_ASSERT(fn)
            if (ASRUtils::get_FunctionType(fn)->m_return_var_type->type == ASR::ttypeType::Character) {
                char *r = e->execfn<char*>(run_fn);
                result.type = EvalResult::string;
                result.str = r;
            } else {
                throw LCompilersException("PythonCompiler::evaluate(): Return type not supported");
            }
        } else if (return_type == "integer1") {
            ASR::symbol_t *fn = ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))
                                    ->m_symtab->get_symbol(run_fn);
            LCOMPILERS_ASSERT(fn)
            if (ASRUtils::get_FunctionType(fn)->m_return_var_type->type == ASR::ttypeType::UnsignedInteger) {
                uint8_t r = e->execfn<int>(run_fn);
                result.type = EvalResult::unsignedInteger1;
                result.u32 = r;
            } else {
                int8_t r = e->execfn<int>(run_fn);
                result.type = EvalResult::integer1;
                result.i32 = r;
            }
        } else if (return_type == "integer2") {
            ASR::symbol_t *fn = ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))
                                    ->m_symtab->get_symbol(run_fn);
            LCOMPILERS_ASSERT(fn)
            if (ASRUtils::get_FunctionType(fn)->m_return_var_type->type == ASR::ttypeType::UnsignedInteger) {
                uint16_t r = e->execfn<int>(run_fn);
                result.type = EvalResult::unsignedInteger2;
                result.u32 = r;
            } else {
                int16_t r = e->execfn<int>(run_fn);
                result.type = EvalResult::integer2;
                result.i32 = r;
            }
        } else if (return_type == "integer4") {
            ASR::symbol_t *fn = ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))
                                    ->m_symtab->get_symbol(run_fn);
            LCOMPILERS_ASSERT(fn)
            if (ASRUtils::get_FunctionType(fn)->m_return_var_type->type == ASR::ttypeType::UnsignedInteger) {
                uint32_t r = e->execfn<uint32_t>(run_fn);
                result.type = EvalResult::unsignedInteger4;
                result.u32 = r;
            } else {
                int32_t r = e->execfn<int32_t>(run_fn);
                result.type = EvalResult::integer4;
                result.i32 = r;
            }
        } else if (return_type == "integer8") {
            ASR::symbol_t *fn = ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))
                                    ->m_symtab->get_symbol(run_fn);
            LCOMPILERS_ASSERT(fn)
            if (ASRUtils::get_FunctionType(fn)->m_return_var_type->type == ASR::ttypeType::UnsignedInteger) {
                uint64_t r = e->execfn<uint64_t>(run_fn);
                result.type = EvalResult::unsignedInteger8;
                result.u64 = r;
            } else {
                int64_t r = e->execfn<int64_t>(run_fn);
                result.type = EvalResult::integer8;
                result.i64 = r;
            }
        } else if (return_type == "real4") {
            float r = e->execfn<float>(run_fn);
            result.type = EvalResult::real4;
            result.f32 = r;
        } else if (return_type == "real8") {
            double r = e->execfn<double>(run_fn);
            result.type = EvalResult::real8;
            result.f64 = r;
        } else if (return_type == "complex4") {
            std::complex<float> r = e->execfn<std::complex<float>>(run_fn);
            result.type = EvalResult::complex4;
            result.c32.re = r.real();
            result.c32.im = r.imag();
        } else if (return_type == "complex8") {
            std::complex<double> r = e->execfn<std::complex<double>>(run_fn);
            result.type = EvalResult::complex8;
            result.c64.re = r.real();
            result.c64.im = r.imag();
        } else if (return_type == "logical") {
            bool r = e->execfn<bool>(run_fn);
            result.type = EvalResult::boolean;
            result.b = r;
        } else if (return_type == "struct") {
            e->execfn<void>(run_fn);
            if (global_underscore_sym) {
                void *r = (void*)e->get_symbol_address(global_underscore_name);
                LCOMPILERS_ASSERT(r)
                result.structure.structure = r;
                result.type = EvalResult::struct_type;
            }
        } else if (return_type == "void") {
            e->execfn<void>(run_fn);
            if (global_underscore_sym && struct_to_print) {
                void *r = (void*)e->get_symbol_address("_" + run_fn);
                LCOMPILERS_ASSERT(r)
                result.structure.structure = r;
                result.type = EvalResult::struct_type;
            } else {
                result.type = EvalResult::statement;
            }
        } else if (return_type == "none") {
            result.type = EvalResult::none;
        } else {
            throw LCompilersException("PythonCompiler::evaluate(): Return type not supported");
        }
    }

    if (call_run_fn) {
        ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))->m_symtab
            ->erase_symbol(run_fn);
    }
    if (global_underscore_symbol) {
        if (ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))->m_symtab->resolve_symbol("_")) {
            ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))->m_symtab
                    ->erase_symbol("_");
        }
        ASR::Variable_t *a = ASR::down_cast<ASR::Variable_t>(global_underscore_symbol);
        ASR::Variable_t *b = al.make_new<ASR::Variable_t>();
        *b = *a;
        Str s;
        s.from_str(al, "_");
        b->m_name = s.c_str(al);
        ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))->m_symtab
                ->add_symbol("_", ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)b));
    }

    eval_count++;
    return result;
#else
    throw LCompilersException("LLVM is not enabled");
#endif
}

Result<std::string> PythonCompiler::get_ast(const std::string &code,
    LocationManager &lm, diag::Diagnostics &diagnostics)
{
    Result<LCompilers::LPython::AST::ast_t*> ast = get_ast2(code, diagnostics);
    if (ast.ok) {
        if (compiler_options.po.tree) {
            return LCompilers::LPython::pickle_tree_python(*ast.result, compiler_options.use_colors);
        } else if (compiler_options.po.json || compiler_options.po.visualize) {
            return LCompilers::LPython::pickle_json(*ast.result, lm);
        }
        return LCompilers::LPython::pickle_python(*ast.result, compiler_options.use_colors,
            compiler_options.indent);
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return ast.error;
    }
}

Result<std::string> PythonCompiler::get_asr(const std::string &code,
    LocationManager &lm, diag::Diagnostics &diagnostics)
{
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code, lm, diagnostics);
    if (asr.ok) {
        if (compiler_options.po.tree) {
            return LCompilers::pickle_tree(*asr.result, compiler_options.use_colors);
        } else if (compiler_options.po.json) {
            return LCompilers::pickle_json(*asr.result, lm, compiler_options.po.no_loc, false);
        }
        return LCompilers::pickle(*asr.result,
            compiler_options.use_colors, compiler_options.indent);
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return asr.error;
    }
}

Result<ASR::TranslationUnit_t*> PythonCompiler::get_asr2(
            const std::string &code_orig, LocationManager &lm,
            diag::Diagnostics &diagnostics)
{
    // Src -> AST
    Result<LCompilers::LPython::AST::ast_t*> res = get_ast2(code_orig, diagnostics);
    LCompilers::LPython::AST::ast_t* ast;
    if (res.ok) {
        ast = res.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }

    // AST -> ASR
    Result<ASR::TranslationUnit_t*> res2 = get_asr3(*ast, diagnostics, lm, true);
    if (res2.ok) {
        return res2.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res2.error;
    }
}

Result<LCompilers::LPython::AST::ast_t*> PythonCompiler::get_ast2(
            const std::string &code_orig, diag::Diagnostics &diagnostics)
{
    // Src -> AST
    const std::string *code=&code_orig;
    std::string tmp;
    Result<LCompilers::LPython::AST::Module_t*>
        res = LCompilers::LPython::parse(al, *code, 0, diagnostics);
    if (res.ok) {
        return (LCompilers::LPython::AST::ast_t*)res.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
}

Result<ASR::TranslationUnit_t*> PythonCompiler::get_asr3(
    LCompilers::LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics,
    LocationManager &lm, bool is_interactive)
{
    ASR::TranslationUnit_t* asr;
    // AST -> ASR
    if (symbol_table) {
        symbol_table->mark_all_variables_external(al);
    }
    auto res = LCompilers::LPython::python_ast_to_asr(al, lm, symbol_table, ast, diagnostics,
        compiler_options, true, "__main__", "", false, is_interactive ? eval_count : 0);
    if (res.ok) {
        asr = res.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
    if (!symbol_table) symbol_table = asr->m_symtab;

    return asr;
}

Result<std::string> PythonCompiler::get_llvm(
    const std::string &code, LocationManager &lm, LCompilers::PassManager& pass_manager,
    diag::Diagnostics &diagnostics
    )
{
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code, lm, pass_manager, diagnostics);
    if (res.ok) {
#ifdef HAVE_LFORTRAN_LLVM
        return res.result->str();
#else
        throw LCompilersException("LLVM is not enabled");
#endif
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
}

Result<std::unique_ptr<LLVMModule>> PythonCompiler::get_llvm2(
    const std::string &code, LocationManager &lm, LCompilers::PassManager& pass_manager,
    diag::Diagnostics &diagnostics)
{
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code, lm, diagnostics);
    if (!asr.ok) {
        return asr.error;
    }
    Result<std::unique_ptr<LLVMModule>> res = get_llvm3(*asr.result, pass_manager,
        diagnostics, lm.files.back().in_filename);
    if (res.ok) {
#ifdef HAVE_LFORTRAN_LLVM
        std::unique_ptr<LLVMModule> m = std::move(res.result);
        return m;
#else
        throw LCompilersException("LLVM is not enabled");
#endif
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
}


Result<std::unique_ptr<LLVMModule>> PythonCompiler::get_llvm3(
#ifdef HAVE_LFORTRAN_LLVM
    ASR::TranslationUnit_t &asr, LCompilers::PassManager& lpm,
    diag::Diagnostics &diagnostics, const std::string &infile
#else
    ASR::TranslationUnit_t &/*asr*/, LCompilers::PassManager&/*lpm*/,
    diag::Diagnostics &/*diagnostics*/,const std::string &/*infile*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    if (compiler_options.emit_debug_info) {
        if (!compiler_options.emit_debug_line_column) {
            diagnostics.add(LCompilers::diag::Diagnostic(
                "The `emit_debug_line_column` is not enabled; please use the "
                "`--debug-with-line-column` option to get the correct "
                "location information",
                LCompilers::diag::Level::Error,
                LCompilers::diag::Stage::Semantic, {})
            );
            Error err;
            return err;
        }
    }
    // ASR -> LLVM
    std::unique_ptr<LCompilers::LLVMModule> m;
    Result<std::unique_ptr<LCompilers::LLVMModule>> res
        = asr_to_llvm(asr, diagnostics,
            e->get_context(), al, lpm, compiler_options,
            run_fn, global_underscore_name, infile);
    if (res.ok) {
        m = std::move(res.result);
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }

    if (compiler_options.po.fast) {
        e->opt(*m->m_m);
    }

    return m;
#else
    throw LCompilersException("LLVM is not enabled");
#endif
}

Result<std::string> PythonCompiler::get_asm(
#ifdef HAVE_LFORTRAN_LLVM
    const std::string &code, LocationManager &lm,
    LCompilers::PassManager& lpm,
    diag::Diagnostics &diagnostics
#else
    const std::string &/*code*/,
    LocationManager &/*lm*/,
    LCompilers::PassManager&/*lpm*/,
    diag::Diagnostics &/*diagnostics*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code, lm, lpm, diagnostics);
    if (res.ok) {
        return e->get_asm(*res.result->m_m);
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
#else
    throw LCompilersException("LLVM is not enabled");
#endif
}


void print_type(ASR::ttype_t *t, void *data, std::string &result);

std::string PythonCompiler::aggregate_type_to_string(const struct EvalResult &r) {
    ASR::ttype_t *asr_type = r.structure.ttype;
    void *data = r.structure.structure;
    size_t *offsets = r.structure.offsets;
    size_t element_size = r.structure.element_size;

    std::string result;

    if (asr_type->type == ASR::ttypeType::List) {
        int32_t size = *(int32_t*)(((char*)data)+offsets[0]);
        void *array = *(void**)(((char*)data)+offsets[2]);
        ASR::ttype_t *element_ttype = ASR::down_cast<ASR::List_t>(asr_type)->m_type;

        result += "[";
        for (int32_t i = 0; i < size - 1; i++) {
            print_type(element_ttype, ((char*)array)+(i*element_size), result);
            result += ", ";
        }
        print_type(element_ttype, ((char*)array)+((size - 1)*element_size), result);
        result += "]";

    } else if (asr_type->type == ASR::ttypeType::Tuple) {
        ASR::Tuple_t *tuple_type = ASR::down_cast<ASR::Tuple_t>(asr_type);
        result += "(";
        for (size_t i = 0; i < tuple_type->n_type - 1; i++) {
            print_type(tuple_type->m_type[i], ((char*)data)+offsets[i], result);
            result += ", ";
        }
        print_type(tuple_type->m_type[tuple_type->n_type - 1], ((char*)data)+offsets[tuple_type->n_type - 1], result);
        result += ")";

    } else if (asr_type->type == ASR::ttypeType::StructType) {
        ASR::StructType_t *class_type = ASR::down_cast<ASR::StructType_t>(asr_type);
        ASR::Struct_t *struct_info = ASR::down_cast<ASR::Struct_t>(class_type->m_derived_type);
        LCOMPILERS_ASSERT(class_type->n_data_member_types == struct_info->n_members)
        result += struct_info->m_name;
        result += "(";
        for (size_t i = 0; i < struct_info->n_members - 1; i++) {
            result += struct_info->m_members[i];
            result += "=";
            print_type(class_type->m_data_member_types[i], ((char*)data)+offsets[i], result);
            result += ", ";
        }
        result += struct_info->m_members[struct_info->n_members - 1];
        result += "=";
        print_type(class_type->m_data_member_types[struct_info->n_members - 1], ((char*)data)+offsets[struct_info->n_members - 1], result);
        result += ")";

    } else {
        throw LCompilersException("PythonCompiler::evaluate(): Return type not supported");
    }

    return result;
}

void PythonCompiler::compute_offsets(llvm::Type *type, ASR::symbol_t *asr_type, EvalResult &result) {
#ifdef HAVE_LFORTRAN_LLVM
        LCOMPILERS_ASSERT(type->isStructTy())

        const llvm::DataLayout &dl = e->get_jit_data_layout();
        size_t elements_count = type->getStructNumElements();
        LCompilers::Vec<size_t> offsets;
        offsets.reserve(al, elements_count);
        for (size_t i = 0; i < elements_count; i++) {
            size_t offset = dl.getStructLayout((llvm::StructType*)type)->getElementOffset(i);
            offsets.push_back(al, offset);
        }
        result.structure.offsets = offsets.p;

        result.structure.ttype = ASR::down_cast<ASR::Variable_t>(asr_type)->m_type;
        if (result.structure.ttype->type == ASR::ttypeType::List) {
            type = type->getStructElementType(2);
            LCOMPILERS_ASSERT(type->isPointerTy())
            result.structure.element_size = e->get_jit_data_layout().getTypeAllocSize(
#if LLVM_VERSION_MAJOR >= 14
                                                type->getNonOpaquePointerElementType()
#else
                                                type->getPointerElementType()
#endif
                                                );
        }
#else
    throw LCompilersException("LLVM is not enabled");
#endif
}

void print_type(ASR::ttype_t *t, void *data, std::string &result) {
    switch (t->type) {
        case ASR::ttypeType::Logical:
            result += (*(bool*)data ? "True" : "False");
            break;
        case ASR::ttypeType::Integer: {
            int64_t a_kind = ASR::down_cast<ASR::Integer_t>(t)->m_kind;
            switch (a_kind) {
                case 1:
                    result += std::to_string(int(*(int8_t*)data));
                    break;
                case 2:
                    result += std::to_string(*(int16_t*)data);
                    break;
                case 4:
                    result += std::to_string(*(int32_t*)data);
                    break;
                case 8:
                    result += std::to_string(*(int64_t*)data);
                    break;
                default:
                    throw LCompilersException("Unaccepted int size");
            }
            break;
        }
        case ASR::ttypeType::UnsignedInteger: {
            int64_t a_kind = ASR::down_cast<ASR::UnsignedInteger_t>(t)->m_kind;
            switch (a_kind) {
                case 1:
                    result += std::to_string(int(*(uint8_t*)data));
                    break;
                case 2:
                    result += std::to_string(*(uint16_t*)data);
                    break;
                case 4:
                    result += std::to_string(*(uint32_t*)data);
                    break;
                case 8:
                    result += std::to_string(*(uint64_t*)data);
                    break;
                default:
                    throw LCompilersException("Unaccepted int size");
            }
            break;
        }
        case ASR::ttypeType::Real: {
            int64_t a_kind = ASR::down_cast<ASR::Real_t>(t)->m_kind;
            switch (a_kind) {
                case 4:
                    result += std::to_string(*(float*)data);
                    break;
                case 8:
                    result += std::to_string(*(double*)data);
                    break;
                default:
                    throw LCompilersException("Unaccepted real size");
            }
            break;
        }
        case ASR::ttypeType::Character:
            result += '"';
            result += std::string(*(char**)data); // TODO: replace \n with \\n
            result += '"';
            break;
        default:
            throw LCompilersException("PythonCompiler::print_type(): type not supported");
    }
}

} // namespace LCompilers::LPython
