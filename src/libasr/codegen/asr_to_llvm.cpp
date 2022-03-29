#include <iostream>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string_view>
#include <utility>

#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_llvm.h>
#include <libasr/pass/do_loops.h>
#include <libasr/pass/for_all.h>
#include <libasr/pass/implied_do_loops.h>
#include <libasr/pass/array_op.h>
#include <libasr/pass/select_case.h>
#include <libasr/pass/global_stmts.h>
#include <libasr/pass/param_to_const.h>
#include <libasr/pass/nested_vars.h>
#include <libasr/pass/print_arr.h>
#include <libasr/pass/arr_slice.h>
#include <libasr/pass/flip_sign.h>
#include <libasr/pass/div_to_mul.h>
#include <libasr/pass/fma.h>
#include <libasr/pass/loop_unroll.h>
#include <libasr/pass/sign_from_value.h>
#include <libasr/pass/class_constructor.h>
#include <libasr/pass/unused_functions.h>
#include <libasr/pass/inline_function_calls.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/codegen/llvm_utils.h>
#include <libasr/codegen/llvm_array_utils.h>


namespace LFortran {

namespace {

    // This exception is used to abort the visitor pattern when an error occurs.
    // This is only used locally in this file, not propagated outside. An error
    // must be already present in ASRToLLVMVisitor::diag before throwing this
    // exception. This is checked with an assert when the CodeGenAbort is
    // caught.
    class CodeGenAbort
    {
    };

    // Local exception that is only used in this file to exit the visitor
    // pattern and caught later (not propagated outside). It accepts an error
    // message that is then appended at the end of ASRToLLVMVisitor::diag.  The
    // `diag` can already contain other errors or warnings.  This is a
    // convenience class. One can also report the error into `diag` directly and
    // call `CodeGenAbort` instead.
    class CodeGenError
    {
    public:
        diag::Diagnostic d;
    public:
        CodeGenError(const std::string &msg)
            : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen)}
        { }

        CodeGenError(const std::string &msg, const Location &loc)
            : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen, {
                diag::Label("", {loc})
            })}
        { }
    };

}


using ASR::is_a;
using ASR::down_cast;
using ASR::down_cast2;

using LFortran::ASRUtils::expr_type;
using LFortran::ASRUtils::symbol_get_past_external;
using LFortran::ASRUtils::EXPR2VAR;
using LFortran::ASRUtils::EXPR2FUN;
using LFortran::ASRUtils::EXPR2SUB;
using LFortran::ASRUtils::intent_local;
using LFortran::ASRUtils::intent_return_var;
using LFortran::ASRUtils::determine_module_dependencies;
using LFortran::ASRUtils::is_arg_dummy;

// Platform dependent fast unique hash:
uint64_t static get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

void printf(llvm::LLVMContext &context, llvm::Module &module,
    llvm::IRBuilder<> &builder, const std::vector<llvm::Value*> &args)
{
    llvm::Function *fn_printf = module.getFunction("_lfortran_printf");
    if (!fn_printf) {
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), {llvm::Type::getInt8PtrTy(context)}, true);
        fn_printf = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, "_lfortran_printf", &module);
    }
    builder.CreateCall(fn_printf, args);
}

void exit(llvm::LLVMContext &context, llvm::Module &module,
    llvm::IRBuilder<> &builder, llvm::Value* exit_code)
{
    llvm::Function *fn_exit = module.getFunction("exit");
    if (!fn_exit) {
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), {llvm::Type::getInt32Ty(context)},
                false);
        fn_exit = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, "exit", &module);
    }
    builder.CreateCall(fn_exit, {exit_code});
}

void string_init(llvm::LLVMContext &context, llvm::Module &module,
        llvm::IRBuilder<> &builder, llvm::Value* arg_size, llvm::Value* arg_string) {
    std::string func_name = "_lfortran_string_init";
    llvm::Function *fn = module.getFunction(func_name);
    if (!fn) {
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), {
                    llvm::Type::getInt32Ty(context),
                    llvm::Type::getInt8PtrTy(context)
                }, true);
        fn = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, func_name, module);
    }
    std::vector<llvm::Value*> args = {arg_size, arg_string};
    builder.CreateCall(fn, args);
}

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
private:
  //! To be used by visit_DerivedRef.
  std::string der_type_name;

  //! Helpful for debugging while testing LLVM code
  void print_util(llvm::Value* v, std::string fmt_chars, std::string endline="\t") {
        std::vector<llvm::Value *> args;
        std::vector<std::string> fmt;
        args.push_back(v);
        fmt.push_back(fmt_chars);
        std::string fmt_str;
        for (size_t i=0; i<fmt.size(); i++) {
            fmt_str += fmt[i];
            if (i < fmt.size()-1) fmt_str += " ";
        }
        fmt_str += endline;
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr(fmt_str);
        std::vector<llvm::Value *> printf_args;
        printf_args.push_back(fmt_ptr);
        printf_args.insert(printf_args.end(), args.begin(), args.end());
        printf(context, *module, *builder, printf_args);
    }

public:
    diag::Diagnostics &diag;
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    Platform platform;
    Allocator &al;

    llvm::Value *tmp;
    llvm::BasicBlock *current_loophead, *current_loopend, *proc_return;
    std::string mangle_prefix;
    bool prototype_only;
    llvm::StructType *complex_type_4, *complex_type_8;
    llvm::StructType *complex_type_4_ptr, *complex_type_8_ptr;
    llvm::PointerType *character_type;

    std::unordered_map<std::uint32_t, std::unordered_map<std::string, llvm::Type*>> arr_arg_type_cache;

    std::map<std::string, std::pair<llvm::Type*, llvm::Type*>> fname2arg_type;

    // Maps for containing information regarding derived types
    std::map<std::string, llvm::StructType*> name2dertype;
    std::map<std::string, std::string> dertype2parent;
    std::map<std::string, std::map<std::string, int>> name2memidx;

    std::map<uint64_t, llvm::Value*> llvm_symtab; // llvm_symtab_value
    std::map<uint64_t, llvm::Function*> llvm_symtab_fn;
    std::map<std::string, uint64_t> llvm_symtab_fn_names;
    std::map<uint64_t, llvm::Value*> llvm_symtab_fn_arg;
    std::map<uint64_t, llvm::BasicBlock*> llvm_goto_targets;

    // Data members for handling nested functions
    std::map<uint64_t, std::vector<uint64_t>> nesting_map; /* For saving the
        relationship between enclosing and nested functions */
    std::vector<uint64_t> nested_globals; /* For saving the hash of variables
        from a parent scope needed in a nested function */
    std::map<uint64_t, std::vector<llvm::Type*>> nested_func_types; /* For
        saving the hash of a parent function needing to give access to
        variables in a nested function, as well as the variable types */
    llvm::StructType* nested_global_struct; /*The struct type that will hold
        variables needed in a nested function; will contain types as given in
        the runtime descriptor member */
    std::string nested_desc_name; // For setting the name of the global struct
    std::vector<uint64_t> nested_call_out; /* Hash of functions containing
        nested functions that can call functions besides the nested functions
        - in such cases we need a means to save the local context */
    llvm::StructType* nested_global_struct_vals; /*Equivalent struct type to
        nested_global_struct, but holding types that nested_global_struct points
        to. Needed in cases where we need to store values and preserve a local
        context */
    llvm::ArrayType* nested_global_stack; /* An array type for holding numerous
        nested_global_struct_vals, serving as a stack to reload values when
        we may are leaving or re-entering states with a need to preserve
        context*/
    std::string nested_stack_name; // The name of the nested_global_stack
    std::string nested_sp_name; /* The stack pointer name for the
        nested_global_stack */
    uint64_t parent_function_hash;
    uint64_t calling_function_hash; /* These hashes are compared to resulted
        from the nested_vars analysis pass to determine if we need to save or
        reload a local scope (and increment or decrement the stack pointer) */
    const ASR::Function_t *parent_function = nullptr;
    const ASR::Subroutine_t *parent_subroutine = nullptr; /* For ensuring we are
        in a nested_function before checking if we need to declare stack
        types */

    std::unique_ptr<LLVMUtils> llvm_utils;
    std::unique_ptr<LLVMArrUtils::Descriptor> arr_descr;

    ASRToLLVMVisitor(Allocator &al, llvm::LLVMContext &context, Platform platform,
        diag::Diagnostics &diagnostics) :
    diag{diagnostics},
    context(context),
    builder(std::make_unique<llvm::IRBuilder<>>(context)),
    platform{platform},
    al{al},
    prototype_only(false),
    llvm_utils(std::make_unique<LLVMUtils>(context, builder.get())),
    arr_descr(LLVMArrUtils::Descriptor::get_descriptor(context,
              builder.get(),
              llvm_utils.get(),
              LLVMArrUtils::DESCR_TYPE::_SimpleCMODescriptor))
    {
    }

    llvm::Value* CreateLoad(llvm::Value *x) {
        return LFortran::LLVM::CreateLoad(*builder, x);
    }


    llvm::Value* CreateGEP(llvm::Value *x, std::vector<llvm::Value *> &idx) {
        return LFortran::LLVM::CreateGEP(*builder, x, idx);
    }

    // Inserts a new block `bb` using the current builder
    // and terminates the previous block if it is not already terminated
    void start_new_block(llvm::BasicBlock *bb) {
        llvm::BasicBlock *last_bb = builder->GetInsertBlock();
        llvm::Function *fn = last_bb->getParent();
        llvm::Instruction *block_terminator = last_bb->getTerminator();
        if (block_terminator == nullptr) {
            // The previous block is not terminated --- terminate it by jumping
            // to our new block
            builder->CreateBr(bb);
        }
        fn->getBasicBlockList().push_back(bb);
        builder->SetInsertPoint(bb);
    }

    inline bool verify_dimensions_t(ASR::dimension_t* m_dims, int n_dims) {
        if( n_dims <= 0 ) {
            return false;
        }
        bool is_ok = true;
        for( int r = 0; r < n_dims; r++ ) {
            if( m_dims[r].m_end == nullptr ) {
                is_ok = false;
                break;
            }
        }
        return is_ok;
    }

    llvm::Type*
    get_el_type(ASR::ttype_t* m_type_, int a_kind) {
        llvm::Type* el_type = nullptr;
        if (ASR::is_a<ASR::Pointer_t>(*m_type_)) {
            ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(m_type_)->m_type;
            switch(t2->type) {
                case ASR::ttypeType::Integer: {
                    el_type = getIntType(a_kind, true);
                    break;
                }
                case ASR::ttypeType::Real: {
                    el_type = getFPType(a_kind, true);
                    break;
                }
                case ASR::ttypeType::Complex: {
                    el_type = getComplexType(a_kind, true);
                    break;
                }
                case ASR::ttypeType::Logical: {
                    el_type = llvm::Type::getInt1Ty(context);
                    break;
                }
                case ASR::ttypeType::Derived: {
                    el_type = getDerivedType(m_type_);
                    break;
                }
                case ASR::ttypeType::Character: {
                    el_type = character_type;
                    break;
                }
                default:
                    break;
            }
        } else {
            switch(m_type_->type) {
                case ASR::ttypeType::Integer: {
                    el_type = getIntType(a_kind);
                    break;
                }
                case ASR::ttypeType::Real: {
                    el_type = getFPType(a_kind);
                    break;
                }
                case ASR::ttypeType::Complex: {
                    el_type = getComplexType(a_kind);
                    break;
                }
                case ASR::ttypeType::Logical: {
                    el_type = llvm::Type::getInt1Ty(context);
                    break;
                }
                case ASR::ttypeType::Derived: {
                    el_type = getDerivedType(m_type_);
                    break;
                }
                case ASR::ttypeType::Character: {
                    el_type = character_type;
                    break;
                }
                default:
                    break;
            }
        }
        return el_type;
    }

    void fill_array_details(llvm::Value* arr, ASR::dimension_t* m_dims,
        int n_dims) {
        std::vector<std::pair<llvm::Value*, llvm::Value*>> llvm_dims;
        for( int r = 0; r < n_dims; r++ ) {
            ASR::dimension_t m_dim = m_dims[r];
            visit_expr(*(m_dim.m_start));
            llvm::Value* start = tmp;
            visit_expr(*(m_dim.m_end));
            llvm::Value* end = tmp;
            llvm_dims.push_back(std::make_pair(start, end));
        }
        arr_descr->fill_array_details(arr, m_dims, n_dims, llvm_dims);
    }

    /*
        This function fills the descriptor
        (pointer to the first element, offset and descriptor of each dimension)
        of the array which are allocated memory in heap.
    */
    inline void fill_malloc_array_details(llvm::Value* arr, ASR::dimension_t* m_dims,
                                            int n_dims) {
        std::vector<std::pair<llvm::Value*, llvm::Value*>> llvm_dims;
        for( int r = 0; r < n_dims; r++ ) {
            ASR::dimension_t m_dim = m_dims[r];
            visit_expr(*(m_dim.m_start));
            llvm::Value* start = tmp;
            visit_expr(*(m_dim.m_end));
            llvm::Value* end = tmp;
            llvm_dims.push_back(std::make_pair(start, end));
        }
        arr_descr->fill_malloc_array_details(arr, n_dims, llvm_dims, module.get());
    }

    inline llvm::Type* getIntType(int a_kind, bool get_pointer=false) {
        llvm::Type* type_ptr = nullptr;
        if( get_pointer ) {
            switch(a_kind)
            {
                case 1:
                    type_ptr = llvm::Type::getInt8PtrTy(context);
                    break;
                case 2:
                    type_ptr = llvm::Type::getInt16PtrTy(context);
                    break;
                case 4:
                    type_ptr = llvm::Type::getInt32PtrTy(context);
                    break;
                case 8:
                    type_ptr = llvm::Type::getInt64PtrTy(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits integer kinds are supported.");
            }
        } else {
            switch(a_kind)
            {
                case 1:
                    type_ptr = llvm::Type::getInt8Ty(context);
                    break;
                case 2:
                    type_ptr = llvm::Type::getInt16Ty(context);
                    break;
                case 4:
                    type_ptr = llvm::Type::getInt32Ty(context);
                    break;
                case 8:
                    type_ptr = llvm::Type::getInt64Ty(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits integer kinds are supported.");
            }
        }
        return type_ptr;
    }

    inline llvm::Type* getFPType(int a_kind, bool get_pointer=false) {
        llvm::Type* type_ptr = nullptr;
        if( get_pointer ) {
            switch(a_kind)
            {
                case 4:
                    type_ptr = llvm::Type::getFloatPtrTy(context);
                    break;
                case 8:
                    type_ptr =  llvm::Type::getDoublePtrTy(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits real kinds are supported.");
            }
        } else {
            switch(a_kind)
            {
                case 4:
                    type_ptr = llvm::Type::getFloatTy(context);
                    break;
                case 8:
                    type_ptr = llvm::Type::getDoubleTy(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits real kinds are supported.");
            }
        }
        return type_ptr;
    }

    inline llvm::Type* getComplexType(int a_kind, bool get_pointer=false) {
        llvm::Type* type = nullptr;
        switch(a_kind)
        {
            case 4:
                type = complex_type_4;
                break;
            case 8:
                type = complex_type_8;
                break;
            default:
                throw CodeGenError("Only 32 and 64 bits complex kinds are supported.");
        }
        if( type != nullptr ) {
            if( get_pointer ) {
                return type->getPointerTo();
            } else {
                return type;
            }
        }
        return nullptr;
    }

    llvm::Type* getDerivedType(ASR::DerivedType_t* der_type, bool is_pointer=false) {
        std::string der_type_name = std::string(der_type->m_name);
        llvm::StructType* der_type_llvm;
        if( name2dertype.find(der_type_name) != name2dertype.end() ) {
            der_type_llvm = name2dertype[der_type_name];
        } else {
            std::vector<llvm::Type*> member_types;
            int member_idx = 0;
            if( der_type->m_parent != nullptr ) {
                ASR::DerivedType_t *par_der_type = ASR::down_cast<ASR::DerivedType_t>(
                                                        symbol_get_past_external(der_type->m_parent));
                llvm::Type* par_llvm = getDerivedType(par_der_type);
                member_types.push_back(par_llvm);
                dertype2parent[der_type_name] = std::string(par_der_type->m_name);
                member_idx += 1;
            }
            std::map<std::string, ASR::symbol_t*> scope = der_type->m_symtab->scope;
            for( auto itr = scope.begin(); itr != scope.end(); itr++ ) {
                ASR::Variable_t* member = (ASR::Variable_t*)(&(itr->second->base));
                llvm::Type* mem_type = nullptr;
                switch( member->m_type->type ) {
                    case ASR::ttypeType::Integer: {
                        int a_kind = down_cast<ASR::Integer_t>(member->m_type)->m_kind;
                        mem_type = getIntType(a_kind);
                        break;
                    }
                    case ASR::ttypeType::Real: {
                        int a_kind = down_cast<ASR::Real_t>(member->m_type)->m_kind;
                        mem_type = getFPType(a_kind);
                        break;
                    }
                    case ASR::ttypeType::Derived: {
                        mem_type = getDerivedType(member->m_type);
                        break;
                    }
                    case ASR::ttypeType::Complex: {
                        int a_kind = down_cast<ASR::Complex_t>(member->m_type)->m_kind;
                        mem_type = getComplexType(a_kind);
                        break;
                    }
                    case ASR::ttypeType::Character: {
                        mem_type = character_type;
                        break;
                    }
                    default:
                        throw CodeGenError("Cannot identify the type of member, '" +
                                            std::string(member->m_name) +
                                            "' in derived type, '" + der_type_name + "'.",
                                            member->base.base.loc);
                }
                member_types.push_back(mem_type);
                name2memidx[der_type_name][std::string(member->m_name)] = member_idx;
                member_idx++;
            }
            der_type_llvm = llvm::StructType::create(context, member_types, der_type_name);
            name2dertype[der_type_name] = der_type_llvm;
        }
        if( is_pointer ) {
            return der_type_llvm->getPointerTo();
        }
        return (llvm::Type*) der_type_llvm;
    }

    llvm::Type* getDerivedType(ASR::ttype_t* _type, bool is_pointer=false) {
        ASR::Derived_t* der = (ASR::Derived_t*)(&(_type->base));
        ASR::symbol_t* der_sym;
        if( der->m_derived_type->type == ASR::symbolType::ExternalSymbol ) {
            ASR::ExternalSymbol_t* der_extr = (ASR::ExternalSymbol_t*)(&(der->m_derived_type->base));
            der_sym = der_extr->m_external;
        } else {
            der_sym = der->m_derived_type;
        }
        ASR::DerivedType_t* der_type = (ASR::DerivedType_t*)(&(der_sym->base));
        return getDerivedType(der_type, is_pointer);
    }


    llvm::Type* getClassType(ASR::ttype_t* _type, bool is_pointer=false) {
        ASR::Class_t* der = (ASR::Class_t*)(&(_type->base));
        ASR::symbol_t* der_sym;
        if( der->m_class_type->type == ASR::symbolType::ExternalSymbol ) {
            ASR::ExternalSymbol_t* der_extr = (ASR::ExternalSymbol_t*)(&(der->m_class_type->base));
            der_sym = der_extr->m_external;
        } else {
            der_sym = der->m_class_type;
        }
        ASR::ClassType_t* der_type = (ASR::ClassType_t*)(&(der_sym->base));
        std::string der_type_name = std::string(der_type->m_name);
        llvm::StructType* der_type_llvm;
        if( name2dertype.find(der_type_name) != name2dertype.end() ) {
            der_type_llvm = name2dertype[der_type_name];
        } else {
            std::map<std::string, ASR::symbol_t*> scope = der_type->m_symtab->scope;
            std::vector<llvm::Type*> member_types;
            int member_idx = 0;
            for( auto itr = scope.begin(); itr != scope.end(); itr++ ) {
                if (!ASR::is_a<ASR::ClassProcedure_t>(*itr->second) &&
                    !ASR::is_a<ASR::GenericProcedure_t>(*itr->second)) {
                    ASR::Variable_t* member = ASR::down_cast<ASR::Variable_t>(itr->second);
                    llvm::Type* mem_type = nullptr;
                    switch( member->m_type->type ) {
                        case ASR::ttypeType::Integer: {
                            int a_kind = down_cast<ASR::Integer_t>(member->m_type)->m_kind;
                            mem_type = getIntType(a_kind);
                            break;
                        }
                        case ASR::ttypeType::Real: {
                            int a_kind = down_cast<ASR::Real_t>(member->m_type)->m_kind;
                            mem_type = getFPType(a_kind);
                            break;
                        }
                        case ASR::ttypeType::Class: {
                            mem_type = getClassType(member->m_type);
                            break;
                        }
                        case ASR::ttypeType::Complex: {
                            int a_kind = down_cast<ASR::Complex_t>(member->m_type)->m_kind;
                            mem_type = getComplexType(a_kind);
                            break;
                        }
                        default:
                            throw CodeGenError("Cannot identify the type of member, '" +
                                                std::string(member->m_name) +
                                                "' in derived type, '" + der_type_name + "'.",
                                                member->base.base.loc);
                    }
                    member_types.push_back(mem_type);
                    name2memidx[der_type_name][std::string(member->m_name)] = member_idx;
                    member_idx++;
                }
            }
            der_type_llvm = llvm::StructType::create(context, member_types, der_type_name);
            name2dertype[der_type_name] = der_type_llvm;
        }
        if( is_pointer ) {
            return der_type_llvm->getPointerTo();
        }
        return (llvm::Type*) der_type_llvm;
    }


    /*
    * Dispatches the required function from runtime library to
    * perform the specified binary operation.
    *
    * @param left_arg llvm::Value* The left argument of the binary operator.
    * @param right_arg llvm::Value* The right argument of the binary operator.
    * @param runtime_func_name std::string The name of the function to be dispatched
    *                                      from runtime library.
    * @returns llvm::Value* The result of the operation.
    *
    * Note
    * ====
    *
    * Internally the call to this function gets transformed into a runtime call:
    * void _lfortran_complex_add(complex* a, complex* b, complex *result)
    *
    * As of now the following values for func_name are supported,
    *
    * _lfortran_complex_add
    * _lfortran_complex_sub
    * _lfortran_complex_div
    * _lfortran_complex_mul
    */
    llvm::Value* lfortran_complex_bin_op(llvm::Value* left_arg, llvm::Value* right_arg,
                                         std::string runtime_func_name,
                                         llvm::Type* complex_type=nullptr)
    {
        if( complex_type == nullptr ) {
            complex_type = complex_type_4;
        }
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {
                        complex_type->getPointerTo(),
                        complex_type->getPointerTo(),
                        complex_type->getPointerTo()
                    }, true);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }

        llvm::AllocaInst *pleft_arg = builder->CreateAlloca(complex_type,
            nullptr);

        builder->CreateStore(left_arg, pleft_arg);
        llvm::AllocaInst *pright_arg = builder->CreateAlloca(complex_type,
            nullptr);
        builder->CreateStore(right_arg, pright_arg);
        llvm::AllocaInst *presult = builder->CreateAlloca(complex_type,
            nullptr);
        std::vector<llvm::Value*> args = {pleft_arg, pright_arg, presult};
        builder->CreateCall(fn, args);
        return CreateLoad(presult);
    }


    llvm::Value* lfortran_strop(llvm::Value* left_arg, llvm::Value* right_arg,
                                         std::string runtime_func_name)
    {
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {
                        character_type->getPointerTo(),
                        character_type->getPointerTo(),
                        character_type->getPointerTo()
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        llvm::AllocaInst *pleft_arg = builder->CreateAlloca(character_type,
            nullptr);
        builder->CreateStore(left_arg, pleft_arg);
        llvm::AllocaInst *pright_arg = builder->CreateAlloca(character_type,
            nullptr);
        builder->CreateStore(right_arg, pright_arg);
        llvm::AllocaInst *presult = builder->CreateAlloca(character_type,
            nullptr);
        std::vector<llvm::Value*> args = {pleft_arg, pright_arg, presult};
        builder->CreateCall(fn, args);
        return CreateLoad(presult);
    }

    llvm::Value* lfortran_str_len(llvm::Value* str)
    {
        std::string runtime_func_name = "_lfortran_str_len";
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getInt32Ty(context), {
                        character_type->getPointerTo()
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        return builder->CreateCall(fn, {str});
    }


    // This function is called as:
    // float complex_re(complex a)
    // And it extracts the real part of the complex number
    llvm::Value *complex_re(llvm::Value *c, llvm::Type* complex_type=nullptr) {
        if( complex_type == nullptr ) {
            complex_type = complex_type_4;
        }
        if( c->getType()->isPointerTy() ) {
            c = CreateLoad(c);
        }
        llvm::AllocaInst *pc = builder->CreateAlloca(complex_type, nullptr);
        builder->CreateStore(c, pc);
        std::vector<llvm::Value *> idx = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, 0))};
        llvm::Value *pim = CreateGEP(pc, idx);
        return CreateLoad(pim);
    }

    llvm::Value *complex_im(llvm::Value *c, llvm::Type* complex_type=nullptr) {
        if( complex_type == nullptr ) {
            complex_type = complex_type_4;
        }
        llvm::AllocaInst *pc = builder->CreateAlloca(complex_type, nullptr);
        builder->CreateStore(c, pc);
        std::vector<llvm::Value *> idx = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, 1))};
        llvm::Value *pim = CreateGEP(pc, idx);
        return CreateLoad(pim);
    }

    llvm::Value *complex_from_floats(llvm::Value *re, llvm::Value *im,
                                     llvm::Type* complex_type=nullptr) {
        if( complex_type == nullptr ) {
            complex_type = complex_type_4;
        }
        llvm::AllocaInst *pres = builder->CreateAlloca(complex_type, nullptr);
        std::vector<llvm::Value *> idx1 = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, 0))};
        std::vector<llvm::Value *> idx2 = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, 1))};
        llvm::Value *pre = CreateGEP(pres, idx1);
        llvm::Value *pim = CreateGEP(pres, idx2);
        builder->CreateStore(re, pre);
        builder->CreateStore(im, pim);
        return CreateLoad(pres);
    }

    llvm::Value *nested_struct_rd(std::vector<llvm::Value*> vals,
            llvm::StructType* rd) {
        llvm::AllocaInst *pres = builder->CreateAlloca(rd, nullptr);
        llvm::Value *pim = CreateGEP(pres, vals);
        return CreateLoad(pim);
    }

    /**
     * @brief This function generates the
     * @detail This is converted to
     *
     *     float lfortran_KEY(float *x)
     *
     *   Where KEY can be any of the supported intrinsics; this is then
     *   transformed into a runtime call:
     *
     *     void _lfortran_KEY(float x, float *result)
     */
    llvm::Value* lfortran_intrinsic(llvm::Function *fn, llvm::Value* pa, int a_kind)
    {
        llvm::Type *presult_type = getFPType(a_kind);
        llvm::AllocaInst *presult = builder->CreateAlloca(presult_type, nullptr);
        llvm::Value *a = CreateLoad(pa);
        std::vector<llvm::Value*> args = {a, presult};
        builder->CreateCall(fn, args);
        return CreateLoad(presult);
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        module = std::make_unique<llvm::Module>("LFortran", context);
        module->setDataLayout("");


        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);

        // Define LLVM types that we might need
        // Complex type is represented as an identified struct in LLVM
        // %complex = type { float, float }
        std::vector<llvm::Type*> els_4 = {
            llvm::Type::getFloatTy(context),
            llvm::Type::getFloatTy(context)};
        std::vector<llvm::Type*> els_8 = {
            llvm::Type::getDoubleTy(context),
            llvm::Type::getDoubleTy(context)};
        std::vector<llvm::Type*> els_4_ptr = {
            llvm::Type::getFloatPtrTy(context),
            llvm::Type::getFloatPtrTy(context)};
        std::vector<llvm::Type*> els_8_ptr = {
            llvm::Type::getDoublePtrTy(context),
            llvm::Type::getDoublePtrTy(context)};
        complex_type_4 = llvm::StructType::create(context, els_4, "complex_4");
        complex_type_8 = llvm::StructType::create(context, els_8, "complex_8");
        complex_type_4_ptr = llvm::StructType::create(context, els_4_ptr, "complex_4_ptr");
        complex_type_8_ptr = llvm::StructType::create(context, els_8_ptr, "complex_8_ptr");
        character_type = llvm::Type::getInt8PtrTy(context);

        llvm::Type* size_arg = (llvm::Type*)llvm::StructType::create(context, std::vector<llvm::Type*>({
                                                                                   arr_descr->get_dimension_descriptor_type(true),
                                                                                    getIntType(4)}), "size_arg");
        fname2arg_type["size"] = std::make_pair(size_arg, size_arg->getPointerTo());
        llvm::Type* bound_arg = static_cast<llvm::Type*>(arr_descr->get_dimension_descriptor_type(true));
        fname2arg_type["lbound"] = std::make_pair(bound_arg, bound_arg->getPointerTo());
        fname2arg_type["ubound"] = std::make_pair(bound_arg, bound_arg->getPointerTo());

        // Process Variables first:
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }

        prototype_only = false;
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Module_t>(*item.second) &&
                item.first.find("lfortran_intrinsic_optimization") != std::string::npos) {
                ASR::Module_t* mod = ASR::down_cast<ASR::Module_t>(item.second);
                for( auto &moditem: mod->m_symtab->scope ) {
                    ASR::symbol_t* sym = ASRUtils::symbol_get_past_external(moditem.second);
                    if (is_a<ASR::Subroutine_t>(*sym)) {
                        visit_Subroutine(*ASR::down_cast<ASR::Subroutine_t>(sym));
                    } else if (is_a<ASR::Function_t>(*sym)) {
                        visit_Function(*ASR::down_cast<ASR::Function_t>(sym));
                    }
                }
            }
        }

        prototype_only = true;
        // Generate function prototypes
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_Function(*ASR::down_cast<ASR::Function_t>(item.second));
            }
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                visit_Subroutine(*ASR::down_cast<ASR::Subroutine_t>(item.second));
            }
        }
        prototype_only = false;

        // TODO: handle depencencies across modules and main program

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = determine_module_dependencies(x);
        for (auto &item : build_order) {
            LFORTRAN_ASSERT(x.m_global_scope->scope.find(item)
                != x.m_global_scope->scope.end());
            ASR::symbol_t *mod = x.m_global_scope->scope[item];
            visit_symbol(*mod);
        }

        // Then do all the procedures
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Function_t>(*item.second)
                || is_a<ASR::Subroutine_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }

        // Then the main program
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }
    }

    void visit_Allocate(const ASR::Allocate_t& x) {
        for( size_t i = 0; i < x.n_args; i++ ) {
            ASR::alloc_arg_t curr_arg = x.m_args[i];
            std::uint32_t h = get_hash((ASR::asr_t*)curr_arg.m_a);
            LFORTRAN_ASSERT(llvm_symtab.find(h) != llvm_symtab.end());
            llvm::Value* x_arr = llvm_symtab[h];
            fill_malloc_array_details(x_arr, curr_arg.m_dims, curr_arg.n_dims);
        }
        if (x.m_stat) {
            ASR::Variable_t *asr_target = EXPR2VAR(x.m_stat);
            uint32_t h = get_hash((ASR::asr_t*)asr_target);
            if (llvm_symtab.find(h) != llvm_symtab.end()) {
                llvm::Value *target, *value;
                target = llvm_symtab[h];
                // Store 0 (success) in the stat variable
                value = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
                builder->CreateStore(value, target);
            } else {
                throw CodeGenError("Stat variable in allocate not found in LLVM symtab");
            }
        }
    }

    void visit_Nullify(const ASR::Nullify_t& x) {
        for( size_t i = 0; i < x.n_vars; i++ ) {
            std::uint32_t h = get_hash((ASR::asr_t*)x.m_vars[i]);
            llvm::Value *target = llvm_symtab[h];
            llvm::Type* tp = target->getType()->getContainedType(0);
            llvm::Value* np = builder->CreateIntToPtr(
                llvm::ConstantInt::get(context, llvm::APInt(32, 0)), tp);
            builder->CreateStore(np, target);
        }
    }

    inline void call_lfortran_free(llvm::Function* fn) {
        llvm::Value* arr = CreateLoad(arr_descr->get_pointer_to_data(tmp));
        llvm::AllocaInst *arg_arr = builder->CreateAlloca(character_type, nullptr);
        builder->CreateStore(builder->CreateBitCast(arr, character_type), arg_arr);
        std::vector<llvm::Value*> args = {CreateLoad(arg_arr)};
        builder->CreateCall(fn, args);
        arr_descr->set_is_allocated_flag(tmp, 0);
    }

    template <typename T>
    void _Deallocate(const T& x) {
        std::string func_name = "_lfortran_free";
        llvm::Function *free_fn = module->getFunction(func_name);
        if (!free_fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {
                        character_type
                    }, true);
            free_fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, func_name, *module);
        }
        for( size_t i = 0; i < x.n_vars; i++ ) {
            const ASR::symbol_t* curr_obj = x.m_vars[i];
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(
                                    symbol_get_past_external(curr_obj));
            fetch_var(v);
            if( x.class_type == ASR::stmtType::ImplicitDeallocate ) {
                llvm::Value *cond = arr_descr->get_is_allocated_flag(tmp);
                llvm::Function *fn = builder->GetInsertBlock()->getParent();
                llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
                llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
                llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
                builder->CreateCondBr(cond, thenBB, elseBB);
                builder->SetInsertPoint(thenBB);
                //print_util(cond, "%d");
                call_lfortran_free(free_fn);
                builder->CreateBr(mergeBB);
                start_new_block(elseBB);
                start_new_block(mergeBB);
            } else {
                call_lfortran_free(free_fn);
            }
        }
    }

    void visit_ImplicitDeallocate(const ASR::ImplicitDeallocate_t& x) {
        _Deallocate<ASR::ImplicitDeallocate_t>(x);
    }

    void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t& x) {
        _Deallocate<ASR::ExplicitDeallocate_t>(x);
    }

    void visit_ArrayRef(const ASR::ArrayRef_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(x.m_v);
        uint32_t v_h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(llvm_symtab.find(v_h) != llvm_symtab.end());
        llvm::Value* array = llvm_symtab[v_h];
        if (is_a<ASR::Character_t>(*x.m_type)
             && ASR::down_cast<ASR::Character_t>(x.m_type)->n_dims == 0) {
            // String indexing:
            if (x.n_args == 1) {
                LFORTRAN_ASSERT(x.m_args[0].m_left)
                LFORTRAN_ASSERT(x.m_args[0].m_right)
                if (ASR::is_a<ASR::Var_t>(*x.m_args[0].m_left)
                  &&ASR::is_a<ASR::Var_t>(*x.m_args[0].m_right)) {
                    ASR::Variable_t *l = EXPR2VAR(x.m_args[0].m_left);
                    ASR::Variable_t *r = EXPR2VAR(x.m_args[0].m_right);
                    if (l == r) {
                        this->visit_expr_wrapper(x.m_args[0].m_left, true);
                        llvm::Value *idx = tmp;
                        idx = builder->CreateSub(idx, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                        //std::vector<llvm::Value*> idx_vec = {llvm::ConstantInt::get(context, llvm::APInt(32, 0)), idx};
                        std::vector<llvm::Value*> idx_vec = {idx};
                        llvm::Value *str = CreateLoad(array);
                        llvm::Value *p = CreateGEP(str, idx_vec);
                        // TODO: Currently the string starts at the right location, but goes to the end of the original string.
                        // We have to allocate a new string, copy it and add null termination.

                        tmp = builder->CreateAlloca(character_type, nullptr);
                        builder->CreateStore(p, tmp);

                        //tmp = p;
                    } else {
                        throw CodeGenError("Only string(a:b) for a==b supported for now.", x.base.base.loc);
                    }
                } else {
                    //throw CodeGenError("Only string(a:b) for a,b variables for now.", x.base.base.loc);
                    // Use the "right" index for now
                    this->visit_expr_wrapper(x.m_args[0].m_right, true);
                    llvm::Value *idx = tmp;
                    idx = builder->CreateSub(idx, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                    //std::vector<llvm::Value*> idx_vec = {llvm::ConstantInt::get(context, llvm::APInt(32, 0)), idx};
                    std::vector<llvm::Value*> idx_vec = {idx};
                    llvm::Value *str = CreateLoad(array);
                    llvm::Value *p = CreateGEP(str, idx_vec);
                    // TODO: Currently the string starts at the right location, but goes to the end of the original string.
                    // We have to allocate a new string, copy it and add null termination.

                    tmp = builder->CreateAlloca(character_type, nullptr);
                    builder->CreateStore(p, tmp);
                }
            } else {
                throw CodeGenError("Only string(a:b) supported for now.", x.base.base.loc);
            }
        } else {
            // Array indexing:
            std::vector<llvm::Value*> indices;
            for( size_t r = 0; r < x.n_args; r++ ) {
                ASR::array_index_t curr_idx = x.m_args[r];
                this->visit_expr_wrapper(curr_idx.m_right, true);
                indices.push_back(tmp);
            }
            tmp = arr_descr->get_single_element(array, indices, x.n_args);
        }
    }

    void visit_DerivedRef(const ASR::DerivedRef_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        der_type_name = "";
        this->visit_expr(*x.m_v);
        ASR::Variable_t* member = down_cast<ASR::Variable_t>(symbol_get_past_external(x.m_m));
        std::string member_name = std::string(member->m_name);
        LFORTRAN_ASSERT(der_type_name.size() != 0);
        while( name2memidx[der_type_name].find(member_name) == name2memidx[der_type_name].end() ) {
            if( dertype2parent.find(der_type_name) == dertype2parent.end() ) {
                throw CodeGenError(der_type_name + " doesn't have any member named " + member_name,
                                    x.base.base.loc);
            }
            tmp = llvm_utils->create_gep(tmp, 0);
            der_type_name = dertype2parent[der_type_name];
        }
        int member_idx = name2memidx[der_type_name][member_name];
        std::vector<llvm::Value*> idx_vec = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, member_idx))};
        llvm::Value* tmp1 = CreateGEP(tmp, idx_vec);
        if( member->m_type->type == ASR::ttypeType::Derived ) {
            ASR::Derived_t* der = (ASR::Derived_t*)(&(member->m_type->base));
            ASR::DerivedType_t* der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
            der_type_name = std::string(der_type->m_name);
            uint32_t h = get_hash((ASR::asr_t*)member);
            if( llvm_symtab.find(h) != llvm_symtab.end() ) {
                tmp = llvm_symtab[h];
            }
        }
        tmp = tmp1;
    }

    void visit_Variable(const ASR::Variable_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        uint32_t h = get_hash((ASR::asr_t*)&x);
        // This happens at global scope, so the intent can only be either local
        // (global variable declared/initialized in this translation unit), or
        // external (global variable not declared/initialized in this
        // translation unit, just referenced).
        LFORTRAN_ASSERT(x.m_intent == intent_local
            || x.m_abi == ASR::abiType::Interactive);
        bool external = (x.m_abi != ASR::abiType::Source);
        llvm::Constant* init_value = nullptr;
        if (x.m_symbolic_value != nullptr){
            this->visit_expr_wrapper(x.m_symbolic_value, true);
            init_value = llvm::dyn_cast<llvm::Constant>(tmp);
        }
        if (x.m_type->type == ASR::ttypeType::Integer) {
            int a_kind = down_cast<ASR::Integer_t>(x.m_type)->m_kind;
            llvm::Type *type;
            int init_value_bits = 8*a_kind;
            type = getIntType(a_kind);
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                type);
            if (!external) {
                if (init_value) {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            init_value);
                } else {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            llvm::ConstantInt::get(context,
                                llvm::APInt(init_value_bits, 0)));
                }
            }
            llvm_symtab[h] = ptr;
        } else if (x.m_type->type == ASR::ttypeType::Real) {
            int a_kind = down_cast<ASR::Real_t>(x.m_type)->m_kind;
            llvm::Type *type;
            int init_value_bits = 8*a_kind;
            type = getFPType(a_kind);
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name, type);
            if (!external) {
                if (init_value) {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            init_value);
                } else {
                    if( init_value_bits == 32 ) {
                        module->getNamedGlobal(x.m_name)->setInitializer(
                                llvm::ConstantFP::get(context,
                                    llvm::APFloat((float)0)));
                    } else if( init_value_bits == 64 ) {
                        module->getNamedGlobal(x.m_name)->setInitializer(
                                llvm::ConstantFP::get(context,
                                    llvm::APFloat((double)0)));
                    }
                }
            }
            llvm_symtab[h] = ptr;
        } else if (x.m_type->type == ASR::ttypeType::Logical) {
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                llvm::Type::getInt1Ty(context));
            if (!external) {
                if (init_value) {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            init_value);
                } else {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            llvm::ConstantInt::get(context,
                                llvm::APInt(1, 0)));
                }
            }
            llvm_symtab[h] = ptr;
        } else {
            throw CodeGenError("Variable type not supported");
        }
    }

    void start_module_init_function_prototype(const ASR::Module_t &x) {
        uint32_t h = get_hash((ASR::asr_t*)&x);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), {}, false);
        LFORTRAN_ASSERT(llvm_symtab_fn.find(h) == llvm_symtab_fn.end());
        std::string module_fn_name = "__lfortran_module_init_" + std::string(x.m_name);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, module_fn_name, module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context, ".entry", F);
        builder->SetInsertPoint(BB);

        llvm_symtab_fn[h] = F;
    }

    void finish_module_init_function_prototype(const ASR::Module_t &x) {
        uint32_t h = get_hash((ASR::asr_t*)&x);
        builder->CreateRetVoid();
        llvm_symtab_fn[h]->removeFromParent();
    }

    void visit_Module(const ASR::Module_t &x) {
        mangle_prefix = "__module_" + std::string(x.m_name) + "_";

        start_module_init_function_prototype(x);

        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(
                        item.second);
                visit_Variable(*v);
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *v = down_cast<ASR::Function_t>(
                        item.second);
                instantiate_function(*v);
                declare_needed_global_types(*v);
            }
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *v = down_cast<ASR::Subroutine_t>(
                        item.second);
                instantiate_subroutine(*v);
                declare_needed_global_types(*v);
            }
        }
        finish_module_init_function_prototype(x);

        visit_procedures(x);
        mangle_prefix = "";
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *v = down_cast<ASR::Function_t>(
                        item.second);
                instantiate_function(*v);
                declare_needed_global_types(*v);
            } else if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *v = down_cast<ASR::Subroutine_t>(
                        item.second);
                instantiate_subroutine(*v);
                declare_needed_global_types(*v);
            }
        }
        declare_needed_global_types(x);
        visit_procedures(x);

        // Generate code for the main program
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context), {}, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, "main", module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);
        declare_vars(x);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        llvm::Value *ret_val2 = llvm::ConstantInt::get(context,
            llvm::APInt(32, 0));
        builder->CreateRet(ret_val2);
    }

    /*
    * This function detects if the current variable is an argument.
    * of a function or argument. Some manipulations are to be done
    * only on arguments and not on local variables.
    */
    bool is_argument(ASR::Variable_t* v, ASR::expr_t** m_args,
                        int n_args) {
        for( int i = 0; i < n_args; i++ ) {
            ASR::expr_t* m_arg = m_args[i];
            uint32_t h_m_arg = get_hash((ASR::asr_t*)m_arg);
            uint32_t h_v = get_hash((ASR::asr_t*)v);
            if( h_m_arg == h_v ) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    void declare_vars(const T &x) {
        llvm::Value *target_var;
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);
                uint32_t h = get_hash((ASR::asr_t*)v);
                llvm::Type *type;
                ASR::ttype_t* m_type_;
                int n_dims = 0, a_kind = 4;
                ASR::dimension_t* m_dims = nullptr;
                bool is_array_type = false;
                bool is_malloc_array_type = false;
                if (v->m_intent == intent_local ||
                    v->m_intent == intent_return_var ||
                    !v->m_intent) {
                    switch (v->m_type->type) {
                        case (ASR::ttypeType::Integer) : {
                            ASR::Integer_t* v_type = down_cast<ASR::Integer_t>(v->m_type);
                            m_type_ = v->m_type;
                            m_dims = v_type->m_dims;
                            n_dims = v_type->n_dims;
                            a_kind = v_type->m_kind;
                            if( n_dims > 0 ) {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    is_malloc_array_type = true;
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type);
                                }
                            } else {
                                type = getIntType(a_kind);
                            }
                            break;
                        }
                        case (ASR::ttypeType::Real) : {
                            ASR::Real_t* v_type = down_cast<ASR::Real_t>(v->m_type);
                            m_type_ = v->m_type;
                            m_dims = v_type->m_dims;
                            n_dims = v_type->n_dims;
                            a_kind = v_type->m_kind;
                            if( n_dims > 0 ) {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    is_malloc_array_type = true;
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type);
                                }
                            } else {
                                type = getFPType(a_kind);
                            }
                            break;
                        }
                        case (ASR::ttypeType::Complex) : {
                            ASR::Complex_t* v_type = down_cast<ASR::Complex_t>(v->m_type);
                            m_type_ = v->m_type;
                            m_dims = v_type->m_dims;
                            n_dims = v_type->n_dims;
                            a_kind = v_type->m_kind;
                            if( n_dims > 0 ) {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    is_malloc_array_type = true;
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type);
                                }
                            } else {
                                type = getComplexType(a_kind);
                            }
                            break;
                        }
                        case (ASR::ttypeType::Character) : {
                            ASR::Character_t* v_type = down_cast<ASR::Character_t>(v->m_type);
                            m_type_ = v->m_type;
                            m_dims = v_type->m_dims;
                            n_dims = v_type->n_dims;
                            a_kind = v_type->m_kind;
                            if( n_dims > 0 ) {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    is_malloc_array_type = true;
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type);
                                }
                            } else {
                                type = character_type;
                            }
                            break;
                        }
                        case (ASR::ttypeType::Logical) : {
                            ASR::Logical_t* v_type = down_cast<ASR::Logical_t>(v->m_type);
                            m_type_ = v->m_type;
                            m_dims = v_type->m_dims;
                            n_dims = v_type->n_dims;
                            a_kind = v_type->m_kind;
                            if( n_dims > 0 ) {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    is_malloc_array_type = true;
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type);
                                }
                            } else {
                                type = llvm::Type::getInt1Ty(context);
                            }
                            break;
                        }
                        case (ASR::ttypeType::Derived) : {
                            ASR::Derived_t* v_type = down_cast<ASR::Derived_t>(v->m_type);
                            m_type_ = v->m_type;
                            m_dims = v_type->m_dims;
                            n_dims = v_type->n_dims;
                            if( n_dims > 0 ) {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    is_malloc_array_type = true;
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type);
                                }
                            } else {
                                type = getDerivedType(m_type_, false);
                            }
                            break;
                        }
                        case (ASR::ttypeType::Pointer) : {
                            ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(v->m_type)->m_type;
                            switch (t2->type) {
                                case (ASR::ttypeType::Integer) : {
                                    a_kind = down_cast<ASR::Integer_t>(t2)->m_kind;
                                    type = getIntType(a_kind, true);
                                    break;
                                }
                                case (ASR::ttypeType::Real) : {
                                    a_kind = down_cast<ASR::Real_t>(t2)->m_kind;
                                    type = getFPType(a_kind, true);
                                    break;
                                }
                                case (ASR::ttypeType::Complex) : {
                                    a_kind = down_cast<ASR::Complex_t>(t2)->m_kind;
                                    type = getComplexType(a_kind, true);
                                    break;
                                }
                                case (ASR::ttypeType::Character) : {
                                    type = character_type;
                                    break;
                                }
                                case (ASR::ttypeType::Derived) : {
                                    throw CodeGenError("Pointers for Derived type not implemented yet in conversion");
                                }
                                case (ASR::ttypeType::Logical) : {
                                    type = llvm::Type::getInt1Ty(context);
                                    break;
                                }
                                default :
                                    throw CodeGenError("Type not implemented");
                            }
                            break;
                        }
                        default :
                            throw CodeGenError("Type not implemented");
                    }
                    /*
                    * The following if block is used for converting any
                    * general array descriptor to a pointer type which
                    * can be passed as an argument in a function call in LLVM IR.
                    */
                    if( x.class_type == ASR::symbolType::Function ||
                        x.class_type == ASR::symbolType::Subroutine ) {
                        std::uint32_t m_h;
                        std::string m_name = std::string(x.m_name);
                        ASR::abiType abi_type = ASR::abiType::Source;
                        bool is_v_arg = false;
                        if( x.class_type == ASR::symbolType::Function ) {
                            ASR::Function_t* _func = (ASR::Function_t*)(&(x.base));
                            m_h = get_hash((ASR::asr_t*)_func);
                            abi_type = _func->m_abi;
                            is_v_arg = is_argument(v, _func->m_args, _func->n_args);
                        } else if( x.class_type == ASR::symbolType::Subroutine ) {
                            ASR::Subroutine_t* _sub = (ASR::Subroutine_t*)(&(x.base));
                            abi_type = _sub->m_abi;
                            m_h = get_hash((ASR::asr_t*)_sub);
                            is_v_arg = is_argument(v, _sub->m_args, _sub->n_args);
                        }
                        if( is_array_type ) {
                            /* The first element in an array descriptor can be either of
                            * llvm::ArrayType or llvm::PointerType. However, a
                            * function only accepts llvm::PointerType for arrays. Hence,
                            * the following if block extracts the pointer to first element
                            * of an array from its descriptor. Note that this happens only
                            * for arguments and not for local function variables.
                            */
                            if( abi_type == ASR::abiType::Source && is_v_arg ) {
                                type = arr_descr->get_argument_type(type, m_h, v->m_name, arr_arg_type_cache);
                                is_array_type = false;
                            } else if( abi_type == ASR::abiType::Intrinsic &&
                                fname2arg_type.find(m_name) != fname2arg_type.end() ) {
                                type = fname2arg_type[m_name].second;
                                is_array_type = false;
                            }
                        }
                    }
                    llvm::AllocaInst *ptr = builder->CreateAlloca(type, nullptr, v->m_name);
                    llvm_symtab[h] = ptr;
                    if( is_malloc_array_type ) {
                        arr_descr->fill_dimension_descriptor(ptr, n_dims);
                    }
                    if( is_array_type && !is_malloc_array_type ) {
                        fill_array_details(ptr, m_dims, n_dims);
                    }
                    if( is_array_type && is_malloc_array_type ) {
                        // Set allocatable arrays as unallocated
                        arr_descr->set_is_allocated_flag(ptr, 0);
                    }
                    if( v->m_symbolic_value != nullptr ) {
                        target_var = ptr;
                        this->visit_expr_wrapper(v->m_symbolic_value, true);
                        llvm::Value *init_value = tmp;
                        builder->CreateStore(init_value, target_var);
                        auto finder = std::find(nested_globals.begin(),
                                nested_globals.end(), h);
                        if (finder != nested_globals.end()) {
                            llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                                    nested_global_struct);
                            int idx = std::distance(nested_globals.begin(),
                                    finder);
                            builder->CreateStore(target_var, llvm_utils->create_gep(ptr,
                                        idx));
                        }
                    } else {
                        if (is_a<ASR::Character_t>(*v->m_type) && !is_array_type) {
                            ASR::Character_t *t = down_cast<ASR::Character_t>(v->m_type);
                            target_var = ptr;
                            int strlen = t->m_len;
                            if (strlen >= 0) {
                                // Compile time length
                                std::string empty(strlen, ' ');
                                llvm::Value *init_value = builder->CreateGlobalStringPtr(s2c(al, empty));
                                builder->CreateStore(init_value, target_var);
                            } else if (strlen == -2) {
                                // Allocatable string. Initialize to `nullptr` (unallocated)
                                llvm::Value *init_value = llvm::Constant::getNullValue(type);
                                builder->CreateStore(init_value, target_var);
                            } else if (strlen == -3) {
                                LFORTRAN_ASSERT(t->m_len_expr)
                                this->visit_expr(*t->m_len_expr);
                                llvm::Value *arg_size = tmp;
                                arg_size = builder->CreateAdd(arg_size, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                                // TODO: this temporary string is never deallocated (leaks memory)
                                llvm::Value *init_value = LLVMArrUtils::lfortran_malloc(context, *module, *builder, arg_size);
                                string_init(context, *module, *builder, arg_size, init_value);
                                builder->CreateStore(init_value, target_var);
                            } else {
                                throw CodeGenError("Unsupported len value in ASR");
                            }
                        }
                    }
                }
            }
        }
    }

    template <typename T>
    std::vector<llvm::Type*> convert_args(const T &x) {
        std::vector<llvm::Type*> args;
        for (size_t i=0; i<x.n_args; i++) {
            if (is_a<ASR::Variable_t>(*symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
                LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
                // We pass all arguments as pointers for now,
                // except bind(C) value arguments that are passed by value
                llvm::Type *type;
                ASR::ttype_t* m_type_;
                int n_dims = 0, a_kind = 4;
                bool is_array_type = false;
                ASR::Variable_t* v = arg;
                switch (arg->m_type->type) {
                    case (ASR::ttypeType::Integer) : {
                        ASR::Integer_t* v_type = down_cast<ASR::Integer_t>(arg->m_type);
                        m_type_ = arg->m_type;
                        n_dims = v_type->n_dims;
                        a_kind = v_type->m_kind;
                        if( n_dims > 0 ) {
                            if (x.m_abi == ASR::abiType::BindC) {
                                // Bind(C) arrays are represened as a pointer
                                type = getIntType(a_kind, true);
                            } else {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type, true);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type, true);
                                }
                            }
                        } else {
                            if (arg->m_abi == ASR::abiType::BindC
                                && arg->m_value_attr) {
                                type = getIntType(a_kind, false);
                            } else {
                                type = getIntType(a_kind, true);
                            }
                        }
                        break;
                    }
                    case (ASR::ttypeType::Pointer) : {
                        ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(arg->m_type);
                        switch (t2->type) {
                            case (ASR::ttypeType::Integer) : {
                                ASR::Integer_t* v_type = down_cast<ASR::Integer_t>(t2);
                                m_type_ = arg->m_type;
                                n_dims = v_type->n_dims;
                                a_kind = v_type->m_kind;
                                type = getIntType(a_kind, true);
                                break;
                            }
                            default:
                                throw CodeGenError("Type not implemented");
                        }
                        break;
                    }
                    case (ASR::ttypeType::Real) : {
                        ASR::Real_t* v_type = down_cast<ASR::Real_t>(arg->m_type);
                        m_type_ = arg->m_type;
                        n_dims = v_type->n_dims;
                        a_kind = v_type->m_kind;
                        if( n_dims > 0 ) {
                            if (x.m_abi == ASR::abiType::BindC) {
                                // Bind(C) arrays are represened as a pointer
                                type = getFPType(a_kind, true);
                            } else {
                                is_array_type = true;
                                llvm::Type* el_type = get_el_type(m_type_, a_kind);
                                if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                    type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type, true);
                                } else {
                                    type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type, true);
                                }
                            }
                        } else {
                            if (arg->m_abi == ASR::abiType::BindC
                                && arg->m_value_attr) {
                                type = getFPType(a_kind, false);
                            } else {
                                type = getFPType(a_kind, true);
                            }
                        }
                        break;
                    }
                    case (ASR::ttypeType::Complex) : {
                        ASR::Complex_t* v_type = down_cast<ASR::Complex_t>(arg->m_type);
                        m_type_ = arg->m_type;
                        n_dims = v_type->n_dims;
                        a_kind = v_type->m_kind;
                        if( n_dims > 0 ) {
                            is_array_type = true;
                            llvm::Type* el_type = get_el_type(m_type_, a_kind);
                            if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type, true);
                            } else {
                                type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type, true);
                            }
                        } else {
                            if (arg->m_abi == ASR::abiType::BindC
                                    && arg->m_value_attr) {
                                if (a_kind == 4) {
                                    if (platform == Platform::Windows) {
                                        // type_fx2 is i64
                                        llvm::Type* type_fx2 = llvm::Type::getInt64Ty(context);
                                        type = type_fx2;
                                    } else if (platform == Platform::macOS_ARM) {
                                        // type_fx2 is [2 x float]
                                        llvm::Type* type_fx2 = llvm::ArrayType::get(llvm::Type::getFloatTy(context), 2);
                                        type = type_fx2;
                                    } else {
                                        // type_fx2 is <2 x float>
                                        llvm::Type* type_fx2 = llvm::FixedVectorType::get(llvm::Type::getFloatTy(context), 2);
                                        type = type_fx2;
                                    }
                                } else {
                                    LFORTRAN_ASSERT(a_kind == 8)
                                    if (platform == Platform::Windows) {
                                        // 128 bit aggregate type is passed by reference
                                        type = getComplexType(a_kind, true);
                                    } else {
                                        // Pass by value
                                        type = getComplexType(a_kind, false);
                                    }
                                }
                            } else {
                                type = getComplexType(a_kind, true);
                            }
                        }
                        break;
                    }
                    case (ASR::ttypeType::Character) :
                        if (arg->m_abi == ASR::abiType::BindC) {
                            type = character_type;
                        } else {
                            type = character_type->getPointerTo();
                        }
                        break;
                    case (ASR::ttypeType::Logical) : {
                        ASR::Logical_t* v_type = down_cast<ASR::Logical_t>(arg->m_type);
                        m_type_ = arg->m_type;
                        n_dims = v_type->n_dims;
                        a_kind = v_type->m_kind;
                        if( n_dims > 0 ) {
                            is_array_type = true;
                            llvm::Type* el_type = get_el_type(m_type_, a_kind);
                            if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type, true);
                            } else {
                                type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type, true);
                            }
                        } else {
                            type = llvm::Type::getInt1PtrTy(context);
                        }
                        break;
                    }
                    case (ASR::ttypeType::Derived) : {
                        ASR::Derived_t* v_type = down_cast<ASR::Derived_t>(arg->m_type);
                        m_type_ = arg->m_type;
                        n_dims = v_type->n_dims;
                        if( n_dims > 0 ) {
                            is_array_type = true;
                            llvm::Type* el_type = get_el_type(m_type_, a_kind);
                            if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type, true);
                            } else {
                                type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type, true);
                            }
                        } else {
                            type = getDerivedType(arg->m_type, true);
                        }
                        break;
                    }
                    case (ASR::ttypeType::Class) : {
                        ASR::Class_t* v_type = down_cast<ASR::Class_t>(arg->m_type);
                        m_type_ = arg->m_type;
                        n_dims = v_type->n_dims;
                        if( n_dims > 0 ) {
                            is_array_type = true;
                            llvm::Type* el_type = get_el_type(m_type_, a_kind);
                            if( v->m_storage == ASR::storage_typeType::Allocatable ) {
                                type = arr_descr->get_malloc_array_type(m_type_, a_kind, n_dims, el_type, true);
                            } else {
                                type = arr_descr->get_array_type(m_type_, a_kind, n_dims, el_type, true);
                            }
                        } else {
                            type = getClassType(arg->m_type, true);
                        }
                        break;
                    }
                    default :
                        LFORTRAN_ASSERT(false);
                }
                std::uint32_t m_h;
                std::string m_name = std::string(x.m_name);
                if( x.class_type == ASR::symbolType::Function ) {
                    ASR::Function_t* _func = (ASR::Function_t*)(&(x.base));
                    m_h = get_hash((ASR::asr_t*)_func);
                } else if( x.class_type == ASR::symbolType::Subroutine ) {
                    ASR::Subroutine_t* _sub = (ASR::Subroutine_t*)(&(x.base));
                    m_h = get_hash((ASR::asr_t*)_sub);
                }
                if( is_array_type ) {
                    if( x.m_abi == ASR::abiType::Source ) {
                        llvm::Type* orig_type = static_cast<llvm::PointerType*>(type)->getElementType();
                        type = arr_descr->get_argument_type(orig_type, m_h, arg->m_name, arr_arg_type_cache);
                        is_array_type = false;
                    } else if( x.m_abi == ASR::abiType::Intrinsic &&
                        fname2arg_type.find(m_name) != fname2arg_type.end()) {
                        type = fname2arg_type[m_name].second;
                        is_array_type = false;
                    }
                }
                args.push_back(type);
            } else if (is_a<ASR::Function_t>(*symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                /* This is likely a procedure passed as an argument. For the
                   type, we need to pass in a function pointer with the
                   correct call signature. */
                ASR::Function_t* fn = ASR::down_cast<ASR::Function_t>(
                    symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                    x.m_args[i])->m_v));
                llvm::Type* type = get_function_type(*fn)->getPointerTo();
                args.push_back(type);
            } else if (is_a<ASR::Subroutine_t>(*symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                ASR::Subroutine_t* fn = ASR::down_cast<ASR::Subroutine_t>(
                    symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                    x.m_args[i])->m_v));
                llvm::Type* type = get_subroutine_type(*fn)->getPointerTo();
                args.push_back(type);
            } else {
                throw CodeGenError("Argument type not implemented");
            }
        }
        return args;
    }


    template <typename T>
    void push_nested_stack(const T &x) {
        bool finder = std::find(nested_call_out.begin(), nested_call_out.end(),
             parent_function_hash) != nested_call_out.end();
        bool is_nested_call = std::find(
            nesting_map[parent_function_hash].begin(),
            nesting_map[parent_function_hash].end(), calling_function_hash)
            != nesting_map[parent_function_hash].end();
        if (nested_func_types[parent_function_hash].size() > 0
            && parent_function_hash != calling_function_hash && finder
            && !is_nested_call){
            llvm::Value *sp_loc = module->getOrInsertGlobal(
                nested_sp_name, llvm::Type::getInt32Ty(context));
            llvm::Value *sp_val = CreateLoad(sp_loc);
            for (auto &item : x->m_symtab->scope) {
                if (is_a<ASR::Variable_t>(*item.second)) {
                    ASR::Variable_t *v = down_cast<ASR::Variable_t>(
                        item.second);
                    uint32_t h = get_hash((ASR::asr_t*)v);
                    auto finder = std::find(nested_globals.begin(),
                            nested_globals.end(), h);
                    if (finder != nested_globals.end()) {
                        int idx = std::distance(nested_globals.begin(),
                            finder);
                        llvm::Value* glob_struct = module->getOrInsertGlobal(
                            nested_desc_name, nested_global_struct);
                        llvm::Value* target = CreateLoad(llvm_utils->create_gep(
                            glob_struct, idx));
                        llvm::Value* glob_stack = module->getOrInsertGlobal(
                            nested_stack_name, nested_global_stack);
                        llvm::Value *glob_stack_gep = llvm_utils->create_gep(glob_stack,
                            sp_val);
                        llvm::Value *glob_stack_elem = llvm_utils->create_gep(
                            glob_stack_gep, idx);
                        builder->CreateStore(CreateLoad(target),
                            glob_stack_elem);
                        llvm::Value *glob_stack_val = CreateLoad(
                            glob_stack_gep);
                        llvm::Value *glob_stack_sp = llvm_utils->create_gep(glob_stack,
                            sp_val);
                        builder->CreateStore(glob_stack_val, glob_stack_sp);
                    }
                }
            }
            builder->CreateStore(builder->CreateAdd(builder->getInt32(1),
                sp_val), sp_loc);
        }
    }


    template <typename T>
    void pop_nested_stack(const T &x) {
        llvm::Function *lfn = builder->GetInsertBlock()->getParent();
        bool finder = std::find(nested_call_out.begin(), nested_call_out.end(),
             calling_function_hash) != nested_call_out.end();
        bool is_nested_call = std::find(
            nesting_map[parent_function_hash].begin(),
            nesting_map[parent_function_hash].end(), calling_function_hash)
            != nesting_map[parent_function_hash].end();
        if (nested_func_types[calling_function_hash].size() > 0
            && calling_function_hash != parent_function_hash && finder
            && !is_nested_call){
            llvm::Value *sp_loc = module->getOrInsertGlobal(nested_sp_name,
                llvm::Type::getInt32Ty(context));
            llvm::Value *sp_val = CreateLoad(sp_loc);
            llvm::BasicBlock *dec_sp = llvm::BasicBlock::Create(context,
                "decrement_sp", lfn);
            llvm::BasicBlock *norm_cont = llvm::BasicBlock::Create(context,
                "normal_continue", lfn);
            llvm::Value *cond = builder->CreateICmpSGT(sp_val,
                builder->getInt32(0));
            builder->CreateCondBr(cond, dec_sp, norm_cont);
            builder->SetInsertPoint(dec_sp);
            builder->CreateStore(builder->CreateAdd(builder->getInt32(-1),
                sp_val), sp_loc);
            for (auto &item : x->m_symtab->scope) {
                if (is_a<ASR::Variable_t>(*item.second)) {
                    ASR::Variable_t *v = down_cast<ASR::Variable_t>(
                        item.second);
                    uint32_t h = get_hash((ASR::asr_t*)v);
                    auto finder = std::find(nested_globals.begin(),
                            nested_globals.end(), h);
                    if (finder != nested_globals.end()) {
                        int idx = std::distance(nested_globals.begin(),
                        finder);
                        llvm::Value* glob_stack = module->getOrInsertGlobal(
                            nested_stack_name, nested_global_stack);
                        llvm::Value *sp_loc = module->getOrInsertGlobal(
                            nested_sp_name, llvm::Type::getInt32Ty(context));
                        llvm::Value *sp_val = CreateLoad(sp_loc);
                        llvm::Value *glob_stack_elem = CreateLoad(
                            llvm_utils->create_gep(llvm_utils->create_gep(glob_stack, sp_val), idx));
                        llvm::Value *glob_struct_loc = module->
                            getOrInsertGlobal(nested_desc_name,
                            nested_global_struct);
                        llvm::Value* target = CreateLoad(
                            llvm_utils->create_gep(glob_struct_loc, idx));
                        builder->CreateStore(glob_stack_elem, target);
                        builder->CreateStore(target, llvm_utils->create_gep(
                            glob_struct_loc, idx));
                    }
                }
            }
            builder->CreateBr(norm_cont);
            builder->SetInsertPoint(norm_cont);
        }
    }

    template<typename T>
    void declare_args(const T &x, llvm::Function &F) {
        size_t i = 0;
        for (llvm::Argument &llvm_arg : F.args()) {
            if (is_a<ASR::Variable_t>(*symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
                LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
                uint32_t h = get_hash((ASR::asr_t*)arg);
                auto finder = std::find(nested_globals.begin(),
                    nested_globals.end(), h);
                if (finder != nested_globals.end()) {
                    llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                            nested_global_struct);
                    int idx = std::distance(nested_globals.begin(),
                            finder);
                    builder->CreateStore(&llvm_arg, llvm_utils->create_gep(ptr,
                                idx));
                }
                std::string arg_s = arg->m_name;
                llvm_arg.setName(arg_s);
                llvm_symtab[h] = &llvm_arg;
            } else if (is_a<ASR::Function_t>(*symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                // Deal with case where procedure passed in as argument
                ASR::Function_t *arg = EXPR2FUN(x.m_args[i]);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                std::string arg_s = arg->m_name;
                llvm_arg.setName(arg_s);
                llvm_symtab_fn_arg[h] = &llvm_arg;
            } else if (is_a<ASR::Subroutine_t>(*symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                // Deal with case where procedure passed in as argument
                ASR::Subroutine_t *arg = EXPR2SUB(x.m_args[i]);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                std::string arg_s = arg->m_name;
                llvm_arg.setName(arg_s);
                llvm_symtab_fn_arg[h] = &llvm_arg;
            }
            i++;
        }
    }

    template <typename T>
    void declare_local_vars(const T &x) {
        declare_vars(x);
    }

    void visit_Function(const ASR::Function_t &x) {
        instantiate_function(x);
        visit_procedures(x);
        generate_function(x);
        parent_function = nullptr;
    }


    void visit_Subroutine(const ASR::Subroutine_t &x) {
        if (x.m_abi != ASR::abiType::Source &&
            x.m_abi != ASR::abiType::Interactive &&
            x.m_abi != ASR::abiType::Intrinsic) {
                return;
        }
        instantiate_subroutine(x);
        visit_procedures(x);
        generate_subroutine(x);
        parent_subroutine = nullptr;
    }




    void instantiate_subroutine(const ASR::Subroutine_t &x){
        uint32_t h = get_hash((ASR::asr_t*)&x);
        llvm::Function *F = nullptr;
        if (llvm_symtab_fn.find(h) != llvm_symtab_fn.end()) {
            /*
            throw CodeGenError("Subroutine code already generated for '"
                + std::string(x.m_name) + "'");
            */
            F = llvm_symtab_fn[h];
        } else {
            llvm::FunctionType *function_type = get_subroutine_type(x);
            std::string fn_name;
            if (x.m_abi == ASR::abiType::BindC) {
                if (x.m_bindc_name) {
                    fn_name = x.m_bindc_name;
                } else {
                    fn_name = x.m_name;
                }
            } else {
                fn_name = mangle_prefix + x.m_name;
            }
            if (llvm_symtab_fn_names.find(fn_name) == llvm_symtab_fn_names.end()) {
                llvm_symtab_fn_names[fn_name] = h;
                F = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, fn_name, module.get());
            } else {
                uint32_t old_h = llvm_symtab_fn_names[fn_name];
                F = llvm_symtab_fn[old_h];
            }
            llvm_symtab_fn[h] = F;

            // Instantiate (pre-declare) all nested interfaces
            for (auto &item : x.m_symtab->scope) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *v = down_cast<ASR::Function_t>(
                            item.second);
                    instantiate_function(*v);
                }
                if (is_a<ASR::Subroutine_t>(*item.second)) {
                    ASR::Subroutine_t *v = down_cast<ASR::Subroutine_t>(
                            item.second);
                    instantiate_subroutine(*v);
                }
            }
        }
    }

    llvm::FunctionType* get_subroutine_type(const ASR::Subroutine_t &x){
        std::vector<llvm::Type*> args = convert_args(x);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), args, false);
        return function_type;
    }


    void generate_subroutine(const ASR::Subroutine_t &x){
        bool interactive = (x.m_abi == ASR::abiType::Interactive);
        if (x.m_deftype == ASR::deftypeType::Implementation) {

            if (interactive) return;

            if (!prototype_only) {
                define_subroutine_entry(x);

                for (size_t i=0; i<x.n_body; i++) {
                    this->visit_stmt(*x.m_body[i]);
                }

                define_subroutine_exit(x);
            }
        }
    }

    void instantiate_function(const ASR::Function_t &x){
        uint32_t h = get_hash((ASR::asr_t*)&x);
        llvm::Function *F = nullptr;
        if (llvm_symtab_fn.find(h) != llvm_symtab_fn.end()) {
            /*
            throw CodeGenError("Function code already generated for '"
                + std::string(x.m_name) + "'");
            */
            F = llvm_symtab_fn[h];
        } else {
            llvm::FunctionType* function_type = get_function_type(x);
            std::string fn_name;
            if (x.m_abi == ASR::abiType::BindC) {
                if (x.m_bindc_name) {
                    fn_name = x.m_bindc_name;
                } else {
                    fn_name = x.m_name;
                }
            } else {
                fn_name = mangle_prefix + x.m_name;
            }
            if (llvm_symtab_fn_names.find(fn_name) == llvm_symtab_fn_names.end()) {
                llvm_symtab_fn_names[fn_name] = h;
                F = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, fn_name, module.get());
            } else {
                uint32_t old_h = llvm_symtab_fn_names[fn_name];
                F = llvm_symtab_fn[old_h];
            }
            llvm_symtab_fn[h] = F;

            // Instantiate (pre-declare) all nested interfaces
            for (auto &item : x.m_symtab->scope) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *v = down_cast<ASR::Function_t>(
                            item.second);
                    instantiate_function(*v);
                }
                if (is_a<ASR::Subroutine_t>(*item.second)) {
                    ASR::Subroutine_t *v = down_cast<ASR::Subroutine_t>(
                            item.second);
                    instantiate_subroutine(*v);
                }
            }
        }
    }


    llvm::FunctionType* get_function_type(const ASR::Function_t &x){
        ASR::ttype_t *return_var_type0 = EXPR2VAR(x.m_return_var)->m_type;
        ASR::ttypeType return_var_type = return_var_type0->type;
        llvm::Type *return_type;
        switch (return_var_type) {
            case (ASR::ttypeType::Integer) : {
                int a_kind = down_cast<ASR::Integer_t>(return_var_type0)->m_kind;
                return_type = getIntType(a_kind);
                break;
            }
            case (ASR::ttypeType::Real) : {
                int a_kind = down_cast<ASR::Real_t>(return_var_type0)->m_kind;
                return_type = getFPType(a_kind);
                break;
            }
            case (ASR::ttypeType::Complex) : {
                int a_kind = down_cast<ASR::Complex_t>(return_var_type0)->m_kind;
                if (a_kind == 4) {
                    if (x.m_abi == ASR::abiType::BindC) {
                        if (platform == Platform::Windows) {
                            // i64
                            return_type = llvm::Type::getInt64Ty(context);
                        } else if (platform == Platform::macOS_ARM) {
                            // {float, float}
                            return_type = getComplexType(a_kind);
                        } else {
                            // <2 x float>
                            return_type = llvm::FixedVectorType::get(llvm::Type::getFloatTy(context), 2);
                        }
                    } else {
                        return_type = getComplexType(a_kind);
                    }
                } else {
                    LFORTRAN_ASSERT(a_kind == 8)
                    if (x.m_abi == ASR::abiType::BindC) {
                        if (platform == Platform::Windows) {
                            // pass as subroutine
                            return_type = getComplexType(a_kind, true);
                            std::vector<llvm::Type*> args = convert_args(x);
                            args.insert(args.begin(), return_type);
                            llvm::FunctionType *function_type = llvm::FunctionType::get(
                                    llvm::Type::getVoidTy(context), args, false);
                            return function_type;
                        } else {
                            return_type = getComplexType(a_kind);
                        }
                    } else {
                        return_type = getComplexType(a_kind);
                    }
                }
                break;
            }
            case (ASR::ttypeType::Character) :
                return_type = character_type;
                break;
            case (ASR::ttypeType::Logical) :
                return_type = llvm::Type::getInt1Ty(context);
                break;
            case (ASR::ttypeType::Derived) :
                throw CodeGenError("Derived return type not implemented yet");
                break;
            default :
                LFORTRAN_ASSERT(false);
                throw CodeGenError("Type not implemented");
        }
        std::vector<llvm::Type*> args = convert_args(x);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                return_type, args, false);
        return function_type;
    }

    template<typename T>
    void declare_needed_global_types(T &x){
        // Check if the procedure has a nested function that needs access to
        // some variables in its local scope
        uint32_t h = get_hash((ASR::asr_t*)&x);
        std::vector<llvm::Type*> nested_type;
        if (nested_func_types[h].size() > 0) {
            nested_type = nested_func_types[h];
            nested_global_struct = llvm::StructType::create(context,
                nested_type, std::string(x.m_name) + "_nstd_types");
            std::vector<llvm::Type*> nested_stack;
            for (size_t i = 0; i < nested_type.size(); i++){
                nested_stack.push_back(nested_type[i]->getContainedType(0));
            }
            nested_desc_name = std::string(x.m_name) + "_nstd_strct";
            module->getOrInsertGlobal(nested_desc_name, nested_global_struct);
            llvm::ConstantAggregateZero *initializer;
            initializer = llvm::ConstantAggregateZero::get(
                nested_global_struct);
            module->getNamedGlobal(nested_desc_name)->setInitializer(
                initializer);
            if (std::find(nested_call_out.begin(), nested_call_out.end(), h) !=
                nested_call_out.end()){
                /* Only declare the stack types if needed (if the function
                enclosing a nested function can call out, potentially leading to
                recursion, etc */
                nested_global_struct_vals =
                    llvm::StructType::create(context, nested_stack,
                    std::string(x.m_name) + "_vals");
                nested_global_stack = llvm::ArrayType::get(
                    nested_global_struct_vals, 1000);
                nested_stack_name = nested_desc_name + "_stack";
                nested_sp_name = "sp_" + std::string(x.m_name);
                llvm::IntegerType *sp = llvm::Type::getInt32Ty(context);
                module->getOrInsertGlobal(nested_stack_name,
                    nested_global_stack);
                module->getOrInsertGlobal(nested_sp_name, sp);
                initializer = llvm::ConstantAggregateZero::get(
                    nested_global_stack);
                module->getNamedGlobal(nested_stack_name)->setInitializer(
                    initializer);
                llvm::ConstantInt *sp_init = llvm::ConstantInt::get(
                    module->getContext(), llvm::APInt(32,0));
                module->getNamedGlobal(nested_sp_name)->setInitializer(
                    sp_init);
            }
        }
    }

    inline void define_function_entry(const ASR::Function_t& x) {
        uint32_t h = get_hash((ASR::asr_t*)&x);
        parent_function = &x;
        parent_function_hash = h;
        llvm::Function* F = llvm_symtab_fn[h];
        proc_return = llvm::BasicBlock::Create(context, "return");
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);
        declare_args(x, *F);
        declare_local_vars(x);
    }


    inline void define_subroutine_entry(const ASR::Subroutine_t& x) {
        uint32_t h = get_hash((ASR::asr_t*)&x);
        parent_subroutine = &x;
        parent_function_hash = h;
        llvm::Function* F = llvm_symtab_fn[h];
        proc_return = llvm::BasicBlock::Create(context, "return");
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);
        declare_args(x, *F);
        declare_local_vars(x);
    }

    inline void define_function_exit(const ASR::Function_t& x) {
        start_new_block(proc_return);
        ASR::Variable_t *asr_retval = EXPR2VAR(x.m_return_var);
        uint32_t h = get_hash((ASR::asr_t*)asr_retval);
        llvm::Value *ret_val = llvm_symtab[h];
        llvm::Value *ret_val2 = CreateLoad(ret_val);
        // Handle Complex type return value for BindC:
        if (x.m_abi == ASR::abiType::BindC) {
            ASR::ttype_t* arg_type = asr_retval->m_type;
            llvm::Value *tmp = ret_val;
            if (is_a<ASR::Complex_t>(*arg_type)) {
                int c_kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
                if (c_kind == 4) {
                    if (platform == Platform::Windows) {
                        // tmp is {float, float}*
                        // type_fx2p is i64*
                        llvm::Type* type_fx2p = llvm::Type::getInt64PtrTy(context);
                        // Convert {float,float}* to i64* using bitcast
                        tmp = builder->CreateBitCast(tmp, type_fx2p);
                        // Then convert i64* -> i64
                        tmp = CreateLoad(tmp);
                    } else if (platform == Platform::macOS_ARM) {
                        // Pass by value
                        tmp = CreateLoad(tmp);
                    } else {
                        // tmp is {float, float}*
                        // type_fx2p is <2 x float>*
                        llvm::Type* type_fx2p = llvm::FixedVectorType::get(llvm::Type::getFloatTy(context), 2)->getPointerTo();
                        // Convert {float,float}* to <2 x float>* using bitcast
                        tmp = builder->CreateBitCast(tmp, type_fx2p);
                        // Then convert <2 x float>* -> <2 x float>
                        tmp = CreateLoad(tmp);
                    }
                } else {
                    LFORTRAN_ASSERT(c_kind == 8)
                    if (platform == Platform::Windows) {
                        // 128 bit aggregate type is passed by reference
                    } else {
                        // Pass by value
                        tmp = CreateLoad(tmp);
                    }
                }
            ret_val2 = tmp;
            }
        }
        builder->CreateRet(ret_val2);
    }


    inline void define_subroutine_exit(const ASR::Subroutine_t& /*x*/) {
        start_new_block(proc_return);
        builder->CreateRetVoid();
    }

    void generate_function(const ASR::Function_t &x){
        bool interactive = (x.m_abi == ASR::abiType::Interactive);
        if (x.m_deftype == ASR::deftypeType::Implementation ) {

            if (interactive) return;

            if (!prototype_only) {
                define_function_entry(x);

                for (size_t i=0; i<x.n_body; i++) {
                    this->visit_stmt(*x.m_body[i]);
                }

                define_function_exit(x);
            }
        } else if( x.m_abi == ASR::abiType::Intrinsic &&
                   x.m_deftype == ASR::deftypeType::Interface ) {
            std::string m_name = x.m_name;
            if( m_name == "size" ) {

                define_function_entry(x);

                // Defines the size intrinsic's body at LLVM level.
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[0]);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                llvm::Value* llvm_arg = llvm_symtab[h];
                ASR::Variable_t *ret = EXPR2VAR(x.m_return_var);
                h = get_hash((ASR::asr_t*)ret);
                llvm::Value* llvm_ret_ptr = llvm_symtab[h];
                llvm::Value* dim_des_val = CreateLoad(llvm_utils->create_gep(llvm_arg, 0));
                llvm::Value* rank = CreateLoad(llvm_utils->create_gep(llvm_arg, 1));
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 1)), llvm_ret_ptr);

                llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
                llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
                llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
                this->current_loophead = loophead;
                this->current_loopend = loopend;

                llvm::Value* r = builder->CreateAlloca(getIntType(4), nullptr);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), r);
                // head
                start_new_block(loophead);
                llvm::Value *cond = builder->CreateICmpSLT(CreateLoad(r), rank);
                builder->CreateCondBr(cond, loopbody, loopend);

                // body
                start_new_block(loopbody);
                llvm::Value* r_val = CreateLoad(r);
                llvm::Value* ret_val = CreateLoad(llvm_ret_ptr);
                llvm::Value* dim_size = arr_descr->get_dimension_size(dim_des_val, r_val);
                ret_val = builder->CreateMul(ret_val, dim_size);
                builder->CreateStore(ret_val, llvm_ret_ptr);
                r_val = builder->CreateAdd(r_val, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                builder->CreateStore(r_val, r);
                builder->CreateBr(loophead);

                // end
                start_new_block(loopend);

                define_function_exit(x);
            } else if( m_name == "lbound" || m_name == "ubound" ) {
                define_function_entry(x);

                // Defines the size intrinsic's body at LLVM level.
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[0]);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                llvm::Value* llvm_arg1 = llvm_symtab[h];

                arg = EXPR2VAR(x.m_args[1]);
                h = get_hash((ASR::asr_t*)arg);
                llvm::Value* llvm_arg2 = llvm_symtab[h];

                ASR::Variable_t *ret = EXPR2VAR(x.m_return_var);
                h = get_hash((ASR::asr_t*)ret);
                llvm::Value* llvm_ret_ptr = llvm_symtab[h];

                llvm::Value* dim_des_val = CreateLoad(llvm_arg1);
                llvm::Value* dim_val = CreateLoad(llvm_arg2);
                llvm::Value* const_1 = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
                dim_val = builder->CreateSub(dim_val, const_1);
                llvm::Value* dim_struct = arr_descr->get_pointer_to_dimension_descriptor(dim_des_val, dim_val);
                llvm::Value* res = nullptr;
                if( m_name == "lbound" ) {
                    res = arr_descr->get_lower_bound(dim_struct);
                } else if( m_name == "ubound" ) {
                    res = arr_descr->get_upper_bound(dim_struct);
                }
                builder->CreateStore(res, llvm_ret_ptr);

                define_function_exit(x);
            }
        }
    }


    template<typename T>
    void visit_procedures(const T &x) {
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }
    }

    void visit_Associate(const ASR::Associate_t& x) {
        ASR::Variable_t *asr_target = EXPR2VAR(x.m_target);
        ASR::Variable_t *asr_value = EXPR2VAR(x.m_value);
        uint32_t value_h = get_hash((ASR::asr_t*)asr_value);
        uint32_t target_h = get_hash((ASR::asr_t*)asr_target);
        builder->CreateStore(llvm_symtab[value_h], llvm_symtab[target_h]);
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        if( x.m_overloaded ) {
            this->visit_stmt(*x.m_overloaded);
            return ;
        }
        llvm::Value *target, *value;
        uint32_t h;
        bool lhs_is_string_arrayref = false;
        if( x.m_target->type == ASR::exprType::ArrayRef ||
            x.m_target->type == ASR::exprType::DerivedRef ) {
            this->visit_expr(*x.m_target);
            target = tmp;
            if (is_a<ASR::ArrayRef_t>(*x.m_target)) {
                ASR::ArrayRef_t *asr_target0 = ASR::down_cast<ASR::ArrayRef_t>(x.m_target);
                if (is_a<ASR::Variable_t>(*asr_target0->m_v)) {
                    ASR::Variable_t *asr_target = ASR::down_cast<ASR::Variable_t>(asr_target0->m_v);
                    if ( is_a<ASR::Character_t>(*asr_target->m_type) ) {
                        ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(asr_target->m_type);
                        if (t->n_dims == 0) {
                            target = CreateLoad(target);
                            lhs_is_string_arrayref = true;
                        }
                    }
                }
            }
        } else {
            ASR::Variable_t *asr_target = EXPR2VAR(x.m_target);
            h = get_hash((ASR::asr_t*)asr_target);
            if (llvm_symtab.find(h) != llvm_symtab.end()) {
                target = llvm_symtab[h];
                if (ASR::is_a<ASR::Pointer_t>(*asr_target->m_type)) {
                    target = CreateLoad(target);
                }
            } else {
                /* Target for assignment not in the symbol table - must be
                assigning to an outer scope from a nested function - see
                nested_05.f90 */
                auto finder = std::find(nested_globals.begin(),
                        nested_globals.end(), h);
                LFORTRAN_ASSERT(finder != nested_globals.end());
                llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                    nested_global_struct);
                int idx = std::distance(nested_globals.begin(), finder);
                target = CreateLoad(llvm_utils->create_gep(ptr, idx));
            }
            if( arr_descr->is_array(target) ) {
                if( asr_target->m_type->type ==
                    ASR::ttypeType::Character ) {
                    target = CreateLoad(arr_descr->get_pointer_to_data(target));
                }
            }
        }
        this->visit_expr_wrapper(x.m_value, true);
        value = tmp;
        if ( is_a<ASR::Character_t>(*expr_type(x.m_value)) ) {
            ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(expr_type(x.m_value));
            if (t->n_dims == 0) {
                if (lhs_is_string_arrayref) {
                    value = CreateLoad(value);
                }
            }
        }
        builder->CreateStore(value, target);
        auto finder = std::find(nested_globals.begin(),
                nested_globals.end(), h);
        if (finder != nested_globals.end()) {
            /* Target for assignment could be in the symbol table - and we are
            assigning to a variable needed in a nested function - see
            nested_04.f90 */
            llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                    nested_global_struct);
            int idx = std::distance(nested_globals.begin(), finder);
            builder->CreateStore(target, llvm_utils->create_gep(ptr, idx));
        }
    }

    inline void visit_expr_wrapper(const ASR::expr_t* x, bool load_ref=false) {
        this->visit_expr(*x);
        if( x->type == ASR::exprType::ArrayRef ||
            x->type == ASR::exprType::DerivedRef ) {
            if( load_ref ) {
                tmp = CreateLoad(tmp);
            }
        }
    }

    void visit_Compare(const ASR::Compare_t &x) {
        if( x.m_overloaded ) {
            this->visit_expr(*x.m_overloaded);
            return ;
        }
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right = tmp;
        LFORTRAN_ASSERT(expr_type(x.m_left)->type == expr_type(x.m_right)->type);
        ASR::ttypeType optype = expr_type(x.m_left)->type;
        if (optype == ASR::ttypeType::Integer) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : {
                    tmp = builder->CreateICmpEQ(left, right);
                    break;
                }
                case (ASR::cmpopType::Gt) : {
                    tmp = builder->CreateICmpSGT(left, right);
                    break;
                }
                case (ASR::cmpopType::GtE) : {
                    tmp = builder->CreateICmpSGE(left, right);
                    break;
                }
                case (ASR::cmpopType::Lt) : {
                    tmp = builder->CreateICmpSLT(left, right);
                    break;
                }
                case (ASR::cmpopType::LtE) : {
                    tmp = builder->CreateICmpSLE(left, right);
                    break;
                }
                case (ASR::cmpopType::NotEq) : {
                    tmp = builder->CreateICmpNE(left, right);
                    break;
                }
                default : {
                    throw CodeGenError("Comparison operator not implemented",
                            x.base.base.loc);
                }
            }
        } else if (optype == ASR::ttypeType::Real) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : {
                    tmp = builder->CreateFCmpUEQ(left, right);
                    break;
                }
                case (ASR::cmpopType::Gt) : {
                    tmp = builder->CreateFCmpUGT(left, right);
                    break;
                }
                case (ASR::cmpopType::GtE) : {
                    tmp = builder->CreateFCmpUGE(left, right);
                    break;
                }
                case (ASR::cmpopType::Lt) : {
                    tmp = builder->CreateFCmpULT(left, right);
                    break;
                }
                case (ASR::cmpopType::LtE) : {
                    tmp = builder->CreateFCmpULE(left, right);
                    break;
                }
                case (ASR::cmpopType::NotEq) : {
                    tmp = builder->CreateFCmpUNE(left, right);
                    break;
                }
                default : {
                    throw CodeGenError("Comparison operator not implemented",
                            x.base.base.loc);
                }
            }
        } else if (optype == ASR::ttypeType::Complex) {
            llvm::Value* real_left = complex_re(left, left->getType());
            llvm::Value* real_right = complex_re(right, right->getType());
            llvm::Value* img_left = complex_im(left, left->getType());
            llvm::Value* img_right = complex_im(right, right->getType());
            llvm::Value *real_res, *img_res;
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : {
                    real_res = builder->CreateFCmpUEQ(real_left, real_right);
                    img_res = builder->CreateFCmpUEQ(img_left, img_right);
                    break;
                }
                case (ASR::cmpopType::NotEq) : {
                    real_res = builder->CreateFCmpUNE(real_left, real_right);
                    img_res = builder->CreateFCmpUNE(img_left, img_right);
                    break;
                }
                default : {
                    throw CodeGenError("Comparison operator not implemented",
                            x.base.base.loc);
                }
            }
            tmp = builder->CreateAnd(real_res, img_res);
        } else if (optype == ASR::ttypeType::Character) {
            // TODO: For now we only compare the first character of the strings
            left = CreateLoad(left);
            right = CreateLoad(right);
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : {
                    tmp = builder->CreateICmpEQ(left, right);
                    break;
                }
                case (ASR::cmpopType::NotEq) : {
                    tmp = builder->CreateICmpNE(left, right);
                    break;
                }
                case (ASR::cmpopType::Gt) : {
                    tmp = builder->CreateICmpUGT(left, right);
                    break;
                }
                case (ASR::cmpopType::GtE) : {
                    tmp = builder->CreateICmpUGE(left, right);
                    break;
                }
                case (ASR::cmpopType::Lt) : {
                    tmp = builder->CreateICmpULT(left, right);
                    break;
                }
                case (ASR::cmpopType::LtE) : {
                    tmp = builder->CreateICmpULE(left, right);
                    break;
                }
                default : {
                    throw CodeGenError("Comparison operator not implemented.",
                            x.base.base.loc);
                }
            }
        } else if (optype == ASR::ttypeType::Logical) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : {
                    tmp = builder->CreateICmpEQ(left, right);
                    break;
                }
                case (ASR::cmpopType::NotEq) : {
                    tmp = builder->CreateICmpNE(left, right);
                    break;
                }
                default : {
                    throw CodeGenError("Comparison operator not implemented.",
                            x.base.base.loc);
                }
            }
        } else {
            throw CodeGenError("Only Integer, Real, Complex, Character, and Logical"
                    " types are supported for comparison.", x.base.base.loc);
        }
    }

    void visit_If(const ASR::If_t &x) {
        this->visit_expr_wrapper(x.m_test, true);
        llvm::Value *cond=tmp;
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        builder->CreateCondBr(cond, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        builder->CreateBr(mergeBB);

        start_new_block(elseBB);
        for (size_t i=0; i<x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
        }

        start_new_block(mergeBB);
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        this->current_loophead = loophead;
        this->current_loopend = loopend;

        // head
        start_new_block(loophead);
        this->visit_expr_wrapper(x.m_test, true);
        llvm::Value *cond = tmp;
        builder->CreateCondBr(cond, loopbody, loopend);

        // body
        start_new_block(loopbody);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        builder->CreateBr(loophead);

        // end
        start_new_block(loopend);
    }

    void visit_Exit(const ASR::Exit_t & /* x */) {
        builder->CreateBr(current_loopend);
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "unreachable_after_exit");
        start_new_block(bb);
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
        builder->CreateBr(current_loophead);
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "unreachable_after_cycle");
        start_new_block(bb);
    }

    void visit_Return(const ASR::Return_t & /* x */) {
        builder->CreateBr(proc_return);
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "unreachable_after_return");
        start_new_block(bb);
    }

    void visit_GoTo(const ASR::GoTo_t &x) {
        if (llvm_goto_targets.find(x.m_target_id) == llvm_goto_targets.end()) {
            // If the target does not exist yet, create it
            llvm::BasicBlock *new_target = llvm::BasicBlock::Create(context, "goto_target");
            llvm_goto_targets[x.m_target_id] = new_target;
        }
        llvm::BasicBlock *target = llvm_goto_targets[x.m_target_id];
        builder->CreateBr(target);
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "unreachable_after_goto");
        start_new_block(bb);
    }

    void visit_GoToTarget(const ASR::GoToTarget_t &x) {
        if (llvm_goto_targets.find(x.m_id) == llvm_goto_targets.end()) {
            // If the target does not exist yet, create it
            llvm::BasicBlock *new_target = llvm::BasicBlock::Create(context, "goto_target");
            llvm_goto_targets[x.m_id] = new_target;
        }
        llvm::BasicBlock *target = llvm_goto_targets[x.m_id];
        start_new_block(target);
    }

    void visit_BoolOp(const ASR::BoolOp_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        if (x.m_type->type == ASR::ttypeType::Logical) {
            switch (x.m_op) {
                case ASR::boolopType::And: {
                    tmp = builder->CreateAnd(left_val, right_val);
                    break;
                };
                case ASR::boolopType::Or: {
                    tmp = builder->CreateOr(left_val, right_val);
                    break;
                };
                case ASR::boolopType::Xor: {
                    tmp = builder->CreateXor(left_val, right_val);
                    break;
                };
                case ASR::boolopType::NEqv: {
                    tmp = builder->CreateXor(left_val, right_val);
                    break;
                };
                case ASR::boolopType::Eqv: {
                    tmp = builder->CreateXor(left_val, right_val);
                    tmp = builder->CreateNot(tmp);
                };
            }
        } else {
            throw CodeGenError("Boolop: Only Logical types can be used with logical operators.");
        }
    }

    void visit_StrOp(const ASR::StrOp_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        switch (x.m_op) {
            case ASR::stropType::Concat: {
                tmp = lfortran_strop(left_val, right_val, "_lfortran_strcat");
                break;
            };
            case ASR::stropType::Repeat: {
                tmp = lfortran_strop(left_val, right_val, "_lfortran_strrepeat");
                break;
            };
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        if( x.m_overloaded ) {
            this->visit_expr(*x.m_overloaded);
            return ;
        }
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        if (ASRUtils::is_integer(*x.m_type)) {
            switch (x.m_op) {
                case ASR::binopType::Add: {
                    tmp = builder->CreateAdd(left_val, right_val);
                    break;
                };
                case ASR::binopType::Sub: {
                    tmp = builder->CreateSub(left_val, right_val);
                    break;
                };
                case ASR::binopType::Mul: {
                    tmp = builder->CreateMul(left_val, right_val);
                    break;
                };
                case ASR::binopType::Div: {
                    tmp = builder->CreateUDiv(left_val, right_val);
                    break;
                };
                case ASR::binopType::Pow: {
                    llvm::Type *type;
                    int a_kind;
                    a_kind = down_cast<ASR::Integer_t>(ASRUtils::type_get_past_pointer(x.m_type))->m_kind;
                    type = getFPType(a_kind);
                    llvm::Value *fleft = builder->CreateSIToFP(left_val,
                            type);
                    llvm::Value *fright = builder->CreateSIToFP(right_val,
                            type);
                    std::string func_name = a_kind == 4 ? "llvm.pow.f32" : "llvm.pow.f64";
                    llvm::Function *fn_pow = module->getFunction(func_name);
                    if (!fn_pow) {
                        llvm::FunctionType *function_type = llvm::FunctionType::get(
                                type, { type, type}, false);
                        fn_pow = llvm::Function::Create(function_type,
                                llvm::Function::ExternalLinkage, func_name,
                                module.get());
                    }
                    tmp = builder->CreateCall(fn_pow, {fleft, fright});
                    type = getIntType(a_kind);
                    tmp = builder->CreateFPToSI(tmp, type);
                    break;
                };
            }
        } else if (ASRUtils::is_real(*x.m_type)) {
            switch (x.m_op) {
                case ASR::binopType::Add: {
                    tmp = builder->CreateFAdd(left_val, right_val);
                    break;
                };
                case ASR::binopType::Sub: {
                    tmp = builder->CreateFSub(left_val, right_val);
                    break;
                };
                case ASR::binopType::Mul: {
                    tmp = builder->CreateFMul(left_val, right_val);
                    break;
                };
                case ASR::binopType::Div: {
                    tmp = builder->CreateFDiv(left_val, right_val);
                    break;
                };
                case ASR::binopType::Pow: {
                    llvm::Type *type;
                    int a_kind;
                    a_kind = down_cast<ASR::Real_t>(ASRUtils::type_get_past_pointer(x.m_type))->m_kind;
                    type = getFPType(a_kind);
                    std::string func_name = a_kind == 4 ? "llvm.pow.f32" : "llvm.pow.f64";
                    llvm::Function *fn_pow = module->getFunction(func_name);
                    if (!fn_pow) {
                        llvm::FunctionType *function_type = llvm::FunctionType::get(
                                type, { type, type }, false);
                        fn_pow = llvm::Function::Create(function_type,
                                llvm::Function::ExternalLinkage, func_name,
                                module.get());
                    }
                    tmp = builder->CreateCall(fn_pow, {left_val, right_val});
                    break;
                };
            }
        } else if (ASRUtils::is_complex(*x.m_type)) {
            llvm::Type *type;
            int a_kind;
            a_kind = down_cast<ASR::Complex_t>(ASRUtils::type_get_past_pointer(x.m_type))->m_kind;
            type = getComplexType(a_kind);
            if( left_val->getType()->isPointerTy() ) {
                left_val = CreateLoad(left_val);
            }
            if( right_val->getType()->isPointerTy() ) {
                right_val = CreateLoad(right_val);
            }
            switch (x.m_op) {
                case ASR::binopType::Add: {
                    tmp = lfortran_complex_bin_op(left_val, right_val, "_lfortran_complex_add", type);
                    break;
                };
                case ASR::binopType::Sub: {
                    tmp = lfortran_complex_bin_op(left_val, right_val, "_lfortran_complex_sub", type);
                    break;
                };
                case ASR::binopType::Mul: {
                    tmp = lfortran_complex_bin_op(left_val, right_val, "_lfortran_complex_mul", type);
                    break;
                };
                case ASR::binopType::Div: {
                    tmp = lfortran_complex_bin_op(left_val, right_val, "_lfortran_complex_div", type);
                    break;
                };
                case ASR::binopType::Pow: {
                    tmp = lfortran_complex_bin_op(left_val, right_val, "_lfortran_complex_pow", type);
                    break;
                };
            }
        } else {
            throw CodeGenError("Binop: Only Real, Integer and Complex types implemented");
        }
    }

    void visit_UnaryOp(const ASR::UnaryOp_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_operand, true);
        if (x.m_type->type == ASR::ttypeType::Integer) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // tmp = tmp;
                return;
            } else if (x.m_op == ASR::unaryopType::USub) {
                llvm::Value *zero = llvm::ConstantInt::get(context,
                    llvm::APInt(ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x.m_operand)) * 8, 0));
                tmp = builder ->CreateSub(zero, tmp);
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet");
            }
        } else if (x.m_type->type == ASR::ttypeType::Real) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // tmp = tmp;
                return;
            } else if (x.m_op == ASR::unaryopType::USub) {
                llvm::Value *zero;
                int a_kind = down_cast<ASR::Real_t>(x.m_type)->m_kind;
                if (a_kind == 4) {
                    zero = llvm::ConstantFP::get(context,
                            llvm::APFloat((float)0.0));
                } else if (a_kind == 8) {
                    zero = llvm::ConstantFP::get(context,
                            llvm::APFloat((double)0.0));
                } else {
                    throw CodeGenError("Unary type kind not implemented yet, only 4 and 8 is");
                }
                tmp = builder ->CreateFSub(zero, tmp);
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet");
            }
        } else if (x.m_type->type == ASR::ttypeType::Logical) {
            if (x.m_op == ASR::unaryopType::Not) {
                tmp = builder ->CreateNot(tmp);
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet in Logical");
            }
        } else {
            throw CodeGenError("UnaryOp: type not supported yet");
        }
    }

    void visit_ConstantInteger(const ASR::ConstantInteger_t &x) {
        int64_t val = x.m_n;
        int a_kind = ((ASR::Integer_t*)(&(x.m_type->base)))->m_kind;
        switch( a_kind ) {

            case 4 : {
                tmp = llvm::ConstantInt::get(context, llvm::APInt(32, static_cast<int32_t>(val), true));
                break;
            }
            case 8 : {
                tmp = llvm::ConstantInt::get(context, llvm::APInt(64, val, true));
                break;
            }
            default : {
                break;
            }

        }
    }

    void visit_ConstantReal(const ASR::ConstantReal_t &x) {
        double val = x.m_r;
        int a_kind = ((ASR::Real_t*)(&(x.m_type->base)))->m_kind;
        switch( a_kind ) {

            case 4 : {
                tmp = llvm::ConstantFP::get(context, llvm::APFloat((float)val));
                break;
            }
            case 8 : {
                tmp = llvm::ConstantFP::get(context, llvm::APFloat(val));
                break;
            }
            default : {
                break;
            }

        }

    }

    void visit_Assert(const ASR::Assert_t &x) {
        this->visit_expr_wrapper(x.m_test, true);
        llvm::Value *cond = tmp;
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        builder->CreateCondBr(cond, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);

        builder->CreateBr(mergeBB);

        start_new_block(elseBB);

        {
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("Assertion failed\n");
            printf(context, *module, *builder, {fmt_ptr});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, *module, *builder, exit_code);
        }

        start_new_block(mergeBB);
    }

    void visit_ComplexConstructor(const ASR::ComplexConstructor_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        throw CodeGenError("ComplexConstructor with runtime arguments not implemented yet.");
    }

    void visit_ConstantComplex(const ASR::ConstantComplex_t &x) {
        double re = x.m_re;
        double im = x.m_im;
        int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        llvm::Value *re2, *im2;
        llvm::Type *type;
        switch( a_kind ) {
            case 4: {
                re2 = llvm::ConstantFP::get(context, llvm::APFloat((float)re));
                type = complex_type_4;
                break;
            }
            case 8: {
                re2 = llvm::ConstantFP::get(context, llvm::APFloat(re));
                type = complex_type_8;
                break;
            }
            default: {
                throw CodeGenError("kind type not supported");
            }
        }
        switch( a_kind ) {
            case 4: {
                im2 = llvm::ConstantFP::get(context, llvm::APFloat((float)im));
                break;
            }
            case 8: {
                im2 = llvm::ConstantFP::get(context, llvm::APFloat(im));
                break;
            }
            default: {
                throw CodeGenError("kind type not supported");
            }
        }
        tmp = complex_from_floats(re2, im2, type);
    }

    void visit_ConstantLogical(const ASR::ConstantLogical_t &x) {
        int val;
        if (x.m_value == true) {
            val = 1;
        } else {
            val = 0;
        }
        tmp = llvm::ConstantInt::get(context, llvm::APInt(1, val));
    }


    void visit_ConstantString(const ASR::ConstantString_t &x) {
        tmp = builder->CreateGlobalStringPtr(x.m_s);
    }

    inline void fetch_ptr(ASR::Variable_t* x) {
        uint32_t x_h = get_hash((ASR::asr_t*)x);
        LFORTRAN_ASSERT(llvm_symtab.find(x_h) != llvm_symtab.end());
        llvm::Value* x_v = llvm_symtab[x_h];
        tmp = CreateLoad(x_v);
        tmp = CreateLoad(tmp);
    }

    inline void fetch_val(ASR::Variable_t* x) {
        uint32_t x_h = get_hash((ASR::asr_t*)x);
        llvm::Value* x_v;
        // Check if x is a needed global here, if so, it should exist as an
        // element in the runtime descriptor, get element pointer and create
        // load
        if (llvm_symtab.find(x_h) == llvm_symtab.end()) {
            LFORTRAN_ASSERT(std::find(nested_globals.begin(),
                    nested_globals.end(), x_h) != nested_globals.end());
            auto finder = std::find(nested_globals.begin(),
                    nested_globals.end(), x_h);
            llvm::Constant *ptr = module->getOrInsertGlobal(nested_desc_name,
                nested_global_struct);
            int idx = std::distance(nested_globals.begin(), finder);
            std::vector<llvm::Value*> idx_vec = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
            x_v = CreateLoad(CreateGEP(ptr, idx_vec));
        } else {
            x_v = llvm_symtab[x_h];
        }
        tmp = CreateLoad(x_v);

        if( arr_descr->is_array(x_v) ) {
            tmp = x_v;
        }
    }

    inline void fetch_var(ASR::Variable_t* x) {
        if (x->m_value) {
            this->visit_expr_wrapper(x->m_value, true);
            return;
        }
        switch( x->m_type->type ) {
            case ASR::ttypeType::Pointer: {
                ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(x->m_type);
                switch (t2->type) {
                    case ASR::ttypeType::Integer:
                    case ASR::ttypeType::Real:
                    case ASR::ttypeType::Complex: {
                        fetch_ptr(x);
                        break;
                    }
                    case ASR::ttypeType::Character:
                    case ASR::ttypeType::Logical:
                    case ASR::ttypeType::Derived: {
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case ASR::ttypeType::Derived: {
                ASR::Derived_t* der = (ASR::Derived_t*)(&(x->m_type->base));
                ASR::DerivedType_t* der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
                der_type_name = std::string(der_type->m_name);
                uint32_t h = get_hash((ASR::asr_t*)x);
                if( llvm_symtab.find(h) != llvm_symtab.end() ) {
                    tmp = llvm_symtab[h];
                }
                break;
            }
            case ASR::ttypeType::Class: {
                ASR::Class_t* der = (ASR::Class_t*)(&(x->m_type->base));
                ASR::ClassType_t* der_type = (ASR::ClassType_t*)(&(der->m_class_type->base));
                der_type_name = std::string(der_type->m_name);
                uint32_t h = get_hash((ASR::asr_t*)x);
                if( llvm_symtab.find(h) != llvm_symtab.end() ) {
                    tmp = llvm_symtab[h];
                }
                break;
            }
            default: {
                fetch_val(x);
                break;
            }
        }
    }

    void visit_Var(const ASR::Var_t &x) {
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(
                symbol_get_past_external(x.m_v));
        fetch_var(v);
    }

    inline ASR::ttype_t* extract_ttype_t_from_expr(ASR::expr_t* expr) {
        return ASRUtils::expr_type(expr);
    }

    void extract_kinds(const ASR::ImplicitCast_t& x,
                       int& arg_kind, int& dest_kind)
    {
        dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
        LFORTRAN_ASSERT(curr_type != nullptr)
        arg_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
    }

    void visit_ImplicitCast(const ASR::ImplicitCast_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (a_kind) {
                    case 4 : {
                        tmp = builder->CreateSIToFP(tmp, llvm::Type::getFloatTy(context));
                        break;
                    }
                    case 8 : {
                        tmp = builder->CreateSIToFP(tmp, llvm::Type::getDoubleTy(context));
                        break;
                    }
                    default : {
                        throw CodeGenError(R"""(Only 32 and 64 bit real kinds are implemented)""",
                                            x.base.base.loc);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                llvm::Type *target_type;
                int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                target_type = getIntType(a_kind);
                tmp = builder->CreateFPToSI(tmp, target_type);
                break;
            }
            case (ASR::cast_kindType::RealToComplex) : {
                llvm::Type *target_type;
                llvm::Value *zero;
                int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch(a_kind)
                {
                    case 4:
                        target_type = complex_type_4;
                        tmp = builder->CreateFPTrunc(tmp, llvm::Type::getFloatTy(context));
                        zero = llvm::ConstantFP::get(context, llvm::APFloat((float)0.0));
                        break;
                    case 8:
                        target_type = complex_type_8;
                        tmp = builder->CreateFPExt(tmp, llvm::Type::getDoubleTy(context));
                        zero = llvm::ConstantFP::get(context, llvm::APFloat(0.0));
                        break;
                    default:
                        throw CodeGenError("Only 32 and 64 bits real kinds are supported.");
                }
                tmp = complex_from_floats(tmp, zero, target_type);
                break;
            }
            case (ASR::cast_kindType::IntegerToComplex) : {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                llvm::Type *target_type;
                llvm::Type *complex_type;
                llvm::Value *zero;
                switch(a_kind)
                {
                    case 4:
                        target_type = llvm::Type::getFloatTy(context);
                        complex_type = complex_type_4;
                        zero = llvm::ConstantFP::get(context, llvm::APFloat((float)0.0));
                        break;
                    case 8:
                        target_type = llvm::Type::getDoubleTy(context);
                        complex_type = complex_type_8;
                        zero = llvm::ConstantFP::get(context, llvm::APFloat(0.0));
                        break;
                    default:
                        throw CodeGenError("Only 32 and 64 bits real kinds are supported.");
                }
                tmp = builder->CreateSIToFP(tmp, target_type);
                tmp = complex_from_floats(tmp, zero, complex_type);
                break;
            }
            case (ASR::cast_kindType::IntegerToLogical) : {
                tmp = builder->CreateICmpNE(tmp, builder->getInt32(0));
                break;
            }
            case (ASR::cast_kindType::RealToReal) : {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if( arg_kind > 0 && dest_kind > 0 &&
                    arg_kind != dest_kind )
                {
                    if( arg_kind == 4 && dest_kind == 8 ) {
                        tmp = builder->CreateFPExt(tmp, llvm::Type::getDoubleTy(context));
                    } else if( arg_kind == 8 && dest_kind == 4 ) {
                        tmp = builder->CreateFPTrunc(tmp, llvm::Type::getFloatTy(context));
                    } else {
                        std::string msg = "Conversion from " + std::to_string(arg_kind) +
                                          " to " + std::to_string(dest_kind) + " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::IntegerToInteger) : {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                if( arg_kind > 0 && dest_kind > 0 &&
                    arg_kind != dest_kind )
                {
                    if( arg_kind == 4 && dest_kind == 8 ) {
                        tmp = builder->CreateSExt(tmp, llvm::Type::getInt64Ty(context));
                    } else if( arg_kind == 8 && dest_kind == 4 ) {
                        tmp = builder->CreateTrunc(tmp, llvm::Type::getInt32Ty(context));
                    } else if( arg_kind == 2 && dest_kind == 4 ) {
                        tmp = builder->CreateSExt(tmp, llvm::Type::getInt32Ty(context));
                    } else if( arg_kind == 4 && dest_kind == 2 ) {
                        tmp = builder->CreateTrunc(tmp, llvm::Type::getInt16Ty(context));
                    } else if( arg_kind == 1 && dest_kind == 4 ) {
                        tmp = builder->CreateSExt(tmp, llvm::Type::getInt32Ty(context));
                    } else if( arg_kind == 4 && dest_kind == 1 ) {
                        tmp = builder->CreateTrunc(tmp, llvm::Type::getInt8Ty(context));
                    } else {
                        std::string msg = "Conversion from " + std::to_string(arg_kind) +
                                          " to " + std::to_string(dest_kind) + " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::ComplexToComplex) : {
                llvm::Type *target_type;
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                llvm::Value *re, *im;
                if( arg_kind > 0 && dest_kind > 0 &&
                    arg_kind != dest_kind )
                {
                    if( arg_kind == 4 && dest_kind == 8 ) {
                        target_type = complex_type_8;
                        re = complex_re(tmp, complex_type_4);
                        re = builder->CreateFPExt(re, llvm::Type::getDoubleTy(context));
                        im = complex_im(tmp, complex_type_4);
                        im = builder->CreateFPExt(im, llvm::Type::getDoubleTy(context));
                    } else if( arg_kind == 8 && dest_kind == 4 ) {
                        target_type = complex_type_4;
                        re = complex_re(tmp, complex_type_8);
                        re = builder->CreateFPTrunc(re, llvm::Type::getFloatTy(context));
                        im = complex_im(tmp, complex_type_8);
                        im = builder->CreateFPTrunc(im, llvm::Type::getFloatTy(context));
                    } else {
                        std::string msg = "Conversion from " + std::to_string(arg_kind) +
                                          " to " + std::to_string(dest_kind) + " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                } else {
                    throw CodeGenError("Negative kinds are not supported.");
                }
                tmp = complex_from_floats(re, im, target_type);
                break;
            }
            case (ASR::cast_kindType::ComplexToReal) : {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                llvm::Value *re;
                if( arg_kind > 0 && dest_kind > 0)
                {
                    if( arg_kind == 4 && dest_kind == 4 ) {
                        // complex(4) -> real(4)
                        re = complex_re(tmp, complex_type_4);
                        tmp = re;
                    } else if( arg_kind == 4 && dest_kind == 8 ) {
                        // complex(4) -> real(8)
                        re = complex_re(tmp, complex_type_4);
                        tmp = builder->CreateFPExt(re, llvm::Type::getDoubleTy(context));
                    } else if( arg_kind == 8 && dest_kind == 4 ) {
                        // complex(8) -> real(4)
                        re = complex_re(tmp, complex_type_8);
                        tmp = builder->CreateFPTrunc(re, llvm::Type::getFloatTy(context));
                    } else if( arg_kind == 8 && dest_kind == 8 ) {
                        // complex(8) -> real(8)
                        re = complex_re(tmp, complex_type_8);
                        tmp = re;
                    } else {
                        std::string msg = "Conversion from " + std::to_string(arg_kind) +
                                          " to " + std::to_string(dest_kind) + " not implemented yet.";
                        throw CodeGenError(msg);
                    }
                } else {
                    throw CodeGenError("Negative kinds are not supported.");
                }
                break;
            }
            default : throw CodeGenError("Cast kind not implemented");
        }
    }

    void visit_Read(const ASR::Read_t &x) {
        if (x.m_fmt != nullptr) {
            diag.codegen_warning_label("format string in read() is not implemented yet and it is currently treated as '*'",
                {x.m_fmt->base.loc}, "treated as '*'");
        }
        if (x.m_unit != nullptr) {
            diag.codegen_error_label("unit in read() is not implemented yet",
                {x.m_unit->base.loc}, "not implemented");
            throw CodeGenAbort();
        }
        diag.codegen_error_label("The intrinsic function read() is not implemented yet in the LLVM backend",
            {x.base.base.loc}, "not implemented");
        throw CodeGenAbort();
    }

    void visit_Print(const ASR::Print_t &x) {
        if (x.m_fmt != nullptr) {
            diag.codegen_warning_label("format string in `print` is not implemented yet and it is currently treated as '*'",
                {x.m_fmt->base.loc}, "treated as '*'");
        }
        handle_print(x);
    }

    void visit_Write(const ASR::Write_t &x) {
        if (x.m_fmt != nullptr) {
            diag.codegen_warning_label("format string in write() is not implemented yet and it is currently treated as '*'",
                {x.m_fmt->base.loc}, "treated as '*'");
        }
        if (x.m_unit != nullptr) {
            diag.codegen_error_label("unit in write() is not implemented yet",
                {x.m_unit->base.loc}, "not implemented");
            throw CodeGenAbort();
        }
        handle_print(x);
    }

    template <typename T>
    void handle_print(const T &x) {
        std::vector<llvm::Value *> args;
        std::vector<std::string> fmt;
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr_wrapper(x.m_values[i], true);
            ASR::expr_t *v = x.m_values[i];
            ASR::ttype_t *t = expr_type(v);
            int a_kind = ASRUtils::extract_kind_from_ttype_t(t);
            if (ASRUtils::is_integer(*t) ||
                ASR::is_a<ASR::Logical_t>(*ASRUtils::type_get_past_pointer(t))) {
                switch( a_kind ) {
                    case 4 : {
                        fmt.push_back("%d");
                        break;
                    }
                    case 8 : {
                        fmt.push_back("%lld");
                        break;
                    }
                    default: {
                        throw CodeGenError(R"""(Printing support is available only
                                            for 32, and 64 bit integer kinds.)""",
                                            x.base.base.loc);
                    }
                }
                args.push_back(tmp);
            } else if (ASRUtils::is_real(*t)) {
                llvm::Value *d;
                switch( a_kind ) {
                    case 4 : {
                        // Cast float to double as a workaround for the fact that
                        // vprintf() seems to cast to double even for %f, which
                        // causes it to print 0.000000.
                        fmt.push_back("%f");
                        d = builder->CreateFPExt(tmp,
                        llvm::Type::getDoubleTy(context));
                        break;
                    }
                    case 8 : {
                        fmt.push_back("%23.17f");
                        d = builder->CreateFPExt(tmp,
                        llvm::Type::getDoubleTy(context));
                        break;
                    }
                    default: {
                        throw CodeGenError(R"""(Printing support is available only
                                            for 32, and 64 bit real kinds.)""",
                                            x.base.base.loc);
                    }
                }
                args.push_back(d);
            } else if (t->type == ASR::ttypeType::Character) {
                fmt.push_back("%s");
                args.push_back(tmp);
            } else if (ASRUtils::is_complex(*t)) {
                llvm::Type *type, *complex_type;
                switch( a_kind ) {
                    case 4 : {
                        // Cast float to double as a workaround for the fact that
                        // vprintf() seems to cast to double even for %f, which
                        // causes it to print 0.000000.
                        fmt.push_back("(%f,%f)");
                        type = llvm::Type::getDoubleTy(context);
                        complex_type = complex_type_4;
                        break;
                    }
                    case 8 : {
                        fmt.push_back("(%lf,%lf)");
                        type = llvm::Type::getDoubleTy(context);
                        complex_type = complex_type_8;
                        break;
                    }
                    default: {
                        throw CodeGenError(R"""(Printing support is available only
                                            for 32, and 64 bit complex kinds.)""",
                                            x.base.base.loc);
                    }
                }
                llvm::Value *d;
                d = builder->CreateFPExt(complex_re(tmp, complex_type), type);
                args.push_back(d);
                d = builder->CreateFPExt(complex_im(tmp, complex_type), type);
                args.push_back(d);
            } else {
                throw LFortranException("Type not implemented");
            }
        }
        std::string fmt_str;
        for (size_t i=0; i<fmt.size(); i++) {
            fmt_str += fmt[i];
            if (i < fmt.size()-1) fmt_str += " ";
        }
        fmt_str += "\n";
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr(fmt_str);
        std::vector<llvm::Value *> printf_args;
        printf_args.push_back(fmt_ptr);
        printf_args.insert(printf_args.end(), args.begin(), args.end());
        printf(context, *module, *builder, printf_args);
    }

    void visit_Stop(const ASR::Stop_t &x) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("STOP\n");
        printf(context, *module, *builder, {fmt_ptr});
        llvm::Value *exit_code;
        if (x.m_code && ASRUtils::expr_type(x.m_code)->type == ASR::ttypeType::Integer) {
            this->visit_expr(*x.m_code);
            exit_code = tmp;
        } else {
            int exit_code_int = 0;
            exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
        }
        exit(context, *module, *builder, exit_code);
    }

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("ERROR STOP\n");
        printf(context, *module, *builder, {fmt_ptr});
        int exit_code_int = 1;
        llvm::Value *exit_code = llvm::ConstantInt::get(context,
                llvm::APInt(32, exit_code_int));
        exit(context, *module, *builder, exit_code);
    }

    template <typename T>
    inline void set_func_subrout_params(T* func_subrout, ASR::abiType& x_abi,
                                        std::uint32_t& m_h, ASR::Variable_t*& orig_arg,
                                        std::string& orig_arg_name, size_t arg_idx) {
        m_h = get_hash((ASR::asr_t*)func_subrout);
        orig_arg = EXPR2VAR(func_subrout->m_args[arg_idx]);
        orig_arg_name = orig_arg->m_name;
        x_abi = func_subrout->m_abi;
    }


    template <typename T>
    std::vector<llvm::Value*> convert_call_args(const T &x, std::string name) {
        std::vector<llvm::Value *> args;
        const ASR::symbol_t* func_subrout = symbol_get_past_external(x.m_name);
        ASR::abiType x_abi = ASR::abiType::Source;
        if( is_a<ASR::Function_t>(*func_subrout) ) {
            ASR::Function_t* func = down_cast<ASR::Function_t>(func_subrout);
            x_abi = func->m_abi;
        } else if( is_a<ASR::Subroutine_t>(*func_subrout) ) {
            ASR::Subroutine_t* sub = down_cast<ASR::Subroutine_t>(func_subrout);
            x_abi = sub->m_abi;
        }
        if( x_abi == ASR::abiType::Intrinsic ) {
            if( name == "size" ) {
                /*
                When size intrinsic is called on a fortran array then the above
                code extracts the dimension descriptor array and its rank from the
                overall array descriptor. It wraps them into a struct (specifically, arg_struct of type, size_arg here)
                and passes to LLVM size. So, if you do, size(a) (a is a fortran array), then at LLVM level,
                @size(%size_arg* %x) is used as call where size_arg
                is described above.
                */
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[0].m_value);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                tmp = llvm_symtab[h];
                llvm::Value* arg_struct = builder->CreateAlloca(fname2arg_type["size"].first, nullptr);
                llvm::Value* first_ele_ptr = arr_descr->get_pointer_to_dimension_descriptor_array(tmp);
                llvm::Value* first_arg_ptr = llvm_utils->create_gep(arg_struct, 0);
                builder->CreateStore(first_ele_ptr, first_arg_ptr);
                llvm::Value* rank_ptr = llvm_utils->create_gep(arg_struct, 1);
                builder->CreateStore(arr_descr->get_rank(tmp), rank_ptr);
                tmp = arg_struct;
                args.push_back(tmp);
                llvm::Value* dim = builder->CreateAlloca(getIntType(4));
                args.push_back(dim);
                llvm::Value* kind = builder->CreateAlloca(getIntType(4));
                args.push_back(kind);
            } else if( name == "lbound" || name == "ubound" ) {
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[0].m_value);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                tmp = llvm_symtab[h];
                llvm::Value* arg1 = builder->CreateAlloca(fname2arg_type[name].first, nullptr);
                llvm::Value* first_ele_ptr = arr_descr->get_pointer_to_dimension_descriptor_array(tmp);
                llvm::Value* first_arg_ptr = arg1;
                builder->CreateStore(first_ele_ptr, first_arg_ptr);
                args.push_back(arg1);
                llvm::Value* arg2 = builder->CreateAlloca(getIntType(4), nullptr);

                this->visit_expr_wrapper(x.m_args[1].m_value, true);
                builder->CreateStore(tmp, arg2);
                args.push_back(arg2);
            }
        }
        if( args.size() == 0 ) {
            for (size_t i=0; i<x.n_args; i++) {
                if (x.m_args[i].m_value->type == ASR::exprType::Var) {
                    if (is_a<ASR::Variable_t>(*symbol_get_past_external(
                            ASR::down_cast<ASR::Var_t>(x.m_args[i].m_value)->m_v))) {
                        ASR::Variable_t *arg = EXPR2VAR(x.m_args[i].m_value);
                        uint32_t h = get_hash((ASR::asr_t*)arg);
                        if (llvm_symtab.find(h) != llvm_symtab.end()) {
                            tmp = llvm_symtab[h];
                            const ASR::symbol_t* func_subrout = symbol_get_past_external(x.m_name);
                            ASR::abiType x_abi = (ASR::abiType) 0;
                            std::uint32_t m_h;
                            ASR::Variable_t *orig_arg = nullptr;
                            std::string orig_arg_name = "";
                            if( func_subrout->type == ASR::symbolType::Function ) {
                                ASR::Function_t* func = down_cast<ASR::Function_t>(func_subrout);
                                set_func_subrout_params(func, x_abi, m_h, orig_arg, orig_arg_name, i);
                            } else if( func_subrout->type == ASR::symbolType::Subroutine ) {
                                ASR::Subroutine_t* sub = down_cast<ASR::Subroutine_t>(func_subrout);
                                set_func_subrout_params(sub, x_abi, m_h, orig_arg, orig_arg_name, i);
                            } else if( func_subrout->type == ASR::symbolType::ClassProcedure ) {
                                ASR::ClassProcedure_t* clss_proc = ASR::down_cast<ASR::ClassProcedure_t>(func_subrout);
                                if( clss_proc->m_proc->type == ASR::symbolType::Subroutine ) {
                                    ASR::Subroutine_t* sub = down_cast<ASR::Subroutine_t>(clss_proc->m_proc);
                                    set_func_subrout_params(sub, x_abi, m_h, orig_arg, orig_arg_name, i);
                                } else if( clss_proc->m_proc->type == ASR::symbolType::Function ) {
                                    ASR::Function_t* func = down_cast<ASR::Function_t>(clss_proc->m_proc);
                                    set_func_subrout_params(func, x_abi, m_h, orig_arg, orig_arg_name, i);
                                }
                            } else {
                                LFORTRAN_ASSERT(false)
                            }
                            if( x_abi == ASR::abiType::Source && arr_descr->is_array(tmp) ) {
                                llvm::Type* new_arr_type = arr_arg_type_cache[m_h][orig_arg_name];
                                tmp = arr_descr->convert_to_argument(tmp, new_arr_type);
                            } else if ( x_abi == ASR::abiType::BindC ) {
                                if( arr_descr->is_array(tmp) ) {
                                    tmp = CreateLoad(arr_descr->get_pointer_to_data(tmp));
                                    llvm::PointerType* tmp_type = static_cast<llvm::PointerType*>(tmp->getType());
                                    if( tmp_type->getElementType()->isArrayTy() ) {
                                        tmp = llvm_utils->create_gep(tmp, 0);
                                    }
                                } else {
                                    if (orig_arg->m_abi == ASR::abiType::BindC
                                        && orig_arg->m_value_attr) {
                                            ASR::ttype_t* arg_type = arg->m_type;
                                            if (is_a<ASR::Complex_t>(*arg_type)) {
                                                int c_kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
                                                if (c_kind == 4) {
                                                    if (platform == Platform::Windows) {
                                                        // tmp is {float, float}*
                                                        // type_fx2p is i64*
                                                        llvm::Type* type_fx2p = llvm::Type::getInt64PtrTy(context);
                                                        // Convert {float,float}* to i64* using bitcast
                                                        tmp = builder->CreateBitCast(tmp, type_fx2p);
                                                        // Then convert i64* -> i64
                                                        tmp = CreateLoad(tmp);
                                                    } else if (platform == Platform::macOS_ARM) {
                                                        // tmp is {float, float}*
                                                        // type_fx2p is [2 x float]*
                                                        llvm::Type* type_fx2p = llvm::ArrayType::get(llvm::Type::getFloatTy(context), 2)->getPointerTo();
                                                        // Convert {float,float}* to [2 x float]* using bitcast
                                                        tmp = builder->CreateBitCast(tmp, type_fx2p);
                                                        // Then convert [2 x float]* -> [2 x float]
                                                        tmp = CreateLoad(tmp);
                                                    } else {
                                                        // tmp is {float, float}*
                                                        // type_fx2p is <2 x float>*
                                                        llvm::Type* type_fx2p = llvm::FixedVectorType::get(llvm::Type::getFloatTy(context), 2)->getPointerTo();
                                                        // Convert {float,float}* to <2 x float>* using bitcast
                                                        tmp = builder->CreateBitCast(tmp, type_fx2p);
                                                        // Then convert <2 x float>* -> <2 x float>
                                                        tmp = CreateLoad(tmp);
                                                    }
                                                } else {
                                                    LFORTRAN_ASSERT(c_kind == 8)
                                                    if (platform == Platform::Windows) {
                                                        // 128 bit aggregate type is passed by reference
                                                    } else {
                                                        // Pass by value
                                                        tmp = CreateLoad(tmp);
                                                    }
                                                }
                                            } else {
                                                // Dereference the pointer argument
                                                // to pass by value
                                                // E.g.:
                                                // i32* -> i32
                                                // {double,double}* -> {double,double}
                                                tmp = CreateLoad(tmp);
                                            }
                                        }
                                }
                            }
                        } else {
                            auto finder = std::find(nested_globals.begin(),
                                    nested_globals.end(), h);
                            LFORTRAN_ASSERT(finder != nested_globals.end());
                            llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                                nested_global_struct);
                            int idx = std::distance(nested_globals.begin(), finder);
                            tmp = CreateLoad(llvm_utils->create_gep(ptr, idx));
                        }
                    } else if (is_a<ASR::Function_t>(*symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(x.m_args[i].m_value)->m_v))) {
                        ASR::Function_t* fn = ASR::down_cast<ASR::Function_t>(
                            symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                            x.m_args[i].m_value)->m_v));
                        uint32_t h = get_hash((ASR::asr_t*)fn);
                        if (fn->m_deftype == ASR::deftypeType::Implementation) {
                            tmp = llvm_symtab_fn[h];
                        } else {
                            // Must be an argument/chained procedure pass
                            tmp = llvm_symtab_fn_arg[h];
                        }
                    } else if (is_a<ASR::Subroutine_t>(*symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(x.m_args[i].m_value)->m_v))) {
                        ASR::Subroutine_t* fn = ASR::down_cast<ASR::Subroutine_t>(
                            symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                            x.m_args[i].m_value)->m_v));
                        uint32_t h = get_hash((ASR::asr_t*)fn);
                        if (fn->m_deftype == ASR::deftypeType::Implementation) {
                            tmp = llvm_symtab_fn[h];
                        } else {
                            // Must be an argument/chained procedure pass
                            tmp = llvm_symtab_fn_arg[h];
                        }
                    }
                } else {
                    this->visit_expr_wrapper(x.m_args[i].m_value);
                    llvm::Value *value=tmp;
                    llvm::Type *target_type;
                    ASR::ttype_t* arg_type = expr_type(x.m_args[i].m_value);
                    bool character_bindc = false;
                    switch (arg_type->type) {
                        case (ASR::ttypeType::Integer) : {
                            int a_kind = down_cast<ASR::Integer_t>(arg_type)->m_kind;
                            target_type = getIntType(a_kind);
                            break;
                        }
                        case (ASR::ttypeType::Real) : {
                            int a_kind = down_cast<ASR::Real_t>(arg_type)->m_kind;
                            target_type = getFPType(a_kind);
                            break;
                        }
                        case (ASR::ttypeType::Complex) : {
                            int a_kind = down_cast<ASR::Complex_t>(arg_type)->m_kind;
                            target_type = getComplexType(a_kind);
                            break;
                        }
                        case (ASR::ttypeType::Character) : {
                            ASR::Variable_t *orig_arg = nullptr;
                            if( func_subrout->type == ASR::symbolType::Function ) {
                                ASR::Function_t* func = down_cast<ASR::Function_t>(func_subrout);
                                orig_arg = EXPR2VAR(func->m_args[i]);
                            } else if( func_subrout->type == ASR::symbolType::Subroutine ) {
                                ASR::Subroutine_t* sub = down_cast<ASR::Subroutine_t>(func_subrout);
                                orig_arg = EXPR2VAR(sub->m_args[i]);
                            } else {
                                LFORTRAN_ASSERT(false)
                            }
                            if (orig_arg->m_abi == ASR::abiType::BindC) {
                                character_bindc = true;
                            }

                            target_type = character_type;
                            break;
                        }
                        case (ASR::ttypeType::Logical) :
                            target_type = llvm::Type::getInt1Ty(context);
                            break;
                        case (ASR::ttypeType::Derived) :
                            break;
                        default :
                            throw CodeGenError("Type not implemented yet.");
                    }
                    switch(arg_type->type) {
                        case ASR::ttypeType::Derived: {
                            tmp = value;
                            break;
                        }
                        default: {
                            if (!character_bindc) {
                                bool use_value = false;
                                ASR::Variable_t *orig_arg = nullptr;
                                if( func_subrout->type == ASR::symbolType::Function ) {
                                    ASR::Function_t* func = down_cast<ASR::Function_t>(func_subrout);
                                    orig_arg = EXPR2VAR(func->m_args[i]);
                                } else if( func_subrout->type == ASR::symbolType::Subroutine ) {
                                    ASR::Subroutine_t* sub = down_cast<ASR::Subroutine_t>(func_subrout);
                                    orig_arg = EXPR2VAR(sub->m_args[i]);
                                } else {
                                    LFORTRAN_ASSERT(false)
                                }
                                if (orig_arg->m_abi == ASR::abiType::BindC
                                    && orig_arg->m_value_attr) {
                                    use_value = true;
                                }
                                if (!use_value) {
                                    // Create alloca to get a pointer, but do it
                                    // at the beginning of the function to avoid
                                    // using alloca inside a loop, which would
                                    // run out of stack
                                    llvm::BasicBlock &entry_block = builder->GetInsertBlock()->getParent()->getEntryBlock();
                                    llvm::IRBuilder<> builder0(context);
                                    builder0.SetInsertPoint(&entry_block, entry_block.getFirstInsertionPt());
                                    llvm::AllocaInst *target = builder0.CreateAlloca(
                                        target_type, nullptr, "call_arg_value");
                                    builder->CreateStore(value, target);
                                    tmp = target;
                                }
                            }
                        }
                    }
                }
                args.push_back(tmp);
            }
        }
        return args;
    }

    void generate_flip_sign(ASR::call_arg_t* m_args) {
        this->visit_expr_wrapper(m_args[0].m_value, true);
        llvm::Value* signal = tmp;
        LFORTRAN_ASSERT(m_args[1].m_value->type == ASR::exprType::Var);
        ASR::Var_t* asr_var = ASR::down_cast<ASR::Var_t>(m_args[1].m_value);
        ASR::Variable_t* asr_variable = ASR::down_cast<ASR::Variable_t>(asr_var->m_v);
        uint32_t x_h = get_hash((ASR::asr_t*)asr_variable);
        llvm::Value* variable = llvm_symtab[x_h];
        // variable = xor(shiftl(int(Nd), 63), variable)
        ASR::ttype_t* signal_type = ASRUtils::expr_type(m_args[0].m_value);
        int signal_kind = ASRUtils::extract_kind_from_ttype_t(signal_type);
        llvm::Value* num_shifts = llvm::ConstantInt::get(context, llvm::APInt(32, signal_kind * 8 - 1));
        llvm::Value* shifted_signal = builder->CreateShl(signal, num_shifts);
        llvm::Value* int_var = builder->CreateBitCast(CreateLoad(variable), shifted_signal->getType());
        tmp = builder->CreateXor(shifted_signal, int_var);
        llvm::PointerType* variable_type = static_cast<llvm::PointerType*>(variable->getType());
        builder->CreateStore(builder->CreateBitCast(tmp, variable_type->getElementType()), variable);
    }

    void generate_fma(ASR::call_arg_t* m_args) {
        this->visit_expr_wrapper(m_args[0].m_value, true);
        llvm::Value* a = tmp;
        this->visit_expr_wrapper(m_args[1].m_value, true);
        llvm::Value* b = tmp;
        this->visit_expr_wrapper(m_args[2].m_value, true);
        llvm::Value* c = tmp;
        tmp = builder->CreateIntrinsic(llvm::Intrinsic::fma,
                {a->getType()},
                {b, c, a});
    }

    void generate_sign_from_value(ASR::call_arg_t* m_args) {
        this->visit_expr_wrapper(m_args[0].m_value, true);
        llvm::Value* arg0 = tmp;
        this->visit_expr_wrapper(m_args[1].m_value, true);
        llvm::Value* arg1 = tmp;
        llvm::Type* common_llvm_type = arg0->getType();
        ASR::ttype_t *arg1_type = ASRUtils::expr_type(m_args[1].m_value);
        uint64_t kind = ASRUtils::extract_kind_from_ttype_t(arg1_type);
        llvm::Value* num_shifts = llvm::ConstantInt::get(context, llvm::APInt(kind * 8, kind * 8 - 1));
        llvm::Value* shifted_one = builder->CreateShl(llvm::ConstantInt::get(context, llvm::APInt(kind * 8, 1)), num_shifts);
        arg1 = builder->CreateBitCast(arg1, shifted_one->getType());
        arg0 = builder->CreateBitCast(arg0, shifted_one->getType());
        tmp = builder->CreateXor(arg0, builder->CreateAnd(shifted_one, arg1));
        tmp = builder->CreateBitCast(tmp, common_llvm_type);
    }

    template <typename T>
    bool generate_optimization_instructions(const T* routine, ASR::call_arg_t* m_args) {
        std::string routine_name = std::string(routine->m_name);
        if( routine_name.find("flipsign") != std::string::npos ) {
            generate_flip_sign(m_args);
            return true;
        } else if( routine_name.find("fma") != std::string::npos ) {
            generate_fma(m_args);
            return true;
        } else if( routine_name.find("signfromvalue") != std::string::npos ) {
            generate_sign_from_value(m_args);
            return true;
        }
        return false;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        if( ASRUtils::is_intrinsic_optimization(x.m_name) ) {
            ASR::Subroutine_t* routine = ASR::down_cast<ASR::Subroutine_t>(
                        ASRUtils::symbol_get_past_external(x.m_name));
            if( generate_optimization_instructions(routine, x.m_args) ) {
                return ;
            }
        }
        ASR::Subroutine_t *s;
        std::vector<llvm::Value*> args;
        const ASR::symbol_t *proc_sym = symbol_get_past_external(x.m_name);
        if (x.m_dt){
            ASR::Variable_t *caller = EXPR2VAR(x.m_dt);
            std::uint32_t h = get_hash((ASR::asr_t*)caller);
            args.push_back(llvm_symtab[h]);
        }
        if (ASR::is_a<ASR::Subroutine_t>(*proc_sym)) {
            s = ASR::down_cast<ASR::Subroutine_t>(proc_sym);
        } else {
            ASR::ClassProcedure_t *clss_proc = ASR::down_cast<
                ASR::ClassProcedure_t>(proc_sym);
            s = ASR::down_cast<ASR::Subroutine_t>(clss_proc->m_proc);
        }
        if (parent_function){
            push_nested_stack(parent_function);
        } else if (parent_subroutine){
            push_nested_stack(parent_subroutine);
        }
        uint32_t h;
        if (s->m_abi == ASR::abiType::LFortranModule) {
            throw CodeGenError("Subroutine LFortran interfaces not implemented yet");
        } else if (s->m_abi == ASR::abiType::Interactive) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::Source) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::BindC) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::Intrinsic) {
            h = get_hash((ASR::asr_t*)s);
        } else {
            throw CodeGenError("ABI type not implemented yet in SubroutineCall.");
        }
        if (llvm_symtab_fn_arg.find(h) != llvm_symtab_fn_arg.end()) {
            // Check if this is a callback function
            llvm::Value* fn = llvm_symtab_fn_arg[h];
            llvm::FunctionType* fntype = llvm_symtab_fn[h]->getFunctionType();
            std::string m_name = std::string(((ASR::Subroutine_t*)(&(x.m_name->base)))->m_name);
            std::vector<llvm::Value *> args = convert_call_args(x, m_name);
            tmp = builder->CreateCall(fntype, fn, args);
        } else if (llvm_symtab_fn.find(h) == llvm_symtab_fn.end()) {
            throw CodeGenError("Subroutine code not generated for '"
                + std::string(s->m_name) + "'");
        } else {
            llvm::Function *fn = llvm_symtab_fn[h];
            std::string m_name = std::string(((ASR::Subroutine_t*)(&(x.m_name->base)))->m_name);
            std::vector<llvm::Value *> args2 = convert_call_args(x, m_name);
            args.insert(args.end(), args2.begin(), args2.end());
            builder->CreateCall(fn, args);
        }
        calling_function_hash = h;
        pop_nested_stack(s);
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        if( ASRUtils::is_intrinsic_optimization(x.m_name) ) {
            ASR::Function_t* routine = ASR::down_cast<ASR::Function_t>(
                        ASRUtils::symbol_get_past_external(x.m_name));
            if( generate_optimization_instructions(routine, x.m_args) ) {
                return ;
            }
        }
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        ASR::Function_t *s = nullptr;
        std::vector<llvm::Value*> args;
        const ASR::symbol_t *proc_sym = symbol_get_past_external(x.m_name);
        if (x.m_dt){
            ASR::Variable_t *caller = EXPR2VAR(x.m_dt);
            std::uint32_t h = get_hash((ASR::asr_t*)caller);
            args.push_back(llvm_symtab[h]);
        }
        if (ASR::is_a<ASR::Function_t>(*proc_sym)) {
            s = ASR::down_cast<ASR::Function_t>(proc_sym);
        } else {
            ASR::ClassProcedure_t *clss_proc = ASR::down_cast<
                ASR::ClassProcedure_t>(proc_sym);
            s = ASR::down_cast<ASR::Function_t>(clss_proc->m_proc);
        }
        if( s == nullptr ) {
            s = ASR::down_cast<ASR::Function_t>(symbol_get_past_external(x.m_name));
        }
        if (parent_function){
            push_nested_stack(parent_function);
        } else if (parent_subroutine){
            push_nested_stack(parent_subroutine);
        }
        bool intrinsic_function = ASRUtils::is_intrinsic_function2(s);
        uint32_t h;
        if (s->m_abi == ASR::abiType::Source && !intrinsic_function) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::LFortranModule) {
            throw CodeGenError("Function LFortran interfaces not implemented yet");
        } else if (s->m_abi == ASR::abiType::Interactive) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::BindC) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::Intrinsic || intrinsic_function) {
            std::string func_name = s->m_name;
            if( fname2arg_type.find(func_name) != fname2arg_type.end() ) {
                h = get_hash((ASR::asr_t*)s);
            } else {
                if (func_name == "len") {
                    std::vector<llvm::Value *> args = convert_call_args(x, "len");
                    LFORTRAN_ASSERT(args.size() == 1)
                    tmp = lfortran_str_len(args[0]);
                    return;
                }
                if( s->m_deftype == ASR::deftypeType::Interface ) {
                    throw CodeGenError("Intrinsic '" + func_name + "' not implemented yet and compile time value is not available.");
                } else {
                    h = get_hash((ASR::asr_t*)s);
                }
            }
        } else {
            throw CodeGenError("ABI type not implemented yet.");
        }
        if (llvm_symtab_fn_arg.find(h) != llvm_symtab_fn_arg.end()) {
            // Check if this is a callback function
            llvm::Value* fn = llvm_symtab_fn_arg[h];
            llvm::FunctionType* fntype = llvm_symtab_fn[h]->getFunctionType();
            std::string m_name = std::string(((ASR::Function_t*)(&(x.m_name->base)))->m_name);
            std::vector<llvm::Value *> args = convert_call_args(x, m_name);
            tmp = builder->CreateCall(fntype, fn, args);
        } else if (llvm_symtab_fn.find(h) == llvm_symtab_fn.end()) {
            throw CodeGenError("Function code not generated for '"
                + std::string(s->m_name) + "'");
        } else {
            llvm::Function *fn = llvm_symtab_fn[h];
            std::string m_name = std::string(((ASR::Function_t*)(&(x.m_name->base)))->m_name);
            std::vector<llvm::Value *> args2 = convert_call_args(x, m_name);
            args.insert(args.end(), args2.begin(), args2.end());
            if (s->m_abi == ASR::abiType::BindC) {
                ASR::ttype_t *return_var_type0 = EXPR2VAR(s->m_return_var)->m_type;
                if (is_a<ASR::Complex_t>(*return_var_type0)) {
                    int a_kind = down_cast<ASR::Complex_t>(return_var_type0)->m_kind;
                    if (a_kind == 8) {
                        if (platform == Platform::Windows) {
                            tmp = builder->CreateAlloca(complex_type_8, nullptr);
                            args.insert(args.begin(), tmp);
                            builder->CreateCall(fn, args);
                            // Convert {double,double}* to {double,double}
                            tmp = CreateLoad(tmp);
                        } else {
                            tmp = builder->CreateCall(fn, args);
                        }
                    } else {
                        tmp = builder->CreateCall(fn, args);
                    }
                } else {
                    tmp = builder->CreateCall(fn, args);
                }
            } else {
                tmp = builder->CreateCall(fn, args);
            }
        }
        if (s->m_abi == ASR::abiType::BindC) {
            ASR::ttype_t *return_var_type0 = EXPR2VAR(s->m_return_var)->m_type;
            if (is_a<ASR::Complex_t>(*return_var_type0)) {
                int a_kind = down_cast<ASR::Complex_t>(return_var_type0)->m_kind;
                if (a_kind == 4) {
                    if (platform == Platform::Windows) {
                        // tmp is i64, have to convert to {float, float}

                        // i64
                        llvm::Type* type_fx2 = llvm::Type::getInt64Ty(context);
                        // Convert i64 to i64*
                        llvm::AllocaInst *p_fx2 = builder->CreateAlloca(type_fx2, nullptr);
                        builder->CreateStore(tmp, p_fx2);
                        // Convert i64* to {float,float}* using bitcast
                        tmp = builder->CreateBitCast(p_fx2, complex_type_4->getPointerTo());
                        // Convert {float,float}* to {float,float}
                        tmp = CreateLoad(tmp);
                    } else if (platform == Platform::macOS_ARM) {
                        // pass
                    } else {
                        // tmp is <2 x float>, have to convert to {float, float}

                        // <2 x float>
                        llvm::Type* type_fx2 = llvm::FixedVectorType::get(llvm::Type::getFloatTy(context), 2);
                        // Convert <2 x float> to <2 x float>*
                        llvm::AllocaInst *p_fx2 = builder->CreateAlloca(type_fx2, nullptr);
                        builder->CreateStore(tmp, p_fx2);
                        // Convert <2 x float>* to {float,float}* using bitcast
                        tmp = builder->CreateBitCast(p_fx2, complex_type_4->getPointerTo());
                        // Convert {float,float}* to {float,float}
                        tmp = CreateLoad(tmp);
                    }
                }
            }
        }
        calling_function_hash = h;
        pop_nested_stack(s);
    }

};



Result<std::unique_ptr<LLVMModule>> asr_to_llvm(ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics,
        llvm::LLVMContext &context, Allocator &al, Platform platform,
        bool fast, const std::string &rl_path, const std::string &run_fn)
{
    ASRToLLVMVisitor v(al, context, platform, diagnostics);
    pass_wrap_global_stmts_into_function(al, asr, run_fn);

    // Uncomment for debugging the ASR after the transformation
    // std::cout << pickle(asr) << std::endl;
    pass_replace_class_constructor(al, asr);
    pass_replace_implied_do_loops(al, asr, rl_path);
    pass_replace_arr_slice(al, asr, rl_path);
    pass_replace_array_op(al, asr, rl_path);
    pass_replace_print_arr(al, asr, rl_path);

    if( fast ) {
        pass_loop_unroll(al, asr, rl_path);
    }

    pass_replace_do_loops(al, asr);
    pass_replace_forall(al, asr);
    pass_replace_select_case(al, asr);
    pass_unused_functions(al, asr);

    if( fast ) {
        pass_replace_flip_sign(al, asr, rl_path);
        pass_replace_sign_from_value(al, asr, rl_path);
        pass_replace_div_to_mul(al, asr, rl_path);
        pass_replace_fma(al, asr, rl_path);
        pass_inline_function_calls(al, asr, rl_path);
    }

    v.nested_func_types = pass_find_nested_vars(asr, context,
        v.nested_globals, v.nested_call_out, v.nesting_map);
    try {
        v.visit_asr((ASR::asr_t&)asr);
    } catch (const CodeGenError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    } catch (const CodeGenAbort &) {
        LFORTRAN_ASSERT(diagnostics.has_error())
        Error error;
        return error;
    }
    std::string msg;
    llvm::raw_string_ostream err(msg);
    if (llvm::verifyModule(*v.module, &err)) {
        std::string buf;
        llvm::raw_string_ostream os(buf);
        v.module->print(os, nullptr);
        std::cout << os.str();
        std::string msg = "asr_to_llvm: module failed verification. Error:\n"
            + err.str();
        diagnostics.diagnostics.push_back(diag::Diagnostic(msg,
            diag::Level::Error, diag::Stage::CodeGen));
        Error error;
        return error;
    };
    return std::make_unique<LLVMModule>(std::move(v.module));
}

} // namespace LFortran
