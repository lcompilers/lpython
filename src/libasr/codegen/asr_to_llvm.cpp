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
#include <llvm/IR/DIBuilder.h>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_llvm.h>
#include <libasr/pass/nested_vars.h>
#include <libasr/pass/pass_manager.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/codegen/llvm_utils.h>
#include <libasr/codegen/llvm_array_utils.h>

#if LLVM_VERSION_MAJOR >= 11
#    define FIXED_VECTOR_TYPE llvm::FixedVectorType
#else
#    define FIXED_VECTOR_TYPE llvm::VectorType
#endif


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
using LFortran::ASRUtils::intent_local;
using LFortran::ASRUtils::intent_return_var;
using LFortran::ASRUtils::determine_module_dependencies;
using LFortran::ASRUtils::is_arg_dummy;

// Platform dependent fast unique hash:
uint64_t static get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
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
  //! To be used by visit_StructInstanceMember.
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
    std::string infile;
    Allocator &al;

    llvm::Value *tmp;
    llvm::BasicBlock *current_loophead, *current_loopend, *proc_return;
    std::string mangle_prefix;
    bool prototype_only;
    llvm::StructType *complex_type_4, *complex_type_8;
    llvm::StructType *complex_type_4_ptr, *complex_type_8_ptr;
    llvm::PointerType *character_type;
    llvm::PointerType *list_type;

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

    std::unique_ptr<LLVMUtils> llvm_utils;
    std::unique_ptr<LLVMList> list_api;
    std::unique_ptr<LLVMTuple> tuple_api;
    std::unique_ptr<LLVMDictInterface> dict_api_lp;
    std::unique_ptr<LLVMDictInterface> dict_api_sc;
    std::unique_ptr<LLVMArrUtils::Descriptor> arr_descr;

    int64_t ptr_loads;
    bool lookup_enum_value_for_nonints;
    bool is_assignment_target;

    CompilerOptions &compiler_options;

    // For handling debug information
    std::unique_ptr<llvm::DIBuilder> DBuilder;
    llvm::DICompileUnit *debug_CU;
    llvm::DIScope *debug_current_scope;
    std::map<uint64_t, llvm::DIScope*> llvm_symtab_fn_discope;
    llvm::DIFile *debug_Unit;

    ASRToLLVMVisitor(Allocator &al, llvm::LLVMContext &context, std::string infile,
        CompilerOptions &compiler_options_, diag::Diagnostics &diagnostics) :
    diag{diagnostics},
    context(context),
    builder(std::make_unique<llvm::IRBuilder<>>(context)),
    infile{infile},
    al{al},
    prototype_only(false),
    llvm_utils(std::make_unique<LLVMUtils>(context, builder.get())),
    list_api(std::make_unique<LLVMList>(context, llvm_utils.get(), builder.get())),
    tuple_api(std::make_unique<LLVMTuple>(context, llvm_utils.get(), builder.get())),
    dict_api_lp(std::make_unique<LLVMDictOptimizedLinearProbing>(context, llvm_utils.get(), builder.get())),
    dict_api_sc(std::make_unique<LLVMDictSeparateChaining>(context, llvm_utils.get(), builder.get())),
    arr_descr(LLVMArrUtils::Descriptor::get_descriptor(context,
              builder.get(),
              llvm_utils.get(),
              LLVMArrUtils::DESCR_TYPE::_SimpleCMODescriptor)),
    ptr_loads(2),
    lookup_enum_value_for_nonints(false),
    is_assignment_target(false),
    compiler_options(compiler_options_)
    {
        llvm_utils->tuple_api = tuple_api.get();
        llvm_utils->list_api = list_api.get();
        llvm_utils->dict_api = nullptr;
        llvm_utils->arr_api = arr_descr.get();
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

    // Note: `create_if_else` and `create_loop` are optional APIs
    // that do not have to be used. Many times, for more complicated
    // things, it might be more readable to just use the LLVM API
    // without any extra layer on top. In some other cases, it might
    // be more readable to use this abstraction.
    template <typename IF, typename ELSE>
    void create_if_else(llvm::Value * cond, IF if_block, ELSE else_block) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();

        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");

        builder->CreateCondBr(cond, thenBB, elseBB);
        builder->SetInsertPoint(thenBB); {
            if_block();
        }
        builder->CreateBr(mergeBB);

        start_new_block(elseBB); {
            else_block();
        }
        start_new_block(mergeBB);
    }

    template <typename Cond, typename Body>
    void create_loop(Cond condition, Body loop_body) {
        dict_api_lp->set_iterators();
        dict_api_sc->set_iterators();
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        this->current_loophead = loophead;
        this->current_loopend = loopend;

        // head
        start_new_block(loophead); {
            llvm::Value* cond = condition();
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        start_new_block(loopbody); {
            loop_body();
            builder->CreateBr(loophead);
        }

        // end
        start_new_block(loopend);
        dict_api_lp->reset_iterators();
        dict_api_sc->reset_iterators();
    }

    void get_type_debug_info(ASR::ttype_t* t, std::string &type_name,
            uint32_t &type_size, uint32_t &type_encoding) {
        type_size = ASRUtils::extract_kind_from_ttype_t(t)*8;
        switch( t->type ) {
            case ASR::ttypeType::Integer: {
                type_name = "integer";
                type_encoding = llvm::dwarf::DW_ATE_signed;
                break;
            }
            case ASR::ttypeType::Logical: {
                type_name = "boolean";
                type_encoding = llvm::dwarf::DW_ATE_boolean;
                break;
            }
            case ASR::ttypeType::Real: {
                if( type_size == 32 ) {
                    type_name = "float";
                } else if( type_size == 64 ) {
                    type_name = "double";
                }
                type_encoding = llvm::dwarf::DW_ATE_float;
                break;
            }
            default : throw LCompilersException("Debug information for the type: `"
                + ASRUtils::type_to_str_python(t) + "` is not yet implemented");
        }
    }

    void debug_get_line_column(const uint32_t &loc_first,
            uint32_t &line, uint32_t &column) {
        LocationManager lm;
        LocationManager::FileLocations fl;
        fl.in_filename = infile;
        lm.files.push_back(fl);
        std::string input = LFortran::read_file(infile);
        lm.init_simple(input);
        lm.file_ends.push_back(input.size());
        lm.pos_to_linecol(lm.output_to_input_pos(loc_first, false),
            line, column, fl.in_filename);
    }

    template <typename T>
    void debug_emit_loc(const T &x) {
        Location loc = x.base.base.loc;
        uint32_t line, column;
        if (compiler_options.emit_debug_line_column) {
            debug_get_line_column(loc.first, line, column);
        } else {
            line = loc.first;
            column = 0;
        }
        builder->SetCurrentDebugLocation(
            llvm::DILocation::get(debug_current_scope->getContext(),
                line, column, debug_current_scope));
    }

    template <typename T>
    void debug_emit_function(const T &x, llvm::DISubprogram *&SP) {
        debug_Unit = DBuilder->createFile(
            debug_CU->getFilename(),
            debug_CU->getDirectory());
        llvm::DIScope *FContext = debug_Unit;
        uint32_t line, column;
        if (compiler_options.emit_debug_line_column) {
            debug_get_line_column(x.base.base.loc.first, line, column);
        } else {
            line = 0;
        }
        std::string fn_debug_name = x.m_name;
        llvm::DIBasicType *return_type_info = nullptr;
        if constexpr (std::is_same_v<T, ASR::Function_t>){
            if(x.m_return_var != nullptr) {
                std::string type_name; uint32_t type_size, type_encoding;
                get_type_debug_info(ASRUtils::expr_type(x.m_return_var),
                    type_name, type_size, type_encoding);
                return_type_info = DBuilder->createBasicType(type_name,
                    type_size, type_encoding);
            }
        } else if constexpr (std::is_same_v<T, ASR::Program_t>) {
            return_type_info = DBuilder->createBasicType("integer", 32,
                llvm::dwarf::DW_ATE_signed);
        }
        llvm::DISubroutineType *return_type = DBuilder->createSubroutineType(
            DBuilder->getOrCreateTypeArray(return_type_info));
        SP = DBuilder->createFunction(
            FContext, fn_debug_name, llvm::StringRef(), debug_Unit,
            line, return_type, 0, // TODO: ScopeLine
            llvm::DINode::FlagPrototyped,
            llvm::DISubprogram::SPFlagDefinition);
        debug_current_scope = SP;
    }

    inline bool verify_dimensions_t(ASR::dimension_t* m_dims, int n_dims) {
        if( n_dims <= 0 ) {
            return false;
        }
        bool is_ok = true;
        for( int r = 0; r < n_dims; r++ ) {
            if( m_dims[r].m_length == nullptr ) {
                is_ok = false;
                break;
            }
        }
        return is_ok;
    }

    llvm::Type*
    get_el_type(ASR::ttype_t* m_type_) {
        int a_kind = ASRUtils::extract_kind_from_ttype_t(m_type_);
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
                case ASR::ttypeType::Struct: {
                    el_type = getStructType(m_type_);
                    break;
                }
                case ASR::ttypeType::Union: {
                    el_type = getUnionType(m_type_);
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
                case ASR::ttypeType::Struct: {
                    el_type = getStructType(m_type_);
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

    void fill_array_details(llvm::Value* arr, llvm::Type* llvm_data_type,
                            ASR::dimension_t* m_dims, int n_dims, bool is_data_only=false) {
        std::vector<std::pair<llvm::Value*, llvm::Value*>> llvm_dims;
        for( int r = 0; r < n_dims; r++ ) {
            ASR::dimension_t m_dim = m_dims[r];
            visit_expr(*(m_dim.m_start));
            llvm::Value* start = tmp;
            visit_expr(*(m_dim.m_length));
            llvm::Value* end = tmp;
            llvm_dims.push_back(std::make_pair(start, end));
        }
        if( is_data_only ) {
            if( !ASRUtils::is_fixed_size_array(m_dims, n_dims) ) {
                llvm::Value* const_1 = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
                llvm::Value* prod = const_1;
                for( int r = 0; r < n_dims; r++ ) {
                    llvm::Value* dim_size = llvm_dims[r].second;
                    prod = builder->CreateMul(prod, dim_size);
                }
                llvm::Value* arr_first = builder->CreateAlloca(llvm_data_type, prod);
                builder->CreateStore(arr_first, arr);
            }
        } else {
            arr_descr->fill_array_details(arr, llvm_data_type, n_dims, llvm_dims);
        }
    }

    /*
        This function fills the descriptor
        (pointer to the first element, offset and descriptor of each dimension)
        of the array which are allocated memory in heap.
    */
    inline void fill_malloc_array_details(llvm::Value* arr, llvm::Type* llvm_data_type,
                                          ASR::dimension_t* m_dims, int n_dims) {
        std::vector<std::pair<llvm::Value*, llvm::Value*>> llvm_dims;
        for( int r = 0; r < n_dims; r++ ) {
            ASR::dimension_t m_dim = m_dims[r];
            visit_expr(*(m_dim.m_start));
            llvm::Value* start = tmp;
            visit_expr(*(m_dim.m_length));
            llvm::Value* end = tmp;
            llvm_dims.push_back(std::make_pair(start, end));
        }
        arr_descr->fill_malloc_array_details(arr, llvm_data_type,
            n_dims, llvm_dims, module.get());
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
                    throw CodeGenError("Only 8, 16, 32 and 64 bits integer kinds are supported.");
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
                    throw CodeGenError("Only 8, 16, 32 and 64 bits integer kinds are supported.");
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

    llvm::Type* getMemberType(ASR::ttype_t* mem_type, ASR::Variable_t* member) {
        llvm::Type* llvm_mem_type = nullptr;
        switch( mem_type->type ) {
            case ASR::ttypeType::Integer: {
                int a_kind = down_cast<ASR::Integer_t>(mem_type)->m_kind;
                llvm_mem_type = getIntType(a_kind);
                break;
            }
            case ASR::ttypeType::Real: {
                int a_kind = down_cast<ASR::Real_t>(mem_type)->m_kind;
                llvm_mem_type = getFPType(a_kind);
                break;
            }
            case ASR::ttypeType::Struct: {
                llvm_mem_type = getStructType(mem_type);
                break;
            }
            case ASR::ttypeType::Enum: {
                llvm_mem_type = llvm::Type::getInt32Ty(context);
                break ;
            }
            case ASR::ttypeType::Union: {
                llvm_mem_type = getUnionType(mem_type);
                break;
            }
            case ASR::ttypeType::Pointer: {
                ASR::Pointer_t* ptr_type = ASR::down_cast<ASR::Pointer_t>(mem_type);
                llvm_mem_type = getMemberType(ptr_type->m_type, member)->getPointerTo();
                break;
            }
            case ASR::ttypeType::Complex: {
                int a_kind = down_cast<ASR::Complex_t>(mem_type)->m_kind;
                llvm_mem_type = getComplexType(a_kind);
                break;
            }
            case ASR::ttypeType::Character: {
                llvm_mem_type = character_type;
                break;
            }
            case ASR::ttypeType::CPtr: {
                llvm_mem_type = llvm::Type::getVoidTy(context)->getPointerTo();
                break;
            }
            default:
                throw CodeGenError("Cannot identify the type of member, '" +
                                    std::string(member->m_name) +
                                    "' in derived type, '" + der_type_name + "'.",
                                    member->base.base.loc);
        }
        return llvm_mem_type;
    }

    llvm::Type* getStructType(ASR::StructType_t* der_type, bool is_pointer=false) {
        std::string der_type_name = std::string(der_type->m_name);
        llvm::StructType* der_type_llvm;
        if( name2dertype.find(der_type_name) != name2dertype.end() ) {
            der_type_llvm = name2dertype[der_type_name];
        } else {
            std::vector<llvm::Type*> member_types;
            int member_idx = 0;
            if( der_type->m_parent != nullptr ) {
                ASR::StructType_t *par_der_type = ASR::down_cast<ASR::StructType_t>(
                                                        symbol_get_past_external(der_type->m_parent));
                llvm::Type* par_llvm = getStructType(par_der_type);
                member_types.push_back(par_llvm);
                dertype2parent[der_type_name] = std::string(par_der_type->m_name);
                member_idx += 1;
            }

            for( size_t i = 0; i < der_type->n_members; i++ ) {
                std::string member_name = der_type->m_members[i];
                ASR::Variable_t* member = ASR::down_cast<ASR::Variable_t>(der_type->m_symtab->get_symbol(member_name));
                llvm::Type* llvm_mem_type = get_type_from_ttype_t_util(member->m_type, member->m_abi);
                member_types.push_back(llvm_mem_type);
                name2memidx[der_type_name][std::string(member->m_name)] = member_idx;
                member_idx++;
            }
            der_type_llvm = llvm::StructType::create(context,
                                member_types,
                                der_type_name,
                                der_type->m_is_packed);
            name2dertype[der_type_name] = der_type_llvm;
        }
        if( is_pointer ) {
            return der_type_llvm->getPointerTo();
        }
        return (llvm::Type*) der_type_llvm;
    }

    llvm::Type* getStructType(ASR::ttype_t* _type, bool is_pointer=false) {
        ASR::Struct_t* der = (ASR::Struct_t*)(&(_type->base));
        ASR::symbol_t* der_sym;
        if( der->m_derived_type->type == ASR::symbolType::ExternalSymbol ) {
            ASR::ExternalSymbol_t* der_extr = (ASR::ExternalSymbol_t*)(&(der->m_derived_type->base));
            der_sym = der_extr->m_external;
        } else {
            der_sym = der->m_derived_type;
        }
        ASR::StructType_t* der_type = (ASR::StructType_t*)(&(der_sym->base));
        return getStructType(der_type, is_pointer);
    }

    llvm::Type* getUnionType(ASR::UnionType_t* union_type, bool is_pointer=false) {
        std::string union_type_name = std::string(union_type->m_name);
        llvm::StructType* union_type_llvm = nullptr;
        if( name2dertype.find(union_type_name) != name2dertype.end() ) {
            union_type_llvm = name2dertype[union_type_name];
        } else {
            const std::map<std::string, ASR::symbol_t*>& scope = union_type->m_symtab->get_scope();
            llvm::DataLayout data_layout(module.get());
            llvm::Type* max_sized_type = nullptr;
            size_t max_type_size = 0;
            for( auto itr = scope.begin(); itr != scope.end(); itr++ ) {
                ASR::Variable_t* member = ASR::down_cast<ASR::Variable_t>(itr->second);
                llvm::Type* llvm_mem_type = getMemberType(member->m_type, member);
                size_t type_size = data_layout.getTypeAllocSize(llvm_mem_type);
                if( max_type_size < type_size ) {
                    max_sized_type = llvm_mem_type;
                    type_size = max_type_size;
                }
            }
            union_type_llvm = llvm::StructType::create(context, {max_sized_type}, union_type_name);
            name2dertype[union_type_name] = union_type_llvm;
        }
        if( is_pointer ) {
            return union_type_llvm->getPointerTo();
        }
        return (llvm::Type*) union_type_llvm;
    }

    llvm::Type* getUnionType(ASR::ttype_t* _type, bool is_pointer=false) {
        ASR::Union_t* union_ = ASR::down_cast<ASR::Union_t>(_type);
        ASR::symbol_t* union_sym = ASRUtils::symbol_get_past_external(union_->m_union_type);
        ASR::UnionType_t* union_type = ASR::down_cast<ASR::UnionType_t>(union_sym);
        return getUnionType(union_type, is_pointer);
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
            const std::map<std::string, ASR::symbol_t*>& scope = der_type->m_symtab->get_scope();
            std::vector<llvm::Type*> member_types;
            int member_idx = 0;
            for( auto itr = scope.begin(); itr != scope.end(); itr++ ) {
                if (!ASR::is_a<ASR::ClassProcedure_t>(*itr->second) &&
                    !ASR::is_a<ASR::GenericProcedure_t>(*itr->second) &&
                    !ASR::is_a<ASR::CustomOperator_t>(*itr->second)) {
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

    llvm::Value* lfortran_str_cmp(llvm::Value* left_arg, llvm::Value* right_arg,
                                         std::string runtime_func_name)
    {
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if(!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getInt1Ty(context), {
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
        std::vector<llvm::Value*> args = {pleft_arg, pright_arg};
        return builder->CreateCall(fn, args);
    }

    llvm::Value* lfortran_strrepeat(llvm::Value* left_arg, llvm::Value* right_arg)
    {
        std::string runtime_func_name = "_lfortran_strrepeat";
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {
                        character_type->getPointerTo(),
                        llvm::Type::getInt32Ty(context),
                        character_type->getPointerTo()
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        llvm::AllocaInst *pleft_arg = builder->CreateAlloca(character_type,
            nullptr);
        builder->CreateStore(left_arg, pleft_arg);
        llvm::AllocaInst *presult = builder->CreateAlloca(character_type,
            nullptr);
        std::vector<llvm::Value*> args = {pleft_arg, right_arg, presult};
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

    llvm::Value* lfortran_str_to_int(llvm::Value* str)
    {
        std::string runtime_func_name = "_lfortran_str_to_int";
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

    llvm::Value* lfortran_str_ord(llvm::Value* str)
    {
        std::string runtime_func_name = "_lfortran_str_ord";
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

    llvm::Value* lfortran_str_chr(llvm::Value* str)
    {
        std::string runtime_func_name = "_lfortran_str_chr";
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    character_type, {
                        llvm::Type::getInt32Ty(context)
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        return builder->CreateCall(fn, {str});
    }

    llvm::Value* lfortran_str_copy(llvm::Value* str, llvm::Value* idx1, llvm::Value* idx2)
    {
        std::string runtime_func_name = "_lfortran_str_copy";
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    character_type, {
                        character_type, llvm::Type::getInt32Ty(context), llvm::Type::getInt32Ty(context)
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        return builder->CreateCall(fn, {str, idx1, idx2});
    }

    llvm::Value* lfortran_str_slice(llvm::Value* str, llvm::Value* idx1, llvm::Value* idx2,
                    llvm::Value* step, llvm::Value* left_present, llvm::Value* right_present)
    {
        std::string runtime_func_name = "_lfortran_str_slice";
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    character_type, {
                        character_type, llvm::Type::getInt32Ty(context),
                        llvm::Type::getInt32Ty(context), llvm::Type::getInt32Ty(context),
                        llvm::Type::getInt1Ty(context), llvm::Type::getInt1Ty(context)
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        return builder->CreateCall(fn, {str, idx1, idx2, step, left_present, right_present});
    }

    llvm::Value* lfortran_type_to_str(llvm::Value* arg, llvm::Type* value_type, std::string type, int value_kind) {
        std::string func_name = "_lfortran_" + type + "_to_str" + std::to_string(value_kind);
         llvm::Function *fn = module->getFunction(func_name);
         if(!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                 character_type, {
                     value_type
                 }, false);
            fn = llvm::Function::Create(function_type,
                     llvm::Function::ExternalLinkage, func_name, *module);
         }
         llvm::Value* res = builder->CreateCall(fn, {arg});
         return res;
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

        if (compiler_options.emit_debug_info) {
            DBuilder = std::make_unique<llvm::DIBuilder>(*module);
            debug_CU = DBuilder->createCompileUnit(
                llvm::dwarf::DW_LANG_C, DBuilder->createFile(infile, "."),
                "LPython Compiler", false, "", 0);
        }

        // All loose statements must be converted to a function, so the items
        // must be empty:
        LCOMPILERS_ASSERT(x.n_items == 0);

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
        list_type = llvm::Type::getInt8PtrTy(context);

        llvm::Type* bound_arg = static_cast<llvm::Type*>(arr_descr->get_dimension_descriptor_type(true));
        fname2arg_type["lbound"] = std::make_pair(bound_arg, bound_arg->getPointerTo());
        fname2arg_type["ubound"] = std::make_pair(bound_arg, bound_arg->getPointerTo());

        // Process Variables first:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (is_a<ASR::Variable_t>(*item.second) ||
                is_a<ASR::EnumType_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }

        prototype_only = false;
        for (auto &item : x.m_global_scope->get_scope()) {
            if (is_a<ASR::Module_t>(*item.second) &&
                item.first.find("lfortran_intrinsic_optimization") != std::string::npos) {
                ASR::Module_t* mod = ASR::down_cast<ASR::Module_t>(item.second);
                for( auto &moditem: mod->m_symtab->get_scope() ) {
                    ASR::symbol_t* sym = ASRUtils::symbol_get_past_external(moditem.second);
                    if (is_a<ASR::Function_t>(*sym)) {
                        visit_Function(*ASR::down_cast<ASR::Function_t>(sym));
                    }
                }
            }
        }

        prototype_only = true;
        // Generate function prototypes
        for (auto &item : x.m_global_scope->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                if (ASR::down_cast<ASR::Function_t>(item.second)->n_type_params == 0) {
                    visit_Function(*ASR::down_cast<ASR::Function_t>(item.second));
                }
            }
        }
        prototype_only = false;

        // TODO: handle depencencies across modules and main program

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = determine_module_dependencies(x);
        for (auto &item : build_order) {
            LCOMPILERS_ASSERT(x.m_global_scope->get_symbol(item)
                != nullptr);
            ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
            visit_symbol(*mod);
        }

        // Then do all the procedures
        for (auto &item : x.m_global_scope->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                if (ASR::down_cast<ASR::Function_t>(item.second)->n_type_params == 0) {
                    visit_symbol(*item.second);
                }
            }
        }

        // Then the main program
        for (auto &item : x.m_global_scope->get_scope()) {
            if (is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }
    }

    void visit_Allocate(const ASR::Allocate_t& x) {
        for( size_t i = 0; i < x.n_args; i++ ) {
            ASR::alloc_arg_t curr_arg = x.m_args[i];
            std::uint32_t h = get_hash((ASR::asr_t*)curr_arg.m_a);
            LCOMPILERS_ASSERT(llvm_symtab.find(h) != llvm_symtab.end());
            llvm::Value* x_arr = llvm_symtab[h];
            ASR::ttype_t* curr_arg_m_a_type = ASRUtils::symbol_type(curr_arg.m_a);
            ASR::ttype_t* asr_data_type = ASRUtils::duplicate_type_without_dims(al,
                curr_arg_m_a_type, curr_arg_m_a_type->base.loc);
            llvm::Type* llvm_data_type = get_type_from_ttype_t_util(asr_data_type);
            fill_malloc_array_details(x_arr, llvm_data_type, curr_arg.m_dims, curr_arg.n_dims);
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
                create_if_else(cond, [=]() {
                    call_lfortran_free(free_fn);
                }, [](){});
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

    void visit_ListConstant(const ASR::ListConstant_t& x) {
        ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(x.m_type);
        bool is_array_type_local = false, is_malloc_array_type_local = false;
        bool is_list_local = false;
        ASR::dimension_t* m_dims_local = nullptr;
        int n_dims_local = -1, a_kind_local = -1;
        llvm::Type* llvm_el_type = get_type_from_ttype_t(list_type->m_type,
                                    ASR::storage_typeType::Default, is_array_type_local,
                                    is_malloc_array_type_local, is_list_local, m_dims_local,
                                    n_dims_local, a_kind_local);
        std::string type_code = ASRUtils::get_type_code(list_type->m_type);
        int32_t type_size = -1;
        if( ASR::is_a<ASR::Character_t>(*list_type->m_type) ||
            LLVM::is_llvm_struct(list_type->m_type) ||
            ASR::is_a<ASR::Complex_t>(*list_type->m_type) ) {
            llvm::DataLayout data_layout(module.get());
            type_size = data_layout.getTypeAllocSize(llvm_el_type);
        } else {
            type_size = ASRUtils::extract_kind_from_ttype_t(list_type->m_type);
        }
        llvm::Type* const_list_type = list_api->get_list_type(llvm_el_type, type_code, type_size);
        llvm::Value* const_list = builder->CreateAlloca(const_list_type, nullptr, "const_list");
        list_api->list_init(type_code, const_list, *module, x.n_args, x.n_args);
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 1;
        for( size_t i = 0; i < x.n_args; i++ ) {
            this->visit_expr(*x.m_args[i]);
            llvm::Value* item = tmp;
            llvm::Value* pos = llvm::ConstantInt::get(context, llvm::APInt(32, i));
            list_api->write_item(const_list, pos, item, list_type->m_type,
                                 false, module.get(), name2memidx);
        }
        ptr_loads = ptr_loads_copy;
        tmp = const_list;
    }

    void set_dict_api(ASR::Dict_t* dict_type) {
        if( ASR::is_a<ASR::Character_t>(*dict_type->m_key_type) ) {
            llvm_utils->dict_api = dict_api_sc.get();
        } else {
            llvm_utils->dict_api = dict_api_lp.get();
        }
    }

    void visit_DictConstant(const ASR::DictConstant_t& x) {
        llvm::Type* const_dict_type = get_dict_type(x.m_type);
        llvm::Value* const_dict = builder->CreateAlloca(const_dict_type, nullptr, "const_dict");
        ASR::Dict_t* x_dict = ASR::down_cast<ASR::Dict_t>(x.m_type);
        set_dict_api(x_dict);
        std::string key_type_code = ASRUtils::get_type_code(x_dict->m_key_type);
        std::string value_type_code = ASRUtils::get_type_code(x_dict->m_value_type);
        llvm_utils->dict_api->dict_init(key_type_code, value_type_code, const_dict, module.get(), x.n_keys);
        int64_t ptr_loads_key = LLVM::is_llvm_struct(x_dict->m_key_type) ? 0 : 2;
        int64_t ptr_loads_value = LLVM::is_llvm_struct(x_dict->m_value_type) ? 0 : 2;
        int64_t ptr_loads_copy = ptr_loads;
        for( size_t i = 0; i < x.n_keys; i++ ) {
            ptr_loads = ptr_loads_key;
            visit_expr(*x.m_keys[i]);
            llvm::Value* key = tmp;
            ptr_loads = ptr_loads_value;
            visit_expr(*x.m_values[i]);
            llvm::Value* value = tmp;
            llvm_utils->dict_api->write_item(const_dict, key, value, module.get(),
                                 x_dict->m_key_type, x_dict->m_value_type, name2memidx);
        }
        ptr_loads = ptr_loads_copy;
        tmp = const_dict;
    }

    void visit_TupleConstant(const ASR::TupleConstant_t& x) {
        ASR::Tuple_t* tuple_type = ASR::down_cast<ASR::Tuple_t>(x.m_type);
        std::string type_code = ASRUtils::get_type_code(tuple_type->m_type,
                                                        tuple_type->n_type);
        std::vector<llvm::Type*> llvm_el_types;
        ASR::storage_typeType m_storage = ASR::storage_typeType::Default;
        bool is_array_type = false, is_malloc_array_type = false;
        bool is_list = false;
        ASR::dimension_t* m_dims = nullptr;
        int n_dims = 0, a_kind = -1;
        for( size_t i = 0; i < tuple_type->n_type; i++ ) {
            llvm_el_types.push_back(get_type_from_ttype_t(tuple_type->m_type[i],
                                    m_storage, is_array_type, is_malloc_array_type,
                                    is_list, m_dims, n_dims, a_kind));
        }
        llvm::Type* const_tuple_type = tuple_api->get_tuple_type(type_code, llvm_el_types);
        llvm::Value* const_tuple = builder->CreateAlloca(const_tuple_type, nullptr, "const_tuple");
        std::vector<llvm::Value*> init_values;
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 2;
        for( size_t i = 0; i < x.n_elements; i++ ) {
            this->visit_expr(*x.m_elements[i]);
            init_values.push_back(tmp);
        }
        ptr_loads = ptr_loads_copy;
        tuple_api->tuple_init(const_tuple, init_values);
        tmp = const_tuple;
    }

    void visit_IntegerBitLen(const ASR::IntegerBitLen_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr(*x.m_a);
        llvm::Value *int_val = tmp;
        int int_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        std::string runtime_func_name = "_lpython_bit_length" + std::to_string(int_kind);
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getInt32Ty(context), {
                        getIntType(int_kind)
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        tmp = builder->CreateCall(fn, {int_val});
    }

    void visit_Ichar(const ASR::Ichar_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr(*x.m_arg);
        llvm::Value *c = tmp;
        std::string runtime_func_name = "_lfortran_ichar";
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context), {
                    llvm::Type::getInt8PtrTy(context)
                }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        tmp = builder->CreateCall(fn, {c});
    }

    void visit_Iachar(const ASR::Iachar_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr(*x.m_arg);
        llvm::Value *c = tmp;
        std::string runtime_func_name = "_lfortran_iachar";
        llvm::Function *fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context), {
                    llvm::Type::getInt8PtrTy(context)
                }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        tmp = builder->CreateCall(fn, {c});
    }

    void visit_ListAppend(const ASR::ListAppend_t& x) {
        ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(ASRUtils::expr_type(x.m_a));
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* plist = tmp;

        ptr_loads = !LLVM::is_llvm_struct(asr_list->m_type);
        this->visit_expr_wrapper(x.m_ele, true);
        llvm::Value *item = tmp;
        ptr_loads = ptr_loads_copy;

        list_api->append(plist, item, asr_list->m_type, module.get(), name2memidx);
    }

    void visit_UnionRef(const ASR::UnionRef_t& x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_v);
        ptr_loads = ptr_loads_copy;
        llvm::Value* union_llvm = tmp;
        ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(x.m_m);
        ASR::ttype_t* member_type_asr = ASRUtils::get_contained_type(member_var->m_type);
        if( ASR::is_a<ASR::Struct_t>(*member_type_asr) ) {
            ASR::Struct_t* d = ASR::down_cast<ASR::Struct_t>(member_type_asr);
            der_type_name = ASRUtils::symbol_name(d->m_derived_type);
        }
        member_type_asr = member_var->m_type;
        llvm::Type* member_type_llvm = getMemberType(member_type_asr, member_var)->getPointerTo();
        tmp = builder->CreateBitCast(union_llvm, member_type_llvm);
        if( is_assignment_target ) {
            return ;
        }
        if( ptr_loads > 0 ) {
            tmp = LLVM::CreateLoad(*builder, tmp);
        }
    }

    void visit_ListItem(const ASR::ListItem_t& x) {
        ASR::ttype_t* el_type = ASRUtils::get_contained_type(
                                        ASRUtils::expr_type(x.m_a));
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* plist = tmp;

        ptr_loads = 1;
        this->visit_expr_wrapper(x.m_pos, true);
        ptr_loads = ptr_loads_copy;
        llvm::Value *pos = tmp;

        tmp = list_api->read_item(plist, pos, compiler_options.enable_bounds_checking, *module,
                (LLVM::is_llvm_struct(el_type) || ptr_loads == 0));
    }

    void visit_DictItem(const ASR::DictItem_t& x) {
        ASR::Dict_t* dict_type = ASR::down_cast<ASR::Dict_t>(
                                    ASRUtils::expr_type(x.m_a));
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* pdict = tmp;

        ptr_loads = !LLVM::is_llvm_struct(dict_type->m_key_type);
        this->visit_expr_wrapper(x.m_key, true);
        ptr_loads = ptr_loads_copy;
        llvm::Value *key = tmp;

        set_dict_api(dict_type);
        tmp = llvm_utils->dict_api->read_item(pdict, key, *module, dict_type,
                                  LLVM::is_llvm_struct(dict_type->m_value_type));
    }

    void visit_DictPop(const ASR::DictPop_t& x) {
        ASR::Dict_t* dict_type = ASR::down_cast<ASR::Dict_t>(
                                    ASRUtils::expr_type(x.m_a));
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* pdict = tmp;

        ptr_loads = !LLVM::is_llvm_struct(dict_type->m_key_type);
        this->visit_expr_wrapper(x.m_key, true);
        ptr_loads = ptr_loads_copy;
        llvm::Value *key = tmp;

        set_dict_api(dict_type);
        tmp = llvm_utils->dict_api->pop_item(pdict, key, *module, dict_type,
                                 LLVM::is_llvm_struct(dict_type->m_value_type));
    }

    void visit_ListLen(const ASR::ListLen_t& x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
        } else {
            int64_t ptr_loads_copy = ptr_loads;
            ptr_loads = 0;
            this->visit_expr(*x.m_arg);
            ptr_loads = ptr_loads_copy;
            llvm::Value* plist = tmp;
            tmp = list_api->len(plist);
        }
    }

    void visit_ListCompare(const ASR::ListCompare_t x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_left);
        llvm::Value* left = tmp;
        this->visit_expr(*x.m_right);
        llvm::Value* right = tmp;
        ptr_loads = ptr_loads_copy;
        tmp = llvm_utils->is_equal_by_value(left, right, *module,
                ASRUtils::expr_type(x.m_left));
        if (x.m_op == ASR::cmpopType::NotEq) {
            tmp = builder->CreateNot(tmp);
        }
    }

    void visit_DictLen(const ASR::DictLen_t& x) {
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            return ;
        }

        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_arg);
        ptr_loads = ptr_loads_copy;
        llvm::Value* pdict = tmp;
        ASR::Dict_t* x_dict = ASR::down_cast<ASR::Dict_t>(ASRUtils::expr_type(x.m_arg));
        set_dict_api(x_dict);
        tmp = llvm_utils->dict_api->len(pdict);
    }

    void visit_ListInsert(const ASR::ListInsert_t& x) {
        ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(
                                    ASRUtils::expr_type(x.m_a));
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* plist = tmp;

        ptr_loads = 1;
        this->visit_expr_wrapper(x.m_pos, true);
        llvm::Value *pos = tmp;

        ptr_loads = !LLVM::is_llvm_struct(asr_list->m_type);
        this->visit_expr_wrapper(x.m_ele, true);
        llvm::Value *item = tmp;
        ptr_loads = ptr_loads_copy;

        list_api->insert_item(plist, pos, item, asr_list->m_type, module.get(), name2memidx);
    }

    void visit_DictInsert(const ASR::DictInsert_t& x) {
        ASR::Dict_t* dict_type = ASR::down_cast<ASR::Dict_t>(
                                    ASRUtils::expr_type(x.m_a));
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* pdict = tmp;

        ptr_loads = !LLVM::is_llvm_struct(dict_type->m_key_type);
        this->visit_expr_wrapper(x.m_key, true);
        llvm::Value *key = tmp;
        ptr_loads = !LLVM::is_llvm_struct(dict_type->m_value_type);
        this->visit_expr_wrapper(x.m_value, true);
        llvm::Value *value = tmp;
        ptr_loads = ptr_loads_copy;

        set_dict_api(dict_type);
        llvm_utils->dict_api->write_item(pdict, key, value, module.get(),
                             dict_type->m_key_type,
                             dict_type->m_value_type, name2memidx);
    }

    void visit_ListRemove(const ASR::ListRemove_t& x) {
        ASR::ttype_t* asr_el_type = ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_a));
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* plist = tmp;

        ptr_loads = !LLVM::is_llvm_struct(asr_el_type);
        this->visit_expr_wrapper(x.m_ele, true);
        ptr_loads = ptr_loads_copy;
        llvm::Value *item = tmp;
        list_api->remove(plist, item, asr_el_type, *module);
    }

    void visit_ListClear(const ASR::ListClear_t& x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        llvm::Value* plist = tmp;
        ptr_loads = ptr_loads_copy;

        list_api->list_clear(plist);
    }

    void visit_TupleCompare(const ASR::TupleCompare_t& x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_left);
        llvm::Value* left = tmp;
        this->visit_expr(*x.m_right);
        llvm::Value* right = tmp;
        ptr_loads = ptr_loads_copy;
        tmp = llvm_utils->is_equal_by_value(left, right, *module,
                ASRUtils::expr_type(x.m_left));
        if (x.m_op == ASR::cmpopType::NotEq) {
            tmp = builder->CreateNot(tmp);
        }
    }

    void visit_TupleLen(const ASR::TupleLen_t& x) {
        LCOMPILERS_ASSERT(x.m_value);
        this->visit_expr(*x.m_value);
    }

    void visit_TupleItem(const ASR::TupleItem_t& x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_a);
        ptr_loads = ptr_loads_copy;
        llvm::Value* ptuple = tmp;

        this->visit_expr_wrapper(x.m_pos, true);
        llvm::Value *pos = tmp;

        tmp = tuple_api->read_item(ptuple, pos, LLVM::is_llvm_struct(x.m_type));
    }

    void visit_ArrayItem(const ASR::ArrayItem_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        ASR::ttype_t* x_mv_type = ASRUtils::expr_type(x.m_v);
        bool is_argument = false;
        llvm::Value* array = nullptr;
        bool is_data_only = false;
        if( ASR::is_a<ASR::Var_t>(*x.m_v) ) {
            ASR::Variable_t *v = ASRUtils::EXPR2VAR(x.m_v);
            if( ASR::is_a<ASR::Struct_t>(*ASRUtils::get_contained_type(v->m_type)) ) {
                ASR::Struct_t* der_type = ASR::down_cast<ASR::Struct_t>(
                    ASRUtils::get_contained_type(v->m_type));
                der_type_name = ASRUtils::symbol_name(ASRUtils::symbol_get_past_external(der_type->m_derived_type));
            }
            uint32_t v_h = get_hash((ASR::asr_t*)v);
            if (llvm_symtab.find(v_h) == llvm_symtab.end()) {
                LCOMPILERS_ASSERT(std::find(nested_globals.begin(),
                        nested_globals.end(), v_h) != nested_globals.end());
                auto finder = std::find(nested_globals.begin(),
                        nested_globals.end(), v_h);
                llvm::Constant *ptr = module->getOrInsertGlobal(nested_desc_name,
                    nested_global_struct);
                int idx = std::distance(nested_globals.begin(), finder);
                std::vector<llvm::Value*> idx_vec = {
                llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
                array = CreateLoad(CreateGEP(ptr, idx_vec));
                is_data_only = true;
            } else {
                array = llvm_symtab[v_h];
            }
            is_argument = (v->m_intent == ASRUtils::intent_in)
                  || (v->m_intent == ASRUtils::intent_out)
                  || (v->m_intent == ASRUtils::intent_inout)
                  || (v->m_intent == ASRUtils::intent_unspecified);
        } else {
            int64_t ptr_loads_copy = ptr_loads;
            ptr_loads = 0;
            this->visit_expr(*x.m_v);
            if( ASR::is_a<ASR::Struct_t>(*x_mv_type) ) {
                ASR::Struct_t* der_type = ASR::down_cast<ASR::Struct_t>(x_mv_type);
                der_type_name = ASRUtils::symbol_name(ASRUtils::symbol_get_past_external(der_type->m_derived_type));
            }
            ptr_loads = ptr_loads_copy;
            array = tmp;
        }
        ASR::dimension_t* m_dims;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(x_mv_type, m_dims);
        if (ASR::is_a<ASR::Character_t>(*x.m_type) && n_dims == 0) {
            // String indexing:
            if (x.n_args != 1) {
                throw CodeGenError("Only string(a) supported for now.", x.base.base.loc);
            }
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Var_t>(*x.m_args[0].m_right));
            this->visit_expr_wrapper(x.m_args[0].m_right, true);
            llvm::Value *p = nullptr;
            llvm::Value *idx = tmp;
            llvm::Value *str = CreateLoad(array);
            if( is_assignment_target ) {
                idx = builder->CreateSub(idx, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                std::vector<llvm::Value*> idx_vec = {idx};
                p = CreateGEP(str, idx_vec);
            } else {
                p = lfortran_str_copy(str, idx, idx);
            }
            // TODO: Currently the string starts at the right location, but goes to the end of the original string.
            // We have to allocate a new string, copy it and add null termination.

            tmp = builder->CreateAlloca(character_type, nullptr);
            builder->CreateStore(p, tmp);

            //tmp = p;
        } else {
            // Array indexing:
            std::vector<llvm::Value*> indices;
            for( size_t r = 0; r < x.n_args; r++ ) {
                ASR::array_index_t curr_idx = x.m_args[r];
                int64_t ptr_loads_copy = ptr_loads;
                ptr_loads = 2;
                this->visit_expr_wrapper(curr_idx.m_right, true);
                ptr_loads = ptr_loads_copy;
                indices.push_back(tmp);
            }
            bool is_bindc_array = ASRUtils::expr_abi(x.m_v) == ASR::abiType::BindC;
            if (ASR::is_a<ASR::Pointer_t>(*x_mv_type) ||
               ((is_bindc_array && !ASRUtils::is_fixed_size_array(m_dims, n_dims)) &&
                ASR::is_a<ASR::StructInstanceMember_t>(*x.m_v))) {
                array = CreateLoad(array);
            }
            is_data_only = is_data_only || (is_argument && !ASRUtils::is_dimension_empty(m_dims, n_dims));
            is_data_only = is_data_only || is_bindc_array;
            Vec<llvm::Value*> llvm_diminfo;
            llvm_diminfo.reserve(al, 2 * x.n_args + 1);
            if( is_data_only ) {
                for( size_t idim = 0; idim < x.n_args; idim++ ) {
                    this->visit_expr_wrapper(m_dims[idim].m_start, true);
                    llvm::Value* dim_start = tmp;
                    this->visit_expr_wrapper(m_dims[idim].m_length, true);
                    llvm::Value* dim_size = tmp;
                    llvm_diminfo.push_back(al, dim_start);
                    llvm_diminfo.push_back(al, dim_size);
                }
            }
            LCOMPILERS_ASSERT(ASRUtils::extract_n_dims_from_ttype(x_mv_type) > 0);
            tmp = arr_descr->get_single_element(array, indices, x.n_args,
                                                is_data_only,
                                                ASRUtils::is_fixed_size_array(m_dims, n_dims) && is_bindc_array,
                                                llvm_diminfo.p);
        }
    }

    void visit_ArraySection(const ASR::ArraySection_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_v);
        ptr_loads = ptr_loads_copy;
        llvm::Value* array = tmp;
        ASR::dimension_t* m_dims;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(
                        ASRUtils::expr_type(x.m_v), m_dims);
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Character_t>(*ASRUtils::expr_type(x.m_v)) &&
                        n_dims == 0);
        // String indexing:
        if (x.n_args == 1) {
            throw CodeGenError("Only string(a:b) supported for now.", x.base.base.loc);
        }

        LCOMPILERS_ASSERT(x.m_args[0].m_left)
        LCOMPILERS_ASSERT(x.m_args[0].m_right)
        //throw CodeGenError("Only string(a:b) for a,b variables for now.", x.base.base.loc);
        // Use the "right" index for now
        this->visit_expr_wrapper(x.m_args[0].m_right, true);
        llvm::Value *idx2 = tmp;
        this->visit_expr_wrapper(x.m_args[0].m_left, true);
        llvm::Value *idx1 = tmp;
        // idx = builder->CreateSub(idx, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
        //std::vector<llvm::Value*> idx_vec = {llvm::ConstantInt::get(context, llvm::APInt(32, 0)), idx};
        // std::vector<llvm::Value*> idx_vec = {idx};
        llvm::Value *str = CreateLoad(array);
        // llvm::Value *p = CreateGEP(str, idx_vec);
        // TODO: Currently the string starts at the right location, but goes to the end of the original string.
        // We have to allocate a new string, copy it and add null termination.
        llvm::Value *p = lfortran_str_copy(str, idx1, idx2);

        tmp = builder->CreateAlloca(character_type, nullptr);
        builder->CreateStore(p, tmp);
    }

    void visit_ArrayReshape(const ASR::ArrayReshape_t& x) {
        this->visit_expr(*x.m_array);
        llvm::Value* array = tmp;
        this->visit_expr(*x.m_shape);
        llvm::Value* shape = tmp;
        ASR::ttype_t* x_m_array_type = ASRUtils::expr_type(x.m_array);
        ASR::ttype_t* asr_data_type = ASRUtils::duplicate_type_without_dims(al,
            x_m_array_type, x_m_array_type->base.loc);
        ASR::ttype_t* asr_shape_type = ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_shape));
        llvm::Type* llvm_data_type = get_type_from_ttype_t_util(asr_data_type);
        tmp = arr_descr->reshape(array, llvm_data_type, shape, asr_shape_type, module.get());
    }

    void lookup_EnumValue(const ASR::EnumValue_t& x) {
        ASR::Enum_t* enum_t = ASR::down_cast<ASR::Enum_t>(x.m_enum_type);
        ASR::EnumType_t* enum_type = ASR::down_cast<ASR::EnumType_t>(enum_t->m_enum_type);
        uint32_t h = get_hash((ASR::asr_t*) enum_type);
        llvm::Value* array = llvm_symtab[h];
        tmp = llvm_utils->create_gep(array, tmp);
        tmp = LLVM::CreateLoad(*builder, llvm_utils->create_gep(tmp, 1));
    }

    void visit_EnumValue(const ASR::EnumValue_t& x) {
        if( x.m_value ) {
            if( ASR::is_a<ASR::Integer_t>(*x.m_type) ) {
                this->visit_expr(*x.m_value);
            } else if( ASR::is_a<ASR::EnumMember_t>(*x.m_v) ) {
                ASR::EnumMember_t* x_enum_member = ASR::down_cast<ASR::EnumMember_t>(x.m_v);
                ASR::Variable_t* x_mv = ASR::down_cast<ASR::Variable_t>(x_enum_member->m_m);
                ASR::Enum_t* enum_t = ASR::down_cast<ASR::Enum_t>(x.m_enum_type);
                ASR::EnumType_t* enum_type = ASR::down_cast<ASR::EnumType_t>(enum_t->m_enum_type);
                for( size_t i = 0; i < enum_type->n_members; i++ ) {
                    if( std::string(enum_type->m_members[i]) == std::string(x_mv->m_name) ) {
                        tmp = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, i));
                        break ;
                    }
                }
                if( lookup_enum_value_for_nonints ) {
                    lookup_EnumValue(x);
                }
            }
            return ;
        }

        visit_expr(*x.m_v);
        if( ASR::is_a<ASR::StructInstanceMember_t>(*x.m_v) ) {
            tmp = LLVM::CreateLoad(*builder, tmp);
        }
        if( !ASR::is_a<ASR::Integer_t>(*x.m_type) && lookup_enum_value_for_nonints ) {
            lookup_EnumValue(x);
        }
    }

    void visit_EnumName(const ASR::EnumName_t& x) {
        if( x.m_value ) {
            this->visit_expr(*x.m_value);
            return ;
        }

        visit_expr(*x.m_v);
        if( ASR::is_a<ASR::StructInstanceMember_t>(*x.m_v) ) {
            tmp = LLVM::CreateLoad(*builder, tmp);
        }
        ASR::Enum_t* enum_t = ASR::down_cast<ASR::Enum_t>(x.m_enum_type);
        ASR::EnumType_t* enum_type = ASR::down_cast<ASR::EnumType_t>(enum_t->m_enum_type);
        uint32_t h = get_hash((ASR::asr_t*) enum_type);
        llvm::Value* array = llvm_symtab[h];
        if( ASR::is_a<ASR::Integer_t>(*enum_type->m_type) ) {
            int64_t min_value = INT64_MAX;

            for( auto itr: enum_type->m_symtab->get_scope() ) {
                ASR::Variable_t* itr_var = ASR::down_cast<ASR::Variable_t>(itr.second);
                ASR::expr_t* value = ASRUtils::expr_value(itr_var->m_symbolic_value);
                int64_t value_int64 = -1;
                ASRUtils::extract_value(value, value_int64);
                min_value = std::min(value_int64, min_value);
            }
            tmp = builder->CreateSub(tmp, llvm::ConstantInt::get(tmp->getType(),
                        llvm::APInt(32, min_value)));
            tmp = llvm_utils->create_gep(array, tmp);
            tmp = llvm_utils->create_gep(tmp, 0);
        }
    }

    void visit_EnumTypeConstructor(const ASR::EnumTypeConstructor_t& x) {
        LCOMPILERS_ASSERT(x.n_args == 1);
        ASR::expr_t* m_arg = x.m_args[0];
        this->visit_expr(*m_arg);
    }

    void visit_UnionTypeConstructor(const ASR::UnionTypeConstructor_t& x) {
        LCOMPILERS_ASSERT(x.n_args == 0);
    }

    void visit_SizeOfType(const ASR::SizeOfType_t& x) {
        llvm::Type* llvm_type = get_type_from_ttype_t_util(x.m_arg);
        llvm::Type* llvm_type_size = get_type_from_ttype_t_util(x.m_type);
        llvm::DataLayout data_layout(module.get());
        int64_t type_size = data_layout.getTypeAllocSize(llvm_type);
        tmp = llvm::ConstantInt::get(llvm_type_size, llvm::APInt(64, type_size));
    }

    void visit_StructInstanceMember(const ASR::StructInstanceMember_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        der_type_name = "";
        ASR::ttype_t* x_m_v_type = ASRUtils::expr_type(x.m_v);
        int64_t ptr_loads_copy = ptr_loads;
        if( ASR::is_a<ASR::UnionRef_t>(*x.m_v) ) {
            ptr_loads = 0;
        } else {
            ptr_loads = 2 - ASR::is_a<ASR::Pointer_t>(*x_m_v_type);
        }
        this->visit_expr(*x.m_v);
        ptr_loads = ptr_loads_copy;
        ASR::Variable_t* member = down_cast<ASR::Variable_t>(symbol_get_past_external(x.m_m));
        std::string member_name = std::string(member->m_name);
        LCOMPILERS_ASSERT(der_type_name.size() != 0);
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
        // if( (ASR::is_a<ASR::StructInstanceMember_t>(*x.m_v) ||
        //      ASR::is_a<ASR::UnionRef_t>(*x.m_v)) &&
        //     is_nested_pointer(tmp) ) {
        //     tmp = CreateLoad(tmp);
        // }
        llvm::Value* tmp1 = CreateGEP(tmp, idx_vec);
        ASR::ttype_t* member_type = member->m_type;
        if( ASR::is_a<ASR::Pointer_t>(*member_type) ) {
            member_type = ASR::down_cast<ASR::Pointer_t>(member_type)->m_type;
        }
        if( member_type->type == ASR::ttypeType::Struct ) {
            ASR::Struct_t* der = (ASR::Struct_t*)(&(member_type->base));
            ASR::StructType_t* der_type = (ASR::StructType_t*)(&(der->m_derived_type->base));
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
        LCOMPILERS_ASSERT(x.m_intent == intent_local
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
        } else if (x.m_type->type == ASR::ttypeType::Character) {
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                    character_type);
            if (!external) {
                if (init_value) {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            init_value);
                } else {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            llvm::Constant::getNullValue(character_type)
                        );
                }
            }
            llvm_symtab[h] = ptr;
        } else if( x.m_type->type == ASR::ttypeType::CPtr ) {
            llvm::Type* void_ptr = llvm::Type::getVoidTy(context)->getPointerTo();
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                void_ptr);
            if (!external) {
                if (init_value) {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            init_value);
                } else {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            llvm::ConstantPointerNull::get(
                                static_cast<llvm::PointerType*>(void_ptr))
                            );
                }
            }
            llvm_symtab[h] = ptr;
        } else if(x.m_type->type == ASR::ttypeType::Pointer) {
            ASR::dimension_t* m_dims = nullptr;
            int n_dims = -1, a_kind = -1;
            bool is_array_type = false, is_malloc_array_type = false, is_list = false;
            llvm::Type* x_ptr = get_type_from_ttype_t(x.m_type, x.m_storage, is_array_type,
                                                      is_malloc_array_type, is_list,
                                                      m_dims, n_dims, a_kind);
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                x_ptr);
            if (!external) {
                if (init_value) {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            init_value);
                } else {
                    module->getNamedGlobal(x.m_name)->setInitializer(
                            llvm::ConstantPointerNull::get(
                                static_cast<llvm::PointerType*>(x_ptr))
                            );
                }
            }
            llvm_symtab[h] = ptr;
        } else if (x.m_type->type == ASR::ttypeType::TypeParameter) {
            // Ignore type variables
        } else {
            throw CodeGenError("Variable type not supported", x.base.base.loc);
        }
    }

    void visit_EnumType(const ASR::EnumType_t& x) {
        if( x.m_enum_value_type == ASR::enumtypeType::IntegerUnique &&
            x.m_abi == ASR::abiType::BindC ) {
            throw CodeGenError("C-interoperation support for non-consecutive but uniquely "
                               "valued integer enums isn't available yet.");
        }
        bool is_integer = ASR::is_a<ASR::Integer_t>(*x.m_type);
        ASR::storage_typeType m_storage = ASR::storage_typeType::Default;
        bool is_array_type = false, is_malloc_array_type = false, is_list = false;
        ASR::dimension_t* m_dims = nullptr;
        int n_dims = -1, a_kind = -1;
        llvm::Type* value_type = get_type_from_ttype_t(x.m_type, m_storage, is_array_type,
                                    is_malloc_array_type, is_list, m_dims, n_dims, a_kind);
        if( is_integer ) {
            int64_t min_value = INT64_MAX;
            int64_t max_value = INT64_MIN;
            size_t max_name_len = 0;
            llvm::Value* itr_value = nullptr;
            for( auto itr: x.m_symtab->get_scope() ) {
                ASR::Variable_t* itr_var = ASR::down_cast<ASR::Variable_t>(itr.second);
                ASR::expr_t* value = ASRUtils::expr_value(itr_var->m_symbolic_value);
                int64_t value_int64 = -1;
                this->visit_expr(*value);
                itr_value = tmp;
                ASRUtils::extract_value(value, value_int64);
                min_value = std::min(value_int64, min_value);
                max_value = std::max(value_int64, max_value);
                max_name_len = std::max(max_name_len, itr.first.size());
            }

            llvm::ArrayType* name_array_type = llvm::ArrayType::get(llvm::Type::getInt8Ty(context),
                                                                    max_name_len + 1);
            llvm::StructType* enum_value_type = llvm::StructType::create({name_array_type, value_type});
            llvm::Constant* empty_vt = llvm::ConstantStruct::get(enum_value_type, {llvm::ConstantArray::get(name_array_type,
                {llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, '\0'))}),
                (llvm::Constant*) itr_value});
            std::vector<llvm::Constant*> enum_value_pairs(max_value - min_value + 1, empty_vt);

            for( auto itr: x.m_symtab->get_scope() ) {
                ASR::Variable_t* itr_var = ASR::down_cast<ASR::Variable_t>(itr.second);
                ASR::expr_t* value = ASRUtils::expr_value(itr_var->m_symbolic_value);
                int64_t value_int64 = -1;
                ASRUtils::extract_value(value, value_int64);
                this->visit_expr(*value);
                std::vector<llvm::Constant*> itr_var_name_v;
                itr_var_name_v.reserve(itr.first.size());
                for( size_t i = 0; i < itr.first.size(); i++ ) {
                    itr_var_name_v.push_back(llvm::ConstantInt::get(
                        llvm::Type::getInt8Ty(context), llvm::APInt(8, itr_var->m_name[i])));
                }
                itr_var_name_v.push_back(llvm::ConstantInt::get(
                    llvm::Type::getInt8Ty(context), llvm::APInt(8, '\0')));
                llvm::Constant* name = llvm::ConstantArray::get(name_array_type, itr_var_name_v);
                enum_value_pairs[value_int64 - min_value] = llvm::ConstantStruct::get(
                    enum_value_type, {name, (llvm::Constant*) tmp});
            }

            llvm::ArrayType* global_enum_array = llvm::ArrayType::get(enum_value_type,
                                                    max_value - min_value + 1);
            llvm::Constant *array = module->getOrInsertGlobal(x.m_name,
                                        global_enum_array);
            module->getNamedGlobal(x.m_name)->setInitializer(
                llvm::ConstantArray::get(global_enum_array, enum_value_pairs));
            uint32_t h = get_hash((ASR::asr_t*)&x);
            llvm_symtab[h] = array;
        } else {
            size_t max_name_len = 0;

            for( auto itr: x.m_symtab->get_scope() ) {
                max_name_len = std::max(max_name_len, itr.first.size());
            }

            llvm::ArrayType* name_array_type = llvm::ArrayType::get(llvm::Type::getInt8Ty(context),
                                                                    max_name_len + 1);
            llvm::StructType* enum_value_type = llvm::StructType::create({name_array_type, value_type});
            std::vector<llvm::Constant*> enum_value_pairs(x.n_members, nullptr);

            for( auto itr: x.m_symtab->get_scope() ) {
                ASR::Variable_t* itr_var = ASR::down_cast<ASR::Variable_t>(itr.second);
                ASR::expr_t* value = itr_var->m_symbolic_value;
                int64_t value_int64 = -1;
                ASRUtils::extract_value(value, value_int64);
                this->visit_expr(*value);
                std::vector<llvm::Constant*> itr_var_name_v;
                itr_var_name_v.reserve(itr.first.size());
                for( size_t i = 0; i < itr.first.size(); i++ ) {
                    itr_var_name_v.push_back(llvm::ConstantInt::get(
                        llvm::Type::getInt8Ty(context), llvm::APInt(8, itr_var->m_name[i])));
                }
                itr_var_name_v.push_back(llvm::ConstantInt::get(
                    llvm::Type::getInt8Ty(context), llvm::APInt(8, '\0')));
                llvm::Constant* name = llvm::ConstantArray::get(name_array_type, itr_var_name_v);
                size_t dest_idx = 0;
                for( size_t j = 0; j < x.n_members; j++ ) {
                    if( std::string(x.m_members[j]) == itr.first ) {
                        dest_idx = j;
                        break ;
                    }
                }
                enum_value_pairs[dest_idx] = llvm::ConstantStruct::get(
                    enum_value_type, {name, (llvm::Constant*) tmp});
            }

            llvm::ArrayType* global_enum_array = llvm::ArrayType::get(enum_value_type,
                                                    x.n_members);
            llvm::Constant *array = module->getOrInsertGlobal(x.m_name,
                                        global_enum_array);
            module->getNamedGlobal(x.m_name)->setInitializer(
                llvm::ConstantArray::get(global_enum_array, enum_value_pairs));
            uint32_t h = get_hash((ASR::asr_t*)&x);
            llvm_symtab[h] = array;
        }
    }

    void start_module_init_function_prototype(const ASR::Module_t &x) {
        uint32_t h = get_hash((ASR::asr_t*)&x);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), {}, false);
        LCOMPILERS_ASSERT(llvm_symtab_fn.find(h) == llvm_symtab_fn.end());
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

        for (auto &item : x.m_symtab->get_scope()) {
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
        }
        finish_module_init_function_prototype(x);

        visit_procedures(x);
        mangle_prefix = "";
    }

    void visit_Program(const ASR::Program_t &x) {
        bool is_dict_present_copy_lp = dict_api_lp->is_dict_present();
        bool is_dict_present_copy_sc = dict_api_sc->is_dict_present();
        dict_api_lp->set_is_dict_present(false);
        dict_api_sc->set_is_dict_present(false);
        llvm_goto_targets.clear();
        // Generate code for nested subroutines and functions first:
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *v = down_cast<ASR::Function_t>(
                        item.second);
                instantiate_function(*v);
                declare_needed_global_types(*v);
            }
        }
        declare_needed_global_types(x);
        visit_procedures(x);

        // Generate code for the main program
        std::vector<llvm::Type*> command_line_args = {
            llvm::Type::getInt32Ty(context),
            character_type->getPointerTo()
        };
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context), command_line_args, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, "main", module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        if (compiler_options.emit_debug_info) {
            llvm::DISubprogram *SP;
            debug_emit_function(x, SP);
            F->setSubprogram(SP);
        }
        builder->SetInsertPoint(BB);

        // Call the `_lpython_set_argv` function to assign command line argument
        // values to `argc` and `argv`.
        {
            if (compiler_options.emit_debug_info) debug_emit_loc(x);
            llvm::Function *fn = module->getFunction("_lpython_set_argv");
            if(!fn) {
                llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {
                        llvm::Type::getInt32Ty(context),
                        character_type->getPointerTo()
                    }, false);
                fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, "_lpython_set_argv", *module);
            }
            std::vector<llvm::Value *> args;
            for (llvm::Argument &llvm_arg : F->args()) {
                args.push_back(&llvm_arg);
            }
            builder->CreateCall(fn, args);
        }

        declare_vars(x);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        llvm::Value *ret_val2 = llvm::ConstantInt::get(context,
            llvm::APInt(32, 0));
        builder->CreateRet(ret_val2);
        dict_api_lp->set_is_dict_present(is_dict_present_copy_lp);
        dict_api_sc->set_is_dict_present(is_dict_present_copy_sc);

        // Finalize the debug info.
        if (compiler_options.emit_debug_info) DBuilder->finalize();
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

    int32_t get_type_size(ASR::ttype_t* asr_type, llvm::Type* llvm_type,
                          int32_t a_kind) {
        if( LLVM::is_llvm_struct(asr_type) ||
            ASR::is_a<ASR::Character_t>(*asr_type) ||
            ASR::is_a<ASR::Complex_t>(*asr_type) ) {
            llvm::DataLayout data_layout(module.get());
            return data_layout.getTypeAllocSize(llvm_type);
        }
        return a_kind;
    }

    llvm::Type* get_dict_type(ASR::ttype_t* asr_type) {
        ASR::Dict_t* asr_dict = ASR::down_cast<ASR::Dict_t>(asr_type);
        bool is_local_array_type = false, is_local_malloc_array_type = false;
        bool is_local_list = false;
        ASR::dimension_t* local_m_dims = nullptr;
        int local_n_dims = 0;
        int local_a_kind = -1;
        ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
        llvm::Type* key_llvm_type = get_type_from_ttype_t(asr_dict->m_key_type, local_m_storage,
                                                            is_local_array_type, is_local_malloc_array_type,
                                                            is_local_list, local_m_dims, local_n_dims,
                                                            local_a_kind);
        int32_t key_type_size = get_type_size(asr_dict->m_key_type, key_llvm_type, local_a_kind);
        llvm::Type* value_llvm_type = get_type_from_ttype_t(asr_dict->m_value_type, local_m_storage,
                                                            is_local_array_type, is_local_malloc_array_type,
                                                            is_local_list, local_m_dims, local_n_dims,
                                                            local_a_kind);
        int32_t value_type_size = get_type_size(asr_dict->m_value_type, value_llvm_type, local_a_kind);
        std::string key_type_code = ASRUtils::get_type_code(asr_dict->m_key_type);
        std::string value_type_code = ASRUtils::get_type_code(asr_dict->m_value_type);
        set_dict_api(asr_dict);
        return llvm_utils->dict_api->get_dict_type(key_type_code, value_type_code, key_type_size,
                                        value_type_size, key_llvm_type, value_llvm_type);
    }

    llvm::Type* get_type_from_ttype_t(ASR::ttype_t* asr_type,
        ASR::storage_typeType m_storage,
        bool& is_array_type, bool& is_malloc_array_type,
        bool& is_list, ASR::dimension_t*& m_dims,
        int& n_dims, int& a_kind, ASR::abiType m_abi=ASR::abiType::Source) {
        llvm::Type* llvm_type = nullptr;
        switch (asr_type->type) {
            case (ASR::ttypeType::Integer) : {
                ASR::Integer_t* v_type = down_cast<ASR::Integer_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    if( m_abi == ASR::abiType::BindC ) {
                        if( ASRUtils::is_fixed_size_array(v_type->m_dims, v_type->n_dims) ) {
                            llvm_type = llvm::ArrayType::get(get_el_type(asr_type), ASRUtils::get_fixed_size_of_array(
                                                                                    v_type->m_dims, v_type->n_dims));
                        } else {
                            llvm_type = get_el_type(asr_type)->getPointerTo();
                        }
                    } else {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(asr_type);
                        if( m_storage == ASR::storage_typeType::Allocatable ) {
                            is_malloc_array_type = true;
                            llvm_type = arr_descr->get_malloc_array_type(asr_type, el_type);
                        } else {
                            llvm_type = arr_descr->get_array_type(asr_type, el_type);
                        }
                    }
                } else {
                    llvm_type = getIntType(a_kind);
                }
                break;
            }
            case (ASR::ttypeType::Real) : {
                ASR::Real_t* v_type = down_cast<ASR::Real_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    if( m_abi == ASR::abiType::BindC ) {
                        if( ASRUtils::is_fixed_size_array(v_type->m_dims, v_type->n_dims) ) {
                            llvm_type = llvm::ArrayType::get(get_el_type(asr_type), ASRUtils::get_fixed_size_of_array(
                                                                                    v_type->m_dims, v_type->n_dims));
                        } else {
                            llvm_type = get_el_type(asr_type)->getPointerTo();
                        }
                    } else {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(asr_type);
                        if( m_storage == ASR::storage_typeType::Allocatable ) {
                            is_malloc_array_type = true;
                            llvm_type = arr_descr->get_malloc_array_type(asr_type, el_type);
                        } else {
                            llvm_type = arr_descr->get_array_type(asr_type, el_type);
                        }
                    }
                } else {
                    llvm_type = getFPType(a_kind);
                }
                break;
            }
            case (ASR::ttypeType::Complex) : {
                ASR::Complex_t* v_type = down_cast<ASR::Complex_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    if( m_abi == ASR::abiType::BindC ) {
                        if( ASRUtils::is_fixed_size_array(v_type->m_dims, v_type->n_dims) ) {
                            llvm_type = llvm::ArrayType::get(get_el_type(asr_type), ASRUtils::get_fixed_size_of_array(
                                                                                    v_type->m_dims, v_type->n_dims));
                        } else {
                            llvm_type = get_el_type(asr_type)->getPointerTo();
                        }
                    } else {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(asr_type);
                        if( m_storage == ASR::storage_typeType::Allocatable ) {
                            is_malloc_array_type = true;
                            llvm_type = arr_descr->get_malloc_array_type(asr_type, el_type);
                        } else {
                            llvm_type = arr_descr->get_array_type(asr_type, el_type);
                        }
                    }
                } else {
                    llvm_type = getComplexType(a_kind);
                }
                break;
            }
            case (ASR::ttypeType::Character) : {
                ASR::Character_t* v_type = down_cast<ASR::Character_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    is_array_type = true;
                    llvm::Type* el_type = get_el_type(asr_type);
                    if( m_storage == ASR::storage_typeType::Allocatable ) {
                        is_malloc_array_type = true;
                        llvm_type = arr_descr->get_malloc_array_type(asr_type, el_type);
                    } else {
                        llvm_type = arr_descr->get_array_type(asr_type, el_type);
                    }
                } else {
                    llvm_type = character_type;
                }
                break;
            }
            case (ASR::ttypeType::Logical) : {
                ASR::Logical_t* v_type = down_cast<ASR::Logical_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    if( m_abi == ASR::abiType::BindC ) {
                        llvm_type = get_el_type(asr_type)->getPointerTo();
                    } else {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(asr_type);
                        if( m_storage == ASR::storage_typeType::Allocatable ) {
                            is_malloc_array_type = true;
                            llvm_type = arr_descr->get_malloc_array_type(asr_type, el_type);
                        } else {
                            llvm_type = arr_descr->get_array_type(asr_type, el_type);
                        }
                    }
                } else {
                    llvm_type = llvm::Type::getInt1Ty(context);
                }
                break;
            }
            case (ASR::ttypeType::Struct) : {
                ASR::Struct_t* v_type = down_cast<ASR::Struct_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                if( n_dims > 0 ) {
                    is_array_type = true;
                    llvm::Type* el_type = get_el_type(asr_type);
                    if( m_storage == ASR::storage_typeType::Allocatable ) {
                        is_malloc_array_type = true;
                        llvm_type = arr_descr->get_malloc_array_type(asr_type, el_type);
                    } else {
                        llvm_type = arr_descr->get_array_type(asr_type, el_type);
                    }
                } else {
                    llvm_type = getStructType(asr_type, false);
                }
                break;
            }
            case (ASR::ttypeType::Union) : {
                ASR::Union_t* v_type = ASR::down_cast<ASR::Union_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                if( n_dims > 0 ) {
                    is_array_type = true;
                    llvm::Type* el_type = get_el_type(asr_type);
                    if( m_storage == ASR::storage_typeType::Allocatable ) {
                        is_malloc_array_type = true;
                        llvm_type = arr_descr->get_malloc_array_type(asr_type, el_type);
                    } else {
                        llvm_type = arr_descr->get_array_type(asr_type, el_type);
                    }
                } else {
                    llvm_type = getUnionType(asr_type, false);
                }
                break;
            }
            case (ASR::ttypeType::Pointer) : {
                ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(asr_type)->m_type;
                llvm_type = get_type_from_ttype_t(t2, m_storage, is_array_type,
                                        is_malloc_array_type, is_list, m_dims,
                                        n_dims, a_kind, m_abi);
                llvm_type = llvm_type->getPointerTo();
                break;
            }
            case (ASR::ttypeType::List) : {
                is_list = true;
                ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(asr_type);
                llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_list->m_type, m_storage,
                                                                 is_array_type, is_malloc_array_type,
                                                                 is_list, m_dims, n_dims,
                                                                 a_kind, m_abi);
                std::string el_type_code = ASRUtils::get_type_code(asr_list->m_type);
                int32_t type_size = -1;
                if( LLVM::is_llvm_struct(asr_list->m_type) ||
                    ASR::is_a<ASR::Character_t>(*asr_list->m_type) ||
                    ASR::is_a<ASR::Complex_t>(*asr_list->m_type) ) {
                    llvm::DataLayout data_layout(module.get());
                    type_size = data_layout.getTypeAllocSize(el_llvm_type);
                } else {
                    type_size = a_kind;
                }
                llvm_type = list_api->get_list_type(el_llvm_type, el_type_code, type_size);
                break;
            }
            case (ASR::ttypeType::Dict): {
                llvm_type = get_dict_type(asr_type);
                break;
            }
            case (ASR::ttypeType::Tuple) : {
                ASR::Tuple_t* asr_tuple = ASR::down_cast<ASR::Tuple_t>(asr_type);
                std::string type_code = ASRUtils::get_type_code(asr_tuple->m_type,
                                                                asr_tuple->n_type);
                std::vector<llvm::Type*> llvm_el_types;
                for( size_t i = 0; i < asr_tuple->n_type; i++ ) {
                    bool is_local_array_type = false, is_local_malloc_array_type = false;
                    bool is_local_list = false;
                    ASR::dimension_t* local_m_dims = nullptr;
                    int local_n_dims = 0;
                    int local_a_kind = -1;
                    ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
                    llvm_el_types.push_back(get_type_from_ttype_t(asr_tuple->m_type[i], local_m_storage,
                                            is_local_array_type, is_local_malloc_array_type,
                                            is_local_list, local_m_dims, local_n_dims, local_a_kind, m_abi));
                }
                llvm_type = tuple_api->get_tuple_type(type_code, llvm_el_types);
                break;
            }
            case (ASR::ttypeType::CPtr) : {
                llvm_type = llvm::Type::getVoidTy(context)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::Enum) : {
                llvm_type = llvm::Type::getInt32Ty(context);
                break ;
            }
            case (ASR::ttypeType::Const) : {
                llvm_type = get_type_from_ttype_t(ASRUtils::get_contained_type(asr_type),
                                m_storage, is_array_type, is_malloc_array_type, is_list,
                                m_dims, n_dims, a_kind, m_abi);
                break;
            }
            default :
                throw CodeGenError("Support for type " + ASRUtils::type_to_str(asr_type) +
                                   " not yet implemented.");
        }
        return llvm_type;
    }

    inline llvm::Type* get_type_from_ttype_t_util(ASR::ttype_t* asr_type, ASR::abiType asr_abi=ASR::abiType::Source) {
        ASR::storage_typeType m_storage_local = ASR::storage_typeType::Default;
        bool is_array_type_local, is_malloc_array_type_local;
        bool is_list_local;
        ASR::dimension_t* m_dims_local;
        int n_dims_local, a_kind_local;
        return get_type_from_ttype_t(asr_type, m_storage_local, is_array_type_local,
                                     is_malloc_array_type_local, is_list_local,
                                     m_dims_local, n_dims_local, a_kind_local, asr_abi);
    }

    void fill_array_details_(llvm::Value* ptr, ASR::dimension_t* m_dims,
        size_t n_dims, bool is_malloc_array_type, bool is_array_type,
        bool is_list, ASR::ttype_t* m_type, bool is_data_only=false) {
        if( is_malloc_array_type &&
            m_type->type != ASR::ttypeType::Pointer &&
            !is_list && !is_data_only ) {
            arr_descr->fill_dimension_descriptor(ptr, n_dims);
        }
        if( is_array_type && !is_malloc_array_type &&
            m_type->type != ASR::ttypeType::Pointer &&
            !is_list ) {
            ASR::ttype_t* asr_data_type = ASRUtils::duplicate_type_without_dims(al, m_type, m_type->base.loc);
            llvm::Type* llvm_data_type = get_type_from_ttype_t_util(asr_data_type);
            fill_array_details(ptr, llvm_data_type, m_dims, n_dims, is_data_only);
        }
        if( is_array_type && is_malloc_array_type &&
            m_type->type != ASR::ttypeType::Pointer &&
            !is_list && !is_data_only ) {
            // Set allocatable arrays as unallocated
            arr_descr->set_is_allocated_flag(ptr, 0);
        }
    }

    void allocate_array_members_of_struct(llvm::Value* ptr, ASR::ttype_t* asr_type) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Struct_t>(*asr_type));
        ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(asr_type);
        ASR::StructType_t* struct_type_t = ASR::down_cast<ASR::StructType_t>(
            ASRUtils::symbol_get_past_external(struct_t->m_derived_type));
        std::string struct_type_name = struct_type_t->m_name;
        for( auto item: struct_type_t->m_symtab->get_scope() ) {
            if( ASR::is_a<ASR::ClassProcedure_t>(*item.second) ||
                ASR::is_a<ASR::GenericProcedure_t>(*item.second) ||
                ASR::is_a<ASR::UnionType_t>(*item.second) ||
                ASR::is_a<ASR::StructType_t>(*item.second) ||
                ASR::is_a<ASR::CustomOperator_t>(*item.second) ) {
                continue ;
            }
            ASR::ttype_t* symbol_type = ASRUtils::symbol_type(item.second);
            int idx = name2memidx[struct_type_name][item.first];
            llvm::Value* ptr_member = llvm_utils->create_gep(ptr, idx);
            if( ASR::is_a<ASR::Variable_t>(*item.second) ) {
                ASR::Variable_t* v = ASR::down_cast<ASR::Variable_t>(item.second);
                if( v->m_symbolic_value ) {
                    visit_expr(*v->m_symbolic_value);
                    LLVM::CreateStore(*builder, tmp, ptr_member);
                }
            }
            if( ASRUtils::is_array(symbol_type) ) {
                // Assume that struct member array is not allocatable
                ASR::dimension_t* m_dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(symbol_type, m_dims);
                bool is_data_only = (ASRUtils::symbol_abi(item.second) == ASR::abiType::BindC &&
                                     ASRUtils::is_fixed_size_array(m_dims, n_dims));
                fill_array_details_(ptr_member, m_dims, n_dims, false, true, false, symbol_type, is_data_only);
            } else if( ASR::is_a<ASR::Struct_t>(*symbol_type) ) {
                allocate_array_members_of_struct(ptr_member, symbol_type);
            }
        }
    }

    template<typename T>
    void declare_vars(const T &x) {
        llvm::Value *target_var;
        uint32_t debug_arg_count = 0;
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
            if (is_a<ASR::Variable_t>(*var_sym)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(var_sym);
                uint32_t h = get_hash((ASR::asr_t*)v);
                llvm::Type *type;
                int n_dims = 0, a_kind = 4;
                ASR::dimension_t* m_dims = nullptr;
                bool is_array_type = false;
                bool is_malloc_array_type = false;
                bool is_list = false;
                if (v->m_intent == intent_local ||
                    v->m_intent == intent_return_var ||
                    !v->m_intent) {
                    type = get_type_from_ttype_t(v->m_type, v->m_storage, is_array_type,
                                                 is_malloc_array_type, is_list, m_dims, n_dims,
                                                 a_kind);
                    /*
                    * The following if block is used for converting any
                    * general array descriptor to a pointer type which
                    * can be passed as an argument in a function call in LLVM IR.
                    */
                    if( x.class_type == ASR::symbolType::Function) {
                        std::uint32_t m_h;
                        std::string m_name = std::string(x.m_name);
                        ASR::abiType abi_type = ASR::abiType::Source;
                        bool is_v_arg = false;
                        if( x.class_type == ASR::symbolType::Function ) {
                            ASR::Function_t* _func = (ASR::Function_t*)(&(x.base));
                            m_h = get_hash((ASR::asr_t*)_func);
                            abi_type = _func->m_abi;
                            is_v_arg = is_argument(v, _func->m_args, _func->n_args);
                        }
                        if( is_array_type && !is_list ) {
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
                    if( ASR::is_a<ASR::Struct_t>(*v->m_type) &&
                        !(is_array_type || is_malloc_array_type) ) {
                        allocate_array_members_of_struct(ptr, v->m_type);
                    }
                    if (compiler_options.emit_debug_info) {
                        // Reset the debug location
                        builder->SetCurrentDebugLocation(nullptr);
                        uint32_t line, column;
                        if (compiler_options.emit_debug_line_column) {
                            debug_get_line_column(v->base.base.loc.first, line, column);
                        } else {
                            line = v->base.base.loc.first;
                            column = 0;
                        }
                        std::string type_name;
                        uint32_t type_size, type_encoding;
                        get_type_debug_info(v->m_type, type_name, type_size,
                            type_encoding);
                        llvm::DILocalVariable *debug_var = DBuilder->createParameterVariable(
                            debug_current_scope, v->m_name, ++debug_arg_count, debug_Unit, line,
                            DBuilder->createBasicType(type_name, type_size, type_encoding), true);
                        DBuilder->insertDeclare(ptr, debug_var, DBuilder->createExpression(),
                            llvm::DILocation::get(debug_current_scope->getContext(),
                            line, 0, debug_current_scope), builder->GetInsertBlock());
                    }

                    if( ASR::is_a<ASR::Struct_t>(*v->m_type) ) {
                        ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(v->m_type);
                        ASR::StructType_t* struct_type = ASR::down_cast<ASR::StructType_t>(
                                ASRUtils::symbol_get_past_external(struct_t->m_derived_type));
                        int64_t alignment_value = -1;
                        if( ASRUtils::extract_value(struct_type->m_alignment, alignment_value) ) {
                            llvm::Align align(alignment_value);
                            ptr->setAlignment(align);
                        }
                    }
                    llvm_symtab[h] = ptr;
                    fill_array_details_(ptr, m_dims, n_dims,
                        is_malloc_array_type,
                        is_array_type, is_list, v->m_type);
                    if( v->m_symbolic_value != nullptr &&
                        !ASR::is_a<ASR::List_t>(*v->m_type)) {
                        target_var = ptr;
                        tmp = nullptr;
                        this->visit_expr_wrapper(v->m_symbolic_value, true);
                        llvm::Value *init_value = tmp;
                        if (ASR::is_a<ASR::ArrayConstant_t>(*v->m_symbolic_value)) {
                            target_var = arr_descr->get_pointer_to_data(target_var);
                        }
                        builder->CreateStore(init_value, target_var);
                        auto finder = std::find(nested_globals.begin(),
                                nested_globals.end(), h);
                        if (finder != nested_globals.end()) {
                            llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                                    nested_global_struct);
                            int idx = std::distance(nested_globals.begin(),
                                    finder);
                            if( is_array_type || is_malloc_array_type ) {
                                target_var = CreateLoad(target_var);
                            }
                            builder->CreateStore(target_var, llvm_utils->create_gep(ptr,
                                        idx));
                        }
                    } else {
                        if (is_a<ASR::Character_t>(*v->m_type) && !is_array_type && !is_list) {
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
                                LCOMPILERS_ASSERT(t->m_len_expr)
                                this->visit_expr(*t->m_len_expr);
                                llvm::Value *arg_size = tmp;
                                arg_size = builder->CreateAdd(arg_size, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                                // TODO: this temporary string is never deallocated (leaks memory)
                                llvm::Value *init_value = LLVM::lfortran_malloc(context, *module, *builder, arg_size);
                                string_init(context, *module, *builder, arg_size, init_value);
                                builder->CreateStore(init_value, target_var);
                            } else {
                                throw CodeGenError("Unsupported len value in ASR");
                            }
                        } else if (is_list) {
                            ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(v->m_type);
                            std::string type_code = ASRUtils::get_type_code(asr_list->m_type);
                            list_api->list_init(type_code, ptr, *module);
                        }
                    }
                }
            }
        }
    }

    llvm::Type* get_arg_type_from_ttype_t(ASR::ttype_t* asr_type,
        ASR::abiType m_abi, ASR::abiType arg_m_abi,
        ASR::storage_typeType m_storage,
        bool arg_m_value_attr,
        int& n_dims, int& a_kind, bool& is_array_type,
        ASR::intentType arg_intent, bool get_pointer=true) {
        llvm::Type* type = nullptr;
        switch (asr_type->type) {
            case (ASR::ttypeType::Integer) : {
                ASR::Integer_t* v_type = down_cast<ASR::Integer_t>(asr_type);
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    if (m_abi == ASR::abiType::BindC ||
                        (!ASRUtils::is_dimension_empty(v_type->m_dims, v_type->n_dims))) {
                        // Bind(C) arrays are represened as a pointer
                        type = getIntType(a_kind, true);
                    } else {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(asr_type);
                        if( m_storage == ASR::storage_typeType::Allocatable ) {
                            type = arr_descr->get_malloc_array_type(asr_type, el_type, get_pointer);
                        } else {
                            type = arr_descr->get_array_type(asr_type, el_type, get_pointer);
                        }
                    }
                } else {
                    if (arg_m_abi == ASR::abiType::BindC
                        && arg_m_value_attr) {
                        type = getIntType(a_kind, false);
                    } else {
                        type = getIntType(a_kind, true);
                    }
                }
                break;
            }
            case (ASR::ttypeType::Pointer) : {
                ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(asr_type);
                type = get_arg_type_from_ttype_t(t2, m_abi, arg_m_abi,
                            m_storage, arg_m_value_attr, n_dims, a_kind,
                            is_array_type, arg_intent, get_pointer);
                type = type->getPointerTo();
                break;
            }
            case (ASR::ttypeType::Const) : {
                ASR::ttype_t *t2 = ASRUtils::get_contained_type(asr_type);
                type = get_arg_type_from_ttype_t(t2, m_abi, arg_m_abi,
                            m_storage, arg_m_value_attr, n_dims, a_kind,
                            is_array_type, arg_intent, get_pointer);
                break;
            }
            case (ASR::ttypeType::Real) : {
                ASR::Real_t* v_type = down_cast<ASR::Real_t>(asr_type);
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    if (m_abi == ASR::abiType::BindC ||
                        (!ASRUtils::is_dimension_empty(v_type->m_dims, v_type->n_dims))) {
                        // Bind(C) arrays are represened as a pointer
                        type = getFPType(a_kind, true);
                    } else {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(asr_type);
                        if( m_storage == ASR::storage_typeType::Allocatable ) {
                            type = arr_descr->get_malloc_array_type(asr_type, el_type, get_pointer);
                        } else {
                            type = arr_descr->get_array_type(asr_type, el_type, get_pointer);
                        }
                    }
                } else {
                    if (arg_m_abi == ASR::abiType::BindC
                        && arg_m_value_attr) {
                        type = getFPType(a_kind, false);
                    } else {
                        type = getFPType(a_kind, true);
                    }
                }
                break;
            }
            case (ASR::ttypeType::Complex) : {
                ASR::Complex_t* v_type = down_cast<ASR::Complex_t>(asr_type);
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if (m_abi != ASR::abiType::BindC &&
                    (!ASRUtils::is_dimension_empty(v_type->m_dims, v_type->n_dims))) {
                    type = getComplexType(a_kind, true);
                } else if( n_dims > 0 ) {
                    is_array_type = true;
                    llvm::Type* el_type = get_el_type(asr_type);
                    if( m_storage == ASR::storage_typeType::Allocatable ) {
                        type = arr_descr->get_malloc_array_type(asr_type, el_type, get_pointer);
                    } else {
                        type = arr_descr->get_array_type(asr_type, el_type, get_pointer);
                    }
                } else {
                    if (arg_m_abi == ASR::abiType::BindC
                            && arg_m_value_attr) {
                        if (a_kind == 4) {
                            if (compiler_options.platform == Platform::Windows) {
                                // type_fx2 is i64
                                llvm::Type* type_fx2 = llvm::Type::getInt64Ty(context);
                                type = type_fx2;
                            } else if (compiler_options.platform == Platform::macOS_ARM) {
                                // type_fx2 is [2 x float]
                                llvm::Type* type_fx2 = llvm::ArrayType::get(llvm::Type::getFloatTy(context), 2);
                                type = type_fx2;
                            } else {
                                // type_fx2 is <2 x float>
                                llvm::Type* type_fx2 = FIXED_VECTOR_TYPE::get(llvm::Type::getFloatTy(context), 2);
                                type = type_fx2;
                            }
                        } else {
                            LCOMPILERS_ASSERT(a_kind == 8)
                            if (compiler_options.platform == Platform::Windows) {
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
                if (arg_m_abi == ASR::abiType::BindC) {
                    type = character_type;
                } else {
                    type = character_type->getPointerTo();
                }
                break;
            case (ASR::ttypeType::Logical) : {
                ASR::Logical_t* v_type = down_cast<ASR::Logical_t>(asr_type);
                n_dims = v_type->n_dims;
                a_kind = v_type->m_kind;
                if( n_dims > 0 ) {
                    if (m_abi == ASR::abiType::BindC ||
                        (!ASRUtils::is_dimension_empty(v_type->m_dims, v_type->n_dims))) {
                        // Bind(C) arrays are represened as a pointer
                        type = llvm::Type::getInt1PtrTy(context);
                    } else {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(asr_type);
                        if( m_storage == ASR::storage_typeType::Allocatable ) {
                            type = arr_descr->get_malloc_array_type(asr_type, el_type, get_pointer);
                        } else {
                            type = arr_descr->get_array_type(asr_type, el_type, get_pointer);
                        }
                    }
                } else {
                    if (arg_m_abi == ASR::abiType::BindC
                        && arg_m_value_attr) {
                        type = llvm::Type::getInt1Ty(context);
                    } else {
                        type = llvm::Type::getInt1PtrTy(context);
                    }
                }
                break;
            }
            case (ASR::ttypeType::Struct) : {
                ASR::Struct_t* v_type = down_cast<ASR::Struct_t>(asr_type);
                n_dims = v_type->n_dims;
                if( n_dims > 0 ) {
                    is_array_type = true;
                    llvm::Type* el_type = get_el_type(asr_type);
                    if( m_storage == ASR::storage_typeType::Allocatable ) {
                        type = arr_descr->get_malloc_array_type(asr_type, el_type, get_pointer);
                    } else {
                        type = arr_descr->get_array_type(asr_type, el_type, get_pointer);
                    }
                } else {
                    type = getStructType(asr_type, true);
                }
                break;
            }
            case (ASR::ttypeType::Class) : {
                ASR::Class_t* v_type = down_cast<ASR::Class_t>(asr_type);
                n_dims = v_type->n_dims;
                if( n_dims > 0 ) {
                    is_array_type = true;
                    llvm::Type* el_type = get_el_type(asr_type);
                    if( m_storage == ASR::storage_typeType::Allocatable ) {
                        type = arr_descr->get_malloc_array_type(asr_type, el_type, get_pointer);
                    } else {
                        type = arr_descr->get_array_type(asr_type, el_type, get_pointer);
                    }
                } else {
                    type = getClassType(asr_type, true);
                }
                break;
            }
            case (ASR::ttypeType::CPtr) : {
                type = llvm::Type::getVoidTy(context)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::Tuple) : {
                type = get_type_from_ttype_t_util(asr_type)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::List) : {
                bool is_array_type = false, is_malloc_array_type = false;
                bool is_list = true;
                ASR::dimension_t *m_dims = nullptr;
                ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(asr_type);
                llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_list->m_type, m_storage,
                                                                 is_array_type,
                                                                 is_malloc_array_type,
                                                                 is_list, m_dims, n_dims,
                                                                 a_kind, m_abi);
                int32_t type_size = -1;
                if( LLVM::is_llvm_struct(asr_list->m_type) ||
                    ASR::is_a<ASR::Character_t>(*asr_list->m_type) ||
                    ASR::is_a<ASR::Complex_t>(*asr_list->m_type) ) {
                    llvm::DataLayout data_layout(module.get());
                    type_size = data_layout.getTypeAllocSize(el_llvm_type);
                } else {
                    type_size = a_kind;
                }
                std::string el_type_code = ASRUtils::get_type_code(asr_list->m_type);
                type = list_api->get_list_type(el_llvm_type, el_type_code, type_size)->getPointerTo();
                break;
            }
            case ASR::ttypeType::Enum: {
                if (arg_m_abi == ASR::abiType::BindC
                    && arg_m_value_attr) {
                    type = llvm::Type::getInt32Ty(context);
                } else {
                    type = llvm::Type::getInt32PtrTy(context);
                }
                break ;
            }
            default :
                LCOMPILERS_ASSERT(false);
        }
        return type;
    }

    template <typename T>
    std::vector<llvm::Type*> convert_args(const T &x) {
        std::vector<llvm::Type*> args;
        for (size_t i=0; i<x.n_args; i++) {
            if (is_a<ASR::Variable_t>(*symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
                LCOMPILERS_ASSERT(is_arg_dummy(arg->m_intent));
                // We pass all arguments as pointers for now,
                // except bind(C) value arguments that are passed by value
                llvm::Type *type = nullptr, *type_original = nullptr;
                int n_dims = 0, a_kind = 4;
                bool is_array_type = false;
                type_original = get_arg_type_from_ttype_t(arg->m_type, x.m_abi,
                                    arg->m_abi, arg->m_storage, arg->m_value_attr,
                                    n_dims, a_kind, is_array_type, arg->m_intent,
                                    false);
                if( is_array_type ) {
                    type = type_original->getPointerTo();
                } else {
                    type = type_original;
                }
                if( arg->m_intent == ASRUtils::intent_out &&
                    ASR::is_a<ASR::CPtr_t>(*arg->m_type) ) {
                    type = type->getPointerTo();
                }
                std::uint32_t m_h;
                std::string m_name = std::string(x.m_name);
                if( x.class_type == ASR::symbolType::Function ) {
                    ASR::Function_t* _func = (ASR::Function_t*)(&(x.base));
                    m_h = get_hash((ASR::asr_t*)_func);
                }
                if( is_array_type && arg->m_type->type != ASR::ttypeType::Pointer ) {
                    if( x.m_abi == ASR::abiType::Source ) {
                        llvm::Type* orig_type = type_original;
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
            for (auto &item : x->m_symtab->get_scope()) {
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
            for (auto &item : x->m_symtab->get_scope()) {
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
                LCOMPILERS_ASSERT(is_arg_dummy(arg->m_intent));
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
            }
            i++;
        }
    }

    template <typename T>
    void declare_local_vars(const T &x) {
        declare_vars(x);
    }

    void visit_Function(const ASR::Function_t &x) {
        bool is_dict_present_copy_lp = dict_api_lp->is_dict_present();
        bool is_dict_present_copy_sc = dict_api_sc->is_dict_present();
        dict_api_lp->set_is_dict_present(false);
        dict_api_sc->set_is_dict_present(false);
        llvm_goto_targets.clear();
        instantiate_function(x);
        if (x.m_deftype == ASR::deftypeType::Interface) {
            // Interface does not have an implementation and it is already
            // declared, so there is nothing to do here
            return;
        }
        visit_procedures(x);
        generate_function(x);
        parent_function = nullptr;
        dict_api_lp->set_is_dict_present(is_dict_present_copy_lp);
        dict_api_sc->set_is_dict_present(is_dict_present_copy_sc);

        // Finalize the debug info.
        if (compiler_options.emit_debug_info) DBuilder->finalize();
    }

    void instantiate_function(const ASR::Function_t &x){
        uint32_t h = get_hash((ASR::asr_t*)&x);
        llvm::Function *F = nullptr;
        llvm::DISubprogram *SP;
        std::string sym_name = x.m_name;
        if (sym_name == "main") {
            sym_name = "_xx_lcompilers_changed_main_xx";
        }
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
                    fn_name = sym_name;
                }
            } else if (x.m_deftype == ASR::deftypeType::Interface &&
                x.m_abi != ASR::abiType::Intrinsic) {
                fn_name = sym_name;
            } else {
                fn_name = mangle_prefix + sym_name;
            }
            if (llvm_symtab_fn_names.find(fn_name) == llvm_symtab_fn_names.end()) {
                llvm_symtab_fn_names[fn_name] = h;
                F = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, fn_name, module.get());

                // Add Debugging information to the LLVM function F
                if (compiler_options.emit_debug_info) {
                    debug_emit_function(x, SP);
                    F->setSubprogram(SP);
                }
            } else {
                uint32_t old_h = llvm_symtab_fn_names[fn_name];
                F = llvm_symtab_fn[old_h];
                if (compiler_options.emit_debug_info) {
                    SP = (llvm::DISubprogram*) llvm_symtab_fn_discope[old_h];
                }
            }
            llvm_symtab_fn[h] = F;
            if (compiler_options.emit_debug_info) llvm_symtab_fn_discope[h] = SP;

            // Instantiate (pre-declare) all nested interfaces
            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *v = down_cast<ASR::Function_t>(
                            item.second);
                    instantiate_function(*v);
                }
            }
        }
    }


    llvm::FunctionType* get_function_type(const ASR::Function_t &x){
        llvm::Type *return_type;
        if (x.m_return_var) {
            ASR::ttype_t *return_var_type0 = EXPR2VAR(x.m_return_var)->m_type;
            ASR::ttypeType return_var_type = return_var_type0->type;
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
                            if (compiler_options.platform == Platform::Windows) {
                                // i64
                                return_type = llvm::Type::getInt64Ty(context);
                            } else if (compiler_options.platform == Platform::macOS_ARM) {
                                // {float, float}
                                return_type = getComplexType(a_kind);
                            } else {
                                // <2 x float>
                                return_type = FIXED_VECTOR_TYPE::get(llvm::Type::getFloatTy(context), 2);
                            }
                        } else {
                            return_type = getComplexType(a_kind);
                        }
                    } else {
                        LCOMPILERS_ASSERT(a_kind == 8)
                        if (x.m_abi == ASR::abiType::BindC) {
                            if (compiler_options.platform == Platform::Windows) {
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
                case (ASR::ttypeType::CPtr) :
                    return_type = llvm::Type::getVoidTy(context)->getPointerTo();
                    break;
                case (ASR::ttypeType::Const) : {
                    return_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(return_var_type0));
                    break;
                }
                case (ASR::ttypeType::Pointer) : {
                    return_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(return_var_type0))->getPointerTo();
                    break;
                }
                case (ASR::ttypeType::Struct) :
                    throw CodeGenError("Struct return type not implemented yet");
                    break;
                case (ASR::ttypeType::Tuple) : {
                    ASR::Tuple_t* asr_tuple = ASR::down_cast<ASR::Tuple_t>(return_var_type0);
                    std::string type_code = ASRUtils::get_type_code(asr_tuple->m_type,
                                                                    asr_tuple->n_type);
                    std::vector<llvm::Type*> llvm_el_types;
                    for( size_t i = 0; i < asr_tuple->n_type; i++ ) {
                        bool is_local_array_type = false, is_local_malloc_array_type = false;
                        bool is_local_list = false;
                        ASR::dimension_t* local_m_dims = nullptr;
                        int local_n_dims = 0;
                        int local_a_kind = -1;
                        ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
                        llvm_el_types.push_back(get_type_from_ttype_t(asr_tuple->m_type[i], local_m_storage,
                                                is_local_array_type, is_local_malloc_array_type,
                                                is_local_list, local_m_dims, local_n_dims, local_a_kind));
                    }
                    return_type = tuple_api->get_tuple_type(type_code, llvm_el_types);
                    break;
                }
                case (ASR::ttypeType::List) : {
                    bool is_array_type = false, is_malloc_array_type = false;
                    bool is_list = true;
                    ASR::dimension_t *m_dims = nullptr;
                    ASR::storage_typeType m_storage = ASR::storage_typeType::Default;
                    int n_dims = 0, a_kind = -1;
                    ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(return_var_type0);
                    llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_list->m_type, m_storage,
                                                                     is_array_type,
                                                                     is_malloc_array_type,
                                                                     is_list, m_dims, n_dims,
                                                                     a_kind);
                    int32_t type_size = -1;
                    if( LLVM::is_llvm_struct(asr_list->m_type) ||
                        ASR::is_a<ASR::Character_t>(*asr_list->m_type) ||
                        ASR::is_a<ASR::Complex_t>(*asr_list->m_type) ) {
                        llvm::DataLayout data_layout(module.get());
                        type_size = data_layout.getTypeAllocSize(el_llvm_type);
                    } else {
                        type_size = a_kind;
                    }
                    std::string el_type_code = ASRUtils::get_type_code(asr_list->m_type);
                    return_type = list_api->get_list_type(el_llvm_type, el_type_code, type_size);
                    break;
                }
                default :
                    throw CodeGenError("Type not implemented " + std::to_string(return_var_type));
            }
        } else {
            return_type = llvm::Type::getVoidTy(context);
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
        if (compiler_options.emit_debug_info) debug_current_scope = llvm_symtab_fn_discope[h];
        proc_return = llvm::BasicBlock::Create(context, "return");
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);
        if (compiler_options.emit_debug_info) debug_emit_loc(x);
        declare_args(x, *F);
        declare_local_vars(x);
    }


    inline void define_function_exit(const ASR::Function_t& x) {
        if (x.m_return_var) {
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
                        if (compiler_options.platform == Platform::Windows) {
                            // tmp is {float, float}*
                            // type_fx2p is i64*
                            llvm::Type* type_fx2p = llvm::Type::getInt64PtrTy(context);
                            // Convert {float,float}* to i64* using bitcast
                            tmp = builder->CreateBitCast(tmp, type_fx2p);
                            // Then convert i64* -> i64
                            tmp = CreateLoad(tmp);
                        } else if (compiler_options.platform == Platform::macOS_ARM) {
                            // Pass by value
                            tmp = CreateLoad(tmp);
                        } else {
                            // tmp is {float, float}*
                            // type_fx2p is <2 x float>*
                            llvm::Type* type_fx2p = FIXED_VECTOR_TYPE::get(llvm::Type::getFloatTy(context), 2)->getPointerTo();
                            // Convert {float,float}* to <2 x float>* using bitcast
                            tmp = builder->CreateBitCast(tmp, type_fx2p);
                            // Then convert <2 x float>* -> <2 x float>
                            tmp = CreateLoad(tmp);
                        }
                    } else {
                        LCOMPILERS_ASSERT(c_kind == 8)
                        if (compiler_options.platform == Platform::Windows) {
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
        } else {
            start_new_block(proc_return);
            builder->CreateRetVoid();
        }
    }

    void generate_function(const ASR::Function_t &x) {
        bool interactive = (x.m_abi == ASR::abiType::Interactive);
        if (x.m_deftype == ASR::deftypeType::Implementation ) {

            if (interactive) return;

            if (compiler_options.generate_object_code
                    && (x.m_abi == ASR::abiType::Intrinsic)
                    && !compiler_options.rtlib) {
                // Skip intrinsic functions in generate_object_code mode
                // They must be later linked
                return;
            }

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
            if( m_name == "lbound" || m_name == "ubound" ) {
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
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }
    }

    bool is_nested_pointer(llvm::Value* val) {
        // TODO: Remove this in future
        // Related issue, https://github.com/lcompilers/lpython/pull/707#issuecomment-1169773106.
        return val->getType()->isPointerTy() &&
               val->getType()->getContainedType(0)->isPointerTy();
    }

    void visit_CLoc(const ASR::CLoc_t& x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_arg);
        ptr_loads = ptr_loads_copy;
        if( is_nested_pointer(tmp) ) {
            tmp = CreateLoad(tmp);
        }
        ASR::ttype_t* arg_type = ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_arg));
        if( arr_descr->is_array(arg_type) ) {
            tmp = CreateLoad(arr_descr->get_pointer_to_data(tmp));
        }
        tmp = builder->CreateBitCast(tmp,
                    llvm::Type::getVoidTy(context)->getPointerTo());
    }


    llvm::Value* GetPointerCPtrUtil(llvm::Value* llvm_tmp, ASR::ttype_t* asr_type) {
        // If the input is a simple variable and not a pointer
        // then this check will fail and load will not happen
        // (which is what we want for simple variables).
        // For pointers, the actual LLVM variable will be a
        // double pointer, so we need to load one time and then
        // use it later on.
        if( is_nested_pointer(llvm_tmp) &&
            !ASR::is_a<ASR::CPtr_t>(*asr_type)) {
            llvm_tmp = CreateLoad(llvm_tmp);
        }
        if( arr_descr->is_array(asr_type) &&
            !ASR::is_a<ASR::CPtr_t>(*asr_type) ) {
            llvm_tmp = CreateLoad(arr_descr->get_pointer_to_data(llvm_tmp));
        }

        // // TODO: refactor this into a function, it is being used a few times
        // llvm::Type *target_type = llvm_tmp->getType();
        // // Create alloca to get a pointer, but do it
        // // at the beginning of the function to avoid
        // // using alloca inside a loop, which would
        // // run out of stack
        // llvm::BasicBlock &entry_block = builder->GetInsertBlock()->getParent()->getEntryBlock();
        // llvm::IRBuilder<> builder0(context);
        // builder0.SetInsertPoint(&entry_block, entry_block.getFirstInsertionPt());
        // llvm::AllocaInst *target = builder0.CreateAlloca(
        //     target_type, nullptr, "call_arg_value_ptr");
        // builder->CreateStore(llvm_tmp, target);
        // llvm_tmp = target;
        return llvm_tmp;
    }

    void visit_GetPointer(const ASR::GetPointer_t& x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_arg);
        ptr_loads = ptr_loads_copy;
        ASR::ttype_t* arg_type = ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_arg));
        tmp = GetPointerCPtrUtil(tmp, arg_type);
    }

    void visit_PointerToCPtr(const ASR::PointerToCPtr_t& x) {
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 0;
        this->visit_expr(*x.m_arg);
        ptr_loads = ptr_loads_copy;
        if( !ASR::is_a<ASR::GetPointer_t>(*x.m_arg) ) {
            ASR::ttype_t* arg_type = ASRUtils::get_contained_type(
                                         ASRUtils::expr_type(x.m_arg));
            tmp = GetPointerCPtrUtil(tmp, arg_type);
        }
        tmp = builder->CreateBitCast(tmp,
                    llvm::Type::getVoidTy(context)->getPointerTo());
    }


    void visit_CPtrToPointer(const ASR::CPtrToPointer_t& x) {
        ASR::expr_t *cptr = x.m_cptr, *fptr = x.m_ptr, *shape = x.m_shape;
        int reduce_loads = 0;
        if( ASR::is_a<ASR::Var_t>(*cptr) ) {
            ASR::Variable_t* cptr_var = ASRUtils::EXPR2VAR(cptr);
            reduce_loads = cptr_var->m_intent == ASRUtils::intent_in;
        }
        if( ASRUtils::is_array(ASRUtils::expr_type(fptr)) ) {
            int64_t ptr_loads_copy = ptr_loads;
            ptr_loads = 1 - reduce_loads;
            this->visit_expr(*cptr);
            llvm::Value* llvm_cptr = tmp;
            ptr_loads = 0;
            this->visit_expr(*fptr);
            llvm::Value* llvm_fptr = tmp;
            ptr_loads = ptr_loads_copy;
            llvm::Value* llvm_shape = nullptr;
            ASR::ttype_t* asr_shape_type = nullptr;
            if( shape ) {
                asr_shape_type = ASRUtils::get_contained_type(ASRUtils::expr_type(shape));
                this->visit_expr(*shape);
                llvm_shape = tmp;
            }
            ASR::ttype_t* fptr_type = ASRUtils::expr_type(fptr);
            llvm::Type* llvm_fptr_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(fptr_type));
            llvm::Value* fptr_array = builder->CreateAlloca(llvm_fptr_type);
            ASR::dimension_t* fptr_dims;
            int fptr_rank = ASRUtils::extract_dimensions_from_ttype(
                                ASRUtils::expr_type(fptr),
                                fptr_dims);
            llvm::Value* llvm_rank = llvm::ConstantInt::get(context, llvm::APInt(32, fptr_rank));
            llvm::Value* dim_des = builder->CreateAlloca(arr_descr->get_dimension_descriptor_type(), llvm_rank);
            builder->CreateStore(dim_des, arr_descr->get_pointer_to_dimension_descriptor_array(fptr_array, false));
            arr_descr->set_rank(fptr_array, llvm_rank);
            builder->CreateStore(fptr_array, llvm_fptr);
            llvm_fptr = fptr_array;
            ASR::ttype_t* fptr_data_type = ASRUtils::duplicate_type_without_dims(al, ASRUtils::get_contained_type(fptr_type), fptr_type->base.loc);
            llvm::Type* llvm_fptr_data_type = get_type_from_ttype_t_util(fptr_data_type);
            llvm::Value* fptr_data = arr_descr->get_pointer_to_data(llvm_fptr);
            llvm::Value* fptr_des = arr_descr->get_pointer_to_dimension_descriptor_array(llvm_fptr);
            llvm::Value* shape_data = llvm_shape;
            if( llvm_shape && !ASR::is_a<ASR::ArrayConstant_t>(*shape) && arr_descr->is_array(asr_shape_type) ) {
                shape_data = CreateLoad(arr_descr->get_pointer_to_data(llvm_shape));
            }
            llvm_cptr = builder->CreateBitCast(llvm_cptr, llvm_fptr_data_type->getPointerTo());
            builder->CreateStore(llvm_cptr, fptr_data);
            for( int i = 0; i < fptr_rank; i++ ) {
                llvm::Value* curr_dim = llvm::ConstantInt::get(context, llvm::APInt(32, i));
                llvm::Value* desi = arr_descr->get_pointer_to_dimension_descriptor(fptr_des, curr_dim);
                llvm::Value* desi_lb = arr_descr->get_lower_bound(desi, false);
                llvm::Value* desi_size = arr_descr->get_dimension_size(fptr_des, curr_dim, false);
                llvm::Value* i32_one = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
                llvm::Value* new_lb = i32_one;
                llvm::Value* new_ub = shape_data ? CreateLoad(llvm_utils->create_ptr_gep(shape_data, i)) : i32_one;
                builder->CreateStore(new_lb, desi_lb);
                builder->CreateStore(builder->CreateAdd(builder->CreateSub(new_ub, new_lb), i32_one), desi_size);
            }
        } else {
            int64_t ptr_loads_copy = ptr_loads;
            ptr_loads = 1 - reduce_loads;
            this->visit_expr(*cptr);
            llvm::Value* llvm_cptr = tmp;
            ptr_loads = 0;
            this->visit_expr(*fptr);
            llvm::Value* llvm_fptr = tmp;
            ptr_loads = ptr_loads_copy;
            llvm::Type* llvm_fptr_type = get_type_from_ttype_t_util(
                ASRUtils::get_contained_type(ASRUtils::expr_type(fptr)));
            llvm_cptr = builder->CreateBitCast(llvm_cptr, llvm_fptr_type->getPointerTo());
            builder->CreateStore(llvm_cptr, llvm_fptr);
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
        if (compiler_options.emit_debug_info) debug_emit_loc(x);
        if( x.m_overloaded ) {
            this->visit_stmt(*x.m_overloaded);
            return ;
        }

        ASR::ttype_t* asr_target_type = ASRUtils::expr_type(x.m_target);
        ASR::ttype_t* asr_value_type = ASRUtils::expr_type(x.m_value);
        bool is_target_list = ASR::is_a<ASR::List_t>(*asr_target_type);
        bool is_value_list = ASR::is_a<ASR::List_t>(*asr_value_type);
        bool is_target_tuple = ASR::is_a<ASR::Tuple_t>(*asr_target_type);
        bool is_value_tuple = ASR::is_a<ASR::Tuple_t>(*asr_value_type);
        bool is_target_dict = ASR::is_a<ASR::Dict_t>(*asr_target_type);
        bool is_value_dict = ASR::is_a<ASR::Dict_t>(*asr_value_type);
        bool is_target_struct = ASR::is_a<ASR::Struct_t>(*asr_target_type);
        bool is_value_struct = ASR::is_a<ASR::Struct_t>(*asr_value_type);
        if( is_target_list && is_value_list ) {
            int64_t ptr_loads_copy = ptr_loads;
            ptr_loads = 0;
            this->visit_expr(*x.m_target);
            llvm::Value* target_list = tmp;
            this->visit_expr(*x.m_value);
            llvm::Value* value_list = tmp;
            ptr_loads = ptr_loads_copy;
            ASR::List_t* value_asr_list = ASR::down_cast<ASR::List_t>(
                                            ASRUtils::expr_type(x.m_value));
            std::string value_type_code = ASRUtils::get_type_code(value_asr_list->m_type);
            list_api->list_deepcopy(value_list, target_list,
                                    value_asr_list, module.get(),
                                    name2memidx);
            return ;
        } else if( is_target_tuple && is_value_tuple ) {
            int64_t ptr_loads_copy = ptr_loads;
            if( ASR::is_a<ASR::TupleConstant_t>(*x.m_target) &&
                !ASR::is_a<ASR::TupleConstant_t>(*x.m_value) ) {
                ptr_loads = 0;
                this->visit_expr(*x.m_value);
                llvm::Value* value_tuple = tmp;
                ASR::TupleConstant_t* const_tuple = ASR::down_cast<ASR::TupleConstant_t>(x.m_target);
                for( size_t i = 0; i < const_tuple->n_elements; i++ ) {
                    ptr_loads = 0;
                    visit_expr(*const_tuple->m_elements[i]);
                    llvm::Value* target_ptr = tmp;
                    llvm::Value* item = tuple_api->read_item(value_tuple, i, false);
                    builder->CreateStore(item, target_ptr);
                }
                ptr_loads = ptr_loads_copy;
            } else if( ASR::is_a<ASR::TupleConstant_t>(*x.m_target) &&
                       ASR::is_a<ASR::TupleConstant_t>(*x.m_value) ) {
                ASR::TupleConstant_t* asr_value_tuple = ASR::down_cast<ASR::TupleConstant_t>(x.m_value);
                Vec<llvm::Value*> src_deepcopies;
                src_deepcopies.reserve(al, asr_value_tuple->n_elements);
                for( size_t i = 0; i < asr_value_tuple->n_elements; i++ ) {
                    ASR::ttype_t* asr_tuple_i_type = ASRUtils::expr_type(asr_value_tuple->m_elements[i]);
                    llvm::Type* llvm_tuple_i_type = get_type_from_ttype_t_util(asr_tuple_i_type);
                    llvm::Value* llvm_tuple_i = builder->CreateAlloca(llvm_tuple_i_type, nullptr);
                    ptr_loads = !LLVM::is_llvm_struct(asr_tuple_i_type);
                    visit_expr(*asr_value_tuple->m_elements[i]);
                    llvm_utils->deepcopy(tmp, llvm_tuple_i, asr_tuple_i_type, module.get(), name2memidx);
                    src_deepcopies.push_back(al, llvm_tuple_i);
                }
                ASR::TupleConstant_t* asr_target_tuple = ASR::down_cast<ASR::TupleConstant_t>(x.m_target);
                for( size_t i = 0; i < asr_target_tuple->n_elements; i++ ) {
                    ptr_loads = 0;
                    visit_expr(*asr_target_tuple->m_elements[i]);
                    LLVM::CreateStore(*builder,
                        LLVM::CreateLoad(*builder, src_deepcopies[i]),
                        tmp
                    );
                }
                ptr_loads = ptr_loads_copy;
            } else {
                ptr_loads = 0;
                this->visit_expr(*x.m_value);
                llvm::Value* value_tuple = tmp;
                this->visit_expr(*x.m_target);
                llvm::Value* target_tuple = tmp;
                ptr_loads = ptr_loads_copy;
                ASR::Tuple_t* value_tuple_type = ASR::down_cast<ASR::Tuple_t>(asr_value_type);
                std::string type_code = ASRUtils::get_type_code(value_tuple_type->m_type,
                                                                value_tuple_type->n_type);
                tuple_api->tuple_deepcopy(value_tuple, target_tuple,
                                          value_tuple_type, module.get(),
                                          name2memidx);
            }
            return ;
        } else if( is_target_dict && is_value_dict ) {
            int64_t ptr_loads_copy = ptr_loads;
            ptr_loads = 0;
            this->visit_expr(*x.m_value);
            llvm::Value* value_dict = tmp;
            this->visit_expr(*x.m_target);
            llvm::Value* target_dict = tmp;
            ptr_loads = ptr_loads_copy;
            ASR::Dict_t* value_dict_type = ASR::down_cast<ASR::Dict_t>(asr_value_type);
            set_dict_api(value_dict_type);
            llvm_utils->dict_api->dict_deepcopy(value_dict, target_dict,
                                    value_dict_type, module.get(), name2memidx);
            return ;
        } else if( is_target_struct && is_value_struct ) {
            int64_t ptr_loads_copy = ptr_loads;
            ptr_loads = 0;
            this->visit_expr(*x.m_value);
            llvm::Value* value_struct = tmp;
            bool is_assignment_target_copy = is_assignment_target;
            is_assignment_target = true;
            this->visit_expr(*x.m_target);
            is_assignment_target = is_assignment_target_copy;
            llvm::Value* target_struct = tmp;
            ptr_loads = ptr_loads_copy;
            llvm_utils->deepcopy(value_struct, target_struct,
                asr_target_type, module.get(), name2memidx);
            return ;
        }

        if( ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
            ASR::is_a<ASR::GetPointer_t>(*x.m_value) ) {
            ASR::Variable_t *asr_target = EXPR2VAR(x.m_target);
            ASR::GetPointer_t* get_ptr = ASR::down_cast<ASR::GetPointer_t>(x.m_value);
            ASR::Variable_t *asr_value = EXPR2VAR(get_ptr->m_arg);
            uint32_t value_h = get_hash((ASR::asr_t*)asr_value);
            uint32_t target_h = get_hash((ASR::asr_t*)asr_target);
            builder->CreateStore(llvm_symtab[value_h], llvm_symtab[target_h]);
            return ;
        }
        llvm::Value *target, *value;
        uint32_t h;
        bool lhs_is_string_arrayref = false;
        if( x.m_target->type == ASR::exprType::ArrayItem ||
            x.m_target->type == ASR::exprType::ArraySection ||
            x.m_target->type == ASR::exprType::StructInstanceMember ||
            x.m_target->type == ASR::exprType::ListItem ||
            x.m_target->type == ASR::exprType::UnionRef ) {
            is_assignment_target = true;
            this->visit_expr(*x.m_target);
            is_assignment_target = false;
            target = tmp;
            if (is_a<ASR::ArrayItem_t>(*x.m_target)) {
                ASR::ArrayItem_t *asr_target0 = ASR::down_cast<ASR::ArrayItem_t>(x.m_target);
                if (is_a<ASR::Var_t>(*asr_target0->m_v)) {
                    ASR::Variable_t *asr_target = ASRUtils::EXPR2VAR(asr_target0->m_v);
                    if ( is_a<ASR::Character_t>(*asr_target->m_type) ) {
                        ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(asr_target->m_type);
                        if (t->n_dims == 0) {
                            target = CreateLoad(target);
                            lhs_is_string_arrayref = true;
                        }
                    }
                }
            } else if (is_a<ASR::ArraySection_t>(*x.m_target)) {
                ASR::ArraySection_t *asr_target0 = ASR::down_cast<ASR::ArraySection_t>(x.m_target);
                if (is_a<ASR::Var_t>(*asr_target0->m_v)) {
                    ASR::Variable_t *asr_target = ASRUtils::EXPR2VAR(asr_target0->m_v);
                    if ( is_a<ASR::Character_t>(*asr_target->m_type) ) {
                        ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(asr_target->m_type);
                        if (t->n_dims == 0) {
                            target = CreateLoad(target);
                            lhs_is_string_arrayref = true;
                        }
                    }
                }
            } else if( ASR::is_a<ASR::ListItem_t>(*x.m_target) ) {
                ASR::ListItem_t* asr_target0 = ASR::down_cast<ASR::ListItem_t>(x.m_target);
                int64_t ptr_loads_copy = ptr_loads;
                ptr_loads = 0;
                this->visit_expr(*asr_target0->m_a);
                ptr_loads = ptr_loads_copy;
                llvm::Value* list = tmp;
                this->visit_expr_wrapper(asr_target0->m_pos, true);
                llvm::Value* pos = tmp;

                target = list_api->read_item(list, pos, compiler_options.enable_bounds_checking,
                                             *module, true);
            }
        } else {
            ASR::Variable_t *asr_target = EXPR2VAR(x.m_target);
            h = get_hash((ASR::asr_t*)asr_target);
            if (llvm_symtab.find(h) != llvm_symtab.end()) {
                target = llvm_symtab[h];
                if (ASR::is_a<ASR::Pointer_t>(*asr_target->m_type) &&
                    !ASR::is_a<ASR::CPtr_t>(
                        *ASR::down_cast<ASR::Pointer_t>(asr_target->m_type)->m_type)) {
                    target = CreateLoad(target);
                }
            } else {
                /* Target for assignment not in the symbol table - must be
                assigning to an outer scope from a nested function - see
                nested_05.f90 */
                auto finder = std::find(nested_globals.begin(),
                        nested_globals.end(), h);
                LCOMPILERS_ASSERT(finder != nested_globals.end());
                llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                    nested_global_struct);
                int idx = std::distance(nested_globals.begin(), finder);
                target = CreateLoad(llvm_utils->create_gep(ptr, idx));
            }
            if( arr_descr->is_array(ASRUtils::get_contained_type(asr_target_type)) ) {
                if( asr_target->m_type->type ==
                    ASR::ttypeType::Character ) {
                    target = CreateLoad(arr_descr->get_pointer_to_data(target));
                }
            }
        }
        if( ASR::is_a<ASR::UnionTypeConstructor_t>(*x.m_value) ) {
            return ;
        }
        ASR::ttype_t* target_type = ASRUtils::expr_type(x.m_target);
        ASR::ttype_t* value_type = ASRUtils::expr_type(x.m_value);
        this->visit_expr_wrapper(x.m_value, true);
        if( ASR::is_a<ASR::Var_t>(*x.m_value) &&
            ASR::is_a<ASR::Union_t>(*value_type) ) {
            tmp = LLVM::CreateLoad(*builder, tmp);
        }
        value = tmp;
        if ( is_a<ASR::Character_t>(*expr_type(x.m_value)) ) {
            ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(expr_type(x.m_value));
            if (t->n_dims == 0) {
                if (lhs_is_string_arrayref) {
                    value = CreateLoad(value);
                }
            }
        }
        if( ASRUtils::is_array(target_type) &&
            ASRUtils::is_array(value_type) &&
            ASRUtils::check_equal_type(target_type, value_type) ) {
            bool data_only_copy = false;
            bool is_target_data_only_array = ASRUtils::expr_abi(x.m_target) == ASR::abiType::BindC;
            bool is_value_data_only_array = ASRUtils::expr_abi(x.m_value) == ASR::abiType::BindC;
            if( is_target_data_only_array || is_value_data_only_array ) {
                llvm::Value *target_data = nullptr, *value_data = nullptr, *llvm_size = nullptr;
                if( is_target_data_only_array ) {
                    target_data = target;
                    ASR::dimension_t* target_dims = nullptr;
                    int target_ndims = ASRUtils::extract_dimensions_from_ttype(target_type, target_dims);
                    size_t target_size = 1;
                    data_only_copy = true;
                    for( int i = 0; i < target_ndims; i++ ) {
                        int dim_length = -1;
                        if( !ASRUtils::extract_value(ASRUtils::expr_value(target_dims[i].m_length), dim_length) ) {
                            data_only_copy = false;
                            break;
                        }
                        target_size *= dim_length;
                    }
                    if( data_only_copy ) {
                        llvm_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                           llvm::APInt(32, target_size));
                        data_only_copy = false;
                    }
                } else {
                    target_data = LLVM::CreateLoad(*builder, arr_descr->get_pointer_to_data(target));
                }
                if( is_value_data_only_array ) {
                    value_data = value;
                    ASR::dimension_t* value_dims = nullptr;
                    int value_ndims = ASRUtils::extract_dimensions_from_ttype(value_type, value_dims);
                    size_t value_size = 1;
                    data_only_copy = true;
                    for( int i = 0; i < value_ndims; i++ ) {
                        int dim_length = -1;
                        if( !ASRUtils::extract_value(ASRUtils::expr_value(value_dims[i].m_length), dim_length) ) {
                            data_only_copy = false;
                            break;
                        }
                        value_size *= dim_length;
                    }
                    if( data_only_copy ) {
                        llvm_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                           llvm::APInt(32, value_size));
                        data_only_copy = false;
                    }
                } else {
                    value_data = LLVM::CreateLoad(*builder, arr_descr->get_pointer_to_data(value));
                }
                if( llvm_size ) {
                    arr_descr->copy_array_data_only(value_data, target_data, module.get(),
                                                    target_type, llvm_size);
                }
            } else {
                arr_descr->copy_array(value, target, module.get(),
                                    target_type, false, false);
            }
        } else {
            builder->CreateStore(value, target);
        }
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
        if (is_a<ASR::ArrayItem_t>(*x.m_target)) {
            ASR::ArrayItem_t *asr_target0 = ASR::down_cast<ASR::ArrayItem_t>(x.m_target);
            if (is_a<ASR::Var_t>(*asr_target0->m_v)) {
                ASR::Variable_t *asr_target = ASRUtils::EXPR2VAR(asr_target0->m_v);
                h = get_hash((ASR::asr_t*)asr_target);
                auto finder = std::find(nested_globals.begin(),
                        nested_globals.end(), h);
                if (finder != nested_globals.end()) {
                    // This is used since array pass use array item visit
                    llvm::Constant *ptr = module->getOrInsertGlobal(nested_desc_name,
                    nested_global_struct);
                    int idx = std::distance(nested_globals.begin(), finder);
                    std::vector<llvm::Value*> idx_vec = {
                    llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                    llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
                    llvm::Value* array = CreateGEP(ptr, idx_vec);
                    std::vector<llvm::Value*> indices;
                    for( size_t r = 0; r < asr_target0->n_args; r++ ) {
                        ASR::array_index_t curr_idx = asr_target0->m_args[r];
                        uint64_t ptr_loads_copy = ptr_loads;
                        ptr_loads = 2;
                        this->visit_expr_wrapper(curr_idx.m_right, true);
                        ptr_loads = ptr_loads_copy;
                        indices.push_back(tmp);
                    }
                    ASR::dimension_t* m_dims;
                    ASRUtils::extract_dimensions_from_ttype(
                                    ASRUtils::expr_type(asr_target0->m_v), m_dims);
                    Vec<llvm::Value*> llvm_diminfo;
                    llvm_diminfo.reserve(al, 2 * asr_target0->n_args + 1);
                    for( size_t idim = 0; idim < asr_target0->n_args; idim++ ) {
                        this->visit_expr_wrapper(m_dims[idim].m_start, true);
                        llvm::Value* dim_start = tmp;
                        this->visit_expr_wrapper(m_dims[idim].m_length, true);
                        llvm::Value* dim_size = tmp;
                        llvm_diminfo.push_back(al, dim_start);
                        llvm_diminfo.push_back(al, dim_size);
                    }
                    tmp = arr_descr->get_single_element(array, indices, asr_target0->n_args,
                                                        true, false, llvm_diminfo.p);
                    builder->CreateStore(target, tmp);
                }
            }
        }
    }

    void visit_AssociateBlockCall(const ASR::AssociateBlockCall_t& x) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::AssociateBlock_t>(*x.m_m));
        ASR::AssociateBlock_t* associate_block = ASR::down_cast<ASR::AssociateBlock_t>(x.m_m);
        declare_vars(*associate_block);
        for (size_t i = 0; i < associate_block->n_body; i++) {
            this->visit_stmt(*(associate_block->m_body[i]));
        }
    }

    void visit_BlockCall(const ASR::BlockCall_t& x) {
        if( x.m_label != -1 ) {
            if( llvm_goto_targets.find(x.m_label) == llvm_goto_targets.end() ) {
                llvm::BasicBlock *new_target = llvm::BasicBlock::Create(context, "goto_target");
                llvm_goto_targets[x.m_label] = new_target;
            }
            start_new_block(llvm_goto_targets[x.m_label]);
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Block_t>(*x.m_m));
        ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(x.m_m);
        declare_vars(*block);
        for (size_t i = 0; i < block->n_body; i++) {
            this->visit_stmt(*(block->m_body[i]));
        }
    }

    inline void visit_expr_wrapper(const ASR::expr_t* x, bool load_ref=false) {
        this->visit_expr(*x);
        if( x->type == ASR::exprType::ArrayItem ||
            x->type == ASR::exprType::ArraySection ||
            x->type == ASR::exprType::StructInstanceMember ) {
            if( load_ref ) {
                tmp = CreateLoad(tmp);
            }
        }
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right = tmp;
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
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right = tmp;
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
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right = tmp;
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
    }

    void visit_StringCompare(const ASR::StringCompare_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right = tmp;
        std::string fn;
        switch (x.m_op) {
            case (ASR::cmpopType::Eq) : {
                fn = "_lpython_str_compare_eq";
                break;
            }
            case (ASR::cmpopType::NotEq) : {
                fn = "_lpython_str_compare_noteq";
                break;
            }
            case (ASR::cmpopType::Gt) : {
                fn = "_lpython_str_compare_gt";
                break;
            }
            case (ASR::cmpopType::GtE) : {
                fn = "_lpython_str_compare_gte";
                break;
            }
            case (ASR::cmpopType::Lt) : {
                fn = "_lpython_str_compare_lt";
                break;
            }
            case (ASR::cmpopType::LtE) : {
                fn = "_lpython_str_compare_lte";
                break;
            }
            default : {
                throw CodeGenError("Comparison operator not implemented",
                        x.base.base.loc);
            }
        }
        tmp = lfortran_str_cmp(left, right, fn);
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right = tmp;
        // i1 -> i32
        left = builder->CreateZExt(left, llvm::Type::getInt32Ty(context));
        right = builder->CreateZExt(right, llvm::Type::getInt32Ty(context));
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
                throw CodeGenError("Comparison operator not implemented",
                        x.base.base.loc);
            }
        }
    }

    void visit_OverloadedCompare(const ASR::OverloadedCompare_t &x) {
        this->visit_expr(*x.m_overloaded);
    }

    void visit_If(const ASR::If_t &x) {
        this->visit_expr_wrapper(x.m_test, true);
        create_if_else(tmp, [=]() {
            for (size_t i=0; i<x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
        }, [=]() {
            for (size_t i=0; i<x.n_orelse; i++) {
                this->visit_stmt(*x.m_orelse[i]);
            }
        });
    }

    void visit_IfExp(const ASR::IfExp_t &x) {
        // IfExp(expr test, expr body, expr orelse, ttype type, expr? value)
        this->visit_expr_wrapper(x.m_test, true);
        llvm::Value *cond = tmp;
        llvm::Value *then_val = nullptr;
        llvm::Value *else_val = nullptr;
        create_if_else(cond, [=, &then_val]() {
            this->visit_expr_wrapper(x.m_body, true);
            then_val = tmp;
        }, [=, &else_val]() {
            this->visit_expr_wrapper(x.m_orelse, true);
            else_val = tmp;
        });
        tmp = builder->CreateSelect(cond, then_val, else_val);
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        create_loop([=]() {
            this->visit_expr_wrapper(x.m_test, true);
            return tmp;
        }, [=]() {
            for (size_t i=0; i<x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
        });
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

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        LCOMPILERS_ASSERT(ASRUtils::is_logical(*x.m_type))
        switch (x.m_op) {
            case ASR::logicalbinopType::And: {
                tmp = builder->CreateAnd(left_val, right_val);
                break;
            };
            case ASR::logicalbinopType::Or: {
                tmp = builder->CreateOr(left_val, right_val);
                break;
            };
            case ASR::logicalbinopType::Xor: {
                tmp = builder->CreateXor(left_val, right_val);
                break;
            };
            case ASR::logicalbinopType::NEqv: {
                tmp = builder->CreateXor(left_val, right_val);
                break;
            };
            case ASR::logicalbinopType::Eqv: {
                tmp = builder->CreateXor(left_val, right_val);
                tmp = builder->CreateNot(tmp);
            };
        }
    }

    void visit_StringRepeat(const ASR::StringRepeat_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        tmp = lfortran_strrepeat(left_val, right_val);
    }

    void visit_StringConcat(const ASR::StringConcat_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        tmp = lfortran_strop(left_val, right_val, "_lfortran_strcat");
    }

    void visit_StringLen(const ASR::StringLen_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::AllocaInst *parg = builder->CreateAlloca(character_type, nullptr);
        builder->CreateStore(tmp, parg);
        tmp = lfortran_str_len(parg);
    }

    void visit_StringOrd(const ASR::StringOrd_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::AllocaInst *parg = builder->CreateAlloca(character_type, nullptr);
        builder->CreateStore(tmp, parg);
        tmp = lfortran_str_ord(parg);
    }

    void visit_StringChr(const ASR::StringChr_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        tmp = lfortran_str_chr(tmp);
    }

    void visit_StringItem(const ASR::StringItem_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_idx, true);
        llvm::Value *idx = tmp;
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::Value *str = tmp;
        tmp = lfortran_str_copy(str, idx, idx);
    }

    void visit_StringSection(const ASR::StringSection_t& x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
         }
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::Value *str = tmp;
        llvm::Value *left, *right, *step;
        llvm::Value *left_present, *right_present;
        if (x.m_start) {
            this->visit_expr_wrapper(x.m_start, true);
            left = tmp;
            left_present = llvm::ConstantInt::get(context,
                llvm::APInt(1, 1));
        } else {
            left = llvm::Constant::getNullValue(llvm::Type::getInt32Ty(context));
            left_present = llvm::ConstantInt::get(context,
                llvm::APInt(1, 0));
        }
        if (x.m_end) {
            this->visit_expr_wrapper(x.m_end, true);
            right = tmp;
            right_present = llvm::ConstantInt::get(context,
                llvm::APInt(1, 1));
        } else {
            right = llvm::Constant::getNullValue(llvm::Type::getInt32Ty(context));
            right_present = llvm::ConstantInt::get(context,
                llvm::APInt(1, 0));
        }
        if (x.m_step) {
            this->visit_expr_wrapper(x.m_step, true);
            step = tmp;
        } else {
            step = llvm::ConstantInt::get(context,
                llvm::APInt(32, 1));
        }
        tmp = lfortran_str_slice(str, left, right, step, left_present, right_present);
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        LCOMPILERS_ASSERT(ASRUtils::is_integer(*x.m_type))
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
            case ASR::binopType::BitOr: {
                tmp = builder->CreateOr(left_val, right_val);
                break;
            }
            case ASR::binopType::BitAnd: {
                tmp = builder->CreateAnd(left_val, right_val);
                break;
            }
            case ASR::binopType::BitXor: {
                tmp = builder->CreateXor(left_val, right_val);
                break;
            }
            case ASR::binopType::BitLShift: {
                tmp = builder->CreateShl(left_val, right_val);
                break;
            }
            case ASR::binopType::BitRShift: {
                tmp = builder->CreateAShr(left_val, right_val);
                break;
            }
        }
    }

    void visit_RealBinOp(const ASR::RealBinOp_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        lookup_enum_value_for_nonints = true;
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        lookup_enum_value_for_nonints = false;
        LCOMPILERS_ASSERT(ASRUtils::is_real(*x.m_type))
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
            default: {
                throw CodeGenError("Binary operator '" + ASRUtils::binop_to_str_python(x.m_op) + "' not supported",
                    x.base.base.loc);
            }
        }
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        LCOMPILERS_ASSERT(ASRUtils::is_complex(*x.m_type));
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
        std::string fn_name;
        switch (x.m_op) {
            case ASR::binopType::Add: {
                if (a_kind == 4) {
                    fn_name = "_lfortran_complex_add_32";
                } else {
                    fn_name = "_lfortran_complex_add_64";
                }
                break;
            };
            case ASR::binopType::Sub: {
                if (a_kind == 4) {
                    fn_name = "_lfortran_complex_sub_32";
                } else {
                    fn_name = "_lfortran_complex_sub_64";
                }
                break;
            };
            case ASR::binopType::Mul: {
                if (a_kind == 4) {
                    fn_name = "_lfortran_complex_mul_32";
                } else {
                    fn_name = "_lfortran_complex_mul_64";
                }
                break;
            };
            case ASR::binopType::Div: {
                if (a_kind == 4) {
                    fn_name = "_lfortran_complex_div_32";
                } else {
                    fn_name = "_lfortran_complex_div_64";
                }
                break;
            };
            case ASR::binopType::Pow: {
                if (a_kind == 4) {
                    fn_name = "_lfortran_complex_pow_32";
                } else {
                    fn_name = "_lfortran_complex_pow_64";
                }
                break;
            };
            default: {
                throw CodeGenError("Binary operator '" + ASRUtils::binop_to_str_python(x.m_op) + "' not supported",
                    x.base.base.loc);
            }
        }
        tmp = lfortran_complex_bin_op(left_val, right_val, fn_name, type);
    }

    void visit_OverloadedBinOp(const ASR::OverloadedBinOp_t &x) {
        this->visit_expr(*x.m_overloaded);
    }

    void visit_IntegerBitNot(const ASR::IntegerBitNot_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        tmp = builder->CreateNot(tmp);
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::Value *zero = llvm::ConstantInt::get(context,
            llvm::APInt(ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x.m_arg)) * 8, 0));
        tmp = builder->CreateSub(zero, tmp);
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::Value *zero;
        int a_kind = down_cast<ASR::Real_t>(x.m_type)->m_kind;
        if (a_kind == 4) {
            zero = llvm::ConstantFP::get(context,
                llvm::APFloat((float)0.0));
        } else if (a_kind == 8) {
            zero = llvm::ConstantFP::get(context,
                llvm::APFloat((double)0.0));
        } else {
            throw CodeGenError("RealUnaryMinus: kind not supported yet");
        }

        tmp = builder->CreateFSub(zero, tmp);
    }

    void visit_ComplexUnaryMinus(const ASR::ComplexUnaryMinus_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::Value *c = tmp;
        double re = 0.0;
        double im = 0.0;
        llvm::Value *re2, *im2;
        llvm::Type *type;
        int a_kind = down_cast<ASR::Complex_t>(x.m_type)->m_kind;
        std::string f_name;
        switch (a_kind) {
            case 4: {
                re2 = llvm::ConstantFP::get(context, llvm::APFloat((float)re));
                im2 = llvm::ConstantFP::get(context, llvm::APFloat((float)im));
                type = complex_type_4;
                f_name = "_lfortran_complex_sub_32";
                break;
            }
            case 8: {
                re2 = llvm::ConstantFP::get(context, llvm::APFloat(re));
                im2 = llvm::ConstantFP::get(context, llvm::APFloat(im));
                type = complex_type_8;
                f_name = "_lfortran_complex_sub_64";
                break;
            }
            default: {
                throw CodeGenError("kind type is not supported");
            }
        }
        tmp = complex_from_floats(re2, im2, type);
        llvm::Value *zero_c = tmp;
        tmp = lfortran_complex_bin_op(zero_c, c, f_name, type);
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        int64_t val = x.m_n;
        int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        switch( a_kind ) {

            case 1: {
                tmp = llvm::ConstantInt::get(context, llvm::APInt(8, val, true));
                break ;
            }
            case 2: {
                tmp = llvm::ConstantInt::get(context, llvm::APInt(16, val, true));
                break ;
            }
            case 4 : {
                tmp = llvm::ConstantInt::get(context, llvm::APInt(32, static_cast<int32_t>(val), true));
                break;
            }
            case 8 : {
                tmp = llvm::ConstantInt::get(context, llvm::APInt(64, val, true));
                break;
            }
            default : {
                throw CodeGenError("Constant integers of " + std::to_string(a_kind)
                                    + " bytes aren't supported yet.");
            }

        }
    }

    void visit_RealConstant(const ASR::RealConstant_t &x) {
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

    void visit_ArrayConstant(const ASR::ArrayConstant_t &x) {
        llvm::Type* el_type;
        if (ASR::is_a<ASR::Integer_t>(*x.m_type)) {
            el_type = getIntType(ASR::down_cast<ASR::Integer_t>(x.m_type)->m_kind);
        } else if (ASR::is_a<ASR::Real_t>(*x.m_type)) {
            switch (ASR::down_cast<ASR::Real_t>(x.m_type)->m_kind) {
                case (4) :
                    el_type = llvm::Type::getFloatTy(context); break;
                case (8) :
                    el_type = llvm::Type::getDoubleTy(context); break;
                default :
                    throw CodeGenError("ConstArray real kind not supported yet");
            }
        } else {
            throw CodeGenError("ConstArray type not supported yet");
        }
        // Create <n x float> type, where `n` is the length of the `x` constant array
        llvm::Type* type_fxn = FIXED_VECTOR_TYPE::get(el_type, x.n_args);
        // Create a pointer <n x float>* to a stack allocated <n x float>
        llvm::AllocaInst *p_fxn = builder->CreateAlloca(type_fxn, nullptr);
        // Assign the array elements to `p_fxn`.
        for (size_t i=0; i < x.n_args; i++) {
            llvm::Value *llvm_el = llvm_utils->create_gep(p_fxn, i);
            ASR::expr_t *el = x.m_args[i];
            llvm::Value *llvm_val;
            if (ASR::is_a<ASR::Integer_t>(*x.m_type)) {
                ASR::IntegerConstant_t *ci = ASR::down_cast<ASR::IntegerConstant_t>(el);
                switch (ASR::down_cast<ASR::Integer_t>(x.m_type)->m_kind) {
                    case (4) : {
                        int32_t el_value = ci->m_n;
                        llvm_val = llvm::ConstantInt::get(context, llvm::APInt(32, static_cast<int32_t>(el_value), true));
                        break;
                    }
                    case (8) : {
                        int64_t el_value = ci->m_n;
                        llvm_val = llvm::ConstantInt::get(context, llvm::APInt(32, el_value, true));
                        break;
                    }
                    default :
                        throw CodeGenError("ConstArray integer kind not supported yet");
                }
            } else if (ASR::is_a<ASR::Real_t>(*x.m_type)) {
                ASR::RealConstant_t *cr = ASR::down_cast<ASR::RealConstant_t>(el);
                switch (ASR::down_cast<ASR::Real_t>(x.m_type)->m_kind) {
                    case (4) : {
                        float el_value = cr->m_r;
                        llvm_val = llvm::ConstantFP::get(context, llvm::APFloat(el_value));
                        break;
                    }
                    case (8) : {
                        double el_value = cr->m_r;
                        llvm_val = llvm::ConstantFP::get(context, llvm::APFloat(el_value));
                        break;
                    }
                    default :
                        throw CodeGenError("ConstArray real kind not supported yet");
                }
            } else {
                throw CodeGenError("ConstArray type not supported yet");
            }
            builder->CreateStore(llvm_val, llvm_el);
        }
        // Return the vector as float* type:
        tmp = llvm_utils->create_gep(p_fxn, 0);
    }

    void visit_Assert(const ASR::Assert_t &x) {
        this->visit_expr_wrapper(x.m_test, true);
        create_if_else(tmp, []() {}, [=]() {
            if (x.m_msg) {
                char* s = ASR::down_cast<ASR::StringConstant_t>(x.m_msg)->m_s;
                llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("AssertionError: %s\n");
                llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(s);
                print_error(context, *module, *builder, {fmt_ptr, fmt_ptr2});
            } else {
                llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("AssertionError\n");
                print_error(context, *module, *builder, {fmt_ptr});
            }
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, *module, *builder, exit_code);
        });
    }

    void visit_ComplexConstructor(const ASR::ComplexConstructor_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        throw CodeGenError("ComplexConstructor with runtime arguments not implemented yet.");
    }

    void visit_ComplexConstant(const ASR::ComplexConstant_t &x) {
        double re = x.m_re;
        double im = x.m_im;
        int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        llvm::Value *re2, *im2;
        llvm::Type *type;
        switch( a_kind ) {
            case 4: {
                re2 = llvm::ConstantFP::get(context, llvm::APFloat((float)re));
                im2 = llvm::ConstantFP::get(context, llvm::APFloat((float)im));
                type = complex_type_4;
                break;
            }
            case 8: {
                re2 = llvm::ConstantFP::get(context, llvm::APFloat(re));
                im2 = llvm::ConstantFP::get(context, llvm::APFloat(im));
                type = complex_type_8;
                break;
            }
            default: {
                throw CodeGenError("kind type is not supported");
            }
        }
        tmp = complex_from_floats(re2, im2, type);
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        int val;
        if (x.m_value == true) {
            val = 1;
        } else {
            val = 0;
        }
        tmp = llvm::ConstantInt::get(context, llvm::APInt(1, val));
    }

    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        llvm::Value *arg = tmp;
        tmp = builder->CreateNot(arg);
    }

    void visit_StringConstant(const ASR::StringConstant_t &x) {
        tmp = builder->CreateGlobalStringPtr(x.m_s);
    }

    inline void fetch_ptr(ASR::Variable_t* x) {
        uint32_t x_h = get_hash((ASR::asr_t*)x);
        LCOMPILERS_ASSERT(llvm_symtab.find(x_h) != llvm_symtab.end());
        llvm::Value* x_v = llvm_symtab[x_h];
        int64_t ptr_loads_copy = ptr_loads;
        tmp = x_v;
        while( ptr_loads_copy-- ) {
            tmp = CreateLoad(tmp);
        }
    }

    inline void fetch_val(ASR::Variable_t* x) {
        uint32_t x_h = get_hash((ASR::asr_t*)x);
        llvm::Value* x_v;
        // Check if x is a needed global here, if so, it should exist as an
        // element in the runtime descriptor, get element pointer and create
        // load
        if (llvm_symtab.find(x_h) == llvm_symtab.end()) {
            LCOMPILERS_ASSERT(std::find(nested_globals.begin(),
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
            if (x->m_value_attr) {
                // Already a value, such as value argument to bind(c)
                tmp = x_v;
                return;
            }
        }
        if( arr_descr->is_array(ASRUtils::get_contained_type(x->m_type)) ) {
            tmp = x_v;
        } else {
            tmp = x_v;
            // Load only once since its a value
            if( ptr_loads > 0 ) {
                tmp = CreateLoad(tmp);
            }
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
                    case ASR::ttypeType::Complex:
                    case ASR::ttypeType::Struct: {
                        if( t2->type == ASR::ttypeType::Struct ) {
                            ASR::Struct_t* d = ASR::down_cast<ASR::Struct_t>(t2);
                            der_type_name = ASRUtils::symbol_name(d->m_derived_type);
                        }
                        fetch_ptr(x);
                        break;
                    }
                    case ASR::ttypeType::Character:
                    case ASR::ttypeType::Logical: {
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case ASR::ttypeType::Struct: {
                ASR::Struct_t* der = (ASR::Struct_t*)(&(x->m_type->base));
                ASR::StructType_t* der_type = (ASR::StructType_t*)(&(der->m_derived_type->base));
                der_type_name = std::string(der_type->m_name);
                uint32_t h = get_hash((ASR::asr_t*)x);
                if( llvm_symtab.find(h) != llvm_symtab.end() ) {
                    tmp = llvm_symtab[h];
                }
                break;
            }
            case ASR::ttypeType::Union: {
                ASR::Union_t* der = ASR::down_cast<ASR::Union_t>(x->m_type);
                ASR::UnionType_t* der_type = ASR::down_cast<ASR::UnionType_t>(
                    ASRUtils::symbol_get_past_external(der->m_union_type));
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

    void extract_kinds(const ASR::Cast_t& x,
                       int& arg_kind, int& dest_kind)
    {
        dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
        LCOMPILERS_ASSERT(curr_type != nullptr)
        arg_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
    }

    void visit_ComplexRe(const ASR::ComplexRe_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
        int arg_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
        int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        llvm::Value *re;
        if (arg_kind == 4 && dest_kind == 4) {
            // complex(4) -> real(4)
            re = complex_re(tmp, complex_type_4);
            tmp = re;
        } else if (arg_kind == 4 && dest_kind == 8) {
            // complex(4) -> real(8)
            re = complex_re(tmp, complex_type_4);
            tmp = builder->CreateFPExt(re, llvm::Type::getDoubleTy(context));
        } else if (arg_kind == 8 && dest_kind == 4) {
            // complex(8) -> real(4)
            re = complex_re(tmp, complex_type_8);
            tmp = builder->CreateFPTrunc(re, llvm::Type::getFloatTy(context));
        } else if (arg_kind == 8 && dest_kind == 8) {
            // complex(8) -> real(8)
            re = complex_re(tmp, complex_type_8);
            tmp = re;
        } else {
            std::string msg = "Conversion from " + std::to_string(arg_kind) +
                              " to " + std::to_string(dest_kind) + " not implemented yet.";
            throw CodeGenError(msg);
        }
    }

    void visit_ComplexIm(const ASR::ComplexIm_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
        int arg_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
        llvm::Function *fn = nullptr;
        llvm::Type *ret_type = nullptr, *complex_type = nullptr;
        llvm::AllocaInst *arg = nullptr;
        std::string runtime_func_name = "";
        if (arg_kind == 4) {
            runtime_func_name = "_lfortran_complex_aimag_32";
            ret_type = llvm::Type::getFloatTy(context);
            complex_type = complex_type_4;
            arg = builder->CreateAlloca(complex_type_4,
                nullptr);
        } else {
             runtime_func_name = "_lfortran_complex_aimag_64";
            ret_type = llvm::Type::getDoubleTy(context);
            complex_type = complex_type_8;
            arg = builder->CreateAlloca(complex_type_8,
                nullptr);
        }
        fn = module->getFunction(runtime_func_name);
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {
                        complex_type->getPointerTo(),
                        ret_type->getPointerTo(),
                    }, true);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, *module);
        }
        this->visit_expr_wrapper(x.m_arg, true);
        builder->CreateStore(tmp, arg);
        llvm::AllocaInst *result = builder->CreateAlloca(ret_type, nullptr);
        std::vector<llvm::Value*> args = {arg, result};
        builder->CreateCall(fn, args);
        tmp = CreateLoad(result);
    }

    void visit_Cast(const ASR::Cast_t &x) {
        if (x.m_value) {
            this->visit_expr_wrapper(x.m_value, true);
            return;
        }
        this->visit_expr_wrapper(x.m_arg, true);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                tmp = builder->CreateSIToFP(tmp, getFPType(a_kind, false));
		        break;
            }
            case (ASR::cast_kindType::LogicalToReal) : {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                tmp = builder->CreateUIToFP(tmp, getFPType(a_kind, false));
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
                ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
                LCOMPILERS_ASSERT(curr_type != nullptr)
                int a_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
                switch (a_kind) {
                    case 1:
                        tmp = builder->CreateICmpNE(tmp, builder->getInt8(0));
                        break;
                    case 2:
                        tmp = builder->CreateICmpNE(tmp, builder->getInt16(0));
                        break;
                    case 4:
                        tmp = builder->CreateICmpNE(tmp, builder->getInt32(0));
                        break;
                    case 8:
                        tmp = builder->CreateICmpNE(tmp, builder->getInt64(0));
                        break;
                }
                break;
            }
            case (ASR::cast_kindType::RealToLogical) : {
                llvm::Value *zero;
                ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
                LCOMPILERS_ASSERT(curr_type != nullptr)
                int a_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
                if (a_kind == 4) {
                    zero = llvm::ConstantFP::get(context, llvm::APFloat((float)0.0));
                } else {
                    zero = llvm::ConstantFP::get(context, llvm::APFloat(0.0));
                }
                tmp = builder->CreateFCmpUNE(tmp, zero);
                break;
            }
            case (ASR::cast_kindType::CharacterToLogical) : {
                llvm::AllocaInst *parg = builder->CreateAlloca(character_type, nullptr);
                builder->CreateStore(tmp, parg);
                tmp = builder->CreateICmpNE(lfortran_str_len(parg), builder->getInt32(0));
                break;
            }
            case (ASR::cast_kindType::CharacterToInteger) : {
                llvm::AllocaInst *parg = builder->CreateAlloca(character_type, nullptr);
                builder->CreateStore(tmp, parg);
                tmp = lfortran_str_to_int(parg);
                break;
            }
            case (ASR::cast_kindType::ComplexToLogical) : {
                // !(c.real == 0.0 && c.imag == 0.0)
                llvm::Value *zero;
                ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
                LCOMPILERS_ASSERT(curr_type != nullptr)
                int a_kind = ASRUtils::extract_kind_from_ttype_t(curr_type);
                if (a_kind == 4) {
                    zero = llvm::ConstantFP::get(context, llvm::APFloat((float)0.0));
                } else {
                    zero = llvm::ConstantFP::get(context, llvm::APFloat(0.0));
                }
                llvm::Value *c_real = complex_re(tmp, tmp->getType());
                llvm::Value *real_check = builder->CreateFCmpUEQ(c_real, zero);
                llvm::Value *c_imag = complex_im(tmp, tmp->getType());
                llvm::Value *imag_check = builder->CreateFCmpUEQ(c_imag, zero);
                tmp = builder->CreateAnd(real_check, imag_check);
                tmp = builder->CreateNot(tmp);
                break;
            }
            case (ASR::cast_kindType::LogicalToInteger) : {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                tmp = builder->CreateZExt(tmp, getIntType(a_kind));
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
                    if (dest_kind > arg_kind) {
                        tmp = builder->CreateSExt(tmp, getIntType(dest_kind));
                    } else {
                        tmp = builder->CreateTrunc(tmp, getIntType(dest_kind));
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
            case (ASR::cast_kindType::ComplexToInteger) : {
                int arg_kind = -1, dest_kind = -1;
                extract_kinds(x, arg_kind, dest_kind);
                llvm::Value *re;
                if (arg_kind > 0 && dest_kind > 0)
                {
                    if (arg_kind == 4) {
                        // complex(4) -> real(8)
                        re = complex_re(tmp, complex_type_4);
                        tmp = re;
                    } else if (arg_kind == 8) {
                        // complex(8) -> real(8)
                        re = complex_re(tmp, complex_type_8);
                        tmp = re;
                    } else {
                        std::string msg = "Unsupported Complex type kind: " + std::to_string(arg_kind);
                        throw CodeGenError(msg);
                    }
                    llvm::Type *target_type;
                    target_type = getIntType(dest_kind);
                    tmp = builder->CreateFPToSI(tmp, target_type);
                } else {
                    throw CodeGenError("Negative kinds are not supported.");
                }
                break;
             }
            case (ASR::cast_kindType::RealToCharacter) : {
                llvm::Value *arg = tmp;
                ASR::ttype_t* arg_type = extract_ttype_t_from_expr(x.m_arg);
                LCOMPILERS_ASSERT(arg_type != nullptr)
                int arg_kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
                tmp = lfortran_type_to_str(arg, getFPType(arg_kind), "float", arg_kind);
                break;
            }
            case (ASR::cast_kindType::IntegerToCharacter) : {
                llvm::Value *arg = tmp;
                ASR::ttype_t* arg_type = extract_ttype_t_from_expr(x.m_arg);
                LCOMPILERS_ASSERT(arg_type != nullptr)
                int arg_kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
                tmp = lfortran_type_to_str(arg, getIntType(arg_kind), "int", arg_kind);
                break;
            }
            case (ASR::cast_kindType::LogicalToCharacter) : {
                llvm::Value *cmp = builder->CreateICmpEQ(tmp, builder->getInt1(0));
                llvm::Value *zero_str = builder->CreateGlobalStringPtr("False");
                llvm::Value *one_str = builder->CreateGlobalStringPtr("True");
                tmp = builder->CreateSelect(cmp, zero_str, one_str);
                break;
            }
            default : throw CodeGenError("Cast kind not implemented");
        }
    }

    void visit_FileRead(const ASR::FileRead_t &x) {
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

    void visit_FileWrite(const ASR::FileWrite_t &x) {
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
        llvm::Value *sep = nullptr;
        llvm::Value *end = nullptr;
        if (x.m_separator) {
            this->visit_expr_wrapper(x.m_separator, true);
            sep = tmp;
        } else {
            sep = builder->CreateGlobalStringPtr(" ");
        }
        if (x.m_end) {
            this->visit_expr_wrapper(x.m_end, true);
            end = tmp;
        } else {
            end = builder->CreateGlobalStringPtr("\n");
        }
        for (size_t i=0; i<x.n_values; i++) {
            int64_t ptr_loads_copy = ptr_loads;
            int reduce_loads = 0;
            ptr_loads = 2;
            if( ASR::is_a<ASR::Var_t>(*x.m_values[i]) ) {
                ASR::Variable_t* var = ASRUtils::EXPR2VAR(x.m_values[i]);
                reduce_loads = var->m_intent == ASRUtils::intent_in;
                if( ASR::is_a<ASR::Pointer_t>(*var->m_type) ) {
                    ptr_loads = 1;
                }
            }
            if (i != 0) {
                fmt.push_back("%s");
                args.push_back(sep);
            }
            ptr_loads = ptr_loads - reduce_loads;
            lookup_enum_value_for_nonints = true;
            this->visit_expr_wrapper(x.m_values[i], true);
            lookup_enum_value_for_nonints = false;
            ptr_loads = ptr_loads_copy;
            ASR::expr_t *v = x.m_values[i];
            ASR::ttype_t *t = ASRUtils::expr_type(v);
            if( ASR::is_a<ASR::Const_t>(*t) ) {
                t = ASRUtils::get_contained_type(t);
            }
            int a_kind = ASRUtils::extract_kind_from_ttype_t(t);
            if( ASR::is_a<ASR::Pointer_t>(*t) && ASR::is_a<ASR::Var_t>(*v) ) {
                if( ASRUtils::is_array(ASRUtils::type_get_past_pointer(t)) ) {
                    tmp = CreateLoad(arr_descr->get_pointer_to_data(tmp));
                }
                fmt.push_back("%lld");
                llvm::Value* d = builder->CreatePtrToInt(tmp, getIntType(8, false));
                args.push_back(d);
                continue;
            }
            if (t->type == ASR::ttypeType::CPtr ||
                (t->type == ASR::ttypeType::Pointer &&
                (ASR::is_a<ASR::Var_t>(*v) || ASR::is_a<ASR::GetPointer_t>(*v)))
               ) {
                fmt.push_back("%lld");
                llvm::Value* d = builder->CreatePtrToInt(tmp, getIntType(8, false));
                args.push_back(d);
            } else if (ASRUtils::is_integer(*t)) {
                switch( a_kind ) {
                    case 1 : {
                        fmt.push_back("%hhi");
                        break;
                    }
                    case 2 : {
                        fmt.push_back("%hi");
                        break;
                    }
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
                                            for 8, 16, 32, and 64 bit integer kinds.)""",
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
                        fmt.push_back("%13.8e");
                        d = builder->CreateFPExt(tmp,
                        llvm::Type::getDoubleTy(context));
                        break;
                    }
                    case 8 : {
                        fmt.push_back("%23.17e");
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
            } else if (ASRUtils::is_logical(*t)) {
                llvm::Value *cmp = builder->CreateICmpEQ(tmp, builder->getInt1(0));
                llvm::Value *zero_str = builder->CreateGlobalStringPtr("False");
                llvm::Value *one_str = builder->CreateGlobalStringPtr("True");
                llvm::Value *str = builder->CreateSelect(cmp, zero_str, one_str);
                fmt.push_back("%s");
                args.push_back(str);
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
            } else if (t->type == ASR::ttypeType::CPtr) {
                fmt.push_back("%lld");
                llvm::Value* d = builder->CreatePtrToInt(tmp, getIntType(8, false));
                args.push_back(d);
            } else if (t->type == ASR::ttypeType::Enum) {
                // TODO: Use recursion to generalise for any underlying type in enum
                fmt.push_back("%d");
                args.push_back(tmp);
            } else {
                throw LCompilersException("Printing support is not available for " +
                    ASRUtils::type_to_str(t) + " type.");
            }
        }
        fmt.push_back("%s");
        args.push_back(end);
        std::string fmt_str;
        for (size_t i=0; i<fmt.size(); i++) {
            fmt_str += fmt[i];
        }
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr(fmt_str);
        std::vector<llvm::Value *> printf_args;
        printf_args.push_back(fmt_ptr);
        printf_args.insert(printf_args.end(), args.begin(), args.end());
        printf(context, *module, *builder, printf_args);
    }

    void visit_Stop(const ASR::Stop_t &x) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("STOP\n");
        print_error(context, *module, *builder, {fmt_ptr});
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
        print_error(context, *module, *builder, {fmt_ptr});
        int exit_code_int = 1;
        llvm::Value *exit_code = llvm::ConstantInt::get(context,
                llvm::APInt(32, exit_code_int));
        exit(context, *module, *builder, exit_code);
    }

    template <typename T>
    inline void set_func_subrout_params(T* func_subrout, ASR::abiType& x_abi,
                                        std::uint32_t& m_h, ASR::Variable_t*& orig_arg,
                                        std::string& orig_arg_name, ASR::intentType& arg_intent,
                                        size_t arg_idx) {
        m_h = get_hash((ASR::asr_t*)func_subrout);
        if( ASR::is_a<ASR::Var_t>(*func_subrout->m_args[arg_idx]) ) {
            ASR::Var_t* arg_var = ASR::down_cast<ASR::Var_t>(func_subrout->m_args[arg_idx]);
            ASR::symbol_t* arg_sym = symbol_get_past_external(arg_var->m_v);
            if( ASR::is_a<ASR::Variable_t>(*arg_sym) ) {
                orig_arg = ASR::down_cast<ASR::Variable_t>(arg_sym);
                orig_arg_name = orig_arg->m_name;
                arg_intent = orig_arg->m_intent;
            }
        }
        x_abi = func_subrout->m_abi;
    }


    template <typename T>
    std::vector<llvm::Value*> convert_call_args(const T &x) {
        std::vector<llvm::Value *> args;
        const ASR::symbol_t* func_subrout = symbol_get_past_external(x.m_name);
        ASR::abiType x_abi = ASR::abiType::Source;
        if( is_a<ASR::Function_t>(*func_subrout) ) {
            ASR::Function_t* func = down_cast<ASR::Function_t>(func_subrout);
            x_abi = func->m_abi;
        }

        for (size_t i=0; i<x.n_args; i++) {
            func_subrout = symbol_get_past_external(x.m_name);
            x_abi = (ASR::abiType) 0;
            ASR::intentType orig_arg_intent = ASR::intentType::Unspecified;
            std::uint32_t m_h;
            ASR::Variable_t *orig_arg = nullptr;
            std::string orig_arg_name = "";
            if( func_subrout->type == ASR::symbolType::Function ) {
                ASR::Function_t* func = down_cast<ASR::Function_t>(func_subrout);
                set_func_subrout_params(func, x_abi, m_h, orig_arg, orig_arg_name, orig_arg_intent, i);
            } else if( func_subrout->type == ASR::symbolType::ClassProcedure ) {
                ASR::ClassProcedure_t* clss_proc = ASR::down_cast<ASR::ClassProcedure_t>(func_subrout);
                if( clss_proc->m_proc->type == ASR::symbolType::Function ) {
                    ASR::Function_t* func = down_cast<ASR::Function_t>(clss_proc->m_proc);
                    set_func_subrout_params(func, x_abi, m_h, orig_arg, orig_arg_name, orig_arg_intent, i);
                }
            } else {
                LCOMPILERS_ASSERT(false)
            }
            if (x.m_args[i].m_value->type == ASR::exprType::Var) {
                if (is_a<ASR::Variable_t>(*symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(x.m_args[i].m_value)->m_v))) {
                    ASR::Variable_t *arg = EXPR2VAR(x.m_args[i].m_value);
                    uint32_t h = get_hash((ASR::asr_t*)arg);
                    if (llvm_symtab.find(h) != llvm_symtab.end()) {
                        tmp = llvm_symtab[h];
                        bool is_data_only_array = false;
                        ASR::dimension_t* dims_arg = nullptr;
                        size_t n_arg = ASRUtils::extract_dimensions_from_ttype(arg->m_type, dims_arg);
                        if( ASRUtils::is_arg_dummy(arg->m_intent) &&
                            !ASRUtils::is_dimension_empty(dims_arg, n_arg) ) {
                            is_data_only_array = true;
                        }
                        if( x_abi == ASR::abiType::Source &&
                            arr_descr->is_array(arg->m_type) &&
                            !is_data_only_array ) {
                            llvm::Type* new_arr_type = arr_arg_type_cache[m_h][orig_arg_name];
                            ASR::dimension_t* dims;
                            size_t n;
                            n = ASRUtils::extract_dimensions_from_ttype(orig_arg->m_type, dims);
                            tmp = arr_descr->convert_to_argument(tmp, arg->m_type, new_arr_type,
                                                                (!ASRUtils::is_dimension_empty(dims, n)));
                        } else if (x_abi == ASR::abiType::Source && ASR::is_a<ASR::CPtr_t>(*arg->m_type)) {
                                if (arg->m_intent == intent_local) {
                                    // Local variable of type
                                    // CPtr is a void**, so we
                                    // have to load it
                                    tmp = CreateLoad(tmp);
                                }
                        } else if ( x_abi == ASR::abiType::BindC ) {
                            if( arr_descr->is_array(ASRUtils::get_contained_type(arg->m_type)) ) {
                                // TODO: we need a dedicated and robust
                                // function that determines from ASR only
                                // if a given array is represented by
                                // a descriptor or with just a pointer.
                                // Until then we use the following heuristic:
                                bool arg_is_using_descriptor = true;
                                if (LLVMArrUtils::is_explicit_shape(arg)) {
                                    if (arg->m_intent != intent_local) {
                                        arg_is_using_descriptor = false;
                                    }
                                }
                                if (arg_is_using_descriptor) {
                                    tmp = CreateLoad(arr_descr->get_pointer_to_data(tmp));
                                }
                            } else {
                                if (orig_arg->m_abi == ASR::abiType::BindC
                                    && orig_arg->m_value_attr) {
                                        ASR::ttype_t* arg_type = arg->m_type;
                                        if( ASR::is_a<ASR::Const_t>(*arg_type) ) {
                                            arg_type = ASR::down_cast<ASR::Const_t>(arg_type)->m_type;
                                        }
                                        if (is_a<ASR::Complex_t>(*arg_type)) {
                                            int c_kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
                                            if (c_kind == 4) {
                                                if (compiler_options.platform == Platform::Windows) {
                                                    // tmp is {float, float}*
                                                    // type_fx2p is i64*
                                                    llvm::Type* type_fx2p = llvm::Type::getInt64PtrTy(context);
                                                    // Convert {float,float}* to i64* using bitcast
                                                    tmp = builder->CreateBitCast(tmp, type_fx2p);
                                                    // Then convert i64* -> i64
                                                    tmp = CreateLoad(tmp);
                                                } else if (compiler_options.platform == Platform::macOS_ARM) {
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
                                                    llvm::Type* type_fx2p = FIXED_VECTOR_TYPE::get(llvm::Type::getFloatTy(context), 2)->getPointerTo();
                                                    // Convert {float,float}* to <2 x float>* using bitcast
                                                    tmp = builder->CreateBitCast(tmp, type_fx2p);
                                                    // Then convert <2 x float>* -> <2 x float>
                                                    tmp = CreateLoad(tmp);
                                                }
                                            } else {
                                                LCOMPILERS_ASSERT(c_kind == 8)
                                                if (compiler_options.platform == Platform::Windows) {
                                                    // 128 bit aggregate type is passed by reference
                                                } else {
                                                    // Pass by value
                                                    tmp = CreateLoad(tmp);
                                                }
                                            }
                                        } else if (is_a<ASR::CPtr_t>(*arg_type)) {
                                            if (arg->m_intent == intent_local) {
                                                // Local variable of type
                                                // CPtr is a void**, so we
                                                // have to load it
                                                tmp = CreateLoad(tmp);
                                            }
                                        } else {
                                            if (!arg->m_value_attr) {
                                                // Dereference the pointer argument (unless it is a CPtr)
                                                // to pass by value
                                                // E.g.:
                                                // i32* -> i32
                                                // {double,double}* -> {double,double}
                                                tmp = CreateLoad(tmp);
                                            }
                                        }
                                    }
                                if (!orig_arg->m_value_attr && arg->m_value_attr) {
                                    llvm::Type *target_type = tmp->getType();
                                    // Create alloca to get a pointer, but do it
                                    // at the beginning of the function to avoid
                                    // using alloca inside a loop, which would
                                    // run out of stack
                                    llvm::BasicBlock &entry_block = builder->GetInsertBlock()->getParent()->getEntryBlock();
                                    llvm::IRBuilder<> builder0(context);
                                    builder0.SetInsertPoint(&entry_block, entry_block.getFirstInsertionPt());
                                    llvm::AllocaInst *target = builder0.CreateAlloca(
                                        target_type, nullptr, "call_arg_value_ptr");
                                    builder->CreateStore(tmp, target);
                                    tmp = target;
                                }
                            }
                        }
                    } else {
                        auto finder = std::find(nested_globals.begin(),
                                nested_globals.end(), h);
                        if (finder == nested_globals.end()) {
                            if (arg->m_value == nullptr) {
                                throw CodeGenError(std::string(arg->m_name) + " isn't defined in any scope.");
                            }
                            this->visit_expr_wrapper(arg->m_value, true);
                            if( x_abi != ASR::abiType::BindC ) {
                                llvm::BasicBlock &entry_block = builder->GetInsertBlock()->getParent()->getEntryBlock();
                                llvm::IRBuilder<> builder0(context);
                                builder0.SetInsertPoint(&entry_block, entry_block.getFirstInsertionPt());
                                llvm::AllocaInst *target = builder0.CreateAlloca(
                                    get_type_from_ttype_t_util(arg->m_type), nullptr, "call_arg_value");
                                builder->CreateStore(tmp, target);
                                tmp = target;
                            }
                        } else {
                            llvm::Value* ptr = module->getOrInsertGlobal(nested_desc_name,
                                nested_global_struct);
                            int idx = std::distance(nested_globals.begin(), finder);
                            tmp = CreateLoad(llvm_utils->create_gep(ptr, idx));
                        }
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
                }
            } else {
                ASR::ttype_t* arg_type = expr_type(x.m_args[i].m_value);
                int64_t ptr_loads_copy = ptr_loads;
                ptr_loads = !LLVM::is_llvm_struct(arg_type);
                this->visit_expr_wrapper(x.m_args[i].m_value);
                if( x_abi == ASR::abiType::BindC ) {
                    if( (ASR::is_a<ASR::ArrayItem_t>(*x.m_args[i].m_value) &&
                         orig_arg_intent ==  ASR::intentType::In) ||
                        ASR::is_a<ASR::StructInstanceMember_t>(*x.m_args[i].m_value) ||
                        (ASR::is_a<ASR::CPtr_t>(*arg_type) &&
                         ASR::is_a<ASR::StructInstanceMember_t>(*x.m_args[i].m_value)) ) {
                        if( ASR::is_a<ASR::StructInstanceMember_t>(*x.m_args[i].m_value) &&
                            ASRUtils::is_array(arg_type) ) {
                            ASR::dimension_t* arg_m_dims = nullptr;
                            size_t n_dims = ASRUtils::extract_dimensions_from_ttype(arg_type, arg_m_dims);
                            if( !(ASRUtils::is_fixed_size_array(arg_m_dims, n_dims) &&
                                  ASRUtils::expr_abi(x.m_args[i].m_value) == ASR::abiType::BindC) ) {
                                tmp = LLVM::CreateLoad(*builder, arr_descr->get_pointer_to_data(tmp));
                            } else {
                                tmp = llvm_utils->create_gep(tmp, llvm::ConstantInt::get(
                                        llvm::Type::getInt32Ty(context), llvm::APInt(32, 0)));
                            }
                        } else {
                            tmp = LLVM::CreateLoad(*builder, tmp);
                        }
                    }
                }
                llvm::Value *value = tmp;
                ptr_loads = ptr_loads_copy;
                llvm::Type *target_type;
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
                        } else {
                            LCOMPILERS_ASSERT(false)
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
                    case (ASR::ttypeType::Enum) :
                        target_type = llvm::Type::getInt32Ty(context);
                        break;
                    case (ASR::ttypeType::Struct) :
                        break;
                    case (ASR::ttypeType::CPtr) :
                        target_type = llvm::Type::getVoidTy(context)->getPointerTo();
                        break;
                    case (ASR::ttypeType::Pointer) : {
                        target_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(arg_type));
                        target_type = target_type->getPointerTo();
                        break;
                    }
                    case (ASR::ttypeType::List) : {
                        target_type = get_type_from_ttype_t_util(arg_type);
                        break ;
                    }
                    case (ASR::ttypeType::Tuple) : {
                        target_type = get_type_from_ttype_t_util(arg_type);
                        break ;
                    }
                    default :
                        throw CodeGenError("Type " + ASRUtils::type_to_str(arg_type) + " not implemented yet.");
                }
                if( ASR::is_a<ASR::EnumValue_t>(*x.m_args[i].m_value) ) {
                    target_type = llvm::Type::getInt32Ty(context);
                }
                switch(arg_type->type) {
                    case ASR::ttypeType::Struct: {
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
                            } else {
                                LCOMPILERS_ASSERT(false)
                            }
                            if (orig_arg->m_abi == ASR::abiType::BindC
                                && orig_arg->m_value_attr) {
                                use_value = true;
                            }
                            if (ASR::is_a<ASR::ArrayItem_t>(*x.m_args[i].m_value)) {
                                use_value = true;
                            }
                            if (!use_value) {
                                // Create alloca to get a pointer, but do it
                                // at the beginning of the function to avoid
                                // using alloca inside a loop, which would
                                // run out of stack
                                if( (ASR::is_a<ASR::ArrayItem_t>(*x.m_args[i].m_value) ||
                                    ASR::is_a<ASR::StructInstanceMember_t>(*x.m_args[i].m_value))
                                        && value->getType()->isPointerTy()) {
                                    value = CreateLoad(value);
                                }
                                if( !ASR::is_a<ASR::CPtr_t>(*arg_type) ) {
                                    llvm::BasicBlock &entry_block = builder->GetInsertBlock()->getParent()->getEntryBlock();
                                    llvm::IRBuilder<> builder0(context);
                                    builder0.SetInsertPoint(&entry_block, entry_block.getFirstInsertionPt());
                                    llvm::AllocaInst *target = builder0.CreateAlloca(
                                        target_type, nullptr, "call_arg_value");
                                    if( ASR::is_a<ASR::Tuple_t>(*arg_type) ||
                                        ASR::is_a<ASR::List_t>(*arg_type) ) {
                                        llvm_utils->deepcopy(value, target, arg_type, module.get(), name2memidx);
                                    } else {
                                        builder->CreateStore(value, target);
                                    }
                                    tmp = target;
                                } else {
                                    tmp = value;
                                }
                            }
                        }
                    }
                }
            }
            args.push_back(tmp);
        }
        return args;
    }

    void generate_flip_sign(ASR::call_arg_t* m_args) {
        this->visit_expr_wrapper(m_args[0].m_value, true);
        llvm::Value* signal = tmp;
        LCOMPILERS_ASSERT(m_args[1].m_value->type == ASR::exprType::Var);
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
        llvm::Type* variable_type = get_type_from_ttype_t_util(asr_variable->m_type);
        builder->CreateStore(builder->CreateBitCast(tmp, variable_type->getPointerTo()), variable);
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
        if (compiler_options.emit_debug_info) debug_emit_loc(x);
        if( ASRUtils::is_intrinsic_optimization(x.m_name) ) {
            ASR::Function_t* routine = ASR::down_cast<ASR::Function_t>(
                        ASRUtils::symbol_get_past_external(x.m_name));
            if( generate_optimization_instructions(routine, x.m_args) ) {
                return ;
            }
        }
        ASR::Function_t *s;
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
        if (parent_function){
            push_nested_stack(parent_function);
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
            std::string m_name = ASR::down_cast<ASR::Function_t>(x.m_name)->m_name;
            args = convert_call_args(x);
            tmp = builder->CreateCall(fntype, fn, args);
        } else if (llvm_symtab_fn.find(h) == llvm_symtab_fn.end()) {
            throw CodeGenError("Subroutine code not generated for '"
                + std::string(s->m_name) + "'");
        } else {
            llvm::Function *fn = llvm_symtab_fn[h];
            std::string m_name = ASRUtils::symbol_name(x.m_name);
            std::vector<llvm::Value *> args2 = convert_call_args(x);
            args.insert(args.end(), args2.begin(), args2.end());
            builder->CreateCall(fn, args);
        }
        calling_function_hash = h;
        pop_nested_stack(s);
    }

    void handle_bitwise_args(const ASR::FunctionCall_t& x, llvm::Value*& arg1,
                             llvm::Value*& arg2) {
        LCOMPILERS_ASSERT(x.n_args == 2);
        tmp = nullptr;
        this->visit_expr_wrapper(x.m_args[0].m_value, true);
        arg1 = tmp;
        tmp = nullptr;
        this->visit_expr_wrapper(x.m_args[1].m_value, true);
        arg2 = tmp;
    }

    void handle_bitwise_xor(const ASR::FunctionCall_t& x) {
        llvm::Value *arg1 = nullptr, *arg2 = nullptr;
        handle_bitwise_args(x, arg1, arg2);
        tmp = builder->CreateXor(arg1, arg2);
    }

    void handle_bitwise_and(const ASR::FunctionCall_t& x) {
        llvm::Value *arg1 = nullptr, *arg2 = nullptr;
        handle_bitwise_args(x, arg1, arg2);
        tmp = builder->CreateAnd(arg1, arg2);
    }

    void handle_bitwise_or(const ASR::FunctionCall_t& x) {
        llvm::Value *arg1 = nullptr, *arg2 = nullptr;
        handle_bitwise_args(x, arg1, arg2);
        tmp = builder->CreateOr(arg1, arg2);
    }

    llvm::Value* CreatePointerToStructReturnValue(llvm::FunctionType* fnty,
                                                  llvm::Value* return_value,
                                                  ASR::ttype_t* asr_return_type) {
        if( !LLVM::is_llvm_struct(asr_return_type) ) {
            return return_value;
        }

        // Call to LLVM APIs not needed to fetch the return type of the function.
        // We can use asr_return_type as well but anyways for compactness I did it here.
        llvm::Value* pointer_to_struct = builder->CreateAlloca(fnty->getReturnType(), nullptr);
        LLVM::CreateStore(*builder, return_value, pointer_to_struct);
        return pointer_to_struct;
    }

    llvm::Value* CreateCallUtil(llvm::FunctionType* fnty, llvm::Function* fn,
                                std::vector<llvm::Value*>& args,
                                ASR::ttype_t* asr_return_type) {
        llvm::Value* return_value = builder->CreateCall(fn, args);
        return CreatePointerToStructReturnValue(fnty, return_value,
                                                asr_return_type);
    }

    llvm::Value* CreateCallUtil(llvm::Function* fn, std::vector<llvm::Value*>& args,
                                ASR::ttype_t* asr_return_type) {
        return CreateCallUtil(fn->getFunctionType(), fn, args, asr_return_type);
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
        if( ASRUtils::is_intrinsic_function2(s) ) {
            std::string symbol_name = ASRUtils::symbol_name(x.m_name);
            if( startswith(symbol_name, "_bitwise_xor") ) {
                handle_bitwise_xor(x);
                return ;
            }
            if( startswith(symbol_name, "_bitwise_and") ) {
                handle_bitwise_and(x);
                return ;
            }
            if( startswith(symbol_name, "_bitwise_or") ) {
                handle_bitwise_or(x);
                return ;
            }
        }
        if (parent_function){
            push_nested_stack(parent_function);
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
                    args = convert_call_args(x);
                    LCOMPILERS_ASSERT(args.size() == 1)
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
            args = convert_call_args(x);
            tmp = builder->CreateCall(fntype, fn, args);
        } else if (llvm_symtab_fn.find(h) == llvm_symtab_fn.end()) {
            throw CodeGenError("Function code not generated for '"
                + std::string(s->m_name) + "'");
        } else {
            llvm::Function *fn = llvm_symtab_fn[h];
            std::string m_name = std::string(((ASR::Function_t*)(&(x.m_name->base)))->m_name);
            std::vector<llvm::Value *> args2 = convert_call_args(x);
            args.insert(args.end(), args2.begin(), args2.end());
            ASR::ttype_t *return_var_type0 = EXPR2VAR(s->m_return_var)->m_type;
            if (s->m_abi == ASR::abiType::BindC) {
                if (is_a<ASR::Complex_t>(*return_var_type0)) {
                    int a_kind = down_cast<ASR::Complex_t>(return_var_type0)->m_kind;
                    if (a_kind == 8) {
                        if (compiler_options.platform == Platform::Windows) {
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
                tmp = CreateCallUtil(fn, args, return_var_type0);
            }
        }
        if (s->m_abi == ASR::abiType::BindC) {
            ASR::ttype_t *return_var_type0 = EXPR2VAR(s->m_return_var)->m_type;
            if (is_a<ASR::Complex_t>(*return_var_type0)) {
                int a_kind = down_cast<ASR::Complex_t>(return_var_type0)->m_kind;
                if (a_kind == 4) {
                    if (compiler_options.platform == Platform::Windows) {
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
                    } else if (compiler_options.platform == Platform::macOS_ARM) {
                        // pass
                    } else {
                        // tmp is <2 x float>, have to convert to {float, float}

                        // <2 x float>
                        llvm::Type* type_fx2 = FIXED_VECTOR_TYPE::get(llvm::Type::getFloatTy(context), 2);
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

    void visit_ArraySize(const ASR::ArraySize_t& x) {
        if( x.m_value ) {
            visit_expr_wrapper(x.m_value, true);
            return ;
        }
        int output_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        int dim_kind = 4;
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 2 - // Sync: instead of 2 - , should this be ptr_loads_copy -
                    (ASRUtils::expr_type(x.m_v)->type ==
                     ASR::ttypeType::Pointer);
        visit_expr_wrapper(x.m_v);
        ptr_loads = ptr_loads_copy;
        llvm::Value* llvm_arg = tmp;
        llvm::Value* llvm_dim = nullptr;
        if( x.m_dim ) {
            visit_expr_wrapper(x.m_dim, true);
            dim_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x.m_dim));
            llvm_dim = tmp;
        }
        tmp = arr_descr->get_array_size(llvm_arg, llvm_dim, output_kind, dim_kind);
    }

    void visit_ArrayBound(const ASR::ArrayBound_t& x) {
        ASR::expr_t* array_value = ASRUtils::expr_value(x.m_v);
        if( array_value && ASR::is_a<ASR::ArrayConstant_t>(*array_value) ) {
            ASR::ArrayConstant_t* array_const = ASR::down_cast<ASR::ArrayConstant_t>(array_value);
            int kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
            size_t bound_value = 0;
            if( x.m_bound == ASR::arrayboundType::LBound ) {
                bound_value = 1;
            } else if( x.m_bound == ASR::arrayboundType::UBound ) {
                bound_value = array_const->n_args;
            } else {
                LCOMPILERS_ASSERT(false);
            }
            tmp = llvm::ConstantInt::get(context, llvm::APInt(kind * 8, bound_value));
            return ;
        }
        int64_t ptr_loads_copy = ptr_loads;
        ptr_loads = 2 - // Sync: instead of 2 - , should this be ptr_loads_copy -
                    (ASRUtils::expr_type(x.m_v)->type ==
                     ASR::ttypeType::Pointer);
        visit_expr_wrapper(x.m_v);
        ptr_loads = ptr_loads_copy;
        llvm::Value* llvm_arg1 = tmp;
        llvm::Value* dim_des_val = arr_descr->get_pointer_to_dimension_descriptor_array(llvm_arg1);
        visit_expr_wrapper(x.m_dim, true);
        llvm::Value* dim_val = tmp;
        llvm::Value* const_1 = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
        dim_val = builder->CreateSub(dim_val, const_1);
        llvm::Value* dim_struct = arr_descr->get_pointer_to_dimension_descriptor(dim_des_val, dim_val);
        llvm::Value* res = nullptr;
        if( x.m_bound == ASR::arrayboundType::LBound ) {
            res = arr_descr->get_lower_bound(dim_struct);
        } else if( x.m_bound == ASR::arrayboundType::UBound ) {
            res = arr_descr->get_upper_bound(dim_struct);
        }
        tmp = res;
    }

};



Result<std::unique_ptr<LLVMModule>> asr_to_llvm(ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics,
        llvm::LLVMContext &context, Allocator &al,
        LCompilers::PassManager& pass_manager,
        CompilerOptions &co, const std::string &run_fn,
        const std::string &infile)
{
#if LLVM_VERSION_MAJOR >= 15
    context.setOpaquePointers(false);
#endif
    ASRToLLVMVisitor v(al, context, infile, co, diagnostics);
    LCompilers::PassOptions pass_options;
    pass_options.runtime_library_dir = co.runtime_library_dir;
    pass_options.mod_files_dir = co.mod_files_dir;
    pass_options.include_dirs = co.include_dirs;
    pass_options.run_fun = run_fn;
    pass_options.always_run = false;
    pass_manager.rtlib = co.rtlib;
    pass_manager.apply_passes(al, &asr, pass_options, diagnostics);

    // Uncomment for debugging the ASR after the transformation
    // std::cout << pickle(asr, true, true, true) << std::endl;

    v.nested_func_types = pass_find_nested_vars(asr, context,
        v.nested_globals, v.nested_call_out, v.nesting_map);
    try {
        v.visit_asr((ASR::asr_t&)asr);
    } catch (const CodeGenError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    } catch (const CodeGenAbort &) {
        LCOMPILERS_ASSERT(diagnostics.has_error())
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
        msg = "asr_to_llvm: module failed verification. Error:\n" + err.str();
        diagnostics.diagnostics.push_back(diag::Diagnostic(msg,
            diag::Level::Error, diag::Stage::CodeGen));
        Error error;
        return error;
    };
    return std::make_unique<LLVMModule>(std::move(v.module));
}

} // namespace LFortran
