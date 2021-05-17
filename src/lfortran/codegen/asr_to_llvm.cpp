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

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/pass/do_loops.h>
#include <lfortran/pass/select_case.h>
#include <lfortran/pass/global_stmts.h>
#include <lfortran/pass/param_to_const.h>
#include <lfortran/pass/nested_vars.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/pickle.h>


namespace LFortran {

using ASR::is_a;
using ASR::down_cast;
using ASR::down_cast2;

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

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
private:
  //!< A map from sin, cos, etc. to the corresponding functions
  std::unordered_map<std::string, llvm::Function *> all_intrinsics;

  //! To be used by visit_DerivedRef.
  std::string der_type_name;

  //! Helpful for debugging while testing LLVM code
  void print_util(llvm::Value* v, std::string fmt_chars) {
        std::vector<llvm::Value *> args;
        std::vector<std::string> fmt;
        args.push_back(v);
        fmt.push_back(fmt_chars);
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

public:
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::Value *tmp;
    llvm::BasicBlock *current_loophead, *current_loopend;
    std::string mangle_prefix;
    bool prototype_only;
    llvm::StructType *complex_type_4, *complex_type_8;
    llvm::StructType *complex_type_4_ptr, *complex_type_8_ptr;
    llvm::PointerType *character_type;
    
    // Data Members for handling arrays
    llvm::StructType* dim_des;
    std::map<int, llvm::ArrayType*> rank2desc;
    std::map<std::pair<std::pair<int, int>, std::pair<int, int>>, llvm::StructType*> tkr2array;

    // Maps for containing information regarding derived types
    std::map<std::string, llvm::StructType*> name2dertype;
    std::map<std::string, std::map<std::string, int>> name2memidx;

    std::map<uint64_t, llvm::Value*> llvm_symtab; // llvm_symtab_value
    std::map<uint64_t, llvm::Function*> llvm_symtab_fn;
    std::map<uint64_t, llvm::Value*> llvm_symtab_fn_arg;
    // Data members for handling nested functions
    std::vector<uint64_t> needed_globals; /* For saving the hash of variables 
        from a parent scope needed in a nested function */
    std::map<uint64_t, std::vector<llvm::Type*>> nested_func_types; /* For 
        saving the hash of a parent function needing to give access to 
        variables in a nested function, as well as the variable types */
    llvm::StructType* needed_global_struct; /*The struct type that will hold 
        variables needed in a nested function; will contain types as given in 
        the runtime descriptor member */
    std::string desc_name; // For setting the name of the global struct

    // Data members for callback functions/procedures as arguments
    std::map<std::string, uint64_t> interface_procs; /* Links a procedure
         implementation to it's string name for adding to the BB */


    ASRToLLVMVisitor(llvm::LLVMContext &context) : context(context),
        prototype_only(false), dim_des(llvm::StructType::create(
            context, 
            std::vector<llvm::Type*>(
                {llvm::Type::getInt32Ty(context), 
                 llvm::Type::getInt32Ty(context), 
                 llvm::Type::getInt32Ty(context),
                 llvm::Type::getInt32Ty(context)}), 
                 "dimension_descriptor")
        )
        {}

    inline llvm::ArrayType* get_dim_des_array(int rank) {
        if( rank2desc.find(rank) != rank2desc.end() ) {
            return rank2desc[rank];
        } 
        rank2desc[rank] = llvm::ArrayType::get(dim_des, rank);
        return rank2desc[rank];
    }

    inline llvm::StructType* get_array_type
    (ASR::ttypeType type_, int a_kind, int rank, ASR::dimension_t* m_dims) {
        int size = 0;
        if( verify_dimensions_t(m_dims, rank) ) {
            size = 1;
            for( int r = 0; r < rank; r++ ) {
                ASR::dimension_t m_dim = m_dims[r];
                int start = ((ASR::ConstantInteger_t*)(m_dim.m_start))->m_n;
                int end = ((ASR::ConstantInteger_t*)(m_dim.m_end))->m_n;
                size *= (end - start + 1);
            }
        }
        std::pair<std::pair<int, int>, std::pair<int, int>> array_key = std::make_pair(std::make_pair((int)type_, a_kind), std::make_pair(rank, size));
        if( tkr2array.find(array_key) != tkr2array.end() ) {
            return tkr2array[array_key];
        }
        llvm::ArrayType* dim_des_array = get_dim_des_array(rank);
        llvm::Type* el_type = nullptr;
        switch(type_) {
            case ASR::ttypeType::Integer: {
                switch(a_kind) {
                    case 4:
                        el_type = llvm::Type::getInt32Ty(context);
                        break;
                    case 8:
                        el_type = llvm::Type::getInt64Ty(context);
                        break;
                    default:
                        throw CodeGenError("Only 32 and 64 bits integer kinds are supported.");
                }
                break;
            }
            case ASR::ttypeType::Real: {
                switch(a_kind) {
                    case 4:
                        el_type = llvm::Type::getFloatPtrTy(context);
                        break;
                    case 8:
                        el_type = llvm::Type::getDoublePtrTy(context);
                        break;
                    default:
                        throw CodeGenError("Only 32 and 64 bits real kinds are supported.");
                }
                break;
            }
            default:
                break;
        }
        std::vector<llvm::Type*> array_type_vec = {
            llvm::ArrayType::get(el_type, size), 
            llvm::Type::getInt32Ty(context),
            dim_des_array};
        tkr2array[array_key] = llvm::StructType::create(context, array_type_vec, "array");
        return tkr2array[array_key];
    }

    inline llvm::Value* create_gep(llvm::Value* ds, int idx) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
        return builder->CreateGEP(ds, idx_vec);
    }

    inline llvm::Value* create_gep(llvm::Value* ds, llvm::Value* idx) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
        idx};
        return builder->CreateGEP(ds, idx_vec);
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

    inline void fill_array_details(llvm::Value* arr, ASR::dimension_t* m_dims, 
                                   int n_dims) {
        if( verify_dimensions_t(m_dims, n_dims) ) {
            llvm::Value* offset_val = create_gep(arr, 1);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), offset_val);
            llvm::Value* dim_des_val = create_gep(arr, 2);
            for( int r = 0; r < n_dims; r++ ) {
                ASR::dimension_t m_dim = m_dims[r];
                llvm::Value* dim_val = create_gep(dim_des_val, r);
                llvm::Value* s_val = create_gep(dim_val, 0);
                llvm::Value* l_val = create_gep(dim_val, 1);
                llvm::Value* u_val = create_gep(dim_val, 2);
                llvm::Value* dim_size_ptr = create_gep(dim_val, 3);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 1)), s_val);
                visit_expr(*(m_dim.m_start));
                builder->CreateStore(tmp, l_val);
                visit_expr(*(m_dim.m_end));
                builder->CreateStore(tmp, u_val);
                u_val = builder->CreateLoad(u_val);
                l_val = builder->CreateLoad(l_val);
                llvm::Value* dim_size = builder->CreateAdd(builder->CreateSub(u_val, l_val), 
                                                           llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                builder->CreateStore(dim_size, dim_size_ptr);
            }
        }
    }

    inline llvm::Type* getIntType(int a_kind, bool get_pointer=false) {
        llvm::Type* type_ptr = nullptr;
        if( get_pointer ) {
            switch(a_kind)
            {
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
        std::string der_type_name = std::string(der_type->m_name);
        llvm::StructType* der_type_llvm;
        if( name2dertype.find(der_type_name) != name2dertype.end() ) {
            der_type_llvm = name2dertype[der_type_name];
        } else {
            std::map<std::string, ASR::symbol_t*> scope = der_type->m_symtab->scope;
            std::vector<llvm::Type*> member_types;
            int member_idx = 0;
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
                    default:
                        throw SemanticError("Cannot identify the type of member, '" + 
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
        return builder->CreateLoad(presult);
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
        return builder->CreateLoad(presult);
    }


    // This function is called as:
    // float complex_re(complex a)
    // And it extracts the real part of the complex number
    llvm::Value *complex_re(llvm::Value *c, llvm::Type* complex_type=nullptr) {
        if( complex_type == nullptr ) {
            complex_type = complex_type_4;
        }
        if( c->getType()->isPointerTy() ) {
            c = builder->CreateLoad(c);
        }
        llvm::AllocaInst *pc = builder->CreateAlloca(complex_type, nullptr);
        builder->CreateStore(c, pc);
        std::vector<llvm::Value *> idx = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, 0))};
        llvm::Value *pim = builder->CreateGEP(pc, idx);
        return builder->CreateLoad(pim);
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
        llvm::Value *pim = builder->CreateGEP(pc, idx);
        return builder->CreateLoad(pim);
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
        llvm::Value *pre = builder->CreateGEP(pres, idx1);
        llvm::Value *pim = builder->CreateGEP(pres, idx2);
        builder->CreateStore(re, pre);
        builder->CreateStore(im, pim);
        return builder->CreateLoad(pres);
    }

    llvm::Value *nested_struct_rd(std::vector<llvm::Value*> vals,
            llvm::StructType* rd) {
        llvm::AllocaInst *pres = builder->CreateAlloca(rd, nullptr);
        llvm::Value *pim = builder->CreateGEP(pres, vals);
        return builder->CreateLoad(pim);
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
        llvm::Value *a = builder->CreateLoad(pa);
        std::vector<llvm::Value*> args = {a, presult};
        builder->CreateCall(fn, args);
        return builder->CreateLoad(presult);
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        module = std::make_unique<llvm::Module>("LFortran", context);
        module->setDataLayout("");
        builder = std::make_unique<llvm::IRBuilder<>>(context);


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

        // Process Variables first:
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                visit_symbol(*item.second);
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

        // Then do all the procedures
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Function_t>(*item.second)
                || is_a<ASR::Subroutine_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = determine_module_dependencies(x);
        for (auto &item : build_order) {
            LFORTRAN_ASSERT(x.m_global_scope->scope.find(item)
                != x.m_global_scope->scope.end());
            ASR::symbol_t *mod = x.m_global_scope->scope[item];
            visit_symbol(*mod);
        }

        // Then the main program
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
            }
        }
    }

    // TODO: Uncomment and implement later
    // void check_single_element(llvm::Value* curr_idx, llvm::Value* arr) {
    // }

    inline llvm::Value* cmo_convertor_single_element(
        llvm::Value* arr, ASR::array_index_t* m_args, 
        int n_args, bool check_for_bounds) {
        llvm::Value* dim_des_arr_ptr = create_gep(arr, 2);
        llvm::Value* prod = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
        llvm::Value* idx = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
        for( int r = 0; r < n_args; r++ ) {
            ASR::array_index_t curr_idx = m_args[r];
            this->visit_expr_wrapper(curr_idx.m_right, true);
            llvm::Value* curr_llvm_idx = tmp;
            llvm::Value* dim_des_ptr = create_gep(dim_des_arr_ptr, r);
            if( check_for_bounds ) {
                llvm::Value* lval = builder->CreateLoad(create_gep(dim_des_ptr, 1));
                curr_llvm_idx = builder->CreateSub(curr_llvm_idx, lval);
                // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
            }
            idx = builder->CreateAdd(idx, builder->CreateMul(prod, curr_llvm_idx));
            llvm::Value* dim_size = builder->CreateLoad(create_gep(dim_des_ptr, 3));
            prod = builder->CreateMul(prod, dim_size);
        }
        return idx;
    }

    inline bool is_explicit_shape(ASR::Variable_t* v) {
        ASR::dimension_t* m_dims;
        int n_dims;
        switch( v->m_type->type ) {
            case ASR::ttypeType::Integer: {
                ASR::Integer_t* v_type = down_cast<ASR::Integer_t>(v->m_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                break;
            }
            case ASR::ttypeType::Real: {
                ASR::Real_t* v_type = down_cast<ASR::Real_t>(v->m_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                break;
            }
            default: {
                throw CodeGenError("Explicit shape checking supported only for integers.");
            }
        }
        return verify_dimensions_t(m_dims, n_dims);
    }

    inline void get_single_element(const ASR::ArrayRef_t& x) {
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(x.m_v);
        uint32_t v_h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(llvm_symtab.find(v_h) != llvm_symtab.end());
        llvm::Value* array = llvm_symtab[v_h];
        bool check_for_bounds = is_explicit_shape(v);
        llvm::Value* idx = cmo_convertor_single_element(array, x.m_args, (int) x.n_args, check_for_bounds);
        llvm::Value* array_ptr = create_gep(array, 0);
        llvm::Value* ptr_to_array_idx = create_gep(array_ptr, idx);
        tmp = ptr_to_array_idx;
    }

    void visit_ArrayRef(const ASR::ArrayRef_t& x) {
        get_single_element(x);
    }

    void visit_DerivedRef(const ASR::DerivedRef_t& x) {
        der_type_name = "";
        this->visit_expr(*x.m_v);
        ASR::Variable_t* member = down_cast<ASR::Variable_t>(symbol_get_past_external(x.m_m));
        std::string member_name = std::string(member->m_name);
        LFORTRAN_ASSERT(der_type_name.size() != 0);
        int member_idx = name2memidx[der_type_name][member_name];
        std::vector<llvm::Value*> idx_vec = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, member_idx))};
        llvm::Value* tmp1 = builder->CreateGEP(tmp, idx_vec);
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
        uint32_t h = get_hash((ASR::asr_t*)&x);
        // This happens at global scope, so the intent can only be either local
        // (global variable declared/initialized in this translation unit), or
        // external (global variable not declared/initialized in this
        // translation unit, just referenced).
        LFORTRAN_ASSERT(x.m_intent == intent_local
            || x.m_abi == ASR::abiType::Interactive);
        bool external = (x.m_abi != ASR::abiType::Source);
        llvm::Constant* init_value = nullptr;
        if (x.m_value != nullptr){
            this->visit_expr_wrapper(x.m_value, true);
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

    void visit_Module(const ASR::Module_t &x) {
        mangle_prefix = "__module_" + std::string(x.m_name) + "_";
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(
                        item.second);
                visit_Variable(*v);
            }
        }
        visit_procedures(x);
        mangle_prefix = "";
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
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

    template<typename T>
    void declare_vars(const T &x) {
        llvm::Value *target_var;
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);
                uint32_t h = get_hash((ASR::asr_t*)v);
                llvm::Type *type;
                ASR::ttypeType type_;
                int n_dims = 0, a_kind = 4;
                ASR::dimension_t* m_dims = nullptr;
                if (v->m_intent == intent_local || 
                    v->m_intent == intent_return_var || 
                    !v->m_intent) { 
                    switch (v->m_type->type) {
                        case (ASR::ttypeType::Integer) : {
                            ASR::Integer_t* v_type = down_cast<ASR::Integer_t>(v->m_type);
                            type_ = v_type->class_type;
                            m_dims = v_type->m_dims;
                            n_dims = v_type->n_dims;
                            a_kind = v_type->m_kind;
                            if( n_dims > 0 ) {
                                type = get_array_type(type_, a_kind, n_dims, m_dims);
                            } else {
                                type = getIntType(a_kind);
                            }
                            break;
                        }
                        case (ASR::ttypeType::Real) : {
                            a_kind = down_cast<ASR::Real_t>(v->m_type)->m_kind;
                            type = getFPType(a_kind);
                            break;
                        }
                        case (ASR::ttypeType::Complex) : {
                            a_kind = down_cast<ASR::Complex_t>(v->m_type)->m_kind;
                            type = getComplexType(a_kind);
                            break;
                        }
                        case (ASR::ttypeType::Character) :
                            type = character_type;
                            break;
                        case (ASR::ttypeType::Logical) :
                            type = llvm::Type::getInt1Ty(context);
                            break;
                        case (ASR::ttypeType::Derived) : {
                            type = getDerivedType(v->m_type, false);
                            break;
                        }
                        case (ASR::ttypeType::IntegerPointer) : {
                            a_kind = down_cast<ASR::IntegerPointer_t>(v->m_type)->m_kind;
                            type = getIntType(a_kind, true);
                            break;
                        } 
                        case (ASR::ttypeType::RealPointer) : {
                            a_kind = down_cast<ASR::RealPointer_t>(v->m_type)->m_kind;
                            type = getFPType(a_kind, true);
                            break;
                        }
                        case (ASR::ttypeType::ComplexPointer) : {
                            a_kind = down_cast<ASR::ComplexPointer_t>(v->m_type)->m_kind;
                            type = getComplexType(a_kind, true);
                            break;
                        }
                        case (ASR::ttypeType::CharacterPointer) : {
                            type = llvm::Type::getInt8PtrTy(context);
                            break;
                        }
                        case (ASR::ttypeType::DerivedPointer) : {
                            throw CodeGenError("Pointers for Derived type not implemented yet in conversion");
                            break;
                        }
                        case (ASR::ttypeType::LogicalPointer) :
                            type = llvm::Type::getInt1Ty(context);
                            break;
                        default :
                            throw CodeGenError("Type not implemented");
                    }
                    llvm::AllocaInst *ptr = builder->CreateAlloca(type, nullptr, v->m_name);
                    llvm_symtab[h] = ptr;
                    fill_array_details(ptr, m_dims, n_dims);
                    if( v->m_value != nullptr ) {
                        target_var = ptr;
                        this->visit_expr_wrapper(v->m_value, true);
                        llvm::Value *init_value = tmp;
                        builder->CreateStore(init_value, target_var);
                        auto finder = std::find(needed_globals.begin(), 
                                needed_globals.end(), h);
                        if (finder != needed_globals.end()) {
                            llvm::Value* ptr = module->getOrInsertGlobal(desc_name, 
                                    needed_global_struct);
                            int idx = std::distance(needed_globals.begin(),
                                    finder);
                            builder->CreateStore(target_var, create_gep(ptr,
                                        idx));
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
                // We pass all arguments as pointers for now
                llvm::Type *type;
                switch (arg->m_type->type) {
                    case (ASR::ttypeType::Integer) : {
                        int a_kind = down_cast<ASR::Integer_t>(arg->m_type)->m_kind;
                        type = getIntType(a_kind, true);
                        break;
                    }
                    case (ASR::ttypeType::Real) : {
                        int a_kind = down_cast<ASR::Real_t>(arg->m_type)->m_kind;
                        type = getFPType(a_kind, true);
                        break;
                    }
                    case (ASR::ttypeType::Complex) : {
                        int a_kind = down_cast<ASR::Complex_t>(arg->m_type)->m_kind;
                        type = getComplexType(a_kind, true);
                        break;
                    }
                    case (ASR::ttypeType::Character) :
                        throw CodeGenError("Character argument type not implemented yet in conversion");
                        break;
                    case (ASR::ttypeType::Logical) :
                        type = llvm::Type::getInt1PtrTy(context);
                        break;
                    case (ASR::ttypeType::Derived) : {
                        type = getDerivedType(arg->m_type, true);
                        break;
                    }
                    default :
                        LFORTRAN_ASSERT(false);
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
                uint32_t h = get_hash((ASR::asr_t*)fn);
                llvm::Type *type;
                type = llvm_symtab_fn[h]->getType();
                args.push_back(type);
            } else if (is_a<ASR::Subroutine_t>(*symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                /* This is likely a procedure passed as an argument. For the
                   type, we need to pass in a function pointer with the
                   correct call signature. */
                ASR::Subroutine_t* fn = ASR::down_cast<ASR::Subroutine_t>(
                    symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                    x.m_args[i])->m_v));
                uint32_t h = get_hash((ASR::asr_t*)fn);
                llvm::Type *type;
                type = llvm_symtab_fn[h]->getType();
                args.push_back(type);
            }
        }
        return args;
    }

    template <typename T>
    void declare_args(const T &x, llvm::Function &F) {
        size_t i = 0;
        for (llvm::Argument &llvm_arg : F.args()) {
            if (is_a<ASR::Variable_t>(*symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
                LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
                uint32_t h = get_hash((ASR::asr_t*)arg);
                auto finder = std::find(needed_globals.begin(),
                    needed_globals.end(), h);
                if (finder != needed_globals.end()) {
                    llvm::Value* ptr = module->getOrInsertGlobal(desc_name,
                            needed_global_struct);
                    int idx = std::distance(needed_globals.begin(),
                            finder);
                    builder->CreateStore(&llvm_arg, create_gep(ptr,
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
        if (x.m_abi != ASR::abiType::Source &&
            x.m_abi != ASR::abiType::Interactive) {
                return;
        }
        // Check if the procedure has a nested function that needs access to
        // some variables in its local scope
        uint32_t h = get_hash((ASR::asr_t*)&x);
        std::vector<llvm::Type*> nested_type;
        if (nested_func_types[h].size() > 0) {
            nested_type = nested_func_types[h];
            needed_global_struct = llvm::StructType::create(
                context, nested_type, x.m_name);
            desc_name = x.m_name;
            std::string desc_string = "_nstd_strct";
            desc_name += desc_string;
            module->getOrInsertGlobal(desc_name, needed_global_struct);
            llvm::ConstantAggregateZero* initializer = 
                llvm::ConstantAggregateZero::get(needed_global_struct);
            module->getNamedGlobal(desc_name)->setInitializer(initializer);
        }
        // Check if the procedure is an interface - if so we will need to add 
        // onto the basic block when we get to the implementation later
        if (x.m_deftype == ASR::Interface) {
            interface_procs[x.m_name] = h;
        }
        visit_procedures(x);
        bool interactive = (x.m_abi == ASR::abiType::Interactive);
        llvm::Function *F = nullptr;
        if (llvm_symtab_fn.find(h) != llvm_symtab_fn.end()) {
            /*
            throw CodeGenError("Function code already generated for '"
                + std::string(x.m_name) + "'");
            */
            F = llvm_symtab_fn[h];
        } else if (interface_procs.find(x.m_name) == interface_procs.end() || 
            interface_procs[x.m_name] == h) {
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
                    return_type = getComplexType(a_kind);
                    break;
                }
                case (ASR::ttypeType::Character) :
                    throw CodeGenError("Character return type not implemented yet");
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
            F = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, mangle_prefix + x.m_name, module.get());
            llvm_symtab_fn[h] = F;
        } else {
            /* TODO: Below approach will not work if there are multiple
               implementations in different scopes as we made a single function
               and simply amend to the basic block. Determine if we need 
               multiple function declarations or can branch between basic 
               blocks for the different implementation */
            F = llvm_symtab_fn[interface_procs[x.m_name]];
            // Insert instruction at end of basic block by getting front of
            // basic block list (just the first basic block, starting at the
            // last instruction)
            llvm::Function::BasicBlockListType* F_bbs = 
                &(F->getBasicBlockList());
            builder->SetInsertPoint(&(F_bbs->front()));

            declare_args(x, *F);

            declare_local_vars(x);

            for (size_t i=0; i<x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
            ASR::Variable_t *asr_retval = EXPR2VAR(x.m_return_var);
            uint32_t h = get_hash((ASR::asr_t*)asr_retval);
            llvm::Value *ret_val = llvm_symtab[h];
            llvm::Value *ret_val2 = builder->CreateLoad(ret_val);
            if (x.m_deftype == ASR::Implementation) {
                builder->CreateRet(ret_val2);
            }
            return;
        }

        if (interactive) return;

        if (!prototype_only) {
            llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                    ".entry", F);
            builder->SetInsertPoint(BB);

            declare_args(x, *F);

            if (x.m_deftype == ASR::Implementation) {
                // If we declare local vars in a function interface, we will
                // allocate the return variable twice (assume we don't need to
                // do anything with local vars for interfaces for now)
                declare_local_vars(x);
            }

            for (size_t i=0; i<x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
            ASR::Variable_t *asr_retval = EXPR2VAR(x.m_return_var);
            uint32_t h = get_hash((ASR::asr_t*)asr_retval);

            // Only create return value if this is an implementation
            if (x.m_deftype == ASR::Implementation) {
                llvm::Value *ret_val = llvm_symtab[h];
                llvm::Value *ret_val2 = builder->CreateLoad(ret_val);
                builder->CreateRet(ret_val2);
            }
        }
    }


    void visit_Subroutine(const ASR::Subroutine_t &x) {
        if (x.m_abi != ASR::abiType::Source &&
            x.m_abi != ASR::abiType::Interactive) {
                return;
        }
        bool interactive = (x.m_abi == ASR::abiType::Interactive);
        // Check if the procedure has a nested function that needs access to
        // some variables in its local scope
        uint32_t h = get_hash((ASR::asr_t*)&x);
        std::vector<llvm::Type*> nested_type;
        if (nested_func_types[h].size() > 0) {
            nested_type = nested_func_types[h];
            needed_global_struct = llvm::StructType::create(
                context, nested_type, x.m_name);
            desc_name = x.m_name;
            std::string desc_string = "_nstd_strct";
            desc_name += desc_string;
            module->getOrInsertGlobal(desc_name, needed_global_struct);
            llvm::ConstantAggregateZero* initializer = 
                llvm::ConstantAggregateZero::get(needed_global_struct);
            module->getNamedGlobal(desc_name)->setInitializer(initializer);
        }
        // Check if the procedure is an interface - if so we will need to add 
        // onto the basic block when we get to the implementation later
        if (x.m_deftype == ASR::Interface) {
            interface_procs[x.m_name] = h;
        }
        visit_procedures(x);
        llvm::Function *F = nullptr;
        if (llvm_symtab_fn.find(h) != llvm_symtab_fn.end()) {
            /*
            throw CodeGenError("Subroutine code already generated for '"
                + std::string(x.m_name) + "'");
            */
            F = llvm_symtab_fn[h];
        } else if (interface_procs.find(x.m_name) == interface_procs.end() || 
            interface_procs[x.m_name] == h) {
            std::vector<llvm::Type*> args = convert_args(x);
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), args, false);
            F = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, mangle_prefix + x.m_name, module.get());
            llvm_symtab_fn[h] = F;

        } else {
            /* TODO: Below approach will not work if there are multiple
               implementations in different scopes as we made a single function
               and simply amend to the basic block. Determine if we need 
               multiple function declarations or can branch between basic 
               blocks for the different implementation */
            F = llvm_symtab_fn[interface_procs[x.m_name]];
            llvm::Function::BasicBlockListType* F_bbs = 
                &(F->getBasicBlockList());
            builder->SetInsertPoint(&(F_bbs->front()));

            declare_args(x, *F);

            declare_local_vars(x);

            for (size_t i=0; i<x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
            // Only create return value if this is an implementation
            // TODO: If this is an interface we are just repeating statements
            // when we get to the implementation, need to fix that
            if (x.m_deftype == ASR::Implementation) {
                builder->CreateRetVoid();
            }
            return;
        }

        if (interactive) return;

        if (!prototype_only) {
            llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                    ".entry", F);
            builder->SetInsertPoint(BB);

            declare_args(x, *F);

            if (x.m_deftype == ASR::Implementation) {
                // Declaring local variables in a subroutine is not quite the
                // same as a function as we are not definitely allocating the
                // return value twice, but assume we don't need this for
                // interfaces for now (same as function)
                declare_local_vars(x);
            }

            for (size_t i=0; i<x.n_body; i++) {
                this->visit_stmt(*x.m_body[i]);
            }
            // Only create return value if this is an implementation
            if (x.m_deftype == ASR::Implementation) {
                builder->CreateRetVoid();
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
        llvm::Value *target, *value;
        uint32_t h;
        if( x.m_target->type == ASR::exprType::ArrayRef || 
            x.m_target->type == ASR::exprType::DerivedRef ) {
            this->visit_expr(*x.m_target);
            target = tmp;   
        } else {
            ASR::Variable_t *asr_target = EXPR2VAR(x.m_target);
            h = get_hash((ASR::asr_t*)asr_target);
            if (llvm_symtab.find(h) != llvm_symtab.end()) {
                switch( asr_target->m_type->type ) {
                    case ASR::ttypeType::IntegerPointer:
                    case ASR::ttypeType::RealPointer:
                    case ASR::ttypeType::ComplexPointer:
                    case ASR::ttypeType::CharacterPointer:
                    case ASR::ttypeType::LogicalPointer:
                    case ASR::ttypeType::DerivedPointer: {
                        target = builder->CreateLoad(llvm_symtab[h]);
                        break;
                    }
                    default: {
                        target = llvm_symtab[h];
                        break;
                    }
                }
                
            } else {
                /* Target for assignment not in the symbol table - must be
                assigning to an outer scope from a nested function - see 
                nested_05.f90 */
                auto finder = std::find(needed_globals.begin(), 
                        needed_globals.end(), h);
                LFORTRAN_ASSERT(finder != needed_globals.end());
                llvm::Value* ptr = module->getOrInsertGlobal(desc_name,
                    needed_global_struct);
                int idx = std::distance(needed_globals.begin(), finder);
                target = builder->CreateLoad(create_gep(ptr, idx));
            }
        }
        this->visit_expr_wrapper(x.m_value, true);
        value = tmp;
        builder->CreateStore(value, target);
        auto finder = std::find(needed_globals.begin(), 
                needed_globals.end(), h);
        if (finder != needed_globals.end()) {
            /* Target for assignment could be in the symbol table - and we are
            assigning to a variable needed in a nested function - see 
            nested_04.f90 */
            llvm::Value* ptr = module->getOrInsertGlobal(desc_name, 
                    needed_global_struct);
            int idx = std::distance(needed_globals.begin(), finder);
            builder->CreateStore(target, create_gep(ptr, idx));
        }
    }

    inline void visit_expr_wrapper(const ASR::expr_t* x, bool load_ref=false) {
        this->visit_expr(*x);
        if( x->type == ASR::exprType::ArrayRef || 
            x->type == ASR::exprType::DerivedRef ) {
            if( load_ref ) {
                tmp = builder->CreateLoad(tmp);
            }
        }
    }

    void visit_Compare(const ASR::Compare_t &x) {
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
                    throw SemanticError("Comparison operator not implemented",
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
                    throw SemanticError("Comparison operator not implemented",
                            x.base.base.loc);
                }
            }
        } else {
            throw CodeGenError("Only Integer and Real implemented in Compare");
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
        llvm::Value *thenV = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
        builder->CreateBr(mergeBB);
        thenBB = builder->GetInsertBlock();
        fn->getBasicBlockList().push_back(elseBB);
        builder->SetInsertPoint(elseBB);
        for (size_t i=0; i<x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
        }
        llvm::Value *elseV = llvm::ConstantInt::get(context, llvm::APInt(32, 2));
        builder->CreateBr(mergeBB);
        elseBB = builder->GetInsertBlock();
        fn->getBasicBlockList().push_back(mergeBB);
        builder->SetInsertPoint(mergeBB);
        llvm::PHINode *PN = builder->CreatePHI(llvm::Type::getInt32Ty(context), 2,
                                        "iftmp");
        PN->addIncoming(thenV, thenBB);
        PN->addIncoming(elseV, elseBB);
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head", fn);
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        this->current_loophead = loophead;
        this->current_loopend = loopend;

        // head
        builder->CreateBr(loophead);
        builder->SetInsertPoint(loophead);
        this->visit_expr_wrapper(x.m_test, true);
        llvm::Value *cond = tmp;
        builder->CreateCondBr(cond, loopbody, loopend);

        // body
        fn->getBasicBlockList().push_back(loopbody);
        builder->SetInsertPoint(loopbody);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        builder->CreateBr(loophead);

        // end
        fn->getBasicBlockList().push_back(loopend);
        builder->SetInsertPoint(loopend);
    }

    void visit_Exit(const ASR::Exit_t & /* x */) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *after = llvm::BasicBlock::Create(context, "after", fn);
        builder->CreateBr(current_loopend);
        builder->SetInsertPoint(after);
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *after = llvm::BasicBlock::Create(context, "after", fn);
        builder->CreateBr(current_loophead);
        builder->SetInsertPoint(after);
    }

    void visit_BoolOp(const ASR::BoolOp_t &x) {
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
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        switch (x.m_op) {
            case ASR::stropType::Concat: {
                tmp = lfortran_strop(left_val, right_val, "_lfortran_strcat");
                break;
            };
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        this->visit_expr_wrapper(x.m_left, true);
        llvm::Value *left_val = tmp;
        this->visit_expr_wrapper(x.m_right, true);
        llvm::Value *right_val = tmp;
        if (x.m_type->type == ASR::ttypeType::Integer || 
            x.m_type->type == ASR::ttypeType::IntegerPointer) {
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
                    a_kind = down_cast<ASR::Integer_t>(x.m_type)->m_kind;
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
        } else if (x.m_type->type == ASR::ttypeType::Real || 
                   x.m_type->type == ASR::ttypeType::RealPointer) {
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
                    a_kind = down_cast<ASR::Real_t>(x.m_type)->m_kind;
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
        } else if (x.m_type->type == ASR::ttypeType::Complex ||
                   x.m_type->type == ASR::ttypeType::ComplexPointer) {
            llvm::Type *type;
            int a_kind;
            if( x.m_type->type == ASR::ttypeType::ComplexPointer ) {
                a_kind = down_cast<ASR::ComplexPointer_t>(x.m_type)->m_kind;
            } else {
                a_kind = down_cast<ASR::Complex_t>(x.m_type)->m_kind;
            }
            type = getComplexType(a_kind);
            if( left_val->getType()->isPointerTy() ) {
                left_val = builder->CreateLoad(left_val);
            }
            if( right_val->getType()->isPointerTy() ) {
                right_val = builder->CreateLoad(right_val);
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
        this->visit_expr_wrapper(x.m_operand, true);
        if (x.m_type->type == ASR::ttypeType::Integer) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // tmp = tmp;
                return;
            } else if (x.m_op == ASR::unaryopType::USub) {
                llvm::Value *zero = llvm::ConstantInt::get(context,
                    llvm::APInt(32, 0));
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
                llvm::Value *zero = llvm::ConstantFP::get(context,
                        llvm::APFloat((float)0.0));
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
        double val = std::atof(x.m_r);
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

    void visit_ConstantComplex(const ASR::ConstantComplex_t &x) {
        LFORTRAN_ASSERT(ASR::is_a<ASR::ConstantReal_t>(*x.m_re));
        LFORTRAN_ASSERT(ASR::is_a<ASR::ConstantReal_t>(*x.m_im));
        double re = std::atof(
            ASR::down_cast<ASR::ConstantReal_t>(x.m_re)->m_r);
        double im = std::atof(
            ASR::down_cast<ASR::ConstantReal_t>(x.m_im)->m_r);
        int a_kind = extract_kind_from_ttype_t(x.m_type);
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


    void visit_Str(const ASR::Str_t &x) {
        tmp = builder->CreateGlobalStringPtr(x.m_s);
    }

    inline void fetch_ptr(ASR::Variable_t* x) {
        uint32_t x_h = get_hash((ASR::asr_t*)x);
        LFORTRAN_ASSERT(llvm_symtab.find(x_h) != llvm_symtab.end());
        llvm::Value* x_v = llvm_symtab[x_h];
        tmp = builder->CreateLoad(x_v);
        tmp = builder->CreateLoad(tmp);
    }

    inline void fetch_val(ASR::Variable_t* x) {
        uint32_t x_h = get_hash((ASR::asr_t*)x);
        llvm::Value* x_v;
        // Check if x is a needed global here, if so, it should exist as an
        // element in the runtime descriptor, get element pointer and create
        // load
        if (llvm_symtab.find(x_h) == llvm_symtab.end()) {
            LFORTRAN_ASSERT(std::find(needed_globals.begin(), 
                    needed_globals.end(), x_h) != needed_globals.end());
            auto finder = std::find(needed_globals.begin(), 
                    needed_globals.end(), x_h);
            llvm::Constant *ptr = module->getOrInsertGlobal(desc_name,
                needed_global_struct);
            int idx = std::distance(needed_globals.begin(), finder);
            std::vector<llvm::Value*> idx_vec = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
            x_v = builder->CreateLoad(builder->CreateGEP(ptr, idx_vec));
        } else {
            x_v = llvm_symtab[x_h];
        }
            tmp = builder->CreateLoad(x_v);
    }

    inline void fetch_var(ASR::Variable_t* x) {
        switch( x->m_type->type ) {
            case ASR::ttypeType::IntegerPointer:
            case ASR::ttypeType::RealPointer:
            case ASR::ttypeType::ComplexPointer: {
                fetch_ptr(x);
                break;
            }
            case ASR::ttypeType::CharacterPointer:
            case ASR::ttypeType::LogicalPointer:
            case ASR::ttypeType::DerivedPointer: {
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

    inline int extract_kind_from_ttype_t(const ASR::ttype_t* curr_type) {
        if( curr_type == nullptr ) {
            return -1;
        }
        switch (curr_type->type) {
            case ASR::ttypeType::Real : {
                return ((ASR::Real_t*)(&(curr_type->base)))->m_kind;
            }
            case ASR::ttypeType::RealPointer : {
                return ((ASR::RealPointer_t*)(&(curr_type->base)))->m_kind;
            }
            case ASR::ttypeType::Integer : {
                return ((ASR::Integer_t*)(&(curr_type->base)))->m_kind;
            } 
            case ASR::ttypeType::IntegerPointer : {
                return ((ASR::IntegerPointer_t*)(&(curr_type->base)))->m_kind;
            }
            case ASR::ttypeType::Complex: {
                return ((ASR::Complex_t*)(&(curr_type->base)))->m_kind;
            }
            case ASR::ttypeType::ComplexPointer: {
                return ((ASR::ComplexPointer_t*)(&(curr_type->base)))->m_kind;
            }
            default : {
                return -1;
            }
        }
    }

    inline ASR::ttype_t* extract_ttype_t_from_expr(ASR::expr_t* expr) {
        ASR::asr_t* base = &(expr->base);
        switch( expr->type ) {
            case ASR::exprType::ConstantReal : {
                return ((ASR::ConstantReal_t*)base)->m_type;
            }
            case ASR::exprType::ConstantInteger : {
                return ((ASR::ConstantInteger_t*)base)->m_type;
            }
            case ASR::exprType::BinOp : {
                return ((ASR::BinOp_t*)base)->m_type;
            } 
            case ASR::exprType::ConstantComplex: {
                return ((ASR::ConstantComplex_t*)base)->m_type;
            }
            default : {
                return nullptr;
            }
        }
    }

    void extract_kinds(const ASR::ImplicitCast_t& x, 
                       int& arg_kind, int& dest_kind)
    {   
        dest_kind = extract_kind_from_ttype_t(x.m_type);
        ASR::ttype_t* curr_type = extract_ttype_t_from_expr(x.m_arg);
        arg_kind = extract_kind_from_ttype_t(curr_type);
    }

    void visit_ImplicitCast(const ASR::ImplicitCast_t &x) {
        this->visit_expr_wrapper(x.m_arg, true);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                int a_kind = extract_kind_from_ttype_t(x.m_type);
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
                        throw SemanticError(R"""(Only 32 and 64 bit real kinds are implemented)""", 
                                            x.base.base.loc);
                    }
                }
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                llvm::Type *target_type;
                int a_kind = extract_kind_from_ttype_t(x.m_type);
                target_type = getIntType(a_kind);
                tmp = builder->CreateFPToSI(tmp, target_type);
                break;
            }
            case (ASR::cast_kindType::RealToComplex) : {
                llvm::Type *target_type;
                llvm::Value *zero;
                int a_kind = extract_kind_from_ttype_t(x.m_type);
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
                int a_kind = extract_kind_from_ttype_t(x.m_type);
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
                if( arg_kind > 0 && dest_kind > 0 )
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
                if( arg_kind > 0 && dest_kind > 0 )
                {
                    if( arg_kind == 4 && dest_kind == 8 ) {
                        tmp = builder->CreateSExt(tmp, llvm::Type::getInt64Ty(context));
                    } else if( arg_kind == 8 && dest_kind == 4 ) {
                        tmp = builder->CreateTrunc(tmp, llvm::Type::getInt32Ty(context));
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
                if( arg_kind > 0 && dest_kind > 0 )
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
            default : throw CodeGenError("Cast kind not implemented");
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        std::vector<llvm::Value *> args;
        std::vector<std::string> fmt;
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr_wrapper(x.m_values[i], true);
            ASR::expr_t *v = x.m_values[i];
            ASR::ttype_t *t = expr_type(v);
            if (t->type == ASR::ttypeType::Integer || 
                t->type == ASR::ttypeType::Logical || 
                t->type == ASR::ttypeType::IntegerPointer) {
                int a_kind = ((ASR::Integer_t*)(&(t->base)))->m_kind;
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
                        throw SemanticError(R"""(Printing support is available only 
                                            for 32, and 64 bit integer kinds.)""", 
                                            x.base.base.loc);
                    }
                }
                args.push_back(tmp);
            } else if (t->type == ASR::ttypeType::Real || 
                       t->type == ASR::ttypeType::RealPointer ) {
                int a_kind = ((ASR::Real_t*)(&(t->base)))->m_kind;
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
                        fmt.push_back("%lf");
                        d = builder->CreateFPExt(tmp,
                        llvm::Type::getDoubleTy(context));
                        break;
                    }
                    default: {
                        throw SemanticError(R"""(Printing support is available only 
                                            for 32, and 64 bit real kinds.)""", 
                                            x.base.base.loc);
                    }
                }
                args.push_back(d);
            } else if (t->type == ASR::ttypeType::Character) {
                fmt.push_back("%s");
                args.push_back(tmp);
            } else if (t->type == ASR::ttypeType::Complex || 
                       t->type == ASR::ttypeType::ComplexPointer) {
                int a_kind = ((ASR::Complex_t*)(&(t->base)))->m_kind;
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
                        throw SemanticError(R"""(Printing support is available only 
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

    void visit_Stop(const ASR::Stop_t & /* x */) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("STOP\n");
        printf(context, *module, *builder, {fmt_ptr});
        int exit_code_int = 0;
        llvm::Value *exit_code = llvm::ConstantInt::get(context,
                llvm::APInt(32, exit_code_int));
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
    std::vector<llvm::Value*> convert_call_args(const T &x) {
        std::vector<llvm::Value *> args;
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i]->type == ASR::exprType::Var) {
                if (is_a<ASR::Variable_t>(*symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                    ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
                    uint32_t h = get_hash((ASR::asr_t*)arg);
                    tmp = llvm_symtab[h];
                } else if (is_a<ASR::Function_t>(*symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                    ASR::Function_t* fn = ASR::down_cast<ASR::Function_t>(
                        symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                        x.m_args[i])->m_v));
                    uint32_t h = get_hash((ASR::asr_t*)fn);
                    if (interface_procs.find(fn->m_name) == 
                            interface_procs.end()) {
                        tmp = llvm_symtab_fn[h];
                    } else {
                        tmp = llvm_symtab_fn[interface_procs[fn->m_name]];
                    }
                } else if (is_a<ASR::Subroutine_t>(*symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                    ASR::Subroutine_t* fn = ASR::down_cast<ASR::Subroutine_t>(
                        symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                        x.m_args[i])->m_v));
                    uint32_t h = get_hash((ASR::asr_t*)fn);
                    if (interface_procs.find(fn->m_name) == 
                            interface_procs.end()) {
                        tmp = llvm_symtab_fn[h];
                    } else {
                        tmp = llvm_symtab_fn[interface_procs[fn->m_name]];
                    }
                }
            } else {
                this->visit_expr_wrapper(x.m_args[i]);
                llvm::Value *value=tmp;
                llvm::Type *target_type;
                ASR::ttype_t* arg_type = expr_type(x.m_args[i]);
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
                    case (ASR::ttypeType::Character) :
                        throw CodeGenError("Character argument type not implemented yet in conversion");
                        break;
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
                        llvm::AllocaInst *target = builder->CreateAlloca(
                            target_type, nullptr);
                        builder->CreateStore(value, target);
                        tmp = target;
                    }
                }
            }
            args.push_back(tmp);
        }
        return args;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(
                symbol_get_past_external(x.m_name));
        uint32_t h;
        if (s->m_abi == ASR::abiType::LFortranModule) {
            throw CodeGenError("Subroutine LFortran interfaces not implemented yet");
        } else if (s->m_abi == ASR::abiType::Interactive) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::Source) {
            h = get_hash((ASR::asr_t*)s);
        } else {
            throw CodeGenError("External type not implemented yet.");
        }
        if (llvm_symtab_fn_arg.find(h) != llvm_symtab_fn_arg.end()) {
            // Check if this is a callback function
            llvm::Value* fn = llvm_symtab_fn_arg[h];
            llvm::FunctionType* fntype = llvm_symtab_fn[h]->getFunctionType();
            std::vector<llvm::Value *> args = convert_call_args(x);
            tmp = builder->CreateCall(fntype, fn, args);
        } else if (llvm_symtab_fn.find(h) == llvm_symtab_fn.end()) {
            throw CodeGenError("Subroutine code not generated for '"
                + std::string(s->m_name) + "'");
        } else {
            llvm::Function *fn = llvm_symtab_fn[h];
            std::vector<llvm::Value *> args = convert_call_args(x);
            builder->CreateCall(fn, args);
        }
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(symbol_get_past_external(x.m_name));
        uint32_t h;
        if (s->m_abi == ASR::abiType::Source) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::LFortranModule) {
            throw CodeGenError("Function LFortran interfaces not implemented yet");
        } else if (s->m_abi == ASR::abiType::Interactive) {
            h = get_hash((ASR::asr_t*)s);
        } else if (s->m_abi == ASR::abiType::Intrinsic) {
            int a_kind = extract_kind_from_ttype_t(x.m_type);
            if (all_intrinsics.empty()) {
                populate_intrinsics(x.m_type);
            }
            // We use an unordered map to get the O(n) operation time
            std::unordered_map<std::string, llvm::Function *>::const_iterator
                find_intrinsic = all_intrinsics.find(s->m_name);
            if (find_intrinsic == all_intrinsics.end()) {
                throw CodeGenError("Intrinsic not implemented yet.");
            } else {
                std::vector<llvm::Value *> args = convert_call_args(x);
                LFORTRAN_ASSERT(args.size() == 1);
                tmp = lfortran_intrinsic(find_intrinsic->second, args[0], a_kind);
                return;
            }
            h = get_hash((ASR::asr_t *)s);
        } else {
            throw CodeGenError("External type not implemented yet.");
        }
        if (llvm_symtab_fn_arg.find(h) != llvm_symtab_fn_arg.end()) {
            // Check if this is a callback function
            llvm::Value* fn = llvm_symtab_fn_arg[h];
            llvm::FunctionType* fntype = llvm_symtab_fn[h]->getFunctionType();
            std::vector<llvm::Value *> args = convert_call_args(x);
            tmp = builder->CreateCall(fntype, fn, args);
        } else if (llvm_symtab_fn.find(h) == llvm_symtab_fn.end()) {
            throw CodeGenError("Function code not generated for '"
                + std::string(s->m_name) + "'");
        } else {
            llvm::Function *fn = llvm_symtab_fn[h];
            std::vector<llvm::Value *> args = convert_call_args(x);
            tmp = builder->CreateCall(fn, args);
        }
    }

    //!< Meant to be called only once
    void populate_intrinsics(ASR::ttype_t* _type) {
        std::vector<std::string> supported = {
            "sin",  "cos",  "tan",  "sinh",  "cosh",  "tanh",
            "asin", "acos", "atan", "asinh", "acosh", "atanh"};

        for (auto sv : supported) {
            auto fname = "_lfortran_" + sv;
            llvm::Function *fn = module->getFunction(fname);
            if (!fn) {
              int a_kind = extract_kind_from_ttype_t(_type);
              llvm::Type *func_type, *ptr_type;
              switch( a_kind ) {
                  case 4: {
                    func_type = llvm::Type::getFloatTy(context);
                    ptr_type = llvm::Type::getFloatPtrTy(context);
                    break;
                  }
                  case 8: {
                    func_type = llvm::Type::getDoubleTy(context);
                    ptr_type = llvm::Type::getDoublePtrTy(context);
                    break;
                  }
                  default: {
                      throw CodeGenError("Kind type not supported");
                  }
              }
              llvm::FunctionType *function_type =
                  llvm::FunctionType::get(llvm::Type::getVoidTy(context),
                                          {func_type,
                                           ptr_type},
                                          false);
              fn = llvm::Function::Create(
                  function_type, llvm::Function::ExternalLinkage, fname, *module);
            }
            all_intrinsics[sv] = fn;
        }
        return;
    }
};



std::unique_ptr<LLVMModule> asr_to_llvm(ASR::TranslationUnit_t &asr,
        llvm::LLVMContext &context, Allocator &al, std::string run_fn)
{
    ASRToLLVMVisitor v(context);
    pass_wrap_global_stmts_into_function(al, asr, run_fn);

    pass_replace_param_to_const(al, asr);
    // Uncomment for debugging the ASR after the transformation
    // std::cout << pickle(asr) << std::endl;

    pass_replace_do_loops(al, asr);
    pass_replace_select_case(al, asr);
    v.nested_func_types = pass_find_nested_vars(asr, context, 
            v.needed_globals);
    v.visit_asr((ASR::asr_t&)asr);
    std::string msg;
    llvm::raw_string_ostream err(msg);
    if (llvm::verifyModule(*v.module, &err)) {
        std::string buf;
        llvm::raw_string_ostream os(buf);
        v.module->print(os, nullptr);
        std::cout << os.str();
        throw CodeGenError("asr_to_llvm: module failed verification. Error:\n"
            + err.str());
    };
    return std::make_unique<LLVMModule>(std::move(v.module));
}

} // namespace LFortran
