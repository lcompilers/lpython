#include <libasr/assert.h>
#include <libasr/codegen/llvm_utils.h>
#include <libasr/codegen/llvm_array_utils.h>
#include <libasr/asr_utils.h>

namespace LCompilers {

    namespace LLVM {

        llvm::Value* CreateLoad(llvm::IRBuilder<> &builder, llvm::Value *x) {
            llvm::Type *t = x->getType();
            LCOMPILERS_ASSERT(t->isPointerTy());
            llvm::Type *t2 = t->getContainedType(0);
            return builder.CreateLoad(t2, x);
        }

        llvm::Value* CreateStore(llvm::IRBuilder<> &builder, llvm::Value *x, llvm::Value *y) {
            LCOMPILERS_ASSERT(y->getType()->isPointerTy());
            return builder.CreateStore(x, y);
        }


        llvm::Value* CreateGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx) {
            llvm::Type *t = x->getType();
            LCOMPILERS_ASSERT(t->isPointerTy());
            llvm::Type *t2 = t->getContainedType(0);
            return builder.CreateGEP(t2, x, idx);
        }

        llvm::Value* CreateInBoundsGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx) {
            llvm::Type *t = x->getType();
            LCOMPILERS_ASSERT(t->isPointerTy());
            llvm::Type *t2 = t->getContainedType(0);
            return builder.CreateInBoundsGEP(t2, x, idx);
        }

        llvm::Value* lfortran_malloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* arg_size) {
            std::string func_name = "_lfortran_malloc";
            llvm::Function *fn = module.getFunction(func_name);
            if (!fn) {
                llvm::FunctionType *function_type = llvm::FunctionType::get(
                        llvm::Type::getInt8PtrTy(context), {
                            llvm::Type::getInt32Ty(context)
                        }, true);
                fn = llvm::Function::Create(function_type,
                        llvm::Function::ExternalLinkage, func_name, module);
            }
            std::vector<llvm::Value*> args = {arg_size};
            return builder.CreateCall(fn, args);
        }

        llvm::Value* lfortran_calloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* count, llvm::Value* type_size) {
            std::string func_name = "_lfortran_calloc";
            llvm::Function *fn = module.getFunction(func_name);
            if (!fn) {
                llvm::FunctionType *function_type = llvm::FunctionType::get(
                        llvm::Type::getInt8PtrTy(context), {
                            llvm::Type::getInt32Ty(context),
                            llvm::Type::getInt32Ty(context)
                        }, true);
                fn = llvm::Function::Create(function_type,
                        llvm::Function::ExternalLinkage, func_name, module);
            }
            std::vector<llvm::Value*> args = {count, type_size};
            return builder.CreateCall(fn, args);
        }

        llvm::Value* lfortran_realloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* ptr, llvm::Value* arg_size) {
            std::string func_name = "_lfortran_realloc";
            llvm::Function *fn = module.getFunction(func_name);
            if (!fn) {
                llvm::FunctionType *function_type = llvm::FunctionType::get(
                        llvm::Type::getInt8PtrTy(context), {
                            llvm::Type::getInt8PtrTy(context),
                            llvm::Type::getInt32Ty(context)
                        }, true);
                fn = llvm::Function::Create(function_type,
                        llvm::Function::ExternalLinkage, func_name, module);
            }
            std::vector<llvm::Value*> args = {
                builder.CreateBitCast(ptr, llvm::Type::getInt8PtrTy(context)),
                arg_size
            };
            return builder.CreateCall(fn, args);
        }

        llvm::Value* lfortran_free(llvm::LLVMContext &context, llvm::Module &module,
                                   llvm::IRBuilder<> &builder, llvm::Value* ptr) {
            std::string func_name = "_lfortran_free";
            llvm::Function *fn = module.getFunction(func_name);
            if (!fn) {
                llvm::FunctionType *function_type = llvm::FunctionType::get(
                        llvm::Type::getVoidTy(context), {
                            llvm::Type::getInt8PtrTy(context)
                        }, true);
                fn = llvm::Function::Create(function_type,
                        llvm::Function::ExternalLinkage, func_name, module);
            }
            std::vector<llvm::Value*> args = {
                builder.CreateBitCast(ptr, llvm::Type::getInt8PtrTy(context)),
            };
            return builder.CreateCall(fn, args);
        }
    } // namespace LLVM

    LLVMUtils::LLVMUtils(llvm::LLVMContext& context,
        llvm::IRBuilder<>* _builder, std::string& der_type_name_,
        std::map<std::string, llvm::StructType*>& name2dertype_,
        std::map<std::string, llvm::StructType*>& name2dercontext_,
        std::vector<std::string>& struct_type_stack_,
        std::map<std::string, std::string>& dertype2parent_,
        std::map<std::string, std::map<std::string, int>>& name2memidx_,
        CompilerOptions &compiler_options_,
        std::unordered_map<std::uint32_t, std::unordered_map<std::string, llvm::Type*>>& arr_arg_type_cache_,
        std::map<std::string, std::pair<llvm::Type*, llvm::Type*>>& fname2arg_type_):
        context(context), builder(std::move(_builder)), str_cmp_itr(nullptr), der_type_name(der_type_name_),
        name2dertype(name2dertype_), name2dercontext(name2dercontext_),
        struct_type_stack(struct_type_stack_), dertype2parent(dertype2parent_),
        name2memidx(name2memidx_), arr_arg_type_cache(arr_arg_type_cache_), fname2arg_type(fname2arg_type_),
        dict_api_lp(nullptr), dict_api_sc(nullptr),
        set_api_lp(nullptr), set_api_sc(nullptr), compiler_options(compiler_options_) {
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
        }

    void LLVMUtils::set_module(llvm::Module* module_) {
        module = module_;
    }

    llvm::Type* LLVMUtils::getMemberType(ASR::ttype_t* mem_type, ASR::Variable_t* member,
        llvm::Module* module) {
        llvm::Type* llvm_mem_type = nullptr;
        switch( mem_type->type ) {
            case ASR::ttypeType::Integer: {
                int a_kind = ASR::down_cast<ASR::Integer_t>(mem_type)->m_kind;
                llvm_mem_type = getIntType(a_kind);
                break;
            }
            case ASR::ttypeType::Real: {
                int a_kind = ASR::down_cast<ASR::Real_t>(mem_type)->m_kind;
                llvm_mem_type = getFPType(a_kind);
                break;
            }
            case ASR::ttypeType::Struct: {
                llvm_mem_type = getStructType(mem_type, module);
                break;
            }
            case ASR::ttypeType::Enum: {
                llvm_mem_type = llvm::Type::getInt32Ty(context);
                break ;
            }
            case ASR::ttypeType::Union: {
                llvm_mem_type = getUnionType(mem_type, module);
                break;
            }
            case ASR::ttypeType::Allocatable: {
                ASR::Allocatable_t* ptr_type = ASR::down_cast<ASR::Allocatable_t>(mem_type);
                llvm_mem_type = getMemberType(ptr_type->m_type, member, module)->getPointerTo();
                break;
            }
            case ASR::ttypeType::Pointer: {
                ASR::Pointer_t* ptr_type = ASR::down_cast<ASR::Pointer_t>(mem_type);
                llvm_mem_type = getMemberType(ptr_type->m_type, member, module)->getPointerTo();
                break;
            }
            case ASR::ttypeType::Complex: {
                int a_kind = ASR::down_cast<ASR::Complex_t>(mem_type)->m_kind;
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

    void LLVMUtils::createStructContext(ASR::StructType_t* der_type) {
        std::string der_type_name = std::string(der_type->m_name);
        if (name2dercontext.find(der_type_name) == name2dercontext.end() ) {
            llvm::StructType* der_type_llvm = llvm::StructType::create(context,
                                {},
                                der_type_name,
                                der_type->m_is_packed);
            name2dercontext[der_type_name] = der_type_llvm;
            if( der_type->m_parent != nullptr ) {
                ASR::StructType_t *par_der_type = ASR::down_cast<ASR::StructType_t>(
                                                ASRUtils::symbol_get_past_external(der_type->m_parent));
                createStructContext(par_der_type);
            }
            for( size_t i = 0; i < der_type->n_members; i++ ) {
                std::string member_name = der_type->m_members[i];
                ASR::symbol_t* sym = der_type->m_symtab->get_symbol(member_name);
                if (ASR::is_a<ASR::StructType_t>(*sym)) {
                    ASR::StructType_t *d_type = ASR::down_cast<ASR::StructType_t>(sym);
                    createStructContext(d_type);
                }
            }
        }
    }

    llvm::Type* LLVMUtils::getStructType(ASR::StructType_t* der_type, llvm::Module* module, bool is_pointer) {
        std::string der_type_name = std::string(der_type->m_name);
        createStructContext(der_type);
        if (std::find(struct_type_stack.begin(), struct_type_stack.end(),
                        der_type_name) != struct_type_stack.end()) {
            LCOMPILERS_ASSERT(name2dercontext.find(der_type_name) != name2dercontext.end());
            return name2dercontext[der_type_name];
        }
        struct_type_stack.push_back(der_type_name);
        llvm::StructType** der_type_llvm;
        if (name2dertype.find(der_type_name) != name2dertype.end() ) {
            der_type_llvm = &name2dertype[der_type_name];
        } else {
            der_type_llvm = &name2dercontext[der_type_name];
            std::vector<llvm::Type*> member_types;
            int member_idx = 0;
            if( der_type->m_parent != nullptr ) {
                ASR::StructType_t *par_der_type = ASR::down_cast<ASR::StructType_t>(
                                                        ASRUtils::symbol_get_past_external(der_type->m_parent));
                llvm::Type* par_llvm = getStructType(par_der_type, module);
                member_types.push_back(par_llvm);
                dertype2parent[der_type_name] = std::string(par_der_type->m_name);
                member_idx += 1;
            }
            for( size_t i = 0; i < der_type->n_members; i++ ) {
                std::string member_name = der_type->m_members[i];
                ASR::Variable_t* member = ASR::down_cast<ASR::Variable_t>(der_type->m_symtab->get_symbol(member_name));
                llvm::Type* llvm_mem_type = get_type_from_ttype_t_util(member->m_type, module, member->m_abi);
                member_types.push_back(llvm_mem_type);
                name2memidx[der_type_name][std::string(member->m_name)] = member_idx;
                member_idx++;
            }
            (*der_type_llvm)->setBody(member_types);
            name2dertype[der_type_name] = *der_type_llvm;
        }
        struct_type_stack.pop_back();
        if ( is_pointer ) {
            return (*der_type_llvm)->getPointerTo();
        }
        return (llvm::Type*) *der_type_llvm;
    }

    llvm::Type* LLVMUtils::getStructType(ASR::ttype_t* _type, llvm::Module* module, bool is_pointer) {
        ASR::StructType_t* der_type;
        if( ASR::is_a<ASR::Struct_t>(*_type) ) {
            ASR::Struct_t* der = ASR::down_cast<ASR::Struct_t>(_type);
            ASR::symbol_t* der_sym = ASRUtils::symbol_get_past_external(der->m_derived_type);
            der_type = ASR::down_cast<ASR::StructType_t>(der_sym);
        } else if( ASR::is_a<ASR::Class_t>(*_type) ) {
            ASR::Class_t* der = ASR::down_cast<ASR::Class_t>(_type);
            ASR::symbol_t* der_sym = ASRUtils::symbol_get_past_external(der->m_class_type);
            der_type = ASR::down_cast<ASR::StructType_t>(der_sym);
        } else {
            LCOMPILERS_ASSERT(false);
            return nullptr; // silence a warning
        }
        llvm::Type* type = getStructType(der_type, module, is_pointer);
        LCOMPILERS_ASSERT(type != nullptr);
        return type;
    }

    llvm::Type* LLVMUtils::getUnionType(ASR::UnionType_t* union_type,
        llvm::Module* module, bool is_pointer) {
        std::string union_type_name = std::string(union_type->m_name);
        llvm::StructType* union_type_llvm = nullptr;
        if( name2dertype.find(union_type_name) != name2dertype.end() ) {
            union_type_llvm = name2dertype[union_type_name];
        } else {
            const std::map<std::string, ASR::symbol_t*>& scope = union_type->m_symtab->get_scope();
            llvm::DataLayout data_layout(module);
            llvm::Type* max_sized_type = nullptr;
            size_t max_type_size = 0;
            for( auto itr = scope.begin(); itr != scope.end(); itr++ ) {
                ASR::Variable_t* member = ASR::down_cast<ASR::Variable_t>(ASRUtils::symbol_get_past_external(itr->second));
                llvm::Type* llvm_mem_type = getMemberType(member->m_type, member, module);
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

    llvm::Type* LLVMUtils::getUnionType(ASR::ttype_t* _type, llvm::Module* module, bool is_pointer) {
        ASR::Union_t* union_ = ASR::down_cast<ASR::Union_t>(_type);
        ASR::symbol_t* union_sym = ASRUtils::symbol_get_past_external(union_->m_union_type);
        ASR::UnionType_t* union_type = ASR::down_cast<ASR::UnionType_t>(union_sym);
        return getUnionType(union_type, module, is_pointer);
    }

    llvm::Type* LLVMUtils::getClassType(ASR::ClassType_t* der_type, bool is_pointer) {
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
                        int a_kind = ASR::down_cast<ASR::Integer_t>(member->m_type)->m_kind;
                        mem_type = getIntType(a_kind);
                        break;
                    }
                    case ASR::ttypeType::Real: {
                        int a_kind = ASR::down_cast<ASR::Real_t>(member->m_type)->m_kind;
                        mem_type = getFPType(a_kind);
                        break;
                    }
                    case ASR::ttypeType::Class: {
                        mem_type = getClassType(member->m_type);
                        break;
                    }
                    case ASR::ttypeType::Complex: {
                        int a_kind = ASR::down_cast<ASR::Complex_t>(member->m_type)->m_kind;
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
        llvm::StructType* der_type_llvm = llvm::StructType::create(context, member_types, der_type_name);
        name2dertype[der_type_name] = der_type_llvm;
        if( is_pointer ) {
            return der_type_llvm->getPointerTo();
        }
        return (llvm::Type*) der_type_llvm;
    }

    llvm::Type* LLVMUtils::getClassType(ASR::StructType_t* der_type, bool is_pointer) {
        std::string der_type_name = std::string(der_type->m_name) + std::string("_polymorphic");
        llvm::StructType* der_type_llvm = nullptr;
        if( name2dertype.find(der_type_name) != name2dertype.end() ) {
            der_type_llvm = name2dertype[der_type_name];
        }
        LCOMPILERS_ASSERT(der_type_llvm != nullptr);
        if( is_pointer ) {
            return der_type_llvm->getPointerTo();
        }
        return (llvm::Type*) der_type_llvm;
    }

    llvm::Type* LLVMUtils::getClassType(ASR::ttype_t* _type, bool is_pointer) {
        ASR::Class_t* der = ASR::down_cast<ASR::Class_t>(_type);
        ASR::symbol_t* der_sym = ASRUtils::symbol_get_past_external(der->m_class_type);
        std::string der_sym_name = ASRUtils::symbol_name(der_sym);
        std::string der_type_name = der_sym_name + std::string("_polymorphic");
        llvm::StructType* der_type_llvm;
        if( name2dertype.find(der_type_name) != name2dertype.end() ) {
            der_type_llvm = name2dertype[der_type_name];
        } else {
            std::vector<llvm::Type*> member_types;
            member_types.push_back(getIntType(8));
            if( der_sym_name == "~abstract_type" ) {
                member_types.push_back(llvm::Type::getVoidTy(context)->getPointerTo());
            } else if( ASR::is_a<ASR::ClassType_t>(*der_sym) ) {
                ASR::ClassType_t* class_type_t = ASR::down_cast<ASR::ClassType_t>(der_sym);
                member_types.push_back(getClassType(class_type_t, is_pointer));
            } else if( ASR::is_a<ASR::StructType_t>(*der_sym) ) {
                ASR::StructType_t* struct_type_t = ASR::down_cast<ASR::StructType_t>(der_sym);
                member_types.push_back(getStructType(struct_type_t, module, is_pointer));
            }
            der_type_llvm = llvm::StructType::create(context, member_types, der_type_name);
            name2dertype[der_type_name] = der_type_llvm;
        }

        return (llvm::Type*) der_type_llvm;
    }

    llvm::Type* LLVMUtils::getFPType(int a_kind, bool get_pointer) {
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

    llvm::Type* LLVMUtils::getComplexType(int a_kind, bool get_pointer) {
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

    llvm::Type* LLVMUtils::get_el_type(ASR::ttype_t* m_type_, llvm::Module* module) {
        int a_kind = ASRUtils::extract_kind_from_ttype_t(m_type_);
        llvm::Type* el_type = nullptr;
        bool is_pointer = LLVM::is_llvm_pointer(*m_type_);
        switch(ASRUtils::type_get_past_pointer(m_type_)->type) {
            case ASR::ttypeType::Integer: {
                el_type = getIntType(a_kind, is_pointer);
                break;
            }
            case ASR::ttypeType::UnsignedInteger: {
                el_type = getIntType(a_kind, is_pointer);
                break;
            }
            case ASR::ttypeType::Real: {
                el_type = getFPType(a_kind, is_pointer);
                break;
            }
            case ASR::ttypeType::Complex: {
                el_type = getComplexType(a_kind, is_pointer);
                break;
            }
            case ASR::ttypeType::Logical: {
                el_type = llvm::Type::getInt1Ty(context);
                break;
            }
            case ASR::ttypeType::Struct: {
                el_type = getStructType(m_type_, module);
                break;
            }
            case ASR::ttypeType::Union: {
                el_type = getUnionType(m_type_, module);
                break;
            }
            case ASR::ttypeType::Class: {
                el_type = getClassType(m_type_);
                break;
            }
            case ASR::ttypeType::Character: {
                el_type = character_type;
                break;
            }
            default:
                LCOMPILERS_ASSERT(false);
                break;
        }
        return el_type;
    }

    int32_t get_type_size(ASR::ttype_t* asr_type, llvm::Type* llvm_type,
                          int32_t a_kind, llvm::Module* module) {
        if( LLVM::is_llvm_struct(asr_type) ||
            ASR::is_a<ASR::Character_t>(*asr_type) ||
            ASR::is_a<ASR::Complex_t>(*asr_type) ) {
            llvm::DataLayout data_layout(module);
            return data_layout.getTypeAllocSize(llvm_type);
        }
        return a_kind;
    }

    llvm::Type* LLVMUtils::get_dict_type(ASR::ttype_t* asr_type, llvm::Module* module) {
        ASR::Dict_t* asr_dict = ASR::down_cast<ASR::Dict_t>(asr_type);
        bool is_local_array_type = false, is_local_malloc_array_type = false;
        bool is_local_list = false;
        ASR::dimension_t* local_m_dims = nullptr;
        int local_n_dims = 0;
        int local_a_kind = -1;
        ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
        llvm::Type* key_llvm_type = get_type_from_ttype_t(asr_dict->m_key_type, nullptr, local_m_storage,
                                                            is_local_array_type, is_local_malloc_array_type,
                                                            is_local_list, local_m_dims, local_n_dims,
                                                            local_a_kind, module);
        int32_t key_type_size = get_type_size(asr_dict->m_key_type, key_llvm_type, local_a_kind, module);
        llvm::Type* value_llvm_type = get_type_from_ttype_t(asr_dict->m_value_type, nullptr, local_m_storage,
                                                            is_local_array_type, is_local_malloc_array_type,
                                                            is_local_list, local_m_dims, local_n_dims,
                                                            local_a_kind, module);
        int32_t value_type_size = get_type_size(asr_dict->m_value_type, value_llvm_type, local_a_kind, module);
        std::string key_type_code = ASRUtils::get_type_code(asr_dict->m_key_type);
        std::string value_type_code = ASRUtils::get_type_code(asr_dict->m_value_type);
        set_dict_api(asr_dict);
        return dict_api->get_dict_type(key_type_code, value_type_code, key_type_size,
                                        value_type_size, key_llvm_type, value_llvm_type);
    }

    llvm::Type* LLVMUtils::get_set_type(ASR::ttype_t* asr_type, llvm::Module* module) {
        ASR::Set_t* asr_set = ASR::down_cast<ASR::Set_t>(asr_type);
        bool is_local_array_type = false, is_local_malloc_array_type = false;
        bool is_local_list = false;
        ASR::dimension_t* local_m_dims = nullptr;
        int local_n_dims = 0;
        int local_a_kind = -1;
        ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
        llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_set->m_type, nullptr, local_m_storage,
                                                            is_local_array_type, is_local_malloc_array_type,
                                                            is_local_list, local_m_dims, local_n_dims,
                                                            local_a_kind, module);
        int32_t el_type_size = get_type_size(asr_set->m_type, el_llvm_type, local_a_kind, module);
        std::string el_type_code = ASRUtils::get_type_code(asr_set->m_type);
        set_set_api(asr_set);
        return set_api->get_set_type(el_type_code, el_type_size,
                                                 el_llvm_type);
    }

    llvm::Type* LLVMUtils::get_arg_type_from_ttype_t(ASR::ttype_t* asr_type,
        ASR::symbol_t *type_declaration, ASR::abiType m_abi, ASR::abiType arg_m_abi,
        ASR::storage_typeType m_storage, bool arg_m_value_attr, int& n_dims,
        int& a_kind, bool& is_array_type, ASR::intentType arg_intent, llvm::Module* module,
        bool get_pointer) {
        llvm::Type* type = nullptr;

        #define handle_llvm_pointers2() bool is_pointer_ = ASR::is_a<ASR::Class_t>(*t2) || \
            (ASR::is_a<ASR::Character_t>(*t2) && arg_m_abi != ASR::abiType::BindC); \
            type = get_arg_type_from_ttype_t(t2, nullptr, m_abi, arg_m_abi, \
                        m_storage, arg_m_value_attr, n_dims, a_kind, \
                        is_array_type, arg_intent, module, get_pointer); \
            if( !is_pointer_ ) { \
                type = type->getPointerTo(); \
            } \

        switch (asr_type->type) {
            case ASR::ttypeType::Array: {
                ASR::Array_t* v_type = ASR::down_cast<ASR::Array_t>(asr_type);
                switch( v_type->m_physical_type ) {
                    case ASR::array_physical_typeType::DescriptorArray: {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(v_type->m_type, module);
                        type = arr_api->get_array_type(asr_type, el_type, get_pointer);
                        break;
                    }
                    case ASR::array_physical_typeType::PointerToDataArray: {
                        type = nullptr;
                        if( ASR::is_a<ASR::Complex_t>(*v_type->m_type) ) {
                            ASR::Complex_t* complex_t = ASR::down_cast<ASR::Complex_t>(v_type->m_type);
                            type = getComplexType(complex_t->m_kind, true);
                        }


                        if( type == nullptr ) {
                            type = get_type_from_ttype_t_util(v_type->m_type, module, arg_m_abi)->getPointerTo();
                        }
                        break;
                    }
                    case ASR::array_physical_typeType::UnboundedPointerToDataArray: {
                        type = nullptr;
                        if( ASR::is_a<ASR::Complex_t>(*v_type->m_type) ) {
                            ASR::Complex_t* complex_t = ASR::down_cast<ASR::Complex_t>(v_type->m_type);
                            type = getComplexType(complex_t->m_kind, true);
                        }


                        if( type == nullptr ) {
                            type = get_type_from_ttype_t_util(v_type->m_type, module, arg_m_abi)->getPointerTo();
                        }
                        break;
                    }
                    case ASR::array_physical_typeType::FixedSizeArray: {
                        type = llvm::ArrayType::get(get_el_type(v_type->m_type, module),
                                        ASRUtils::get_fixed_size_of_array(
                                            v_type->m_dims, v_type->n_dims))->getPointerTo();
                        break;
                    }
                    case ASR::array_physical_typeType::CharacterArraySinglePointer: {
                        // type = character_type->getPointerTo();
                        // is_array_type = true;
                        // llvm::Type* el_type = get_el_type(v_type->m_type, module);
                        // type = arr_api->get_array_type(asr_type, el_type, get_pointer);
                        // break;
                        if (ASRUtils::is_fixed_size_array(v_type->m_dims, v_type->n_dims)) {
                            // llvm_type = character_type; -- @character_01.c = internal global i8* null
                            // llvm_type = character_type->getPointerTo(); -- @character_01.c = internal global i8** null
                            // llvm_type = llvm::ArrayType::get(character_type,
                            //     ASRUtils::get_fixed_size_of_array(v_type->m_dims, v_type->n_dims))->getPointerTo();
                            // -- @character_01 = internal global [2 x i8*]* zeroinitializer

                            type = llvm::ArrayType::get(character_type,
                                ASRUtils::get_fixed_size_of_array(v_type->m_dims, v_type->n_dims));
                            break;
                        } else if (ASRUtils::is_dimension_empty(v_type->m_dims, v_type->n_dims)) {
                            // Treat it as a DescriptorArray
                            is_array_type = true;
                            llvm::Type* el_type = character_type;
                            type = arr_api->get_array_type(asr_type, el_type);
                            break;
                        } else {
                            LCOMPILERS_ASSERT(false);
                            break;
                        }
                    }
                    default: {
                        LCOMPILERS_ASSERT(false);
                    }
                }
                break;
            }
            case (ASR::ttypeType::Integer) : {
                ASR::Integer_t* v_type = ASR::down_cast<ASR::Integer_t>(asr_type);
                a_kind = v_type->m_kind;
                if (arg_m_abi == ASR::abiType::BindC
                    && arg_m_value_attr) {
                    type = getIntType(a_kind, false);
                } else {
                    type = getIntType(a_kind, true);
                }
                break;
            }
            case (ASR::ttypeType::UnsignedInteger) : {
                ASR::UnsignedInteger_t* v_type = ASR::down_cast<ASR::UnsignedInteger_t>(asr_type);
                a_kind = v_type->m_kind;
                if (arg_m_abi == ASR::abiType::BindC
                    && arg_m_value_attr) {
                    type = getIntType(a_kind, false);
                } else {
                    type = getIntType(a_kind, true);
                }
                break;
            }
            case (ASR::ttypeType::Pointer) : {
                ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(asr_type);
                handle_llvm_pointers2()
                break;
            }
            case (ASR::ttypeType::Allocatable) : {
                ASR::ttype_t *t2 = ASRUtils::type_get_past_allocatable(asr_type);
                handle_llvm_pointers2()
                break;
            }
            case (ASR::ttypeType::Real) : {
                ASR::Real_t* v_type = ASR::down_cast<ASR::Real_t>(asr_type);
                a_kind = v_type->m_kind;
                if (arg_m_abi == ASR::abiType::BindC
                    && arg_m_value_attr) {
                    type = getFPType(a_kind, false);
                } else {
                    type = getFPType(a_kind, true);
                }
                break;
            }
            case (ASR::ttypeType::Complex) : {
                ASR::Complex_t* v_type = ASR::down_cast<ASR::Complex_t>(asr_type);
                a_kind = v_type->m_kind;
                if (m_abi != ASR::abiType::BindC) {
                    type = getComplexType(a_kind, true);
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
            case (ASR::ttypeType::Character) : {
                ASR::Character_t* v_type = ASR::down_cast<ASR::Character_t>(asr_type);
                a_kind = v_type->m_kind;
                if (arg_m_abi == ASR::abiType::BindC) {
                    type = character_type;
                } else {
                    type = character_type->getPointerTo();
                }
                break;
            }
            case (ASR::ttypeType::Logical) : {
                ASR::Logical_t* v_type = ASR::down_cast<ASR::Logical_t>(asr_type);
                a_kind = v_type->m_kind;
                if (arg_m_abi == ASR::abiType::BindC
                    && arg_m_value_attr) {
                    type = llvm::Type::getInt1Ty(context);
                } else {
                    type = llvm::Type::getInt1PtrTy(context);
                }
                break;
            }
            case (ASR::ttypeType::Struct) : {
                type = getStructType(asr_type, module, true);
                break;
            }
            case (ASR::ttypeType::Class) : {
                type = getClassType(asr_type, true)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::CPtr) : {
                type = llvm::Type::getVoidTy(context)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::Tuple) : {
                type = get_type_from_ttype_t_util(asr_type, module)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::List) : {
                bool is_array_type = false, is_malloc_array_type = false;
                bool is_list = true;
                ASR::dimension_t *m_dims = nullptr;
                ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(asr_type);
                llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_list->m_type, nullptr, m_storage,
                                                                 is_array_type,
                                                                 is_malloc_array_type,
                                                                 is_list, m_dims, n_dims,
                                                                 a_kind, module, m_abi);
                int32_t type_size = -1;
                if( LLVM::is_llvm_struct(asr_list->m_type) ||
                    ASR::is_a<ASR::Character_t>(*asr_list->m_type) ||
                    ASR::is_a<ASR::Complex_t>(*asr_list->m_type) ) {
                    llvm::DataLayout data_layout(module);
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
            case (ASR::ttypeType::Dict): {
                ASR::Dict_t* asr_dict = ASR::down_cast<ASR::Dict_t>(asr_type);
                std::string key_type_code = ASRUtils::get_type_code(asr_dict->m_key_type);
                std::string value_type_code = ASRUtils::get_type_code(asr_dict->m_value_type);

                bool is_array_type = false, is_malloc_array_type = false;
                bool is_list = false;
                ASR::dimension_t* m_dims = nullptr;
                llvm::Type* key_llvm_type = get_type_from_ttype_t(asr_dict->m_key_type, type_declaration, m_storage,
                                                                  is_array_type,
                                                                  is_malloc_array_type,
                                                                  is_list, m_dims, n_dims,
                                                                  a_kind, module, m_abi);
                llvm::Type* value_llvm_type = get_type_from_ttype_t(asr_dict->m_value_type, type_declaration, m_storage,
                                                                    is_array_type,
                                                                    is_malloc_array_type,
                                                                    is_list, m_dims, n_dims,
                                                                    a_kind, module, m_abi);
                int32_t key_type_size = get_type_size(asr_dict->m_key_type, key_llvm_type, a_kind, module);
                int32_t value_type_size = get_type_size(asr_dict->m_value_type, value_llvm_type, a_kind, module);
                set_dict_api(asr_dict);
                type = dict_api->get_dict_type(key_type_code, value_type_code,
                                                key_type_size, value_type_size,
                                                key_llvm_type, value_llvm_type)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::Set): {
                ASR::Set_t* asr_set = ASR::down_cast<ASR::Set_t>(asr_type);
                std::string el_type_code = ASRUtils::get_type_code(asr_set->m_type);

                bool is_array_type = false, is_malloc_array_type = false;
                bool is_list = false;
                ASR::dimension_t* m_dims = nullptr;
                llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_set->m_type, type_declaration, m_storage,
                                                                  is_array_type,
                                                                  is_malloc_array_type,
                                                                  is_list, m_dims, n_dims,
                                                                  a_kind, module, m_abi);
                int32_t el_type_size = get_type_size(asr_set->m_type, el_llvm_type, a_kind, module);
                set_set_api(asr_set);
                type = set_api->get_set_type(el_type_code, el_type_size, el_llvm_type)->getPointerTo();
                break;
            }
            case ASR::ttypeType::FunctionType: {
                ASR::Function_t* fn = ASR::down_cast<ASR::Function_t>(
                    ASRUtils::symbol_get_past_external(type_declaration));
                type = get_function_type(*fn, module)->getPointerTo();
                break ;
            }
            default :
                LCOMPILERS_ASSERT(false);
        }
        return type;
    }

    void LLVMUtils::set_dict_api(ASR::Dict_t* dict_type) {
        if( ASR::is_a<ASR::Character_t>(*dict_type->m_key_type) ) {
            dict_api = dict_api_sc;
        } else {
            dict_api = dict_api_lp;
        }
    }

    void LLVMUtils::set_set_api(ASR::Set_t* /*set_type*/) {
        // As per benchmarks, separate chaining
        // does not provide significant gains over
        // linear probing.
        set_api = set_api_lp;
    }

    std::vector<llvm::Type*> LLVMUtils::convert_args(const ASR::Function_t& x, llvm::Module* module) {
        std::vector<llvm::Type*> args;
        for (size_t i=0; i<x.n_args; i++) {
            if (ASR::is_a<ASR::Variable_t>(*ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                ASR::Variable_t *arg = ASRUtils::EXPR2VAR(x.m_args[i]);
                LCOMPILERS_ASSERT(ASRUtils::is_arg_dummy(arg->m_intent));
                // We pass all arguments as pointers for now,
                // except bind(C) value arguments that are passed by value
                llvm::Type *type = nullptr, *type_original = nullptr;
                int n_dims = 0, a_kind = 4;
                bool is_array_type = false;
                type_original = get_arg_type_from_ttype_t(arg->m_type,
                    arg->m_type_declaration,
                    ASRUtils::get_FunctionType(x)->m_abi,
                    arg->m_abi, arg->m_storage, arg->m_value_attr,
                    n_dims, a_kind, is_array_type, arg->m_intent,
                    module, false);
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
                if( is_array_type && !LLVM::is_llvm_pointer(*arg->m_type) ) {
                    if( ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::Source ) {
                        llvm::Type* orig_type = type_original;
                        type = arr_api->get_argument_type(orig_type, m_h, arg->m_name, arr_arg_type_cache);
                        is_array_type = false;
                    } else if( ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::Intrinsic &&
                        fname2arg_type.find(m_name) != fname2arg_type.end()) {
                        type = fname2arg_type[m_name].second;
                        is_array_type = false;
                    }
                }
                args.push_back(type);
            } else if (ASR::is_a<ASR::Function_t>(*ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v))) {
                /* This is likely a procedure passed as an argument. For the
                   type, we need to pass in a function pointer with the
                   correct call signature. */
                ASR::Function_t* fn = ASR::down_cast<ASR::Function_t>(
                    ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Var_t>(
                    x.m_args[i])->m_v));
                llvm::Type* type = get_function_type(*fn, module)->getPointerTo();
                args.push_back(type);
            } else {
                throw CodeGenError("Argument type not implemented");
            }
        }
        return args;
    }

    llvm::FunctionType* LLVMUtils::get_function_type(const ASR::Function_t &x, llvm::Module* module) {
        llvm::Type *return_type;
        if (x.m_return_var) {
            ASR::ttype_t *return_var_type0 = ASRUtils::EXPR2VAR(x.m_return_var)->m_type;
            ASR::ttypeType return_var_type = return_var_type0->type;
            switch (return_var_type) {
                case (ASR::ttypeType::Integer) : {
                    int a_kind = ASR::down_cast<ASR::Integer_t>(return_var_type0)->m_kind;
                    return_type = getIntType(a_kind);
                    break;
                }
                case (ASR::ttypeType::UnsignedInteger) : {
                    int a_kind = ASR::down_cast<ASR::UnsignedInteger_t>(return_var_type0)->m_kind;
                    return_type = getIntType(a_kind);
                    break;
                }
                case (ASR::ttypeType::Real) : {
                    int a_kind = ASR::down_cast<ASR::Real_t>(return_var_type0)->m_kind;
                    return_type = getFPType(a_kind);
                    break;
                }
                case (ASR::ttypeType::Complex) : {
                    int a_kind = ASR::down_cast<ASR::Complex_t>(return_var_type0)->m_kind;
                    if (a_kind == 4) {
                        if (ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::BindC) {
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
                        if (ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::BindC) {
                            if (compiler_options.platform == Platform::Windows) {
                                // pass as subroutine
                                return_type = getComplexType(a_kind, true);
                                std::vector<llvm::Type*> args = convert_args(x, module);
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
                case (ASR::ttypeType::Pointer) : {
                    return_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(return_var_type0), module)->getPointerTo();
                    break;
                }
                case (ASR::ttypeType::Allocatable) : {
                    // TODO: Do getPointerTo as well.
                    return_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(return_var_type0), module);
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
                        llvm_el_types.push_back(get_type_from_ttype_t(
                                asr_tuple->m_type[i], nullptr, local_m_storage,
                                is_local_array_type, is_local_malloc_array_type,
                                is_local_list, local_m_dims, local_n_dims, local_a_kind, module));
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
                    llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_list->m_type, nullptr, m_storage,
                        is_array_type, is_malloc_array_type, is_list, m_dims, n_dims, a_kind, module);
                    int32_t type_size = -1;
                    if( LLVM::is_llvm_struct(asr_list->m_type) ||
                        ASR::is_a<ASR::Character_t>(*asr_list->m_type) ||
                        ASR::is_a<ASR::Complex_t>(*asr_list->m_type) ) {
                        llvm::DataLayout data_layout(module);
                        type_size = data_layout.getTypeAllocSize(el_llvm_type);
                    } else {
                        type_size = a_kind;
                    }
                    std::string el_type_code = ASRUtils::get_type_code(asr_list->m_type);
                    return_type = list_api->get_list_type(el_llvm_type, el_type_code, type_size);
                    break;
                }
                case (ASR::ttypeType::Dict) : {
                    ASR::Dict_t* asr_dict = ASR::down_cast<ASR::Dict_t>(return_var_type0);
                    std::string key_type_code = ASRUtils::get_type_code(asr_dict->m_key_type);
                    std::string value_type_code = ASRUtils::get_type_code(asr_dict->m_value_type);

                    bool is_local_array_type = false, is_local_malloc_array_type = false;
                    bool is_local_list = false;
                    ASR::dimension_t* local_m_dims = nullptr;
                    ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
                    int local_n_dims = 0, local_a_kind = -1;

                    llvm::Type* key_llvm_type = get_type_from_ttype_t(asr_dict->m_key_type,
                        nullptr, local_m_storage, is_local_array_type, is_local_malloc_array_type,
                        is_local_list, local_m_dims, local_n_dims, local_a_kind, module);
                    llvm::Type* value_llvm_type = get_type_from_ttype_t(asr_dict->m_value_type,
                        nullptr, local_m_storage,is_local_array_type, is_local_malloc_array_type,
                        is_local_list, local_m_dims, local_n_dims, local_a_kind, module);
                    int32_t key_type_size = get_type_size(asr_dict->m_key_type, key_llvm_type, local_a_kind, module);
                    int32_t value_type_size = get_type_size(asr_dict->m_value_type, value_llvm_type, local_a_kind, module);

                    set_dict_api(asr_dict);

                    return_type = dict_api->get_dict_type(key_type_code, value_type_code, key_type_size,value_type_size, key_llvm_type, value_llvm_type);
                    break;
                }
                case (ASR::ttypeType::Set) : {
                    ASR::Set_t* asr_set = ASR::down_cast<ASR::Set_t>(return_var_type0);
                    std::string el_type_code = ASRUtils::get_type_code(asr_set->m_type);

                    bool is_local_array_type = false, is_local_malloc_array_type = false;
                    bool is_local_list = false;
                    ASR::dimension_t* local_m_dims = nullptr;
                    ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
                    int local_n_dims = 0, local_a_kind = -1;

                    llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_set->m_type,
                        nullptr, local_m_storage, is_local_array_type, is_local_malloc_array_type,
                        is_local_list, local_m_dims, local_n_dims, local_a_kind, module);
                    int32_t el_type_size = get_type_size(asr_set->m_type, el_llvm_type, local_a_kind, module);

                    set_set_api(asr_set);

                    return_type = set_api->get_set_type(el_type_code, el_type_size, el_llvm_type);
                    break;
                }
                default :
                    throw CodeGenError("Type not implemented " + std::to_string(return_var_type));
            }
        } else {
            return_type = llvm::Type::getVoidTy(context);
        }
        std::vector<llvm::Type*> args = convert_args(x, module);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                return_type, args, false);
        return function_type;
    }

    std::vector<llvm::Type*> LLVMUtils::convert_args(ASR::FunctionType_t* x, llvm::Module* module) {
        std::vector<llvm::Type*> args;
        for (size_t i=0; i < x->n_arg_types; i++) {
            llvm::Type *type = nullptr, *type_original = nullptr;
            int n_dims = 0, a_kind = 4;
            bool is_array_type = false;
            type_original = get_arg_type_from_ttype_t(x->m_arg_types[i],
                nullptr, x->m_abi, x->m_abi, ASR::storage_typeType::Default,
                false, n_dims, a_kind, is_array_type, ASR::intentType::Unspecified,
                module, false);
            if( is_array_type ) {
                type = type_original->getPointerTo();
            } else {
                type = type_original;
            }
            args.push_back(type);
        }
        return args;
    }

    llvm::FunctionType* LLVMUtils::get_function_type(ASR::FunctionType_t* x, llvm::Module* module) {
        llvm::Type *return_type;
        if (x->m_return_var_type) {
            ASR::ttype_t* return_var_type0 = x->m_return_var_type;
            ASR::ttypeType return_var_type = return_var_type0->type;
            switch (return_var_type) {
                case (ASR::ttypeType::Integer) : {
                    int a_kind = ASR::down_cast<ASR::Integer_t>(return_var_type0)->m_kind;
                    return_type = getIntType(a_kind);
                    break;
                }
                case (ASR::ttypeType::UnsignedInteger) : {
                    int a_kind = ASR::down_cast<ASR::UnsignedInteger_t>(return_var_type0)->m_kind;
                    return_type = getIntType(a_kind);
                    break;
                }
                case (ASR::ttypeType::Real) : {
                    int a_kind = ASR::down_cast<ASR::Real_t>(return_var_type0)->m_kind;
                    return_type = getFPType(a_kind);
                    break;
                }
                case (ASR::ttypeType::Complex) : {
                    int a_kind = ASR::down_cast<ASR::Complex_t>(return_var_type0)->m_kind;
                    if (a_kind == 4) {
                        if (x->m_abi == ASR::abiType::BindC) {
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
                        if (x->m_abi == ASR::abiType::BindC) {
                            if (compiler_options.platform == Platform::Windows) {
                                // pass as subroutine
                                return_type = getComplexType(a_kind, true);
                                std::vector<llvm::Type*> args = convert_args(x, module);
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
                case (ASR::ttypeType::Pointer) : {
                    return_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(return_var_type0), module)->getPointerTo();
                    break;
                }
                case (ASR::ttypeType::Allocatable) : {
                    // TODO: Do getPointerTo as well.
                    return_type = get_type_from_ttype_t_util(ASRUtils::get_contained_type(return_var_type0), module);
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
                        llvm_el_types.push_back(get_type_from_ttype_t(
                                asr_tuple->m_type[i], nullptr, local_m_storage,
                                is_local_array_type, is_local_malloc_array_type,
                                is_local_list, local_m_dims, local_n_dims, local_a_kind, module));
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
                    llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_list->m_type, nullptr, m_storage,
                        is_array_type, is_malloc_array_type, is_list, m_dims, n_dims, a_kind, module);
                    int32_t type_size = -1;
                    if( LLVM::is_llvm_struct(asr_list->m_type) ||
                        ASR::is_a<ASR::Character_t>(*asr_list->m_type) ||
                        ASR::is_a<ASR::Complex_t>(*asr_list->m_type) ) {
                        llvm::DataLayout data_layout(module);
                        type_size = data_layout.getTypeAllocSize(el_llvm_type);
                    } else {
                        type_size = a_kind;
                    }
                    std::string el_type_code = ASRUtils::get_type_code(asr_list->m_type);
                    return_type = list_api->get_list_type(el_llvm_type, el_type_code, type_size);
                    break;
                }
                case (ASR::ttypeType::Dict) : {
                    ASR::Dict_t* asr_dict = ASR::down_cast<ASR::Dict_t>(return_var_type0);
                    std::string key_type_code = ASRUtils::get_type_code(asr_dict->m_key_type);
                    std::string value_type_code = ASRUtils::get_type_code(asr_dict->m_value_type);

                    bool is_local_array_type = false, is_local_malloc_array_type = false;
                    bool is_local_list = false;
                    ASR::dimension_t* local_m_dims = nullptr;
                    ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
                    int local_n_dims = 0, local_a_kind = -1;

                    llvm::Type* key_llvm_type = get_type_from_ttype_t(asr_dict->m_key_type,
                        nullptr, local_m_storage, is_local_array_type, is_local_malloc_array_type,
                        is_local_list, local_m_dims, local_n_dims, local_a_kind, module);
                    llvm::Type* value_llvm_type = get_type_from_ttype_t(asr_dict->m_value_type,
                        nullptr, local_m_storage,is_local_array_type, is_local_malloc_array_type,
                        is_local_list, local_m_dims, local_n_dims, local_a_kind, module);
                    int32_t key_type_size = get_type_size(asr_dict->m_key_type, key_llvm_type, local_a_kind, module);
                    int32_t value_type_size = get_type_size(asr_dict->m_value_type, value_llvm_type, local_a_kind, module);

                    set_dict_api(asr_dict);

                    return_type = dict_api->get_dict_type(key_type_code, value_type_code, key_type_size,value_type_size, key_llvm_type, value_llvm_type);
                    break;
                }
                case (ASR::ttypeType::Set) : {
                    ASR::Set_t* asr_set = ASR::down_cast<ASR::Set_t>(return_var_type0);
                    std::string el_type_code = ASRUtils::get_type_code(asr_set->m_type);

                    bool is_local_array_type = false, is_local_malloc_array_type = false;
                    bool is_local_list = false;
                    ASR::dimension_t* local_m_dims = nullptr;
                    ASR::storage_typeType local_m_storage = ASR::storage_typeType::Default;
                    int local_n_dims = 0, local_a_kind = -1;

                    llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_set->m_type,
                        nullptr, local_m_storage, is_local_array_type, is_local_malloc_array_type,
                        is_local_list, local_m_dims, local_n_dims, local_a_kind, module);
                    int32_t el_type_size = get_type_size(asr_set->m_type, el_llvm_type, local_a_kind, module);

                    set_set_api(asr_set);

                    return_type = set_api->get_set_type(el_type_code, el_type_size, el_llvm_type);
                    break;
                }
                default :
                    throw CodeGenError("Type not implemented " + std::to_string(return_var_type));
            }
        } else {
            return_type = llvm::Type::getVoidTy(context);
        }
        std::vector<llvm::Type*> args = convert_args(x, module);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                return_type, args, false);
        return function_type;
    }

    llvm::Type* LLVMUtils::get_type_from_ttype_t(ASR::ttype_t* asr_type,
        ASR::symbol_t *type_declaration, ASR::storage_typeType m_storage,
        bool& is_array_type, bool& is_malloc_array_type, bool& is_list,
        ASR::dimension_t*& m_dims, int& n_dims, int& a_kind, llvm::Module* module,
        ASR::abiType m_abi, bool is_pointer) {
        llvm::Type* llvm_type = nullptr;

        #define handle_llvm_pointers1()                                         \
            if (n_dims == 0 && ASR::is_a<ASR::Character_t>(*t2)) {              \
                llvm_type = character_type;                                     \
            } else {                                                            \
                llvm_type = get_type_from_ttype_t(t2, nullptr, m_storage,       \
                    is_array_type, is_malloc_array_type, is_list, m_dims,       \
                    n_dims, a_kind, module, m_abi, is_pointer_);                \
                if( !is_pointer_ ) {                                            \
                    llvm_type = llvm_type->getPointerTo();                      \
                }                                                               \
            }

        switch (asr_type->type) {
            case ASR::ttypeType::Array: {
                ASR::Array_t* v_type = ASR::down_cast<ASR::Array_t>(asr_type);
                m_dims = v_type->m_dims;
                n_dims = v_type->n_dims;
                a_kind = ASRUtils::extract_kind_from_ttype_t(v_type->m_type);
                                switch( v_type->m_physical_type ) {
                    case ASR::array_physical_typeType::DescriptorArray: {
                        is_array_type = true;
                        llvm::Type* el_type = get_el_type(v_type->m_type, module);
                        llvm_type = arr_api->get_array_type(asr_type, el_type);
                        break;
                    }
                    case ASR::array_physical_typeType::PointerToDataArray: {
                        llvm_type = get_el_type(v_type->m_type, module)->getPointerTo();
                        break;
                    }
                    case ASR::array_physical_typeType::FixedSizeArray: {
                        LCOMPILERS_ASSERT(ASRUtils::is_fixed_size_array(v_type->m_dims, v_type->n_dims));
                        llvm_type = llvm::ArrayType::get(get_el_type(v_type->m_type, module),
                                        ASRUtils::get_fixed_size_of_array(
                                            v_type->m_dims, v_type->n_dims));
                        break;
                    }
                    case ASR::array_physical_typeType::SIMDArray: {
                        llvm_type = llvm::VectorType::get(get_el_type(v_type->m_type, module),
                            ASRUtils::get_fixed_size_of_array(v_type->m_dims, v_type->n_dims), false);
                        break;
                    }
                    case ASR::array_physical_typeType::CharacterArraySinglePointer: {
                        if (ASRUtils::is_fixed_size_array(v_type->m_dims, v_type->n_dims)) {
                            llvm_type = llvm::ArrayType::get(character_type,
                                ASRUtils::get_fixed_size_of_array(v_type->m_dims, v_type->n_dims));
                            break;
                        } else if (ASRUtils::is_dimension_empty(v_type->m_dims, v_type->n_dims)) {
                            // Treat it as a DescriptorArray
                            is_array_type = true;
                            llvm::Type* el_type = character_type;
                            llvm_type = arr_api->get_array_type(asr_type, el_type);
                            break;
                        } else {
                            LCOMPILERS_ASSERT(false);
                            break;
                        }
                    }
                    default: {
                        LCOMPILERS_ASSERT(false);
                    }
                }
                break ;
            }
            case (ASR::ttypeType::Integer) : {
                ASR::Integer_t* v_type = ASR::down_cast<ASR::Integer_t>(asr_type);
                a_kind = v_type->m_kind;
                llvm_type = getIntType(a_kind);
                break;
            }
            case (ASR::ttypeType::UnsignedInteger) : {
                ASR::UnsignedInteger_t* v_type = ASR::down_cast<ASR::UnsignedInteger_t>(asr_type);
                a_kind = v_type->m_kind;
                // LLVM does not distinguish signed and unsigned integer types
                // Only integer operations can be signed/unsigned
                llvm_type = getIntType(a_kind);
                break;
            }
            case (ASR::ttypeType::Real) : {
                ASR::Real_t* v_type = ASR::down_cast<ASR::Real_t>(asr_type);
                a_kind = v_type->m_kind;
                llvm_type = getFPType(a_kind);
                break;
            }
            case (ASR::ttypeType::Complex) : {
                ASR::Complex_t* v_type = ASR::down_cast<ASR::Complex_t>(asr_type);
                a_kind = v_type->m_kind;
                llvm_type = getComplexType(a_kind);
                break;
            }
            case (ASR::ttypeType::Character) : {
                ASR::Character_t* v_type = ASR::down_cast<ASR::Character_t>(asr_type);
                a_kind = v_type->m_kind;
                llvm_type = character_type;
                break;
            }
            case (ASR::ttypeType::Logical) : {
                ASR::Logical_t* v_type = ASR::down_cast<ASR::Logical_t>(asr_type);
                a_kind = v_type->m_kind;
                llvm_type = llvm::Type::getInt1Ty(context);
                break;
            }
            case (ASR::ttypeType::Struct) : {
                llvm_type = getStructType(asr_type, module, false);
                break;
            }
            case (ASR::ttypeType::Class) : {
                llvm_type = getClassType(asr_type, is_pointer);
                break;
            }
            case (ASR::ttypeType::Union) : {
                llvm_type = getUnionType(asr_type, module, false);
                break;
            }
            case (ASR::ttypeType::Pointer) : {
                ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(asr_type)->m_type;
                bool is_pointer_ = ( ASR::is_a<ASR::Class_t>(*t2) ||
                    (ASR::is_a<ASR::Character_t>(*t2) && m_abi != ASR::abiType::BindC) );
                is_malloc_array_type = ASRUtils::is_array(t2);
                handle_llvm_pointers1()
                break;
            }
            case (ASR::ttypeType::Allocatable) : {
                ASR::ttype_t *t2 = ASR::down_cast<ASR::Allocatable_t>(asr_type)->m_type;
                bool is_pointer_ = (ASR::is_a<ASR::Character_t>(*t2)
                    && m_abi != ASR::abiType::BindC);
                is_malloc_array_type = ASRUtils::is_array(t2);
                handle_llvm_pointers1()
                break;
            }
            case (ASR::ttypeType::List) : {
                is_list = true;
                ASR::List_t* asr_list = ASR::down_cast<ASR::List_t>(asr_type);
                llvm::Type* el_llvm_type = get_type_from_ttype_t(asr_list->m_type, nullptr, m_storage,
                                                                 is_array_type, is_malloc_array_type,
                                                                 is_list, m_dims, n_dims,
                                                                 a_kind, module, m_abi);
                std::string el_type_code = ASRUtils::get_type_code(asr_list->m_type);
                int32_t type_size = -1;
                if( LLVM::is_llvm_struct(asr_list->m_type) ||
                    ASR::is_a<ASR::Character_t>(*asr_list->m_type) ||
                    ASR::is_a<ASR::Complex_t>(*asr_list->m_type) ) {
                    llvm::DataLayout data_layout(module);
                    type_size = data_layout.getTypeAllocSize(el_llvm_type);
                } else {
                    type_size = a_kind;
                }
                llvm_type = list_api->get_list_type(el_llvm_type, el_type_code, type_size);
                break;
            }
            case (ASR::ttypeType::Dict): {
                llvm_type = get_dict_type(asr_type, module);
                break;
            }
            case (ASR::ttypeType::Set): {
                llvm_type = get_set_type(asr_type, module);
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
                    llvm_el_types.push_back(get_type_from_ttype_t(asr_tuple->m_type[i], nullptr, local_m_storage,
                        is_local_array_type, is_local_malloc_array_type,
                        is_local_list, local_m_dims, local_n_dims, local_a_kind, module, m_abi));
                }
                llvm_type = tuple_api->get_tuple_type(type_code, llvm_el_types);
                break;
            }
            case (ASR::ttypeType::CPtr) : {
                a_kind = 8;
                llvm_type = llvm::Type::getVoidTy(context)->getPointerTo();
                break;
            }
            case (ASR::ttypeType::Enum) : {
                llvm_type = llvm::Type::getInt32Ty(context);
                break ;
            }
            case (ASR::ttypeType::FunctionType) : {
                if( type_declaration ) {
                    ASR::Function_t* fn = ASR::down_cast<ASR::Function_t>(
                        ASRUtils::symbol_get_past_external(type_declaration));
                    llvm_type = get_function_type(*fn, module)->getPointerTo();
                } else {
                    ASR::FunctionType_t* func_type = ASR::down_cast<ASR::FunctionType_t>(asr_type);
                    llvm_type = get_function_type(func_type, module)->getPointerTo();
                }
                break;
            }
            default :
                throw CodeGenError("Support for type " + ASRUtils::type_to_str(asr_type) +
                                   " not yet implemented.");
        }
        return llvm_type;
    }

    llvm::Type* LLVMUtils::get_type_from_ttype_t_util(ASR::ttype_t* asr_type,
        llvm::Module* module, ASR::abiType asr_abi) {
        ASR::storage_typeType m_storage_local = ASR::storage_typeType::Default;
        bool is_array_type_local, is_malloc_array_type_local;
        bool is_list_local;
        ASR::dimension_t* m_dims_local;
        int n_dims_local, a_kind_local;
        return get_type_from_ttype_t(asr_type, nullptr, m_storage_local, is_array_type_local,
                                     is_malloc_array_type_local, is_list_local,
                                     m_dims_local, n_dims_local, a_kind_local, module, asr_abi);
    }

    llvm::Value* LLVMUtils::create_gep(llvm::Value* ds, int idx) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
        return LLVM::CreateGEP(*builder, ds, idx_vec);
    }

    llvm::Value* LLVMUtils::create_gep(llvm::Value* ds, llvm::Value* idx) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
        idx};
        return LLVM::CreateGEP(*builder, ds, idx_vec);
    }

    llvm::Value* LLVMUtils::create_ptr_gep(llvm::Value* ptr, int idx) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
        return LLVM::CreateInBoundsGEP(*builder, ptr, idx_vec);
    }

    llvm::Value* LLVMUtils::create_ptr_gep(llvm::Value* ptr, llvm::Value* idx) {
        std::vector<llvm::Value*> idx_vec = {idx};
        return LLVM::CreateInBoundsGEP(*builder, ptr, idx_vec);
    }

    llvm::Type* LLVMUtils::getIntType(int a_kind, bool get_pointer) {
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
                    LCOMPILERS_ASSERT(false);
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
                    LCOMPILERS_ASSERT(false);
            }
        }
        return type_ptr;
    }

    void LLVMUtils::start_new_block(llvm::BasicBlock *bb) {
        llvm::BasicBlock *last_bb = builder->GetInsertBlock();
        llvm::Function *fn = last_bb->getParent();
        llvm::Instruction *block_terminator = last_bb->getTerminator();
        if (block_terminator == nullptr) {
            // The previous block is not terminated --- terminate it by jumping
            // to our new block
            builder->CreateBr(bb);
        }
#if LLVM_VERSION_MAJOR >= 16
        fn->insert(fn->end(), bb);
#else
        fn->getBasicBlockList().push_back(bb);
#endif
        builder->SetInsertPoint(bb);
    }

    llvm::Value* LLVMUtils::lfortran_str_cmp(llvm::Value* left_arg, llvm::Value* right_arg,
                                             std::string runtime_func_name, llvm::Module& module)
    {
        llvm::Type* character_type = llvm::Type::getInt8PtrTy(context);
        llvm::Function *fn = module.getFunction(runtime_func_name);
        if(!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getInt1Ty(context), {
                        character_type->getPointerTo(),
                        character_type->getPointerTo()
                    }, false);
            fn = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, runtime_func_name, module);
        }
        get_builder0()
        llvm::AllocaInst *pleft_arg = builder0.CreateAlloca(character_type, nullptr);
        LLVM::CreateStore(*builder, left_arg, pleft_arg);
        llvm::AllocaInst *pright_arg = builder0.CreateAlloca(character_type, nullptr);
        LLVM::CreateStore(*builder, right_arg, pright_arg);
        std::vector<llvm::Value*> args = {pleft_arg, pright_arg};
        return builder->CreateCall(fn, args);
    }

    llvm::Value* LLVMUtils::is_equal_by_value(llvm::Value* left, llvm::Value* right,
                                              llvm::Module& module, ASR::ttype_t* asr_type) {
        switch( asr_type->type ) {
            case ASR::ttypeType::Integer: {
                return builder->CreateICmpEQ(left, right);
            }
            case ASR::ttypeType::Logical: {
                llvm::Value* left_i32 = builder->CreateZExt(left, llvm::Type::getInt32Ty(context));
                llvm::Value* right_i32 = builder->CreateZExt(right, llvm::Type::getInt32Ty(context));
                return builder->CreateICmpEQ(left_i32, right_i32);
            }
            case ASR::ttypeType::Real: {
                return builder->CreateFCmpOEQ(left, right);
            }
            case ASR::ttypeType::Character: {
                get_builder0()
                str_cmp_itr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
                llvm::Value* null_char = llvm::ConstantInt::get(llvm::Type::getInt8Ty(context),
                                                            llvm::APInt(8, '\0'));
                llvm::Value* idx = str_cmp_itr;
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0)),
                    idx);
                llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
                llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
                llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

                // head
                start_new_block(loophead);
                {
                    llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                    llvm::Value* l = LLVM::CreateLoad(*builder, create_ptr_gep(left, i));
                    llvm::Value* r = LLVM::CreateLoad(*builder, create_ptr_gep(right, i));
                    llvm::Value *cond = builder->CreateAnd(
                        builder->CreateICmpNE(l, null_char),
                        builder->CreateICmpNE(r, null_char)
                    );
                    cond = builder->CreateAnd(cond, builder->CreateICmpEQ(l, r));
                    builder->CreateCondBr(cond, loopbody, loopend);
                }

                // body
                start_new_block(loopbody);
                {
                    llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                    i = builder->CreateAdd(i, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, 1)));
                    LLVM::CreateStore(*builder, i, idx);
                }

                builder->CreateBr(loophead);

                // end
                start_new_block(loopend);
                llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                llvm::Value* l = LLVM::CreateLoad(*builder, create_ptr_gep(left, i));
                llvm::Value* r = LLVM::CreateLoad(*builder, create_ptr_gep(right, i));
                return builder->CreateICmpEQ(l, r);
            }
            case ASR::ttypeType::Tuple: {
                ASR::Tuple_t* tuple_type = ASR::down_cast<ASR::Tuple_t>(asr_type);
                return tuple_api->check_tuple_equality(left, right, tuple_type, context,
                                                       builder, module);
            }
            case ASR::ttypeType::List: {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(asr_type);
                return list_api->check_list_equality(left, right, list_type->m_type,
                                                     context, builder, module);
            }
            default: {
                throw LCompilersException("LLVMUtils::is_equal_by_value isn't implemented for " +
                                          ASRUtils::type_to_str_python(asr_type));
            }
        }
    }

    llvm::Value* LLVMUtils::is_ineq_by_value(llvm::Value* left, llvm::Value* right,
                                             llvm::Module& module, ASR::ttype_t* asr_type,
                                             int8_t overload_id, ASR::ttype_t* int32_type) {
        /**
         * overloads:
         *  0    <
         *  1    <=
         *  2    >
         *  3    >=
         */
        llvm::CmpInst::Predicate pred;

        switch( asr_type->type ) {
            case ASR::ttypeType::Integer:
            case ASR::ttypeType::Logical: {
                if( asr_type->type == ASR::ttypeType::Logical ) {
                    left = builder->CreateZExt(left, llvm::Type::getInt32Ty(context));
                    right = builder->CreateZExt(right, llvm::Type::getInt32Ty(context));
                }
                switch( overload_id ) {
                    case 0: {
                        pred = llvm::CmpInst::Predicate::ICMP_SLT;
                        break;
                    }
                    case 1: {
                        pred = llvm::CmpInst::Predicate::ICMP_SLE;
                        break;
                    }
                    case 2: {
                        pred = llvm::CmpInst::Predicate::ICMP_SGT;
                        break;
                    }
                    case 3: {
                        pred = llvm::CmpInst::Predicate::ICMP_SGE;
                        break;
                    }
                    default: {
                        throw CodeGenError("Un-recognized overload-id: " + std::to_string(overload_id));
                    }
                }
                return builder->CreateICmp(pred, left, right);
            }
            case ASR::ttypeType::Real: {
                switch( overload_id ) {
                    case 0: {
                        pred = llvm::CmpInst::Predicate::FCMP_OLT;
                        break;
                    }
                    case 1: {
                        pred = llvm::CmpInst::Predicate::FCMP_OLE;
                        break;
                    }
                    case 2: {
                        pred = llvm::CmpInst::Predicate::FCMP_OGT;
                        break;
                    }
                    case 3: {
                        pred = llvm::CmpInst::Predicate::FCMP_OGE;
                        break;
                    }
                    default: {
                        throw CodeGenError("Un-recognized overload-id: " + std::to_string(overload_id));
                    }
                }
                return builder->CreateFCmp(pred, left, right);
            }
            case ASR::ttypeType::Character: {
                get_builder0()
                str_cmp_itr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
                llvm::Value* null_char = llvm::ConstantInt::get(llvm::Type::getInt8Ty(context),
                                                            llvm::APInt(8, '\0'));
                llvm::Value* idx = str_cmp_itr;
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0)),
                    idx);
                llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
                llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
                llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

                // head
                start_new_block(loophead);
                {
                    llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                    llvm::Value* l = LLVM::CreateLoad(*builder, create_ptr_gep(left, i));
                    llvm::Value* r = LLVM::CreateLoad(*builder, create_ptr_gep(right, i));
                    llvm::Value *cond = builder->CreateAnd(
                        builder->CreateICmpNE(l, null_char),
                        builder->CreateICmpNE(r, null_char)
                    );
                    switch( overload_id ) {
                        case 0: {
                            pred = llvm::CmpInst::Predicate::ICMP_ULT;
                            break;
                        }
                        case 1: {
                            pred = llvm::CmpInst::Predicate::ICMP_ULE;
                            break;
                        }
                        case 2: {
                            pred = llvm::CmpInst::Predicate::ICMP_UGT;
                            break;
                        }
                        case 3: {
                            pred = llvm::CmpInst::Predicate::ICMP_UGE;
                            break;
                        }
                        default: {
                            throw CodeGenError("Un-recognized overload-id: " + std::to_string(overload_id));
                        }
                    }
                    cond = builder->CreateAnd(cond, builder->CreateICmp(pred, l, r));
                    builder->CreateCondBr(cond, loopbody, loopend);
                }

                // body
                start_new_block(loopbody);
                {
                    llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                    i = builder->CreateAdd(i, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, 1)));
                    LLVM::CreateStore(*builder, i, idx);
                }

                builder->CreateBr(loophead);

                // end
                start_new_block(loopend);
                llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                llvm::Value* l = LLVM::CreateLoad(*builder, create_ptr_gep(left, i));
                llvm::Value* r = LLVM::CreateLoad(*builder, create_ptr_gep(right, i));
                return builder->CreateICmpULT(l, r);
            }
            case ASR::ttypeType::Tuple: {
                ASR::Tuple_t* tuple_type = ASR::down_cast<ASR::Tuple_t>(asr_type);
                return tuple_api->check_tuple_inequality(left, right, tuple_type, context,
                                                       builder, module, overload_id);
            }
            case ASR::ttypeType::List: {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(asr_type);
                return list_api->check_list_inequality(left, right, list_type->m_type,
                                                     context, builder, module,
                                                     overload_id, int32_type);
            }
            default: {
                throw LCompilersException("LLVMUtils::is_ineq_by_value isn't implemented for " +
                                          ASRUtils::type_to_str_python(asr_type));
            }
        }
    }

    void LLVMUtils::deepcopy(llvm::Value* src, llvm::Value* dest,
                             ASR::ttype_t* asr_type, llvm::Module* module,
                             std::map<std::string, std::map<std::string, int>>& name2memidx) {
        switch( ASRUtils::type_get_past_array(asr_type)->type ) {
            case ASR::ttypeType::Integer:
            case ASR::ttypeType::UnsignedInteger:
            case ASR::ttypeType::Real:
            case ASR::ttypeType::Logical:
            case ASR::ttypeType::Complex: {
                if( ASRUtils::is_array(asr_type) ) {
                    ASR::array_physical_typeType physical_type = ASRUtils::extract_physical_type(asr_type);
                    switch( physical_type ) {
                        case ASR::array_physical_typeType::DescriptorArray: {
                            arr_api->copy_array(src, dest, module, asr_type, false, false);
                            break;
                        }
                        case ASR::array_physical_typeType::FixedSizeArray: {
                            src = create_gep(src, 0);
                            dest = create_gep(dest, 0);
                            ASR::dimension_t* asr_dims = nullptr;
                            size_t asr_n_dims = ASRUtils::extract_dimensions_from_ttype(asr_type, asr_dims);
                            int64_t size = ASRUtils::get_fixed_size_of_array(asr_dims, asr_n_dims);
                            llvm::Type* llvm_data_type = get_type_from_ttype_t_util(ASRUtils::type_get_past_array(
                                ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(asr_type))), module);
                            llvm::DataLayout data_layout(module);
                            uint64_t data_size = data_layout.getTypeAllocSize(llvm_data_type);
                            llvm::Value* llvm_size = llvm::ConstantInt::get(context, llvm::APInt(32, size));
                            llvm_size = builder->CreateMul(llvm_size,
                                llvm::ConstantInt::get(context, llvm::APInt(32, data_size)));
                            builder->CreateMemCpy(dest, llvm::MaybeAlign(), src, llvm::MaybeAlign(), llvm_size);
                            break;
                        }
                        default: {
                            LCOMPILERS_ASSERT(false);
                        }
                    }
                } else {
                    LLVM::CreateStore(*builder, src, dest);
                }
                break ;
            };
            case ASR::ttypeType::Character:
            case ASR::ttypeType::FunctionType:
            case ASR::ttypeType::CPtr: {
                LLVM::CreateStore(*builder, src, dest);
                break ;
            }
            case ASR::ttypeType::Allocatable: {
                ASR::Allocatable_t* alloc_type = ASR::down_cast<ASR::Allocatable_t>(asr_type);
                if( ASR::is_a<ASR::Character_t>(*alloc_type->m_type) ) {
                    lfortran_str_copy(dest, src, true, *module, *builder, context);
                } else {
                    LLVM::CreateStore(*builder, src, dest);
                }
                break;
            }
            case ASR::ttypeType::Tuple: {
                ASR::Tuple_t* tuple_type = ASR::down_cast<ASR::Tuple_t>(asr_type);
                tuple_api->tuple_deepcopy(src, dest, tuple_type, module, name2memidx);
                break ;
            }
            case ASR::ttypeType::List: {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(asr_type);
                list_api->list_deepcopy(src, dest, list_type, module, name2memidx);
                break ;
            }
            case ASR::ttypeType::Dict: {
                ASR::Dict_t* dict_type = ASR::down_cast<ASR::Dict_t>(asr_type);
                set_dict_api(dict_type);
                dict_api->dict_deepcopy(src, dest, dict_type, module, name2memidx);
                break ;
            }
            case ASR::ttypeType::Struct: {
                ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(asr_type);
                ASR::StructType_t* struct_type_t = ASR::down_cast<ASR::StructType_t>(
                    ASRUtils::symbol_get_past_external(struct_t->m_derived_type));
                std::string der_type_name = std::string(struct_type_t->m_name);
                while( struct_type_t != nullptr ) {
                    for( auto item: struct_type_t->m_symtab->get_scope() ) {
                        if( ASR::is_a<ASR::ClassProcedure_t>(*item.second) ||
                            ASR::is_a<ASR::CustomOperator_t>(*item.second) ) {
                            continue ;
                        }
                        std::string mem_name = item.first;
                        int mem_idx = name2memidx[der_type_name][mem_name];
                        llvm::Value* src_member = create_gep(src, mem_idx);
                        if( !LLVM::is_llvm_struct(ASRUtils::symbol_type(item.second)) &&
                            !ASRUtils::is_array(ASRUtils::symbol_type(item.second)) ) {
                            src_member = LLVM::CreateLoad(*builder, src_member);
                        }
                        llvm::Value* dest_member = create_gep(dest, mem_idx);
                        deepcopy(src_member, dest_member,
                            ASRUtils::symbol_type(item.second),
                            module, name2memidx);
                    }
                    if( struct_type_t->m_parent != nullptr ) {
                        ASR::StructType_t* parent_struct_type_t =
                            ASR::down_cast<ASR::StructType_t>(struct_type_t->m_parent);
                        struct_type_t = parent_struct_type_t;
                    } else {
                        struct_type_t = nullptr;
                    }
                }
                break ;
            }
            default: {
                throw LCompilersException("LLVMUtils::deepcopy isn't implemented for " +
                                          ASRUtils::type_to_str(asr_type));
            }
        }
    }

    LLVMList::LLVMList(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        context(context_),
        llvm_utils(std::move(llvm_utils_)),
        builder(std::move(builder_)) {}

    LLVMDictInterface::LLVMDictInterface(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        context(context_),
        llvm_utils(std::move(llvm_utils_)),
        builder(std::move(builder_)),
        pos_ptr(nullptr), is_key_matching_var(nullptr),
        idx_ptr(nullptr), hash_iter(nullptr),
        hash_value(nullptr), polynomial_powers(nullptr),
        chain_itr(nullptr), chain_itr_prev(nullptr),
        old_capacity(nullptr), old_key_value_pairs(nullptr),
        old_key_mask(nullptr), is_dict_present_(false) {
    }

    LLVMDict::LLVMDict(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        LLVMDictInterface(context_, llvm_utils_, builder_) {
    }

    LLVMDictSeparateChaining::LLVMDictSeparateChaining(
        llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        LLVMDictInterface(context_, llvm_utils_, builder_) {
    }

    LLVMDictOptimizedLinearProbing::LLVMDictOptimizedLinearProbing(
        llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        LLVMDict(context_, llvm_utils_, builder_) {
        }

    llvm::Type* LLVMList::get_list_type(llvm::Type* el_type, std::string& type_code,
                                        int32_t type_size) {
        if( typecode2listtype.find(type_code) != typecode2listtype.end() ) {
            return std::get<0>(typecode2listtype[type_code]);
        }
        std::vector<llvm::Type*> list_type_vec = {llvm::Type::getInt32Ty(context),
                                                  llvm::Type::getInt32Ty(context),
                                                  el_type->getPointerTo()};
        llvm::StructType* list_desc = llvm::StructType::create(context, list_type_vec, "list");
        typecode2listtype[type_code] = std::make_tuple(list_desc, type_size, el_type);
        return list_desc;
    }

    llvm::Type* LLVMDict::get_dict_type(std::string key_type_code, std::string value_type_code,
        int32_t key_type_size, int32_t value_type_size,
        llvm::Type* key_type, llvm::Type* value_type) {
        is_dict_present_ = true;
        std::pair<std::string, std::string> llvm_key = std::make_pair(key_type_code, value_type_code);
        if( typecode2dicttype.find(llvm_key) != typecode2dicttype.end() ) {
            return std::get<0>(typecode2dicttype[llvm_key]);
        }

        llvm::Type* key_list_type = llvm_utils->list_api->get_list_type(key_type,
                                        key_type_code, key_type_size);
        llvm::Type* value_list_type = llvm_utils->list_api->get_list_type(value_type,
                                        value_type_code, value_type_size);
        std::vector<llvm::Type*> dict_type_vec = {llvm::Type::getInt32Ty(context),
                                                  key_list_type, value_list_type,
                                                  llvm::Type::getInt8PtrTy(context)};
        llvm::Type* dict_desc = llvm::StructType::create(context, dict_type_vec, "dict");
        typecode2dicttype[llvm_key] = std::make_tuple(dict_desc,
                                        std::make_pair(key_type_size, value_type_size),
                                        std::make_pair(key_type, value_type));
        return dict_desc;
    }

    llvm::Type* LLVMDictSeparateChaining::get_key_value_pair_type(
        std::string key_type_code, std::string value_type_code) {
        std::pair<std::string, std::string> llvm_key = std::make_pair(key_type_code, value_type_code);
        return typecode2kvstruct[llvm_key];
    }

    llvm::Type* LLVMDictSeparateChaining::get_key_value_pair_type(
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type) {
        std::string key_type_code = ASRUtils::get_type_code(key_asr_type);
        std::string value_type_code = ASRUtils::get_type_code(value_asr_type);
        return get_key_value_pair_type(key_type_code, value_type_code);
    }

    llvm::Type* LLVMDictSeparateChaining::get_dict_type(
        std::string key_type_code, std::string value_type_code,
        int32_t key_type_size, int32_t value_type_size,
        llvm::Type* key_type, llvm::Type* value_type) {
        is_dict_present_ = true;
        std::pair<std::string, std::string> llvm_key = std::make_pair(key_type_code, value_type_code);
        if( typecode2dicttype.find(llvm_key) != typecode2dicttype.end() ) {
            return std::get<0>(typecode2dicttype[llvm_key]);
        }

        std::vector<llvm::Type*> key_value_vec = {key_type, value_type,
                                                  llvm::Type::getInt8PtrTy(context)};
        llvm::Type* key_value_pair = llvm::StructType::create(context, key_value_vec, "key_value");
        std::vector<llvm::Type*> dict_type_vec = {llvm::Type::getInt32Ty(context),
                                                  llvm::Type::getInt32Ty(context),
                                                  llvm::Type::getInt32Ty(context),
                                                  key_value_pair->getPointerTo(),
                                                  llvm::Type::getInt8PtrTy(context),
                                                  llvm::Type::getInt1Ty(context)};
        llvm::Type* dict_desc = llvm::StructType::create(context, dict_type_vec, "dict");
        typecode2dicttype[llvm_key] = std::make_tuple(dict_desc,
                                        std::make_pair(key_type_size, value_type_size),
                                        std::make_pair(key_type, value_type));
        typecode2kvstruct[llvm_key] = key_value_pair;
        return dict_desc;
    }

    llvm::Value* LLVMList::get_pointer_to_list_data(llvm::Value* list) {
        return llvm_utils->create_gep(list, 2);
    }

    llvm::Value* LLVMList::get_pointer_to_current_end_point(llvm::Value* list) {
        return llvm_utils->create_gep(list, 0);
    }

    llvm::Value* LLVMList::get_pointer_to_current_capacity(llvm::Value* list) {
        return llvm_utils->create_gep(list, 1);
    }

    void LLVMList::list_init(std::string& type_code, llvm::Value* list,
                   llvm::Module& module, int32_t initial_capacity, int32_t n) {
        if( typecode2listtype.find(type_code) == typecode2listtype.end() ) {
            throw LCompilersException("list for " + type_code + " not declared yet.");
        }
        int32_t type_size = std::get<1>(typecode2listtype[type_code]);
        llvm::Value* arg_size = llvm::ConstantInt::get(context,
                                    llvm::APInt(32, type_size * initial_capacity));

        llvm::Value* list_data = LLVM::lfortran_malloc(context, module, *builder,
                                                       arg_size);
        llvm::Type* el_type = std::get<2>(typecode2listtype[type_code]);
        list_data = builder->CreateBitCast(list_data, el_type->getPointerTo());
        llvm::Value* list_data_ptr = get_pointer_to_list_data(list);
        builder->CreateStore(list_data, list_data_ptr);
        llvm::Value* current_end_point = llvm::ConstantInt::get(context, llvm::APInt(32, n));
        llvm::Value* current_capacity = llvm::ConstantInt::get(context, llvm::APInt(32, initial_capacity));
        builder->CreateStore(current_end_point, get_pointer_to_current_end_point(list));
        builder->CreateStore(current_capacity, get_pointer_to_current_capacity(list));
    }

    void LLVMList::list_init(std::string& type_code, llvm::Value* list,
                             llvm::Module& module, llvm::Value* initial_capacity,
                             llvm::Value* n) {
        if( typecode2listtype.find(type_code) == typecode2listtype.end() ) {
            throw LCompilersException("list for " + type_code + " not declared yet.");
        }
        int32_t type_size = std::get<1>(typecode2listtype[type_code]);
        llvm::Value* llvm_type_size = llvm::ConstantInt::get(context, llvm::APInt(32, type_size));
        llvm::Value* arg_size = builder->CreateMul(llvm_type_size, initial_capacity);
        llvm::Value* list_data = LLVM::lfortran_malloc(context, module, *builder, arg_size);

        llvm::Type* el_type = std::get<2>(typecode2listtype[type_code]);
        list_data = builder->CreateBitCast(list_data, el_type->getPointerTo());
        llvm::Value* list_data_ptr = get_pointer_to_list_data(list);
        builder->CreateStore(list_data, list_data_ptr);
        builder->CreateStore(n, get_pointer_to_current_end_point(list));
        builder->CreateStore(initial_capacity, get_pointer_to_current_capacity(list));
    }

    llvm::Value* LLVMDict::get_key_list(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 1);
    }

    llvm::Value* LLVMDictSeparateChaining::get_pointer_to_key_value_pairs(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 3);
    }

    llvm::Value* LLVMDictSeparateChaining::get_key_list(llvm::Value* /*dict*/) {
        return nullptr;
    }

    llvm::Value* LLVMDict::get_value_list(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 2);
    }

    llvm::Value* LLVMDictSeparateChaining::get_value_list(llvm::Value* /*dict*/) {
        return nullptr;
    }

    llvm::Value* LLVMDict::get_pointer_to_occupancy(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 0);
    }

    llvm::Value* LLVMDictSeparateChaining::get_pointer_to_occupancy(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 0);
    }

    llvm::Value* LLVMDictSeparateChaining::get_pointer_to_rehash_flag(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 5);
    }

    llvm::Value* LLVMDictSeparateChaining::get_pointer_to_number_of_filled_buckets(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 1);
    }

    llvm::Value* LLVMDict::get_pointer_to_capacity(llvm::Value* dict) {
        return llvm_utils->list_api->get_pointer_to_current_capacity(
                    get_value_list(dict));
    }

    llvm::Value* LLVMDictSeparateChaining::get_pointer_to_capacity(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 2);
    }

    void LLVMDict::dict_init(std::string key_type_code, std::string value_type_code,
                             llvm::Value* dict, llvm::Module* module, size_t initial_capacity) {
        llvm::Value* n_ptr = get_pointer_to_occupancy(dict);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                           llvm::APInt(32, 0)), n_ptr);
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm_utils->list_api->list_init(key_type_code, key_list, *module,
                                        initial_capacity, initial_capacity);
        llvm_utils->list_api->list_init(value_type_code, value_list, *module,
                                        initial_capacity, initial_capacity);
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_capacity = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, initial_capacity));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* key_mask = LLVM::lfortran_calloc(context, *module, *builder, llvm_capacity,
                                                      llvm_mask_size);
        LLVM::CreateStore(*builder, key_mask, get_pointer_to_keymask(dict));
    }

    void LLVMDictSeparateChaining::dict_init(std::string key_type_code, std::string value_type_code,
        llvm::Value* dict, llvm::Module* module, size_t initial_capacity) {
        llvm::Value* llvm_capacity = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, initial_capacity));
        llvm::Value* rehash_flag_ptr = get_pointer_to_rehash_flag(dict);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), llvm::APInt(1, 1)), rehash_flag_ptr);
        dict_init_given_initial_capacity(key_type_code, value_type_code, dict, module, llvm_capacity);
    }

    void LLVMDictSeparateChaining::dict_init_given_initial_capacity(
        std::string key_type_code, std::string value_type_code,
        llvm::Value* dict, llvm::Module* module, llvm::Value* llvm_capacity) {
        llvm::Value* rehash_flag_ptr = get_pointer_to_rehash_flag(dict);
        llvm::Value* rehash_flag = LLVM::CreateLoad(*builder, rehash_flag_ptr);
        llvm::Value* llvm_zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0));
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
        LLVM::CreateStore(*builder, llvm_zero, occupancy_ptr);
        llvm::Value* num_buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(dict);
        LLVM::CreateStore(*builder, llvm_zero, num_buckets_filled_ptr);

        llvm::DataLayout data_layout(module);
        llvm::Type* key_value_pair_type = get_key_value_pair_type(key_type_code, value_type_code);
        size_t key_value_type_size = data_layout.getTypeAllocSize(key_value_pair_type);
        llvm::Value* llvm_key_value_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, key_value_type_size));
        llvm::Value* malloc_size = builder->CreateMul(llvm_capacity, llvm_key_value_size);
        llvm::Value* key_value_ptr = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
        rehash_flag = builder->CreateAnd(rehash_flag,
                        builder->CreateICmpNE(key_value_ptr,
                        llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)))
                    );
        key_value_ptr = builder->CreateBitCast(key_value_ptr, key_value_pair_type->getPointerTo());
        LLVM::CreateStore(*builder, key_value_ptr, get_pointer_to_key_value_pairs(dict));

        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* key_mask = LLVM::lfortran_calloc(context, *module, *builder, llvm_capacity,
                                                      llvm_mask_size);
        rehash_flag = builder->CreateAnd(rehash_flag,
                        builder->CreateICmpNE(key_mask,
                        llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)))
                    );
        LLVM::CreateStore(*builder, key_mask, get_pointer_to_keymask(dict));

        llvm::Value* capacity_ptr = get_pointer_to_capacity(dict);
        LLVM::CreateStore(*builder, llvm_capacity, capacity_ptr);
        LLVM::CreateStore(*builder, rehash_flag, rehash_flag_ptr);
    }

    void LLVMList::list_deepcopy(llvm::Value* src, llvm::Value* dest,
        ASR::List_t* list_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        list_deepcopy(src, dest, list_type->m_type, module, name2memidx);
    }

    void LLVMList::list_deepcopy(llvm::Value* src, llvm::Value* dest,
                                 ASR::ttype_t* element_type, llvm::Module* module,
                                 std::map<std::string, std::map<std::string, int>>& name2memidx) {
        LCOMPILERS_ASSERT(src->getType() == dest->getType());
        std::string src_type_code = ASRUtils::get_type_code(element_type);
        llvm::Value* src_end_point = LLVM::CreateLoad(*builder, get_pointer_to_current_end_point(src));
        llvm::Value* src_capacity = LLVM::CreateLoad(*builder, get_pointer_to_current_capacity(src));
        llvm::Value* dest_end_point_ptr = get_pointer_to_current_end_point(dest);
        llvm::Value* dest_capacity_ptr = get_pointer_to_current_capacity(dest);
        builder->CreateStore(src_end_point, dest_end_point_ptr);
        builder->CreateStore(src_capacity, dest_capacity_ptr);
        llvm::Value* src_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(src));
        int32_t type_size = std::get<1>(typecode2listtype[src_type_code]);
        llvm::Value* arg_size = builder->CreateMul(llvm::ConstantInt::get(context,
                                                   llvm::APInt(32, type_size)), src_capacity);
        llvm::Value* copy_data = LLVM::lfortran_malloc(context, *module, *builder,
                                                       arg_size);
        llvm::Type* el_type = std::get<2>(typecode2listtype[src_type_code]);
        copy_data = builder->CreateBitCast(copy_data, el_type->getPointerTo());

        // We consider the case when the element type of a list is defined by a struct
        // which may also contain non-trivial structs (such as in case of list[list[f64]],
        // list[tuple[f64]]). We need to make sure that all the data inside those structs
        // is deepcopied and not just the address of the first element of those structs.
        // Hence we dive deeper into the lowest level of nested types and deepcopy everything
        // properly. If we don't consider this case then the data only from first level of nested types
        // will be deep copied and rest will be shallow copied. The importance of this case
        // can be figured out by goind through, integration_tests/test_list_06.py and
        // integration_tests/test_list_07.py.
        if( LLVM::is_llvm_struct(element_type) ) {
            builder->CreateStore(copy_data, get_pointer_to_list_data(dest));
            // TODO: Should be created outside the user loop and not here.
            // LLVMList should treat them as data members and create them
            // only if they are NULL
            get_builder0()
            llvm::AllocaInst *pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context),
                                                              nullptr);
            LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                               llvm::APInt(32, 0)), pos_ptr);

            llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
            llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
            llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

            // head
            llvm_utils->start_new_block(loophead);
            {
                llvm::Value *cond = builder->CreateICmpSGT(
                                            src_end_point,
                                            LLVM::CreateLoad(*builder, pos_ptr));
                builder->CreateCondBr(cond, loopbody, loopend);
            }

            // body
            llvm_utils->start_new_block(loopbody);
            {
                llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
                llvm::Value* srci = read_item(src, pos, false, *module, true);
                llvm::Value* desti = read_item(dest, pos, false, *module, true);
                llvm_utils->deepcopy(srci, desti, element_type, module, name2memidx);
                llvm::Value* tmp = builder->CreateAdd(
                            pos,
                            llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                LLVM::CreateStore(*builder, tmp, pos_ptr);
            }

            builder->CreateBr(loophead);

            // end
            llvm_utils->start_new_block(loopend);
        } else {
            builder->CreateMemCpy(copy_data, llvm::MaybeAlign(), src_data,
                                  llvm::MaybeAlign(), arg_size);
            builder->CreateStore(copy_data, get_pointer_to_list_data(dest));
        }
    }

    void LLVMDict::dict_deepcopy(llvm::Value* src, llvm::Value* dest,
                                 ASR::Dict_t* dict_type, llvm::Module* module,
                                 std::map<std::string, std::map<std::string, int>>& name2memidx) {
        LCOMPILERS_ASSERT(src->getType() == dest->getType());
        llvm::Value* src_occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(src));
        llvm::Value* dest_occupancy_ptr = get_pointer_to_occupancy(dest);
        LLVM::CreateStore(*builder, src_occupancy, dest_occupancy_ptr);

        llvm::Value* src_key_list = get_key_list(src);
        llvm::Value* dest_key_list = get_key_list(dest);
        llvm_utils->list_api->list_deepcopy(src_key_list, dest_key_list,
                                            dict_type->m_key_type, module,
                                            name2memidx);

        llvm::Value* src_value_list = get_value_list(src);
        llvm::Value* dest_value_list = get_value_list(dest);
        llvm_utils->list_api->list_deepcopy(src_value_list, dest_value_list,
                                            dict_type->m_value_type, module, name2memidx);

        llvm::Value* src_key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(src));
        llvm::Value* dest_key_mask_ptr = get_pointer_to_keymask(dest);
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* src_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(src));
        llvm::Value* dest_key_mask = LLVM::lfortran_calloc(context, *module, *builder, src_capacity,
                                                      llvm_mask_size);
        builder->CreateMemCpy(dest_key_mask, llvm::MaybeAlign(), src_key_mask,
                              llvm::MaybeAlign(), builder->CreateMul(src_capacity, llvm_mask_size));
        LLVM::CreateStore(*builder, dest_key_mask, dest_key_mask_ptr);
    }

    void LLVMDictSeparateChaining::deepcopy_key_value_pair_linked_list(
        llvm::Value* srci, llvm::Value* desti, llvm::Value* dest_key_value_pairs,
        ASR::Dict_t* dict_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        get_builder0()
        src_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        dest_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        llvm::Type* key_value_pair_type = get_key_value_pair_type(dict_type->m_key_type, dict_type->m_value_type)->getPointerTo();
        LLVM::CreateStore(*builder,
            builder->CreateBitCast(srci, llvm::Type::getInt8PtrTy(context)),
            src_itr);
        LLVM::CreateStore(*builder,
            builder->CreateBitCast(desti, llvm::Type::getInt8PtrTy(context)),
            dest_itr);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpNE(
                LLVM::CreateLoad(*builder, src_itr),
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))
            );
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* curr_src = builder->CreateBitCast(LLVM::CreateLoad(*builder, src_itr),
                key_value_pair_type);
            llvm::Value* curr_dest = builder->CreateBitCast(LLVM::CreateLoad(*builder, dest_itr),
                key_value_pair_type);
            llvm::Value* src_key_ptr = llvm_utils->create_gep(curr_src, 0);
            llvm::Value* src_value_ptr = llvm_utils->create_gep(curr_src, 1);
            llvm::Value *src_key = src_key_ptr, *src_value = src_value_ptr;
            if( !LLVM::is_llvm_struct(dict_type->m_key_type) ) {
                src_key = LLVM::CreateLoad(*builder, src_key_ptr);
            }
            if( !LLVM::is_llvm_struct(dict_type->m_value_type) ) {
                src_value = LLVM::CreateLoad(*builder, src_value_ptr);
            }
            llvm::Value* dest_key_ptr = llvm_utils->create_gep(curr_dest, 0);
            llvm::Value* dest_value_ptr = llvm_utils->create_gep(curr_dest, 1);
            llvm_utils->deepcopy(src_key, dest_key_ptr, dict_type->m_key_type, module, name2memidx);
            llvm_utils->deepcopy(src_value, dest_value_ptr, dict_type->m_value_type, module, name2memidx);

            llvm::Value* src_next_ptr = LLVM::CreateLoad(*builder, llvm_utils->create_gep(curr_src, 2));
            llvm::Value* curr_dest_next_ptr = llvm_utils->create_gep(curr_dest, 2);
            LLVM::CreateStore(*builder, src_next_ptr, src_itr);
            llvm::Function *fn = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
            llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
            llvm::Value* src_next_exists = builder->CreateICmpNE(src_next_ptr,
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)));
            builder->CreateCondBr(src_next_exists, thenBB, elseBB);
            builder->SetInsertPoint(thenBB);
            {
                llvm::Value* next_idx = LLVM::CreateLoad(*builder, next_ptr);
                llvm::Value* dest_next_ptr = llvm_utils->create_ptr_gep(dest_key_value_pairs, next_idx);
                dest_next_ptr = builder->CreateBitCast(dest_next_ptr, llvm::Type::getInt8PtrTy(context));
                LLVM::CreateStore(*builder, dest_next_ptr, curr_dest_next_ptr);
                LLVM::CreateStore(*builder, dest_next_ptr, dest_itr);
                next_idx = builder->CreateAdd(next_idx, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                              llvm::APInt(32, 1)));
                LLVM::CreateStore(*builder, next_idx, next_ptr);
            }
            builder->CreateBr(mergeBB);
            llvm_utils->start_new_block(elseBB);
            {
                LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)),
                    curr_dest_next_ptr
                );
            }
            llvm_utils->start_new_block(mergeBB);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMDictSeparateChaining::write_key_value_pair_linked_list(
        llvm::Value* kv_ll, llvm::Value* dict, llvm::Value* capacity,
        ASR::ttype_t* m_key_type, ASR::ttype_t* m_value_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        get_builder0()
        src_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        llvm::Type* key_value_pair_type = get_key_value_pair_type(m_key_type, m_value_type)->getPointerTo();
        LLVM::CreateStore(*builder,
            builder->CreateBitCast(kv_ll, llvm::Type::getInt8PtrTy(context)),
            src_itr);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpNE(
                LLVM::CreateLoad(*builder, src_itr),
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))
            );
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* curr_src = builder->CreateBitCast(LLVM::CreateLoad(*builder, src_itr),
                key_value_pair_type);
            llvm::Value* src_key_ptr = llvm_utils->create_gep(curr_src, 0);
            llvm::Value* src_value_ptr = llvm_utils->create_gep(curr_src, 1);
            llvm::Value *src_key = src_key_ptr, *src_value = src_value_ptr;
            if( !LLVM::is_llvm_struct(m_key_type) ) {
                src_key = LLVM::CreateLoad(*builder, src_key_ptr);
            }
            if( !LLVM::is_llvm_struct(m_value_type) ) {
                src_value = LLVM::CreateLoad(*builder, src_value_ptr);
            }
            llvm::Value* key_hash = get_key_hash(capacity, src_key, m_key_type, *module);
            resolve_collision_for_write(
                dict, key_hash, src_key,
                src_value, module,
                m_key_type, m_value_type,
                name2memidx);

            llvm::Value* src_next_ptr = LLVM::CreateLoad(*builder, llvm_utils->create_gep(curr_src, 2));
            LLVM::CreateStore(*builder, src_next_ptr, src_itr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMDictSeparateChaining::dict_deepcopy(
        llvm::Value* src, llvm::Value* dest,
        ASR::Dict_t* dict_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        llvm::Value* src_occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(src));
        llvm::Value* src_filled_buckets = LLVM::CreateLoad(*builder, get_pointer_to_number_of_filled_buckets(src));
        llvm::Value* src_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(src));
        llvm::Value* src_key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(src));
        llvm::Value* src_rehash_flag = LLVM::CreateLoad(*builder, get_pointer_to_rehash_flag(src));
        LLVM::CreateStore(*builder, src_occupancy, get_pointer_to_occupancy(dest));
        LLVM::CreateStore(*builder, src_filled_buckets, get_pointer_to_number_of_filled_buckets(dest));
        LLVM::CreateStore(*builder, src_capacity, get_pointer_to_capacity(dest));
        LLVM::CreateStore(*builder, src_rehash_flag, get_pointer_to_rehash_flag(dest));
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* malloc_size = builder->CreateMul(src_capacity, llvm_mask_size);
        llvm::Value* dest_key_mask = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
        LLVM::CreateStore(*builder, dest_key_mask, get_pointer_to_keymask(dest));

        malloc_size = builder->CreateSub(src_occupancy, src_filled_buckets);
        malloc_size = builder->CreateAdd(src_capacity, malloc_size);
        size_t kv_struct_size = data_layout.getTypeAllocSize(get_key_value_pair_type(dict_type->m_key_type,
                                dict_type->m_value_type));
        llvm::Value* llvm_kv_struct_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, kv_struct_size));
        malloc_size = builder->CreateMul(malloc_size, llvm_kv_struct_size);
        llvm::Value* dest_key_value_pairs = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
        dest_key_value_pairs = builder->CreateBitCast(
            dest_key_value_pairs,
            get_key_value_pair_type(dict_type->m_key_type, dict_type->m_value_type)->getPointerTo());
        get_builder0()
        copy_itr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        next_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        llvm::Value* llvm_zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0));
        LLVM::CreateStore(*builder, llvm_zero, copy_itr);
        LLVM::CreateStore(*builder, src_capacity, next_ptr);

        llvm::Value* src_key_value_pairs = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(src));
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(
                                        src_capacity,
                                        LLVM::CreateLoad(*builder, copy_itr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* itr = LLVM::CreateLoad(*builder, copy_itr);
            llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(src_key_mask, itr));
            LLVM::CreateStore(*builder, key_mask_value,
                llvm_utils->create_ptr_gep(dest_key_mask, itr));
            llvm::Value* is_key_set = builder->CreateICmpEQ(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));

            llvm_utils->create_if_else(is_key_set, [&]() {
                llvm::Value* srci = llvm_utils->create_ptr_gep(src_key_value_pairs, itr);
                llvm::Value* desti = llvm_utils->create_ptr_gep(dest_key_value_pairs, itr);
                deepcopy_key_value_pair_linked_list(srci, desti, dest_key_value_pairs,
                    dict_type, module, name2memidx);
            }, [=]() {
            });
            llvm::Value* tmp = builder->CreateAdd(
                        itr,
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, copy_itr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
        LLVM::CreateStore(*builder, dest_key_value_pairs, get_pointer_to_key_value_pairs(dest));
    }

    void LLVMList::check_index_within_bounds(llvm::Value* list,
                                    llvm::Value* pos, llvm::Module& module) {
        llvm::Value* end_point = LLVM::CreateLoad(*builder,
                                    get_pointer_to_current_end_point(list));
        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                   llvm::APInt(32, 0));

        llvm::Value* cond = builder->CreateOr(
                                builder->CreateICmpSGE(pos, end_point),
                                builder->CreateICmpSLT(pos, zero));
        llvm_utils->create_if_else(cond, [&]() {
            std::string index_error = "IndexError: %s%d%s%d\n",
            message1 = "List index is out of range. Index range is (0, ",
            message2 = "), but the given index is ";
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr(index_error);
            llvm::Value *fmt_ptr1 = builder->CreateGlobalStringPtr(message1);
            llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message2);
            llvm::Value *end_minus_one = builder->CreateSub(end_point,
                llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            print_error(context, module, *builder, {fmt_ptr, fmt_ptr1,
                end_minus_one, fmt_ptr2, pos});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, module, *builder, exit_code);
        }, [=]() {
        });
    }

    void LLVMList::write_item(llvm::Value* list, llvm::Value* pos,
                              llvm::Value* item, ASR::ttype_t* asr_type,
                              bool enable_bounds_checking, llvm::Module* module,
                              std::map<std::string, std::map<std::string, int>>& name2memidx) {
        if( enable_bounds_checking ) {
            check_index_within_bounds(list, pos, *module);
        }
        llvm::Value* list_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        llvm::Value* element_ptr = llvm_utils->create_ptr_gep(list_data, pos);
        llvm_utils->deepcopy(item, element_ptr, asr_type, module, name2memidx);
    }

    void LLVMList::write_item(llvm::Value* list, llvm::Value* pos,
                              llvm::Value* item, bool enable_bounds_checking,
                              llvm::Module& module) {
        if( enable_bounds_checking ) {
            check_index_within_bounds(list, pos, module);
        }
        llvm::Value* list_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        llvm::Value* element_ptr = llvm_utils->create_ptr_gep(list_data, pos);
        LLVM::CreateStore(*builder, item, element_ptr);
    }

    llvm::Value* LLVMDict::get_pointer_to_keymask(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 3);
    }

    llvm::Value* LLVMDictSeparateChaining::get_pointer_to_keymask(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 4);
    }

    void LLVMDict::resolve_collision(
        llvm::Value* capacity, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* key_list,
        llvm::Value* key_mask, llvm::Module& module,
        ASR::ttype_t* key_asr_type, bool for_read) {
        get_builder0()
        pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        is_key_matching_var = builder0.CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);
        LLVM::CreateStore(*builder, key_hash, pos_ptr);


        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");


        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(key_mask, pos));
            llvm::Value* is_key_skip = builder->CreateICmpEQ(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3)));
            llvm::Value* is_key_set = builder->CreateICmpNE(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            llvm::Value* is_key_matching = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
                                                                  llvm::APInt(1, 0));
            LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            llvm::Value* compare_keys = builder->CreateAnd(is_key_set,
                                            builder->CreateNot(is_key_skip));
            llvm_utils->create_if_else(compare_keys, [&]() {
                llvm::Value* original_key = llvm_utils->list_api->read_item(key_list, pos,
                                    false, module, LLVM::is_llvm_struct(key_asr_type));
                is_key_matching = llvm_utils->is_equal_by_value(key, original_key, module,
                                                                key_asr_type);
                LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            }, [=]() {
            });

            // TODO: Allow safe exit if pos becomes key_hash again.
            // Ideally should not happen as dict will be resized once
            // load factor touches a threshold (which will always be less than 1)
            // so there will be some key which will not be set. However for safety
            // we can add an exit from the loop with a error message.
            llvm::Value *cond = nullptr;
            if( for_read ) {
                cond = builder->CreateAnd(is_key_set, builder->CreateNot(
                            LLVM::CreateLoad(*builder, is_key_matching_var)));
                cond = builder->CreateOr(is_key_skip, cond);
            } else {
                cond = builder->CreateAnd(is_key_set, builder->CreateNot(is_key_skip));
                cond = builder->CreateAnd(cond, builder->CreateNot(
                            LLVM::CreateLoad(*builder, is_key_matching_var)));
            }
            builder->CreateCondBr(cond, loopbody, loopend);
        }


        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            pos = builder->CreateAdd(pos, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                 llvm::APInt(32, 1)));
            pos = builder->CreateSRem(pos, capacity);
            LLVM::CreateStore(*builder, pos, pos_ptr);
        }


        builder->CreateBr(loophead);


        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMDictOptimizedLinearProbing::resolve_collision(
        llvm::Value* capacity, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* key_list,
        llvm::Value* key_mask, llvm::Module& module,
        ASR::ttype_t* key_asr_type, bool for_read) {

        /**
         * C++ equivalent:
         *
         * pos = key_hash;
         *
         * while( true ) {
         *     is_key_skip = key_mask_value == 3;     // tombstone
         *     is_key_set = key_mask_value != 0;
         *     is_key_matching = 0;
         *
         *     compare_keys = is_key_set && !is_key_skip;
         *     if( compare_keys ) {
         *         original_key = key_list[pos];
         *         is_key_matching = key == original_key;
         *     }
         *
         *     cond;
         *     if( for_read ) {
         *         // for reading, continue to next pos
         *         // even if current pos is tombstone
         *         cond = (is_key_set && !is_key_matching) || is_key_skip;
         *     }
         *     else {
         *         // for writing, do not continue
         *         // if current pos is tombstone
         *         cond = is_key_set && !is_key_matching && !is_key_skip;
         *     }
         *
         *     if( cond ) {
         *         pos += 1;
         *         pos %= capacity;
         *     }
         *     else {
         *         break;
         *     }
         * }
         *
         */

        get_builder0()
        if( !for_read ) {
            pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        }
        is_key_matching_var = builder0.CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);

        LLVM::CreateStore(*builder, key_hash, pos_ptr);

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(key_mask, pos));
            llvm::Value* is_key_skip = builder->CreateICmpEQ(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3)));
            llvm::Value* is_key_set = builder->CreateICmpNE(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            llvm::Value* is_key_matching = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
                                                                llvm::APInt(1, 0));
            LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            llvm::Value* compare_keys = builder->CreateAnd(is_key_set,
                                            builder->CreateNot(is_key_skip));
            llvm_utils->create_if_else(compare_keys, [&]() {
                llvm::Value* original_key = llvm_utils->list_api->read_item(key_list, pos,
                                false, module, LLVM::is_llvm_struct(key_asr_type));
                is_key_matching = llvm_utils->is_equal_by_value(key, original_key, module,
                                                                key_asr_type);
                LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            }, [=]() {
            });
            // TODO: Allow safe exit if pos becomes key_hash again.
            // Ideally should not happen as dict will be resized once
            // load factor touches a threshold (which will always be less than 1)
            // so there will be some key which will not be set. However for safety
            // we can add an exit from the loop with a error message.
            llvm::Value *cond = nullptr;
            if( for_read ) {
                cond = builder->CreateAnd(is_key_set, builder->CreateNot(
                            LLVM::CreateLoad(*builder, is_key_matching_var)));
                cond = builder->CreateOr(is_key_skip, cond);
            } else {
                cond = builder->CreateAnd(is_key_set, builder->CreateNot(is_key_skip));
                cond = builder->CreateAnd(cond, builder->CreateNot(
                            LLVM::CreateLoad(*builder, is_key_matching_var)));
            }
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            pos = builder->CreateAdd(pos, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                llvm::APInt(32, 1)));
            pos = builder->CreateSRem(pos, capacity);
            LLVM::CreateStore(*builder, pos, pos_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMDictSeparateChaining::resolve_collision(
        llvm::Value* /*capacity*/, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* key_value_pair_linked_list,
        llvm::Type* kv_pair_type, llvm::Value* key_mask,
        llvm::Module& module, ASR::ttype_t* key_asr_type) {
        /**
         * C++ equivalent:
         *
         * chain_itr_prev = nullptr;
         *
         * ll_exists = key_mask_value == 1;
         * if( ll_exists ) {
         *     chain_itr = ll_head;
         * }
         * else {
         *     chain_itr = nullptr;
         * }
         * is_key_matching = 0;
         *
         * while( chain_itr != nullptr && !is_key_matching ) {
         *     is_key_matching = (key == kv_struct_key);
         *     if( !is_key_matching ) {
         *         // update for next iteration
         *         chain_itr_prev = chain_itr;
         *         chain_itr = next_kv_struct;  // (*chain_itr)[2]
         *     }
         * }
         *
         * // now, chain_itr either points to kv or is nullptr
         *
         */

        get_builder0()
        chain_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        chain_itr_prev = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        is_key_matching_var = builder0.CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);

        LLVM::CreateStore(*builder,
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)), chain_itr_prev);
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(key_mask, key_hash));
        llvm_utils->create_if_else(builder->CreateICmpEQ(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1))), [&]() {
            llvm::Value* kv_ll_i8 = builder->CreateBitCast(key_value_pair_linked_list,
                                                            llvm::Type::getInt8PtrTy(context));
            LLVM::CreateStore(*builder, kv_ll_i8, chain_itr);
        }, [&]() {
            LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)), chain_itr);
        });
        LLVM::CreateStore(*builder,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(1, 0)),
            is_key_matching_var
        );
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpNE(
                LLVM::CreateLoad(*builder, chain_itr),
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))
            );
            cond = builder->CreateAnd(cond, builder->CreateNot(LLVM::CreateLoad(
                                      *builder, is_key_matching_var)));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* kv_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);
            llvm::Value* kv_struct = builder->CreateBitCast(kv_struct_i8, kv_pair_type->getPointerTo());
            llvm::Value* kv_struct_key = llvm_utils->create_gep(kv_struct, 0);
            if( !LLVM::is_llvm_struct(key_asr_type) ) {
                kv_struct_key = LLVM::CreateLoad(*builder, kv_struct_key);
            }
            LLVM::CreateStore(*builder, llvm_utils->is_equal_by_value(key, kv_struct_key,
                                module, key_asr_type), is_key_matching_var);
            llvm_utils->create_if_else(builder->CreateNot(LLVM::CreateLoad(*builder, is_key_matching_var)), [&]() {
                LLVM::CreateStore(*builder, kv_struct_i8, chain_itr_prev);
                llvm::Value* next_kv_struct = LLVM::CreateLoad(*builder, llvm_utils->create_gep(kv_struct, 2));
                LLVM::CreateStore(*builder, next_kv_struct, chain_itr);
            }, []() {});
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMDict::resolve_collision_for_write(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* value,
        llvm::Module* module, ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, *module, key_asr_type);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm_utils->list_api->write_item(key_list, pos, key,
                                         key_asr_type, false, module, name2memidx);
        llvm_utils->list_api->write_item(value_list, pos, value,
                                         value_asr_type, false, module, name2memidx);
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(key_mask, pos));
        llvm::Value* is_slot_empty = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
        is_slot_empty = builder->CreateOr(is_slot_empty, builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3))));
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
        is_slot_empty = builder->CreateZExt(is_slot_empty, llvm::Type::getInt32Ty(context));
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        LLVM::CreateStore(*builder, builder->CreateAdd(occupancy, is_slot_empty),
                          occupancy_ptr);
        LLVM::CreateStore(*builder,
                          llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)),
                          llvm_utils->create_ptr_gep(key_mask, pos));
    }

    void LLVMDictOptimizedLinearProbing::resolve_collision_for_write(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* value,
        llvm::Module* module, ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {

        /**
         * C++ equivalent:
         *
         * resolve_collision();     // modifies pos

         * key_list[pos] = key;
         * value_list[pos] = value;

         * key_mask_value = key_mask[pos];
         * is_slot_empty = key_mask_value == 0 || key_mask_value == 3;
         * occupancy += is_slot_empty;

         * linear_prob_happened = (key_hash != pos) || (key_mask[key_hash] == 2);
         * set_max_2 = linear_prob_happened ? 2 : 1;
         * key_mask[key_hash] = set_max_2;
         * key_mask[pos] = set_max_2;
         *
         */

        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, *module, key_asr_type);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm_utils->list_api->write_item(key_list, pos, key,
                                         key_asr_type, false, module, name2memidx);
        llvm_utils->list_api->write_item(value_list, pos, value,
                                         value_asr_type, false, module, name2memidx);

        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(key_mask, pos));
        llvm::Value* is_slot_empty = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
        is_slot_empty = builder->CreateOr(is_slot_empty, builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3))));
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
        is_slot_empty = builder->CreateZExt(is_slot_empty, llvm::Type::getInt32Ty(context));
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        LLVM::CreateStore(*builder, builder->CreateAdd(occupancy, is_slot_empty),
                          occupancy_ptr);

        llvm::Value* linear_prob_happened = builder->CreateICmpNE(key_hash, pos);
        linear_prob_happened = builder->CreateOr(linear_prob_happened,
            builder->CreateICmpEQ(
                LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(key_mask, key_hash)),
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 2)
            ))
        );
        llvm::Value* set_max_2 = builder->CreateSelect(linear_prob_happened,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 2)),
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(key_mask, key_hash));
        LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(key_mask, pos));
    }

    void LLVMDictSeparateChaining::resolve_collision_for_write(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* value,
        llvm::Module* module, ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {

        /**
         * C++ equivalent:
         *
         * kv_linked_list = key_value_pairs[key_hash];
         * resolve_collision(key);   // modifies chain_itr
         * do_insert = chain_itr == nullptr;
         *
         * if( do_insert ) {
         *     if( chain_itr_prev != nullptr ) {
         *         new_kv_struct = malloc(kv_struct_size);
         *         new_kv_struct[0] = key;
         *         new_kv_struct[1] = value;
         *         new_kv_struct[2] = nullptr;
         *         chain_itr_prev[2] = new_kv_struct;
         *     }
         *     else {
         *         kv_linked_list[0] = key;
         *         kv_linked_list[1] = value;
         *         kv_linked_list[2] = nullptr;
         *     }
         *     occupancy += 1;
         * }
         * else {
         *     kv_struct[0] = key;
         *     kv_struct[1] = value;
         * }
         *
         * buckets_filled_delta = key_mask[key_hash] == 0;
         * buckets_filled += buckets_filled_delta;
         * key_mask[key_hash] = 1;
         *
         */

        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_value_pairs = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(dict));
        llvm::Value* key_value_pair_linked_list = llvm_utils->create_ptr_gep(key_value_pairs, key_hash);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Type* kv_struct_type = get_key_value_pair_type(key_asr_type, value_asr_type);
        this->resolve_collision(capacity, key_hash, key, key_value_pair_linked_list,
                                kv_struct_type, key_mask, *module, key_asr_type);
        llvm::Value* kv_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);

        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* do_insert = builder->CreateICmpEQ(kv_struct_i8,
            llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)));
        builder->CreateCondBr(do_insert, thenBB, elseBB);

        builder->SetInsertPoint(thenBB);
        {
            llvm_utils->create_if_else(builder->CreateICmpNE(
                    LLVM::CreateLoad(*builder, chain_itr_prev),
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))), [&]() {
                llvm::DataLayout data_layout(module);
                size_t kv_struct_size = data_layout.getTypeAllocSize(kv_struct_type);
                llvm::Value* malloc_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), kv_struct_size);
                llvm::Value* new_kv_struct_i8 = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
                llvm::Value* new_kv_struct = builder->CreateBitCast(new_kv_struct_i8, kv_struct_type->getPointerTo());
                llvm_utils->deepcopy(key, llvm_utils->create_gep(new_kv_struct, 0), key_asr_type, module, name2memidx);
                llvm_utils->deepcopy(value, llvm_utils->create_gep(new_kv_struct, 1), value_asr_type, module, name2memidx);
                LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)),
                    llvm_utils->create_gep(new_kv_struct, 2));
                llvm::Value* kv_struct_prev_i8 = LLVM::CreateLoad(*builder, chain_itr_prev);
                llvm::Value* kv_struct_prev = builder->CreateBitCast(kv_struct_prev_i8, kv_struct_type->getPointerTo());
                LLVM::CreateStore(*builder, new_kv_struct_i8, llvm_utils->create_gep(kv_struct_prev, 2));
            }, [&]() {
                llvm_utils->deepcopy(key, llvm_utils->create_gep(key_value_pair_linked_list, 0), key_asr_type, module, name2memidx);
                llvm_utils->deepcopy(value, llvm_utils->create_gep(key_value_pair_linked_list, 1), value_asr_type, module, name2memidx);
                LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)),
                    llvm_utils->create_gep(key_value_pair_linked_list, 2));
            });

            llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
            llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
            occupancy = builder->CreateAdd(occupancy,
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1));
            LLVM::CreateStore(*builder, occupancy, occupancy_ptr);
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            llvm::Value* kv_struct = builder->CreateBitCast(kv_struct_i8, kv_struct_type->getPointerTo());
            llvm_utils->deepcopy(key, llvm_utils->create_gep(kv_struct, 0), key_asr_type, module, name2memidx);
            llvm_utils->deepcopy(value, llvm_utils->create_gep(kv_struct, 1), value_asr_type, module, name2memidx);
        }
        llvm_utils->start_new_block(mergeBB);
        llvm::Value* buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(dict);
        llvm::Value* key_mask_value_ptr = llvm_utils->create_ptr_gep(key_mask, key_hash);
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder, key_mask_value_ptr);
        llvm::Value* buckets_filled_delta = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
        llvm::Value* buckets_filled = LLVM::CreateLoad(*builder, buckets_filled_ptr);
        buckets_filled = builder->CreateAdd(
            buckets_filled,
            builder->CreateZExt(buckets_filled_delta, llvm::Type::getInt32Ty(context))
        );
        LLVM::CreateStore(*builder, buckets_filled, buckets_filled_ptr);
        LLVM::CreateStore(*builder,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)),
            key_mask_value_ptr);
    }

    llvm::Value* LLVMDict::resolve_collision_for_read(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* /*value_asr_type*/) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, module, key_asr_type, true);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm::Value* item = llvm_utils->list_api->read_item(value_list, pos, false, module, true);
        return item;
    }

    llvm::Value* LLVMDict::resolve_collision_for_read_with_bound_check(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* /*value_asr_type*/) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, module, key_asr_type, true);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm::Value* is_key_matching = llvm_utils->is_equal_by_value(key,
            llvm_utils->list_api->read_item(key_list, pos, false, module,
                LLVM::is_llvm_struct(key_asr_type)), module, key_asr_type);

        llvm_utils->create_if_else(is_key_matching, [&]() {
        }, [&]() {
            std::string message = "The dict does not contain the specified key";
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("KeyError: %s\n");
            llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
            print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, module, *builder, exit_code);
        });
        llvm::Value* item = llvm_utils->list_api->read_item(value_list, pos,
                                                false, module, false);
        return item;
    }

    void LLVMDict::_check_key_present_or_default(llvm::Module& module, llvm::Value *key, llvm::Value *key_list,
        ASR::ttype_t* key_asr_type, llvm::Value *value_list, llvm::Value *pos,
        llvm::Value *def_value, llvm::Value* &result) {
        llvm::Value* is_key_matching = llvm_utils->is_equal_by_value(key,
            llvm_utils->list_api->read_item(key_list, pos, false, module,
                LLVM::is_llvm_struct(key_asr_type)), module, key_asr_type);
        llvm_utils->create_if_else(is_key_matching, [&]() {
            llvm::Value* item = llvm_utils->list_api->read_item(value_list, pos,
                                                false, module, false);
            LLVM::CreateStore(*builder, item, result);
        }, [=]() {
            LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, def_value), result);
        });
    }

    llvm::Value* LLVMDict::resolve_collision_for_read_with_default(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
        llvm::Value* def_value) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, module, key_asr_type, true);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(key_asr_type),
            ASRUtils::get_type_code(value_asr_type)
        );
        get_builder0()
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        llvm::Value* result = builder0.CreateAlloca(value_type, nullptr);
        _check_key_present_or_default(module, key, key_list, key_asr_type, value_list,
                                        pos, def_value, result);
        return result;
    }

    llvm::Value* LLVMDictOptimizedLinearProbing::resolve_collision_for_read_with_bound_check(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* /*value_asr_type*/) {

        /**
         * C++ equivalent:
         *
         * key_mask_value = key_mask[key_hash];
         * is_prob_not_needed = key_mask_value == 1;
         * if( is_prob_not_needed ) {
         *     is_key_matching = key == key_list[key_hash];
         *     if( is_key_matching ) {
         *         pos = key_hash;
         *     }
         *     else {
         *         exit(1); // key not present
         *     }
         * }
         * else {
         *     resolve_collision(key, for_read=true);  // modifies pos
         * }
         *
         * is_key_matching = key == key_list[pos];
         * if( !is_key_matching ) {
         *     exit(1); // key not present
         * }
         *
         * return value_list[pos];
         */

        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        get_builder0()
        pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                                        llvm_utils->create_ptr_gep(key_mask, key_hash));
        llvm::Value* is_prob_not_neeeded = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        builder->CreateCondBr(is_prob_not_neeeded, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        {
            // A single by value comparison is needed even though
            // we don't need to do linear probing. This is because
            // the user can provide a key which is absent in the dict
            // but is giving the same hash value as one of the keys present in the dict.
            // In the above case we will end up returning value for a key
            // which is not present in the dict. Instead we should return an error
            // which is done in the below code.
            llvm::Value* is_key_matching = llvm_utils->is_equal_by_value(key,
                llvm_utils->list_api->read_item(key_list, key_hash, false, module,
                    LLVM::is_llvm_struct(key_asr_type)), module, key_asr_type);

            llvm_utils->create_if_else(is_key_matching, [=]() {
                LLVM::CreateStore(*builder, key_hash, pos_ptr);
            }, [&]() {
                std::string message = "The dict does not contain the specified key";
                llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("KeyError: %s\n");
                llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
                print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
                int exit_code_int = 1;
                llvm::Value *exit_code = llvm::ConstantInt::get(context,
                        llvm::APInt(32, exit_code_int));
                exit(context, module, *builder, exit_code);
            });
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            this->resolve_collision(capacity, key_hash, key, key_list, key_mask,
                           module, key_asr_type, true);
        }
        llvm_utils->start_new_block(mergeBB);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        // Check if the actual key is present or not
        llvm::Value* is_key_matching = llvm_utils->is_equal_by_value(key,
                llvm_utils->list_api->read_item(key_list, pos, false, module,
                    LLVM::is_llvm_struct(key_asr_type)), module, key_asr_type);

        llvm_utils->create_if_else(is_key_matching, [&]() {
        }, [&]() {
            std::string message = "The dict does not contain the specified key";
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("KeyError: %s\n");
            llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
            print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, module, *builder, exit_code);
        });
        llvm::Value* item = llvm_utils->list_api->read_item(value_list, pos,
                                                        false, module, true);
        return item;
    }

    llvm::Value* LLVMDictOptimizedLinearProbing::resolve_collision_for_read(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* /*value_asr_type*/) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        get_builder0()
        pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                                        llvm_utils->create_ptr_gep(key_mask, key_hash));
        llvm::Value* is_prob_not_neeeded = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        builder->CreateCondBr(is_prob_not_neeeded, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        {
            // A single by value comparison is needed even though
            // we don't need to do linear probing. This is because
            // the user can provide a key which is absent in the dict
            // but is giving the same hash value as one of the keys present in the dict.
            // In the above case we will end up returning value for a key
            // which is not present in the dict. Instead we should return an error
            // which is done in the below code.
            llvm::Value* is_key_matching = llvm_utils->is_equal_by_value(key,
                llvm_utils->list_api->read_item(key_list, key_hash, false, module,
                    LLVM::is_llvm_struct(key_asr_type)), module, key_asr_type);

            llvm_utils->create_if_else(is_key_matching, [=]() {
                LLVM::CreateStore(*builder, key_hash, pos_ptr);
            }, [=]() {
            });
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            this->resolve_collision(capacity, key_hash, key, key_list, key_mask,
                           module, key_asr_type, true);
        }
        llvm_utils->start_new_block(mergeBB);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm::Value* item = llvm_utils->list_api->read_item(value_list, pos,
                                                        false, module, true);
        return item;
    }

    llvm::Value* LLVMDictOptimizedLinearProbing::resolve_collision_for_read_with_default(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
        llvm::Value *def_value) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        get_builder0()
        pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(key_asr_type),
            ASRUtils::get_type_code(value_asr_type)
        );
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        llvm::Value* result = builder0.CreateAlloca(value_type, nullptr);
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                                        llvm_utils->create_ptr_gep(key_mask, key_hash));
        llvm::Value* is_prob_not_neeeded = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        builder->CreateCondBr(is_prob_not_neeeded, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        {
            llvm::Value* is_key_matching = llvm_utils->is_equal_by_value(key,
                llvm_utils->list_api->read_item(key_list, key_hash, false, module,
                    LLVM::is_llvm_struct(key_asr_type)), module, key_asr_type);
            llvm_utils->create_if_else(is_key_matching, [=]() {
                LLVM::CreateStore(*builder, key_hash, pos_ptr);
            }, [=]() {
                LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, def_value), result);
            });
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            this->resolve_collision(capacity, key_hash, key, key_list, key_mask,
                           module, key_asr_type, true);
        }
        llvm_utils->start_new_block(mergeBB);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        _check_key_present_or_default(module, key, key_list, key_asr_type, value_list,
                                        pos, def_value, result);
        return result;
    }

    llvm::Value* LLVMDictSeparateChaining::resolve_collision_for_read(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type) {
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_value_pairs = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(dict));
        llvm::Value* key_value_pair_linked_list = llvm_utils->create_ptr_gep(key_value_pairs, key_hash);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Type* kv_struct_type = get_key_value_pair_type(key_asr_type, value_asr_type);
        this->resolve_collision(capacity, key_hash, key, key_value_pair_linked_list,
                                kv_struct_type, key_mask, module, key_asr_type);
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(key_asr_type),
            ASRUtils::get_type_code(value_asr_type)
        );
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        get_builder0()
        tmp_value_ptr = builder0.CreateAlloca(value_type, nullptr);
        llvm::Value* kv_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);
        llvm::Value* kv_struct = builder->CreateBitCast(kv_struct_i8, kv_struct_type->getPointerTo());
        llvm::Value* value = LLVM::CreateLoad(*builder, llvm_utils->create_gep(kv_struct, 1));
        LLVM::CreateStore(*builder, value, tmp_value_ptr);
        return tmp_value_ptr;
    }

    llvm::Value* LLVMDictSeparateChaining::resolve_collision_for_read_with_bound_check(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type) {
        /**
         * C++ equivalent:
         *
         * resolve_collision(key);   // modified chain_itr
         * does_kv_exist = key_mask[key_hash] == 1 && chain_itr != nullptr;
         * if( !does_key_exist ) {
         *     exit(1); // KeyError
         * }
         *
         */

        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_value_pairs = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(dict));
        llvm::Value* key_value_pair_linked_list = llvm_utils->create_ptr_gep(key_value_pairs, key_hash);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Type* kv_struct_type = get_key_value_pair_type(key_asr_type, value_asr_type);
        this->resolve_collision(capacity, key_hash, key, key_value_pair_linked_list,
                                kv_struct_type, key_mask, module, key_asr_type);
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(key_asr_type),
            ASRUtils::get_type_code(value_asr_type)
        );
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        get_builder0()
        tmp_value_ptr = builder0.CreateAlloca(value_type, nullptr);
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(key_mask, key_hash));
        llvm::Value* does_kv_exists = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        does_kv_exists = builder->CreateAnd(does_kv_exists,
            builder->CreateICmpNE(LLVM::CreateLoad(*builder, chain_itr),
            llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)))
        );

        llvm_utils->create_if_else(does_kv_exists, [&]() {
            llvm::Value* kv_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);
            llvm::Value* kv_struct = builder->CreateBitCast(kv_struct_i8, kv_struct_type->getPointerTo());
            llvm::Value* value = LLVM::CreateLoad(*builder, llvm_utils->create_gep(kv_struct, 1));
            LLVM::CreateStore(*builder, value, tmp_value_ptr);
        }, [&]() {
            std::string message = "The dict does not contain the specified key";
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("KeyError: %s\n");
            llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
            print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, module, *builder, exit_code);
        });
        return tmp_value_ptr;
    }

    llvm::Value* LLVMDictSeparateChaining::resolve_collision_for_read_with_default(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type, llvm::Value *def_value) {
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_value_pairs = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(dict));
        llvm::Value* key_value_pair_linked_list = llvm_utils->create_ptr_gep(key_value_pairs, key_hash);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Type* kv_struct_type = get_key_value_pair_type(key_asr_type, value_asr_type);
        this->resolve_collision(capacity, key_hash, key, key_value_pair_linked_list,
                                kv_struct_type, key_mask, module, key_asr_type);
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(key_asr_type),
            ASRUtils::get_type_code(value_asr_type)
        );
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        get_builder0()
        tmp_value_ptr = builder0.CreateAlloca(value_type, nullptr);
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(key_mask, key_hash));
        llvm::Value* does_kv_exists = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        does_kv_exists = builder->CreateAnd(does_kv_exists,
            builder->CreateICmpNE(LLVM::CreateLoad(*builder, chain_itr),
            llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)))
        );

        llvm_utils->create_if_else(does_kv_exists, [&]() {
            llvm::Value* kv_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);
            llvm::Value* kv_struct = builder->CreateBitCast(kv_struct_i8, kv_struct_type->getPointerTo());
            llvm::Value* value = LLVM::CreateLoad(*builder, llvm_utils->create_gep(kv_struct, 1));
            LLVM::CreateStore(*builder, value, tmp_value_ptr);
        }, [&]() {
            LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, def_value), tmp_value_ptr);
        });
        return tmp_value_ptr;
    }

    llvm::Value* LLVMDictInterface::get_key_hash(llvm::Value* capacity, llvm::Value* key,
        ASR::ttype_t* key_asr_type, llvm::Module& module) {
        // Write specialised hash functions for intrinsic types
        // This is to avoid unnecessary calls to C-runtime and do
        // as much as possible in LLVM directly.
        switch( key_asr_type->type ) {
            case ASR::ttypeType::Integer: {
                // Simple modulo with the capacity of the dict.
                // We can update it later to do a better hash function
                // which produces lesser collisions.

                llvm::Value* int_hash = builder->CreateZExtOrTrunc(
                    builder->CreateURem(key,
                    builder->CreateZExtOrTrunc(capacity, key->getType())),
                    capacity->getType()
                );
                return int_hash;
            }
            case ASR::ttypeType::Character: {
                // Polynomial rolling hash function for strings
                llvm::Value* null_char = llvm::ConstantInt::get(llvm::Type::getInt8Ty(context),
                                                                llvm::APInt(8, '\0'));
                llvm::Value* p = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 31));
                llvm::Value* m = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 100000009));
                get_builder0()
                hash_value = builder0.CreateAlloca(llvm::Type::getInt64Ty(context), nullptr, "hash_value");
                hash_iter = builder0.CreateAlloca(llvm::Type::getInt64Ty(context), nullptr, "hash_iter");
                polynomial_powers = builder0.CreateAlloca(llvm::Type::getInt64Ty(context), nullptr, "p_pow");
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 0)),
                    hash_value);
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 1)),
                    polynomial_powers);
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 0)),
                    hash_iter);
                llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
                llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
                llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

                // head
                llvm_utils->start_new_block(loophead);
                {
                    llvm::Value* i = LLVM::CreateLoad(*builder, hash_iter);
                    llvm::Value* c = LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(key, i));
                    llvm::Value *cond = builder->CreateICmpNE(c, null_char);
                    builder->CreateCondBr(cond, loopbody, loopend);
                }

                // body
                llvm_utils->start_new_block(loopbody);
                {
                    // for c in key:
                    //     hash_value = (hash_value + (ord(c) + 1) * p_pow) % m
                    //     p_pow = (p_pow * p) % m
                    llvm::Value* i = LLVM::CreateLoad(*builder, hash_iter);
                    llvm::Value* c = LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(key, i));
                    llvm::Value* p_pow = LLVM::CreateLoad(*builder, polynomial_powers);
                    llvm::Value* hash = LLVM::CreateLoad(*builder, hash_value);
                    c = builder->CreateZExt(c, llvm::Type::getInt64Ty(context));
                    c = builder->CreateAdd(c, llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 1)));
                    c = builder->CreateMul(c, p_pow);
                    c = builder->CreateSRem(c, m);
                    hash = builder->CreateAdd(hash, c);
                    hash = builder->CreateSRem(hash, m);
                    LLVM::CreateStore(*builder, hash, hash_value);
                    p_pow = builder->CreateMul(p_pow, p);
                    p_pow = builder->CreateSRem(p_pow, m);
                    LLVM::CreateStore(*builder, p_pow, polynomial_powers);
                    i = builder->CreateAdd(i, llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 1)));
                    LLVM::CreateStore(*builder, i, hash_iter);
                }

                builder->CreateBr(loophead);

                // end
                llvm_utils->start_new_block(loopend);
                llvm::Value* hash = LLVM::CreateLoad(*builder, hash_value);
                hash = builder->CreateTrunc(hash, llvm::Type::getInt32Ty(context));
                return builder->CreateSRem(hash, capacity);
            }
            case ASR::ttypeType::Tuple: {
                llvm::Value* tuple_hash = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0));
                ASR::Tuple_t* asr_tuple = ASR::down_cast<ASR::Tuple_t>(key_asr_type);
                for( size_t i = 0; i < asr_tuple->n_type; i++ ) {
                    llvm::Value* llvm_tuple_i = llvm_utils->tuple_api->read_item(key, i,
                                                    LLVM::is_llvm_struct(asr_tuple->m_type[i]));
                    tuple_hash = builder->CreateAdd(tuple_hash, get_key_hash(capacity, llvm_tuple_i,
                                                                             asr_tuple->m_type[i], module));
                    tuple_hash = builder->CreateSRem(tuple_hash, capacity);
                }
                return tuple_hash;
            }
            case ASR::ttypeType::Logical: {
                // (int32_t)key % capacity
                // modulo is required for the case when dict has a single key, `True`
                llvm::Value* key_i32 = builder->CreateZExt(key, llvm::Type::getInt32Ty(context));
                llvm::Value* logical_hash = builder->CreateZExtOrTrunc(
                    builder->CreateURem(key_i32,
                    builder->CreateZExtOrTrunc(capacity, key_i32->getType())),
                    capacity->getType()
                );
                return logical_hash;
            }
            default: {
                throw LCompilersException("Hashing " + ASRUtils::type_to_str_python(key_asr_type) +
                                          " isn't implemented yet.");
            }
        }
    }

    void LLVMDict::rehash(llvm::Value* dict, llvm::Module* module,
        ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        get_builder0()

        llvm::Value* capacity_ptr = get_pointer_to_capacity(dict);
        llvm::Value* old_capacity = LLVM::CreateLoad(*builder, capacity_ptr);
        llvm::Value* capacity = builder->CreateMul(old_capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 2)));
        capacity = builder->CreateAdd(capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 1)));
        LLVM::CreateStore(*builder, capacity, capacity_ptr);

        std::string key_type_code = ASRUtils::get_type_code(key_asr_type);
        std::string value_type_code = ASRUtils::get_type_code(value_asr_type);
        std::pair<std::string, std::string> dict_type_key = std::make_pair(key_type_code, value_type_code);
        llvm::Type* key_llvm_type = std::get<2>(typecode2dicttype[dict_type_key]).first;
        llvm::Type* value_llvm_type = std::get<2>(typecode2dicttype[dict_type_key]).second;
        int32_t key_type_size = std::get<1>(typecode2dicttype[dict_type_key]).first;
        int32_t value_type_size = std::get<1>(typecode2dicttype[dict_type_key]).second;

        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* new_key_list = builder0.CreateAlloca(llvm_utils->list_api->get_list_type(key_llvm_type,
                                                          key_type_code, key_type_size), nullptr);
        llvm_utils->list_api->list_init(key_type_code, new_key_list, *module, capacity, capacity);

        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* new_value_list = builder0.CreateAlloca(llvm_utils->list_api->get_list_type(value_llvm_type,
                                                            value_type_code, value_type_size), nullptr);
        llvm_utils->list_api->list_init(value_type_code, new_value_list, *module, capacity, capacity);

        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* new_key_mask = LLVM::lfortran_calloc(context, *module, *builder, capacity,
                                                          llvm_mask_size);

        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        idx_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
            llvm::APInt(32, 0)), idx_ptr);

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(old_capacity, LLVM::CreateLoad(*builder, idx_ptr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* idx = LLVM::CreateLoad(*builder, idx_ptr);
            llvm::Function *fn = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
            llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
            llvm::Value* is_key_set = LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(key_mask, idx));
            is_key_set = builder->CreateICmpNE(is_key_set,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            builder->CreateCondBr(is_key_set, thenBB, elseBB);
            builder->SetInsertPoint(thenBB);
            {
                llvm::Value* key = llvm_utils->list_api->read_item(key_list, idx,
                        false, *module, LLVM::is_llvm_struct(key_asr_type));
                llvm::Value* value = llvm_utils->list_api->read_item(value_list,
                    idx, false, *module, LLVM::is_llvm_struct(value_asr_type));
                llvm::Value* key_hash = get_key_hash(current_capacity, key, key_asr_type, *module);
                this->resolve_collision(current_capacity, key_hash, key, new_key_list,
                               new_key_mask, *module, key_asr_type);
                llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
                llvm::Value* key_dest = llvm_utils->list_api->read_item(
                                    new_key_list, pos, false, *module, true);
                llvm_utils->deepcopy(key, key_dest, key_asr_type, module, name2memidx);
                llvm::Value* value_dest = llvm_utils->list_api->read_item(
                                    new_value_list, pos, false, *module, true);
                llvm_utils->deepcopy(value, value_dest, value_asr_type, module, name2memidx);

                llvm::Value* linear_prob_happened = builder->CreateICmpNE(key_hash, pos);
                llvm::Value* set_max_2 = builder->CreateSelect(linear_prob_happened,
                    llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 2)),
                    llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
                LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(new_key_mask, key_hash));
                LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(new_key_mask, pos));
            }
            builder->CreateBr(mergeBB);

            llvm_utils->start_new_block(elseBB);
            llvm_utils->start_new_block(mergeBB);
            idx = builder->CreateAdd(idx, llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, idx, idx_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        // TODO: Free key_list, value_list and key_mask
        llvm_utils->list_api->free_data(key_list, *module);
        llvm_utils->list_api->free_data(value_list, *module);
        LLVM::lfortran_free(context, *module, *builder, key_mask);
        LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, new_key_list), key_list);
        LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, new_value_list), value_list);
        LLVM::CreateStore(*builder, new_key_mask, get_pointer_to_keymask(dict));
    }

    void LLVMDictSeparateChaining::rehash(
        llvm::Value* dict, llvm::Module* module,
        ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        get_builder0()
        old_capacity = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        old_occupancy = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        old_number_of_buckets_filled = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        idx_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        old_key_value_pairs = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        old_key_mask = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        llvm::Value* capacity_ptr = get_pointer_to_capacity(dict);
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
        llvm::Value* number_of_buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(dict);
        llvm::Value* old_capacity_value = LLVM::CreateLoad(*builder, capacity_ptr);
        LLVM::CreateStore(*builder, old_capacity_value, old_capacity);
        LLVM::CreateStore(*builder,
            LLVM::CreateLoad(*builder, occupancy_ptr),
            old_occupancy
        );
        LLVM::CreateStore(*builder,
            LLVM::CreateLoad(*builder, number_of_buckets_filled_ptr),
            old_number_of_buckets_filled
        );
        llvm::Value* old_key_mask_value = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* old_key_value_pairs_value = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(dict));
        old_key_value_pairs_value = builder->CreateBitCast(old_key_value_pairs_value, llvm::Type::getInt8PtrTy(context));
        LLVM::CreateStore(*builder, old_key_mask_value, old_key_mask);
        LLVM::CreateStore(*builder, old_key_value_pairs_value, old_key_value_pairs);

        llvm::Value* capacity = builder->CreateMul(old_capacity_value, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 3)));
        capacity = builder->CreateAdd(capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 1)));
        dict_init_given_initial_capacity(ASRUtils::get_type_code(key_asr_type),
                                         ASRUtils::get_type_code(value_asr_type),
                                         dict, module, capacity);
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB_rehash = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB_rehash = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB_rehash = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* rehash_flag = LLVM::CreateLoad(*builder, get_pointer_to_rehash_flag(dict));
        builder->CreateCondBr(rehash_flag, thenBB_rehash, elseBB_rehash);
        builder->SetInsertPoint(thenBB_rehash);
        old_key_value_pairs_value = LLVM::CreateLoad(*builder, old_key_value_pairs);
        old_key_value_pairs_value = builder->CreateBitCast(old_key_value_pairs_value,
            get_key_value_pair_type(key_asr_type, value_asr_type)->getPointerTo());
        old_key_mask_value = LLVM::CreateLoad(*builder, old_key_mask);
        old_capacity_value = LLVM::CreateLoad(*builder, old_capacity);
        capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0)), idx_ptr);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(
                                        old_capacity_value,
                                        LLVM::CreateLoad(*builder, idx_ptr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* itr = LLVM::CreateLoad(*builder, idx_ptr);
            llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(old_key_mask_value, itr));
            llvm::Value* is_key_set = builder->CreateICmpEQ(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));

            llvm_utils->create_if_else(is_key_set, [&]() {
                llvm::Value* srci = llvm_utils->create_ptr_gep(old_key_value_pairs_value, itr);
                write_key_value_pair_linked_list(srci, dict, capacity, key_asr_type, value_asr_type, module, name2memidx);
            }, [=]() {
            });
            llvm::Value* tmp = builder->CreateAdd(
                        itr,
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, idx_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
        builder->CreateBr(mergeBB_rehash);
        llvm_utils->start_new_block(elseBB_rehash);
        {
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_capacity),
                get_pointer_to_capacity(dict)
            );
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_occupancy),
                get_pointer_to_occupancy(dict)
            );
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_number_of_buckets_filled),
                get_pointer_to_number_of_filled_buckets(dict)
            );
            LLVM::CreateStore(*builder,
                builder->CreateBitCast(
                    LLVM::CreateLoad(*builder, old_key_value_pairs),
                    get_key_value_pair_type(key_asr_type, value_asr_type)->getPointerTo()
                ),
                get_pointer_to_key_value_pairs(dict)
            );
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_key_mask),
                get_pointer_to_keymask(dict)
            );
        }
        llvm_utils->start_new_block(mergeBB_rehash);
    }

    void LLVMDict::rehash_all_at_once_if_needed(llvm::Value* dict, llvm::Module* module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        /**
         * C++ equivalent:
         *
         * // this condition will be true with 0 capacity too
         * rehash_condition = 5 * occupancy >= 3 * capacity;
         * if( rehash_condition ) {
         *     rehash();
         * }
         *
         */

        llvm::Value* occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        // Threshold hash is chosen from https://en.wikipedia.org/wiki/Hash_table#Load_factor
        // occupancy / capacity >= 0.6 is same as 5 * occupancy >= 3 * capacity
        llvm::Value* occupancy_times_5 = builder->CreateMul(occupancy, llvm::ConstantInt::get(
                                llvm::Type::getInt32Ty(context), llvm::APInt(32, 5)));
        llvm::Value* capacity_times_3 = builder->CreateMul(capacity, llvm::ConstantInt::get(
                                llvm::Type::getInt32Ty(context), llvm::APInt(32, 3)));
        llvm_utils->create_if_else(builder->CreateICmpSGE(occupancy_times_5,
                                    capacity_times_3), [&]() {
            rehash(dict, module, key_asr_type, value_asr_type, name2memidx);
        }, []() {});
    }

    void LLVMDictSeparateChaining::rehash_all_at_once_if_needed(
        llvm::Value* dict, llvm::Module* module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {

        /**
         * C++ equivalent:
         *
         * // this condition will be true with 0 buckets_filled too
         * rehash_condition = rehash_flag && (occupancy >= 2 * buckets_filled);
         * if( rehash_condition ) {
         *     rehash();
         * }
         *
         */

        llvm::Value* occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(dict));
        llvm::Value* buckets_filled = LLVM::CreateLoad(*builder, get_pointer_to_number_of_filled_buckets(dict));
        llvm::Value* rehash_condition = LLVM::CreateLoad(*builder, get_pointer_to_rehash_flag(dict));
        llvm::Value* buckets_filled_times_2 = builder->CreateMul(buckets_filled, llvm::ConstantInt::get(
                                llvm::Type::getInt32Ty(context), llvm::APInt(32, 2)));
        rehash_condition = builder->CreateAnd(rehash_condition,
            builder->CreateICmpSGE(occupancy, buckets_filled_times_2));
        llvm_utils->create_if_else(rehash_condition, [&]() {
            rehash(dict, module, key_asr_type, value_asr_type, name2memidx);
        }, [=]() {
        });
    }

    void LLVMDictInterface::write_item(llvm::Value* dict, llvm::Value* key,
                              llvm::Value* value, llvm::Module* module,
                              ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                              std::map<std::string, std::map<std::string, int>>& name2memidx) {
        rehash_all_at_once_if_needed(dict, module, key_asr_type, value_asr_type, name2memidx);
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, key_asr_type, *module);
        this->resolve_collision_for_write(dict, key_hash, key, value, module,
                                          key_asr_type, value_asr_type, name2memidx);
        // A second rehash ensures that the threshold is not breached at any point.
        // It can be shown mathematically that rehashing twice would only occur for small dictionaries,
        // for example, for threshold set in linear probing, it occurs only when len(dict) <= 2
        rehash_all_at_once_if_needed(dict, module, key_asr_type, value_asr_type, name2memidx);
    }

    llvm::Value* LLVMDict::read_item(llvm::Value* dict, llvm::Value* key,
                             llvm::Module& module, ASR::Dict_t* dict_type, bool enable_bounds_checking,
                             bool get_pointer) {
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, dict_type->m_key_type, module);
        llvm::Value* value_ptr;
        if (enable_bounds_checking) {
            value_ptr = this->resolve_collision_for_read_with_bound_check(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type);
        } else {
            value_ptr = this->resolve_collision_for_read(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type);
        }
        if( get_pointer ) {
            return value_ptr;
        }
        return LLVM::CreateLoad(*builder, value_ptr);
    }

    llvm::Value* LLVMDict::get_item(llvm::Value* dict, llvm::Value* key,
                             llvm::Module& module, ASR::Dict_t* dict_type, llvm::Value* def_value,
                             bool get_pointer) {
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, dict_type->m_key_type, module);
        llvm::Value* value_ptr = this->resolve_collision_for_read_with_default(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type,
                                                                  def_value);
        if( get_pointer ) {
            return value_ptr;
        }
        return LLVM::CreateLoad(*builder, value_ptr);
    }

    llvm::Value* LLVMDictSeparateChaining::read_item(llvm::Value* dict, llvm::Value* key,
        llvm::Module& module, ASR::Dict_t* dict_type, bool enable_bounds_checking, bool get_pointer) {
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, dict_type->m_key_type, module);
        llvm::Value* value_ptr;
        if (enable_bounds_checking) {
            value_ptr = this->resolve_collision_for_read_with_bound_check(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type);
        } else {
            value_ptr = this->resolve_collision_for_read(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type);
        }
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(dict_type->m_key_type),
            ASRUtils::get_type_code(dict_type->m_value_type)
        );
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        value_ptr = builder->CreateBitCast(value_ptr, value_type->getPointerTo());
        if( get_pointer ) {
            return value_ptr;
        }
        return LLVM::CreateLoad(*builder, value_ptr);
    }

    llvm::Value* LLVMDictSeparateChaining::get_item(llvm::Value* dict, llvm::Value* key,
        llvm::Module& module, ASR::Dict_t* dict_type, llvm::Value* def_value, bool get_pointer) {
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, dict_type->m_key_type, module);
        llvm::Value* value_ptr = this->resolve_collision_for_read_with_default(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type,
                                                                  def_value);
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(dict_type->m_key_type),
            ASRUtils::get_type_code(dict_type->m_value_type)
        );
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        value_ptr = builder->CreateBitCast(value_ptr, value_type->getPointerTo());
        if( get_pointer ) {
            return value_ptr;
        }
        return LLVM::CreateLoad(*builder, value_ptr);
    }

    llvm::Value* LLVMDict::pop_item(llvm::Value* dict, llvm::Value* key,
        llvm::Module& module, ASR::Dict_t* dict_type,
        bool get_pointer) {
        /**
         * C++ equivalent:
         *
         * resolve_collision_for_read_with_bound_check(key);  // modifies pos
         * key_mask[pos] = 3;    // tombstone marker
         * occupancy -= 1;
         */

        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, dict_type->m_key_type, module);
        llvm::Value* value_ptr = this->resolve_collision_for_read_with_bound_check(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* key_mask_i = llvm_utils->create_ptr_gep(key_mask, pos);
        llvm::Value* tombstone_marker = llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3));
        LLVM::CreateStore(*builder, tombstone_marker, key_mask_i);

        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        occupancy = builder->CreateSub(occupancy, llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
        LLVM::CreateStore(*builder, occupancy, occupancy_ptr);

        if( get_pointer ) {
            std::string key_type_code = ASRUtils::get_type_code(dict_type->m_key_type);
            std::string value_type_code = ASRUtils::get_type_code(dict_type->m_value_type);
            llvm::Type* llvm_value_type = std::get<2>(typecode2dicttype[std::make_pair(
                key_type_code, value_type_code)]).second;
            get_builder0()
            llvm::Value* return_ptr = builder0.CreateAlloca(llvm_value_type, nullptr);
            LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, value_ptr), return_ptr);
            return return_ptr;
        }

        return LLVM::CreateLoad(*builder, value_ptr);
    }

    llvm::Value* LLVMDictSeparateChaining::pop_item(
        llvm::Value* dict, llvm::Value* key,
        llvm::Module& module, ASR::Dict_t* dict_type,
        bool get_pointer) {
        /**
         * C++ equivalent:
         *
         * // modifies chain_itr and chain_itr_prev
         * resolve_collision_for_read_with_bound_check(key);
         *
         * if(chain_itr_prev != nullptr) {
         *     chain_itr_prev[2] = chain_itr[2]; // next
         * }
         * else {
         *     // head of linked list removed
         *     if( chain_itr[2] == nullptr ) {
         *         // this linked list is now empty
         *         key_mask[key_hash] = 0;
         *         num_buckets_filled--;
         *     }
         *     else {
         *         // not empty yet
         *         key_value_pairs[key_hash] = chain_itr[2];
         *     }
         * }
         *
         * occupancy--;
         *
         */

        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, dict_type->m_key_type, module);
        llvm::Value* value_ptr = this->resolve_collision_for_read_with_bound_check(dict, key_hash, key, module,
                                                                  dict_type->m_key_type, dict_type->m_value_type);
        std::pair<std::string, std::string> llvm_key = std::make_pair(
            ASRUtils::get_type_code(dict_type->m_key_type),
            ASRUtils::get_type_code(dict_type->m_value_type)
        );
        llvm::Type* value_type = std::get<2>(typecode2dicttype[llvm_key]).second;
        value_ptr = builder->CreateBitCast(value_ptr, value_type->getPointerTo());
        llvm::Value* prev = LLVM::CreateLoad(*builder, chain_itr_prev);
        llvm::Value* found = LLVM::CreateLoad(*builder, chain_itr);
        llvm::Type* kv_struct_type = get_key_value_pair_type(dict_type->m_key_type, dict_type->m_value_type);
        found = builder->CreateBitCast(found, kv_struct_type->getPointerTo());
        llvm::Value* found_next = LLVM::CreateLoad(*builder, llvm_utils->create_gep(found, 2));

        llvm_utils->create_if_else(builder->CreateICmpNE(prev,
                        llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))), [&]() {
            prev = builder->CreateBitCast(prev, kv_struct_type->getPointerTo());
            LLVM::CreateStore(*builder, found_next, llvm_utils->create_gep(prev, 2));
        }, [&]() {
            llvm_utils->create_if_else(builder->CreateICmpEQ(found_next,
                        llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))), [&]() {
                llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
                LLVM::CreateStore(
                    *builder,
                    llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)),
                    llvm_utils->create_ptr_gep(key_mask, key_hash)
                );
                llvm::Value* num_buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(dict);
                llvm::Value* num_buckets_filled = LLVM::CreateLoad(*builder, num_buckets_filled_ptr);
                num_buckets_filled = builder->CreateSub(num_buckets_filled, llvm::ConstantInt::get(
                            llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
                LLVM::CreateStore(*builder, num_buckets_filled, num_buckets_filled_ptr);
            }, [&]() {
                found_next = builder->CreateBitCast(found_next, kv_struct_type->getPointerTo());
                llvm::Value* key_value_pairs = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(dict));
                LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, found_next),
                                    llvm_utils->create_ptr_gep(key_value_pairs, key_hash));
            });
        });

        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        occupancy = builder->CreateSub(occupancy, llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
        LLVM::CreateStore(*builder, occupancy, occupancy_ptr);

        if( get_pointer ) {
            std::string key_type_code = ASRUtils::get_type_code(dict_type->m_key_type);
            std::string value_type_code = ASRUtils::get_type_code(dict_type->m_value_type);
            llvm::Type* llvm_value_type = std::get<2>(typecode2dicttype[std::make_pair(
                key_type_code, value_type_code)]).second;
            get_builder0()
            llvm::Value* return_ptr = builder0.CreateAlloca(llvm_value_type, nullptr);
            LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, value_ptr), return_ptr);
            return return_ptr;
        }

        return LLVM::CreateLoad(*builder, value_ptr);
    }

    void LLVMDict::get_elements_list(llvm::Value* dict,
        llvm::Value* elements_list, ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type, llvm::Module& module,
        std::map<std::string, std::map<std::string, int>>& name2memidx,
        bool key_or_value) {

        /**
         * C++ equivalent:
         *
         * // key_or_value = 0 for keys, 1 for values
         *
         * idx = 0;
         *
         * while( capacity > idx ) {
         *     el = key_or_value_list[idx];
         *     key_mask_value = key_mask[idx];
         *
         *     is_key_skip = key_mask_value == 3;     // tombstone
         *     is_key_set = key_mask_value != 0;
         *     add_el = is_key_set && !is_key_skip;
         *     if( add_el ) {
         *         elements_list.append(el);
         *     }
         *
         *     idx++;
         * }
         *
         */

        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* el_list = key_or_value == 0 ? get_key_list(dict) : get_value_list(dict);
        ASR::ttype_t* el_asr_type = key_or_value == 0 ? key_asr_type : value_asr_type;
        get_builder0();
        idx_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
            llvm::APInt(32, 0)), idx_ptr);

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(capacity, LLVM::CreateLoad(*builder, idx_ptr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* idx = LLVM::CreateLoad(*builder, idx_ptr);
            llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(key_mask, idx));
            llvm::Value* is_key_skip = builder->CreateICmpEQ(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3)));
            llvm::Value* is_key_set = builder->CreateICmpNE(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));

            llvm::Value* add_el = builder->CreateAnd(is_key_set,
                                            builder->CreateNot(is_key_skip));
            llvm_utils->create_if_else(add_el, [&]() {
                llvm::Value* el = llvm_utils->list_api->read_item(el_list, idx,
                        false, module, LLVM::is_llvm_struct(el_asr_type));
                llvm_utils->list_api->append(elements_list, el,
                                             el_asr_type, &module, name2memidx);
            }, [=]() {
            });

            idx = builder->CreateAdd(idx, llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, idx, idx_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMDictSeparateChaining::get_elements_list(llvm::Value* dict,
        llvm::Value* elements_list, ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type, llvm::Module& module,
        std::map<std::string, std::map<std::string, int>>& name2memidx,
        bool key_or_value) {
        get_builder0()
        idx_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        chain_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                            llvm::APInt(32, 0)), idx_ptr);

        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* key_value_pairs = LLVM::CreateLoad(*builder, get_pointer_to_key_value_pairs(dict));
        llvm::Type* kv_pair_type = get_key_value_pair_type(key_asr_type, value_asr_type);
        ASR::ttype_t* el_asr_type = key_or_value == 0 ? key_asr_type : value_asr_type;
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(
                                        capacity,
                                        LLVM::CreateLoad(*builder, idx_ptr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* idx = LLVM::CreateLoad(*builder, idx_ptr);
            llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(key_mask, idx));
            llvm::Value* is_key_set = builder->CreateICmpEQ(key_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));

            llvm_utils->create_if_else(is_key_set, [&]() {
                llvm::Value* dict_i = llvm_utils->create_ptr_gep(key_value_pairs, idx);
                llvm::Value* kv_ll_i8 = builder->CreateBitCast(dict_i, llvm::Type::getInt8PtrTy(context));
                LLVM::CreateStore(*builder, kv_ll_i8, chain_itr);

                llvm::BasicBlock *loop2head = llvm::BasicBlock::Create(context, "loop2.head");
                llvm::BasicBlock *loop2body = llvm::BasicBlock::Create(context, "loop2.body");
                llvm::BasicBlock *loop2end = llvm::BasicBlock::Create(context, "loop2.end");

                // head
                llvm_utils->start_new_block(loop2head);
                {
                    llvm::Value *cond = builder->CreateICmpNE(
                        LLVM::CreateLoad(*builder, chain_itr),
                        llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))
                    );
                    builder->CreateCondBr(cond, loop2body, loop2end);
                }

                // body
                llvm_utils->start_new_block(loop2body);
                {
                    llvm::Value* kv_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);
                    llvm::Value* kv_struct = builder->CreateBitCast(kv_struct_i8, kv_pair_type->getPointerTo());
                    llvm::Value* kv_el = llvm_utils->create_gep(kv_struct, key_or_value);
                    if( !LLVM::is_llvm_struct(el_asr_type) ) {
                        kv_el = LLVM::CreateLoad(*builder, kv_el);
                    }
                    llvm_utils->list_api->append(elements_list, kv_el,
                                                 el_asr_type, &module, name2memidx);
                    llvm::Value* next_kv_struct = LLVM::CreateLoad(*builder, llvm_utils->create_gep(kv_struct, 2));
                    LLVM::CreateStore(*builder, next_kv_struct, chain_itr);
                }

                builder->CreateBr(loop2head);

                // end
                llvm_utils->start_new_block(loop2end);
            }, [=]() {
            });
            llvm::Value* tmp = builder->CreateAdd(idx,
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, idx_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    llvm::Value* LLVMList::read_item(llvm::Value* list, llvm::Value* pos,
                                     bool enable_bounds_checking,
                                     llvm::Module& module, bool get_pointer) {
        if( enable_bounds_checking ) {
            check_index_within_bounds(list, pos, module);
        }
        llvm::Value* list_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        llvm::Value* element_ptr = llvm_utils->create_ptr_gep(list_data, pos);
        if( get_pointer ) {
            return element_ptr;
        }
        return LLVM::CreateLoad(*builder, element_ptr);
    }

    llvm::Value* LLVMList::len(llvm::Value* list) {
        return LLVM::CreateLoad(*builder, get_pointer_to_current_end_point(list));
    }

    llvm::Value* LLVMDict::len(llvm::Value* dict) {
        return LLVM::CreateLoad(*builder, get_pointer_to_occupancy(dict));
    }

    llvm::Value* LLVMDictSeparateChaining::len(llvm::Value* dict) {
        return LLVM::CreateLoad(*builder, get_pointer_to_occupancy(dict)) ;
    }

    bool LLVMDictInterface::is_dict_present() {
        return is_dict_present_;
    }

    void LLVMDictInterface::set_is_dict_present(bool value) {
        is_dict_present_ = value;
    }

    LLVMDictInterface::~LLVMDictInterface() {
        typecode2dicttype.clear();
    }

    LLVMDict::~LLVMDict() {
    }

    LLVMDictSeparateChaining::~LLVMDictSeparateChaining() {
    }

    LLVMDictOptimizedLinearProbing::~LLVMDictOptimizedLinearProbing() {}

    void LLVMList::resize_if_needed(llvm::Value* list, llvm::Value* n,
                                    llvm::Value* capacity, int32_t type_size,
                                    llvm::Type* el_type, llvm::Module* module) {
        llvm::Value *cond = builder->CreateICmpEQ(n, capacity);
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        builder->CreateCondBr(cond, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        llvm::Value* new_capacity = builder->CreateMul(llvm::ConstantInt::get(context,
                                                       llvm::APInt(32, 2)), capacity);
        new_capacity = builder->CreateAdd(new_capacity, llvm::ConstantInt::get(context,
                                                        llvm::APInt(32, 1)));
        llvm::Value* arg_size = builder->CreateMul(llvm::ConstantInt::get(context,
                                                   llvm::APInt(32, type_size)),
                                                   new_capacity);
        llvm::Value* copy_data_ptr = get_pointer_to_list_data(list);
        llvm::Value* copy_data = LLVM::CreateLoad(*builder, copy_data_ptr);
        copy_data = LLVM::lfortran_realloc(context, *module, *builder,
                                           copy_data, arg_size);
        copy_data = builder->CreateBitCast(copy_data, el_type->getPointerTo());
        builder->CreateStore(copy_data, copy_data_ptr);
        builder->CreateStore(new_capacity, get_pointer_to_current_capacity(list));
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        llvm_utils->start_new_block(mergeBB);
    }

    void LLVMList::shift_end_point_by_one(llvm::Value* list) {
        llvm::Value* end_point_ptr = get_pointer_to_current_end_point(list);
        llvm::Value* end_point = LLVM::CreateLoad(*builder, end_point_ptr);
        end_point = builder->CreateAdd(end_point, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
        builder->CreateStore(end_point, end_point_ptr);
    }

    void LLVMList::append(llvm::Value* list, llvm::Value* item,
                          ASR::ttype_t* asr_type, llvm::Module* module,
                          std::map<std::string, std::map<std::string, int>>& name2memidx) {
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder, get_pointer_to_current_end_point(list));
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_current_capacity(list));
        std::string type_code = ASRUtils::get_type_code(asr_type);
        int type_size = std::get<1>(typecode2listtype[type_code]);
        llvm::Type* el_type = std::get<2>(typecode2listtype[type_code]);
        resize_if_needed(list, current_end_point, current_capacity,
                         type_size, el_type, module);
        write_item(list, current_end_point, item, asr_type, false, module, name2memidx);
        shift_end_point_by_one(list);
    }

    void LLVMList::insert_item(llvm::Value* list, llvm::Value* pos,
                               llvm::Value* item, ASR::ttype_t* asr_type,
                               llvm::Module* module,
                               std::map<std::string, std::map<std::string, int>>& name2memidx) {
        std::string type_code = ASRUtils::get_type_code(asr_type);
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder,
                                        get_pointer_to_current_end_point(list));
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder,
                                        get_pointer_to_current_capacity(list));
        int type_size = std::get<1>(typecode2listtype[type_code]);
        llvm::Type* el_type = std::get<2>(typecode2listtype[type_code]);
        resize_if_needed(list, current_end_point, current_capacity,
                         type_size, el_type, module);

        /* While loop equivalent in C++:
         *  end_point         // nth index of list
         *  pos               // ith index to insert the element
         *  pos_ptr = pos;
         *  tmp_ptr = list[pos];
         *  tmp = 0;
         *
         * while(end_point > pos_ptr) {
         *      tmp           = list[pos + 1];
         *      list[pos + 1] = tmp_ptr;
         *      tmp_ptr       = tmp;
         *      pos_ptr++;
         *  }
         *
         * list[pos] = item;
         */

        // TODO: Should be created outside the user loop and not here.
        // LLVMList should treat them as data members and create them
        // only if they are NULL
        get_builder0()
        llvm::AllocaInst *tmp_ptr = builder0.CreateAlloca(el_type, nullptr);
        LLVM::CreateStore(*builder, read_item(list, pos, false, *module, false), tmp_ptr);
        llvm::Value* tmp = nullptr;

        // TODO: Should be created outside the user loop and not here.
        // LLVMList should treat them as data members and create them
        // only if they are NULL
        llvm::AllocaInst *pos_ptr = builder0.CreateAlloca(
                                    llvm::Type::getInt32Ty(context), nullptr);
        LLVM::CreateStore(*builder, pos, pos_ptr);

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(
                                        current_end_point,
                                        LLVM::CreateLoad(*builder, pos_ptr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* next_index = builder->CreateAdd(
                            LLVM::CreateLoad(*builder, pos_ptr),
                            llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            tmp = read_item(list, next_index, false, *module, false);
            write_item(list, next_index, LLVM::CreateLoad(*builder, tmp_ptr), false, *module);
            LLVM::CreateStore(*builder, tmp, tmp_ptr);

            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, pos_ptr),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, pos_ptr);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        write_item(list, pos, item, asr_type, false, module, name2memidx);
        shift_end_point_by_one(list);
    }

    void LLVMList::reserve(llvm::Value* list, llvm::Value* n,
                           ASR::ttype_t* asr_type, llvm::Module* module) {
        /**
         * C++ equivalent
         *
         * if( n > current_capacity ) {
         *     list_data = realloc(list_data, sizeof(el_type) * n);
         * }
         *
         */
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_current_capacity(list));
        std::string type_code = ASRUtils::get_type_code(asr_type);
        int type_size = std::get<1>(typecode2listtype[type_code]);
        llvm::Type* el_type = std::get<2>(typecode2listtype[type_code]);
        llvm_utils->create_if_else(builder->CreateICmpSGT(n, capacity), [&]() {
            llvm::Value* arg_size = builder->CreateMul(llvm::ConstantInt::get(context,
                                                    llvm::APInt(32, type_size)), n);
            llvm::Value* copy_data_ptr = get_pointer_to_list_data(list);
            llvm::Value* copy_data = LLVM::CreateLoad(*builder, copy_data_ptr);
            copy_data = LLVM::lfortran_realloc(context, *module, *builder,
                                            copy_data, arg_size);
            copy_data = builder->CreateBitCast(copy_data, el_type->getPointerTo());
            builder->CreateStore(copy_data, copy_data_ptr);
            builder->CreateStore(n, get_pointer_to_current_capacity(list));
        }, []() {});
    }

    void LLVMList::reverse(llvm::Value* list, llvm::Module& module) {

        /* Equivalent in C++:
         *
         * int i = 0;
         * int j = end_point - 1;
         *
         * tmp;
         *
         * while(j > i) {
         *      tmp = list[i];
         *      list[i] = list[j];
         *      list[j] = tmp;
         *      i = i + 1;
         *      j = j - 1;
         *  }
         */

        llvm::Value* end_point = LLVM::CreateLoad(*builder,
                                        get_pointer_to_current_end_point(list));

        llvm::Type* pos_type = llvm::Type::getInt32Ty(context);
        get_builder0()
        llvm::AllocaInst *i = builder0.CreateAlloca(pos_type, nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), i);       // i = 0
        llvm::AllocaInst *j = builder0.CreateAlloca(pos_type, nullptr);
        llvm::Value* tmp = nullptr;
        tmp = builder->CreateSub(end_point, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
        LLVM::CreateStore(*builder, tmp, j);        // j = end_point - 1

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(LLVM::CreateLoad(*builder, j), LLVM::CreateLoad(*builder, i));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            tmp = read_item(list, LLVM::CreateLoad(*builder, i),
                false, module, false);    // tmp = list[i]
            write_item(list, LLVM::CreateLoad(*builder, i),
                        read_item(list, LLVM::CreateLoad(*builder, j),
                        false, module, false),
                        false, module);    // list[i] = list[j]
            write_item(list, LLVM::CreateLoad(*builder, j),
                        tmp, false, module);    // list[j] = tmp

            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, i),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, i);
            tmp = builder->CreateSub(
                        LLVM::CreateLoad(*builder, j),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, j);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    llvm::Value* LLVMList::find_item_position(llvm::Value* list,
        llvm::Value* item, ASR::ttype_t* item_type, llvm::Module& module,
        llvm::Value* start, llvm::Value* end) {
        llvm::Type* pos_type = llvm::Type::getInt32Ty(context);

        // TODO: Should be created outside the user loop and not here.
        // LLVMList should treat them as data members and create them
        // only if they are NULL
        get_builder0()
        llvm::AllocaInst *i = builder0.CreateAlloca(pos_type, nullptr);
        if(start) {
            LLVM::CreateStore(*builder, start, i);
        }
        else {
            LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                context, llvm::APInt(32, 0)), i);
        }
        llvm::Value* end_point = nullptr;
        if(end) {
            end_point = end;
        }
        else {
            end_point = LLVM::CreateLoad(*builder,
                            get_pointer_to_current_end_point(list));
        }
        llvm::Value* tmp = nullptr;

        /* Equivalent in C++:
         * int i = start;
         * while(list[i] != item && end_point > i) {
         *     i++;
         * }
         *
         * if (i == end_point) {
         *    std::cout << "The list does not contain the element";
         * }
         */

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value* left_arg = read_item(list, LLVM::CreateLoad(*builder, i),
                false, module, LLVM::is_llvm_struct(item_type));
            llvm::Value* is_item_not_equal = builder->CreateNot(
                                                llvm_utils->is_equal_by_value(
                                                    left_arg, item,
                                                    module, item_type)
                                            );
            llvm::Value *cond = builder->CreateAnd(is_item_not_equal,
                                                   builder->CreateICmpSGT(end_point,
                                                    LLVM::CreateLoad(*builder, i)));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, i),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, i);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        llvm::Value* cond = builder->CreateICmpEQ(
                              LLVM::CreateLoad(*builder, i), end_point);
        llvm::Value* start_greater_than_end = builder->CreateICmpSGE(
                                                LLVM::CreateLoad(*builder, i), end_point);
        llvm::Value* condition = builder->CreateOr(cond, start_greater_than_end);
        llvm_utils->create_if_else(condition, [&]() {
            std::string message = "The list does not contain the element: ";
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("ValueError: %s%d\n");
            llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
            print_error(context, module, *builder, {fmt_ptr, fmt_ptr2, item});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, module, *builder, exit_code);
        }, [=]() {
        });
        return LLVM::CreateLoad(*builder, i);
    }

    llvm::Value* LLVMList::index(llvm::Value* list, llvm::Value* item,
                                llvm::Value* start, llvm::Value* end,
                                ASR::ttype_t* item_type, llvm::Module& module) {
        return LLVMList::find_item_position(list, item, item_type, module, start, end);
    }

    llvm::Value* LLVMList::count(llvm::Value* list, llvm::Value* item,
                                ASR::ttype_t* item_type, llvm::Module& module) {
        llvm::Type* pos_type = llvm::Type::getInt32Ty(context);
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder,
                                        get_pointer_to_current_end_point(list));
        get_builder0()
        llvm::AllocaInst *i = builder0.CreateAlloca(pos_type, nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), i);
        llvm::AllocaInst *cnt = builder0.CreateAlloca(pos_type, nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), cnt);
        llvm::Value* tmp = nullptr;

        /* Equivalent in C++:
         * int i = 0;
         * int cnt = 0;
         * while(end_point > i) {
         *     if(list[i] == item) {
         *         tmp = cnt+1;
         *         cnt = tmp;
         *     }
         *     tmp = i+1;
         *     i = tmp;
         * }
         */

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(current_end_point,
                                         LLVM::CreateLoad(*builder, i));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            // if occurrence found, increment cnt
            llvm::Value* left_arg = read_item(list, LLVM::CreateLoad(*builder, i),
                false, module, LLVM::is_llvm_struct(item_type));
            llvm::Value* cond = llvm_utils->is_equal_by_value(left_arg, item, module, item_type);
            llvm_utils->create_if_else(cond, [&]() {
                tmp = builder->CreateAdd(
                            LLVM::CreateLoad(*builder, cnt),
                            llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                LLVM::CreateStore(*builder, tmp, cnt);
            }, [=]() {
            });
            // increment i
            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, i),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, i);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        return LLVM::CreateLoad(*builder, cnt);
    }

    void LLVMList::remove(llvm::Value* list, llvm::Value* item,
                          ASR::ttype_t* item_type, llvm::Module& module) {
        get_builder0()

        llvm::Type* pos_type = llvm::Type::getInt32Ty(context);
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder,
                                        get_pointer_to_current_end_point(list));
        // TODO: Should be created outside the user loop and not here.
        // LLVMList should treat them as data members and create them
        // only if they are NULL
        llvm::AllocaInst *item_pos = builder0.CreateAlloca(pos_type, nullptr);
        llvm::Value* tmp = LLVMList::find_item_position(list, item, item_type, module);
        LLVM::CreateStore(*builder, tmp, item_pos);

        /* While loop equivalent in C++:
         * item_pos = find_item_position();
         * while(end_point > item_pos) {
         *     tmp = item_pos + 1;
         *     list[item_pos] = list[tmp];
         *     item_pos = tmp;
         * }
         */

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(current_end_point,
                                         LLVM::CreateLoad(*builder, item_pos));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, item_pos),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            write_item(list, LLVM::CreateLoad(*builder, item_pos),
                read_item(list, tmp, false, module, false), false, module);
            LLVM::CreateStore(*builder, tmp, item_pos);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        // Decrement end point by one
        llvm::Value* end_point_ptr = get_pointer_to_current_end_point(list);
        llvm::Value* end_point = LLVM::CreateLoad(*builder, end_point_ptr);
        end_point = builder->CreateSub(end_point, llvm::ConstantInt::get(
                                       context, llvm::APInt(32, 1)));
        builder->CreateStore(end_point, end_point_ptr);
    }

    llvm::Value* LLVMList::pop_last(llvm::Value* list, ASR::ttype_t* list_type, llvm::Module& module) {
        // If list is empty, output error

        llvm::Value* end_point_ptr = get_pointer_to_current_end_point(list);
        llvm::Value* end_point = LLVM::CreateLoad(*builder, end_point_ptr);

        llvm::Value* cond = builder->CreateICmpEQ(llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), end_point);
        llvm_utils->create_if_else(cond, [&]() {
            std::string message = "pop from empty list";
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("IndexError: %s\n");
            llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
            print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, module, *builder, exit_code);
        }, [=]() {
        });

        // Get last element of list
        llvm::Value* tmp = builder->CreateSub(end_point, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 1)));
        tmp = read_item(list, tmp, false, module, LLVM::is_llvm_struct(list_type));

        // Decrement end point by one
        end_point = builder->CreateSub(end_point, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 1)));
        builder->CreateStore(end_point, end_point_ptr);
        return tmp;
    }

    llvm::Value* LLVMList::pop_position(llvm::Value* list, llvm::Value* pos,
        ASR::ttype_t* list_element_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        get_builder0()
        /* Equivalent in C++:
         * while(end_point > pos + 1) {
         *     tmp = pos + 1;
         *     list[pos] = list[tmp];
         *     pos = tmp;
         * }
         */

        llvm::Value* end_point_ptr = get_pointer_to_current_end_point(list);
        llvm::Value* end_point = LLVM::CreateLoad(*builder, end_point_ptr);

        llvm::AllocaInst *pos_ptr = builder0.CreateAlloca(
                                    llvm::Type::getInt32Ty(context), nullptr);
        LLVM::CreateStore(*builder, pos, pos_ptr);
        llvm::Value* tmp = nullptr;

        // Get element to return
        llvm::Value* item = read_item(list, LLVM::CreateLoad(*builder, pos_ptr),
                                      true, *module, LLVM::is_llvm_struct(list_element_type));
        if( LLVM::is_llvm_struct(list_element_type) ) {
            std::string list_element_type_code = ASRUtils::get_type_code(list_element_type);
            LCOMPILERS_ASSERT(typecode2listtype.find(list_element_type_code) != typecode2listtype.end());
            llvm::AllocaInst *target = builder0.CreateAlloca(
                std::get<2>(typecode2listtype[list_element_type_code]), nullptr,
                "pop_position_item");
            llvm_utils->deepcopy(item, target, list_element_type, module, name2memidx);
            item = target;
        }

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(end_point, builder->CreateAdd(
                                    LLVM::CreateLoad(*builder, pos_ptr),
                                    llvm::ConstantInt::get(context, llvm::APInt(32, 1))));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, pos_ptr),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            write_item(list, LLVM::CreateLoad(*builder, pos_ptr),
                read_item(list, tmp, false, *module, false), false, *module);
            LLVM::CreateStore(*builder, tmp, pos_ptr);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        // Decrement end point by one
        end_point = builder->CreateSub(end_point, llvm::ConstantInt::get(
                                       context, llvm::APInt(32, 1)));
        builder->CreateStore(end_point, end_point_ptr);

        return item;
    }

    void LLVMList::list_clear(llvm::Value* list) {
        llvm::Value* end_point_ptr = get_pointer_to_current_end_point(list);
        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                   llvm::APInt(32, 0));
        LLVM::CreateStore(*builder, zero, end_point_ptr);
    }

    void LLVMList::free_data(llvm::Value* list, llvm::Module& module) {
        llvm::Value* data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        LLVM::lfortran_free(context, module, *builder, data);
    }

    llvm::Value* LLVMList::check_list_equality(llvm::Value* l1, llvm::Value* l2,
                                                ASR::ttype_t* item_type,
                                                 llvm::LLVMContext& context,
                                                 llvm::IRBuilder<>* builder,
                                                 llvm::Module& module) {
        get_builder0()
        llvm::AllocaInst *is_equal = builder0.CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(context, llvm::APInt(1, 1)), is_equal);
        llvm::Value *a_len = llvm_utils->list_api->len(l1);
        llvm::Value *b_len = llvm_utils->list_api->len(l2);
        llvm::Value *cond = builder->CreateICmpEQ(a_len, b_len);
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        builder->CreateCondBr(cond, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        llvm::AllocaInst *idx = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), idx);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

            // head
           llvm_utils->start_new_block(loophead);
            {
                llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                llvm::Value* cnd = builder->CreateICmpSLT(i, a_len);
                builder->CreateCondBr(cnd, loopbody, loopend);
            }

            // body
            llvm_utils->start_new_block(loopbody);
            {
                llvm::Value* i = LLVM::CreateLoad(*builder, idx);
                llvm::Value* left_arg = llvm_utils->list_api->read_item(l1, i,
                        false, module, LLVM::is_llvm_struct(item_type));
                llvm::Value* right_arg = llvm_utils->list_api->read_item(l2, i,
                        false, module, LLVM::is_llvm_struct(item_type));
                llvm::Value* res = llvm_utils->is_equal_by_value(left_arg, right_arg, module,
                                        item_type);
                res = builder->CreateAnd(LLVM::CreateLoad(*builder, is_equal), res);
                LLVM::CreateStore(*builder, res, is_equal);
                i = builder->CreateAdd(i, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                        llvm::APInt(32, 1)));
                LLVM::CreateStore(*builder, i, idx);
            }

            builder->CreateBr(loophead);

            // end
            llvm_utils->start_new_block(loopend);

        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(context, llvm::APInt(1, 0)), is_equal);
        llvm_utils->start_new_block(mergeBB);
        return LLVM::CreateLoad(*builder, is_equal);
    }

    llvm::Value* LLVMList::check_list_inequality(llvm::Value* l1, llvm::Value* l2,
                                                 ASR::ttype_t* item_type,
                                                 llvm::LLVMContext& context,
                                                 llvm::IRBuilder<>* builder,
                                                 llvm::Module& module, int8_t overload_id,
                                                 ASR::ttype_t* int32_type) {
        /**
         * Equivalent in C++
         *
         * equality_holds = 1;
         * inequality_holds = 0;
         * i = 0;
         *
         * while( i < a_len && i < b_len && equality_holds ) {
         *     equality_holds &= (a[i] == b[i]);
         *     inequality_holds |= (a[i] op b[i]);
         *     i++;
         * }
         *
         * if( (i == a_len || i == b_len) && equality_holds ) {
         *     inequality_holds = a_len op b_len;
         * }
         *
         */

        get_builder0()
        llvm::AllocaInst *equality_holds = builder0.CreateAlloca(
                                                llvm::Type::getInt1Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(context, llvm::APInt(1, 1)),
                          equality_holds);
        llvm::AllocaInst *inequality_holds = builder0.CreateAlloca(
                                                llvm::Type::getInt1Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(context, llvm::APInt(1, 0)),
                          inequality_holds);

        llvm::Value *a_len = llvm_utils->list_api->len(l1);
        llvm::Value *b_len = llvm_utils->list_api->len(l2);
        llvm::AllocaInst *idx = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), idx);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value* i = LLVM::CreateLoad(*builder, idx);
            llvm::Value* cnd = builder->CreateICmpSLT(i, a_len);
            cnd = builder->CreateAnd(cnd, builder->CreateICmpSLT(i, b_len));
            cnd = builder->CreateAnd(cnd, LLVM::CreateLoad(*builder, equality_holds));
            builder->CreateCondBr(cnd, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* i = LLVM::CreateLoad(*builder, idx);
            llvm::Value* left_arg = llvm_utils->list_api->read_item(l1, i,
                    false, module, LLVM::is_llvm_struct(item_type));
            llvm::Value* right_arg = llvm_utils->list_api->read_item(l2, i,
                    false, module, LLVM::is_llvm_struct(item_type));
            llvm::Value* res = llvm_utils->is_ineq_by_value(left_arg, right_arg, module,
                                    item_type, overload_id);
            res = builder->CreateOr(LLVM::CreateLoad(*builder, inequality_holds), res);
            LLVM::CreateStore(*builder, res, inequality_holds);
            res = llvm_utils->is_equal_by_value(left_arg, right_arg, module,
                                    item_type);
            res = builder->CreateAnd(LLVM::CreateLoad(*builder, equality_holds), res);
            LLVM::CreateStore(*builder, res, equality_holds);
            i = builder->CreateAdd(i, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                    llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, i, idx);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        llvm::Value* cond = builder->CreateICmpEQ(LLVM::CreateLoad(*builder, idx),
                                                  a_len);
        cond = builder->CreateOr(cond, builder->CreateICmpEQ(
                                 LLVM::CreateLoad(*builder, idx), b_len));
        cond = builder->CreateAnd(cond, LLVM::CreateLoad(*builder, equality_holds));
        llvm_utils->create_if_else(cond, [&]() {
            LLVM::CreateStore(*builder, llvm_utils->is_ineq_by_value(a_len, b_len,
                module, int32_type, overload_id), inequality_holds);
        }, []() {
            // LLVM::CreateStore(*builder, llvm::ConstantInt::get(
            //     context, llvm::APInt(1, 0)), inequality_holds);
        });

        return LLVM::CreateLoad(*builder, inequality_holds);
    }

    void LLVMList::list_repeat_copy(llvm::Value* repeat_list, llvm::Value* init_list,
                                    llvm::Value* num_times, llvm::Value* init_list_len,
                                    llvm::Module* module) {
        get_builder0()
        llvm::Type* pos_type = llvm::Type::getInt32Ty(context);
        llvm::AllocaInst *i = builder0.CreateAlloca(pos_type, nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), i);       // i = 0
        llvm::AllocaInst *j = builder0.CreateAlloca(pos_type, nullptr);
        llvm::Value* tmp = nullptr;

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(num_times,
                                                       LLVM::CreateLoad(*builder, i));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                        context, llvm::APInt(32, 0)), j);       // j = 0

            llvm::BasicBlock *loop2head = llvm::BasicBlock::Create(context, "loop2.head");
            llvm::BasicBlock *loop2body = llvm::BasicBlock::Create(context, "loop2.body");
            llvm::BasicBlock *loop2end = llvm::BasicBlock::Create(context, "loop2.end");

            // head
            llvm_utils->start_new_block(loop2head);
            {
                llvm::Value *cond2 = builder->CreateICmpSGT(init_list_len,
                                                            LLVM::CreateLoad(*builder, j));
                builder->CreateCondBr(cond2, loop2body, loop2end);
            }

            // body
            llvm_utils->start_new_block(loop2body);
            {
                tmp = builder->CreateMul(init_list_len, LLVM::CreateLoad(*builder, i));
                tmp = builder->CreateAdd(tmp, LLVM::CreateLoad(*builder, j));
                write_item(repeat_list, tmp,
                           read_item(init_list, LLVM::CreateLoad(*builder, j),
                                     false, *module, false),
                           false, *module);
                tmp = builder->CreateAdd(
                            LLVM::CreateLoad(*builder, j),
                            llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                LLVM::CreateStore(*builder, tmp, j);
            }
            builder->CreateBr(loop2head);

            // end
            llvm_utils->start_new_block(loop2end);

            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, i),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, i);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    LLVMTuple::LLVMTuple(llvm::LLVMContext& context_,
                         LLVMUtils* llvm_utils_,
                         llvm::IRBuilder<>* builder_) :
    context(context_), llvm_utils(llvm_utils_), builder(builder_) {}

    llvm::Type* LLVMTuple::get_tuple_type(std::string& type_code,
                                          std::vector<llvm::Type*>& el_types) {
        if( typecode2tupletype.find(type_code) != typecode2tupletype.end() ) {
            return typecode2tupletype[type_code].first;
        }

        llvm::Type* llvm_tuple_type = llvm::StructType::create(context, el_types, "tuple");
        typecode2tupletype[type_code] = std::make_pair(llvm_tuple_type, el_types.size());
        return llvm_tuple_type;
    }

    llvm::Value* LLVMTuple::read_item(llvm::Value* llvm_tuple, llvm::Value* pos,
                                      bool get_pointer) {
        llvm::Value* item = llvm_utils->create_gep(llvm_tuple, pos);
        if( get_pointer ) {
            return item;
        }
        return LLVM::CreateLoad(*builder, item);
    }

    llvm::Value* LLVMTuple::read_item(llvm::Value* llvm_tuple, size_t pos,
                                      bool get_pointer) {
        llvm::Value* llvm_pos = llvm::ConstantInt::get(context, llvm::APInt(32, pos));
        return read_item(llvm_tuple, llvm_pos, get_pointer);
    }

    void LLVMTuple::tuple_init(llvm::Value* llvm_tuple, std::vector<llvm::Value*>& values,
        ASR::Tuple_t* tuple_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        for( size_t i = 0; i < values.size(); i++ ) {
            llvm::Value* item_ptr = read_item(llvm_tuple, i, true);
            llvm_utils->deepcopy(values[i], item_ptr,
                                 tuple_type->m_type[i], module,
                                 name2memidx);
        }
    }

    void LLVMTuple::tuple_deepcopy(llvm::Value* src, llvm::Value* dest,
        ASR::Tuple_t* tuple_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        LCOMPILERS_ASSERT(src->getType() == dest->getType());
        for( size_t i = 0; i < tuple_type->n_type; i++ ) {
            llvm::Value* src_item = read_item(src, i, LLVM::is_llvm_struct(
                                              tuple_type->m_type[i]));
            llvm::Value* dest_item_ptr = read_item(dest, i, true);
            llvm_utils->deepcopy(src_item, dest_item_ptr,
                                 tuple_type->m_type[i], module,
                                 name2memidx);
        }
    }

    llvm::Value* LLVMTuple::check_tuple_equality(llvm::Value* t1, llvm::Value* t2,
                                                 ASR::Tuple_t* tuple_type,
                                                 llvm::LLVMContext& context,
                                                 llvm::IRBuilder<>* builder,
                                                 llvm::Module& module) {
        llvm::Value* is_equal = llvm::ConstantInt::get(context, llvm::APInt(1, 1));
        for( size_t i = 0; i < tuple_type->n_type; i++ ) {
            llvm::Value* t1i = llvm_utils->tuple_api->read_item(t1, i, LLVM::is_llvm_struct(
                                              tuple_type->m_type[i]));
            llvm::Value* t2i = llvm_utils->tuple_api->read_item(t2, i, LLVM::is_llvm_struct(
                                              tuple_type->m_type[i]));
            llvm::Value* is_t1_eq_t2 = llvm_utils->is_equal_by_value(t1i, t2i, module,
                                        tuple_type->m_type[i]);
            is_equal = builder->CreateAnd(is_equal, is_t1_eq_t2);
        }
        return is_equal;
    }

    llvm::Value* LLVMTuple::check_tuple_inequality(llvm::Value* t1, llvm::Value* t2,
                                                 ASR::Tuple_t* tuple_type,
                                                 llvm::LLVMContext& context,
                                                 llvm::IRBuilder<>* builder,
                                                 llvm::Module& module, int8_t overload_id) {
        /**
         * Equivalent in C++
         *
         * equality_holds = 1;
         * inequality_holds = 0;
         * i = 0;
         *
         * // owing to compile-time access of indices,
         * // loop is unrolled into multiple if statements
         * while( i < a_len && equality_holds ) {
         *     inequality_holds |= (a[i] op b[i]);
         *     equality_holds &= (a[i] == b[i]);
         *     i++;
         * }
         *
         * return inequality_holds;
         *
         */

        get_builder0()
        llvm::AllocaInst *equality_holds = builder0.CreateAlloca(
                                                llvm::Type::getInt1Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(context, llvm::APInt(1, 1)),
                          equality_holds);
        llvm::AllocaInst *inequality_holds = builder0.CreateAlloca(
                                                llvm::Type::getInt1Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(context, llvm::APInt(1, 0)),
                          inequality_holds);

        for( size_t i = 0; i < tuple_type->n_type; i++ ) {
            llvm_utils->create_if_else(LLVM::CreateLoad(*builder, equality_holds), [&]() {
                llvm::Value* t1i = llvm_utils->tuple_api->read_item(t1, i, LLVM::is_llvm_struct(
                                                tuple_type->m_type[i]));
                llvm::Value* t2i = llvm_utils->tuple_api->read_item(t2, i, LLVM::is_llvm_struct(
                                                tuple_type->m_type[i]));
                llvm::Value* res = llvm_utils->is_ineq_by_value(t1i, t2i, module,
                                                tuple_type->m_type[i], overload_id);
                res = builder->CreateOr(LLVM::CreateLoad(*builder, inequality_holds), res);
                LLVM::CreateStore(*builder, res, inequality_holds);
                res = llvm_utils->is_equal_by_value(t1i, t2i, module, tuple_type->m_type[i]);
                res = builder->CreateAnd(LLVM::CreateLoad(*builder, equality_holds), res);
                LLVM::CreateStore(*builder, res, equality_holds);
            }, [](){});
        }

        return LLVM::CreateLoad(*builder, inequality_holds);
    }

    void LLVMTuple::concat(llvm::Value* t1, llvm::Value* t2, ASR::Tuple_t* tuple_type_1,
                           ASR::Tuple_t* tuple_type_2, llvm::Value* concat_tuple,
                           ASR::Tuple_t* concat_tuple_type, llvm::Module& module,
                           std::map<std::string, std::map<std::string, int>>& name2memidx) {
        std::vector<llvm::Value*> values;
        for( size_t i = 0; i < tuple_type_1->n_type; i++ ) {
            values.push_back(llvm_utils->tuple_api->read_item(t1, i,
                LLVM::is_llvm_struct(tuple_type_1->m_type[i])));
        }
        for( size_t i = 0; i < tuple_type_2->n_type; i++ ) {
            values.push_back(llvm_utils->tuple_api->read_item(t2, i,
                LLVM::is_llvm_struct(tuple_type_2->m_type[i])));
        }
        tuple_init(concat_tuple, values, concat_tuple_type,
                   &module, name2memidx);
    }

    LLVMSetInterface::LLVMSetInterface(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        context(context_),
        llvm_utils(std::move(llvm_utils_)),
        builder(std::move(builder_)),
        pos_ptr(nullptr), is_el_matching_var(nullptr),
        idx_ptr(nullptr), hash_iter(nullptr),
        hash_value(nullptr), polynomial_powers(nullptr),
        chain_itr(nullptr), chain_itr_prev(nullptr),
        old_capacity(nullptr), old_elems(nullptr),
        old_el_mask(nullptr), is_set_present_(false) {
    }

    bool LLVMSetInterface::is_set_present() {
        return is_set_present_;
    }

    void LLVMSetInterface::set_is_set_present(bool value) {
        is_set_present_ = value;
    }

    LLVMSetLinearProbing::LLVMSetLinearProbing(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        LLVMSetInterface(context_, llvm_utils_, builder_) {
    }

    LLVMSetSeparateChaining::LLVMSetSeparateChaining(
        llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        LLVMSetInterface(context_, llvm_utils_, builder_) {
    }

    LLVMSetInterface::~LLVMSetInterface() {
        typecode2settype.clear();
    }

    LLVMSetLinearProbing::~LLVMSetLinearProbing() {
    }

    LLVMSetSeparateChaining::~LLVMSetSeparateChaining() {
    }

    llvm::Value* LLVMSetLinearProbing::get_pointer_to_occupancy(llvm::Value* set) {
        return llvm_utils->create_gep(set, 0);
    }

    llvm::Value* LLVMSetLinearProbing::get_pointer_to_capacity(llvm::Value* set) {
        return llvm_utils->list_api->get_pointer_to_current_capacity(
                            get_el_list(set));
    }

    llvm::Value* LLVMSetLinearProbing::get_el_list(llvm::Value* set) {
        return llvm_utils->create_gep(set, 1);
    }

    llvm::Value* LLVMSetLinearProbing::get_pointer_to_mask(llvm::Value* set) {
        return llvm_utils->create_gep(set, 2);
    }

    llvm::Value* LLVMSetSeparateChaining::get_el_list(llvm::Value* /*set*/) {
        return nullptr;
    }

    llvm::Value* LLVMSetSeparateChaining::get_pointer_to_occupancy(llvm::Value* set) {
        return llvm_utils->create_gep(set, 0);
    }

    llvm::Value* LLVMSetSeparateChaining::get_pointer_to_number_of_filled_buckets(llvm::Value* set) {
        return llvm_utils->create_gep(set, 1);
    }

    llvm::Value* LLVMSetSeparateChaining::get_pointer_to_capacity(llvm::Value* set) {
        return llvm_utils->create_gep(set, 2);
    }

    llvm::Value* LLVMSetSeparateChaining::get_pointer_to_elems(llvm::Value* set) {
        return llvm_utils->create_gep(set, 3);
    }

    llvm::Value* LLVMSetSeparateChaining::get_pointer_to_mask(llvm::Value* set) {
        return llvm_utils->create_gep(set, 4);
    }

    llvm::Value* LLVMSetSeparateChaining::get_pointer_to_rehash_flag(llvm::Value* set) {
        return llvm_utils->create_gep(set, 5);
    }

    llvm::Type* LLVMSetLinearProbing::get_set_type(std::string type_code, int32_t type_size,
        llvm::Type* el_type) {
        is_set_present_ = true;
        if( typecode2settype.find(type_code) != typecode2settype.end() ) {
            return std::get<0>(typecode2settype[type_code]);
        }

        llvm::Type* el_list_type = llvm_utils->list_api->get_list_type(el_type,
                                        type_code, type_size);
        std::vector<llvm::Type*> set_type_vec = {llvm::Type::getInt32Ty(context),
                                                el_list_type,
                                                llvm::Type::getInt8PtrTy(context)};
        llvm::Type* set_desc = llvm::StructType::create(context, set_type_vec, "set");
        typecode2settype[type_code] = std::make_tuple(set_desc, type_size, el_type);
        return set_desc;
    }

    llvm::Type* LLVMSetSeparateChaining::get_set_type(
        std::string el_type_code, int32_t el_type_size, llvm::Type* el_type) {
        is_set_present_ = true;
        if( typecode2settype.find(el_type_code) != typecode2settype.end() ) {
            return std::get<0>(typecode2settype[el_type_code]);
        }

        std::vector<llvm::Type*> el_vec = {el_type, llvm::Type::getInt8PtrTy(context)};
        llvm::Type* elstruct = llvm::StructType::create(context, el_vec, "el");
        std::vector<llvm::Type*> set_type_vec = {llvm::Type::getInt32Ty(context),
                                                  llvm::Type::getInt32Ty(context),
                                                  llvm::Type::getInt32Ty(context),
                                                  elstruct->getPointerTo(),
                                                  llvm::Type::getInt8PtrTy(context),
                                                  llvm::Type::getInt1Ty(context)};
        llvm::Type* set_desc = llvm::StructType::create(context, set_type_vec, "set");
        typecode2settype[el_type_code] = std::make_tuple(set_desc, el_type_size, el_type);
        typecode2elstruct[el_type_code] = elstruct;
        return set_desc;
    }

    void LLVMSetLinearProbing::set_init(std::string type_code, llvm::Value* set,
                           llvm::Module* module, size_t initial_capacity) {
        llvm::Value* n_ptr = get_pointer_to_occupancy(set);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                           llvm::APInt(32, 0)), n_ptr);
        llvm::Value* el_list = get_el_list(set);
        llvm_utils->list_api->list_init(type_code, el_list, *module,
                                        initial_capacity, initial_capacity);
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_capacity = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, initial_capacity));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* el_mask = LLVM::lfortran_calloc(context, *module, *builder, llvm_capacity,
                                                      llvm_mask_size);
        LLVM::CreateStore(*builder, el_mask, get_pointer_to_mask(set));
    }

    void LLVMSetSeparateChaining::set_init(
        std::string el_type_code, llvm::Value* set,
        llvm::Module* module, size_t initial_capacity) {
        llvm::Value* llvm_capacity = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                        llvm::APInt(32, initial_capacity));
        llvm::Value* rehash_flag_ptr = get_pointer_to_rehash_flag(set);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
                            llvm::APInt(1, 1)), rehash_flag_ptr);
        set_init_given_initial_capacity(el_type_code, set, module, llvm_capacity);
    }

    void LLVMSetSeparateChaining::set_init_given_initial_capacity(
        std::string el_type_code, llvm::Value* set,
        llvm::Module* module, llvm::Value* llvm_capacity) {
        llvm::Value* rehash_flag_ptr = get_pointer_to_rehash_flag(set);
        llvm::Value* rehash_flag = LLVM::CreateLoad(*builder, rehash_flag_ptr);
        llvm::Value* llvm_zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0));
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(set);
        LLVM::CreateStore(*builder, llvm_zero, occupancy_ptr);
        llvm::Value* num_buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(set);
        LLVM::CreateStore(*builder, llvm_zero, num_buckets_filled_ptr);

        llvm::DataLayout data_layout(module);
        llvm::Type* el_type = typecode2elstruct[el_type_code];
        size_t el_type_size = data_layout.getTypeAllocSize(el_type);
        llvm::Value* llvm_el_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, el_type_size));
        llvm::Value* malloc_size = builder->CreateMul(llvm_capacity, llvm_el_size);
        llvm::Value* el_ptr = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
        rehash_flag = builder->CreateAnd(rehash_flag,
                        builder->CreateICmpNE(el_ptr,
                        llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)))
                    );
        el_ptr = builder->CreateBitCast(el_ptr, el_type->getPointerTo());
        LLVM::CreateStore(*builder, el_ptr, get_pointer_to_elems(set));

        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* el_mask = LLVM::lfortran_calloc(context, *module, *builder, llvm_capacity,
                                                      llvm_mask_size);
        rehash_flag = builder->CreateAnd(rehash_flag,
                        builder->CreateICmpNE(el_mask,
                        llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)))
                    );
        LLVM::CreateStore(*builder, el_mask, get_pointer_to_mask(set));

        llvm::Value* capacity_ptr = get_pointer_to_capacity(set);
        LLVM::CreateStore(*builder, llvm_capacity, capacity_ptr);
        LLVM::CreateStore(*builder, rehash_flag, rehash_flag_ptr);
    }

    llvm::Value* LLVMSetInterface::get_el_hash(
        llvm::Value* capacity, llvm::Value* el,
        ASR::ttype_t* el_asr_type, llvm::Module& module) {
        // Write specialised hash functions for intrinsic types
        // This is to avoid unnecessary calls to C-runtime and do
        // as much as possible in LLVM directly.
        switch( el_asr_type->type ) {
            case ASR::ttypeType::Integer: {
                // Simple modulo with the capacity of the set.
                // We can update it later to do a better hash function
                // which produces lesser collisions.

                llvm::Value* int_hash = builder->CreateZExtOrTrunc(
                    builder->CreateURem(el,
                    builder->CreateZExtOrTrunc(capacity, el->getType())),
                    capacity->getType()
                );
                return int_hash;
            }
            case ASR::ttypeType::Character: {
                // Polynomial rolling hash function for strings
                llvm::Value* null_char = llvm::ConstantInt::get(llvm::Type::getInt8Ty(context),
                                                                llvm::APInt(8, '\0'));
                llvm::Value* p = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 31));
                llvm::Value* m = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 100000009));
                get_builder0()
                hash_value = builder0.CreateAlloca(llvm::Type::getInt64Ty(context), nullptr, "hash_value");
                hash_iter = builder0.CreateAlloca(llvm::Type::getInt64Ty(context), nullptr, "hash_iter");
                polynomial_powers = builder0.CreateAlloca(llvm::Type::getInt64Ty(context), nullptr, "p_pow");
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 0)),
                    hash_value);
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 1)),
                    polynomial_powers);
                LLVM::CreateStore(*builder,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 0)),
                    hash_iter);
                llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
                llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
                llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

                // head
                llvm_utils->start_new_block(loophead);
                {
                    llvm::Value* i = LLVM::CreateLoad(*builder, hash_iter);
                    llvm::Value* c = LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(el, i));
                    llvm::Value *cond = builder->CreateICmpNE(c, null_char);
                    builder->CreateCondBr(cond, loopbody, loopend);
                }

                // body
                llvm_utils->start_new_block(loopbody);
                {
                    // for c in el:
                    //     hash_value = (hash_value + (ord(c) + 1) * p_pow) % m
                    //     p_pow = (p_pow * p) % m
                    llvm::Value* i = LLVM::CreateLoad(*builder, hash_iter);
                    llvm::Value* c = LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(el, i));
                    llvm::Value* p_pow = LLVM::CreateLoad(*builder, polynomial_powers);
                    llvm::Value* hash = LLVM::CreateLoad(*builder, hash_value);
                    c = builder->CreateZExt(c, llvm::Type::getInt64Ty(context));
                    c = builder->CreateAdd(c, llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 1)));
                    c = builder->CreateMul(c, p_pow);
                    c = builder->CreateSRem(c, m);
                    hash = builder->CreateAdd(hash, c);
                    hash = builder->CreateSRem(hash, m);
                    LLVM::CreateStore(*builder, hash, hash_value);
                    p_pow = builder->CreateMul(p_pow, p);
                    p_pow = builder->CreateSRem(p_pow, m);
                    LLVM::CreateStore(*builder, p_pow, polynomial_powers);
                    i = builder->CreateAdd(i, llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::APInt(64, 1)));
                    LLVM::CreateStore(*builder, i, hash_iter);
                }

                builder->CreateBr(loophead);

                // end
                llvm_utils->start_new_block(loopend);
                llvm::Value* hash = LLVM::CreateLoad(*builder, hash_value);
                hash = builder->CreateTrunc(hash, llvm::Type::getInt32Ty(context));
                return builder->CreateSRem(hash, capacity);
            }
            case ASR::ttypeType::Tuple: {
                llvm::Value* tuple_hash = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0));
                ASR::Tuple_t* asr_tuple = ASR::down_cast<ASR::Tuple_t>(el_asr_type);
                for( size_t i = 0; i < asr_tuple->n_type; i++ ) {
                    llvm::Value* llvm_tuple_i = llvm_utils->tuple_api->read_item(el, i,
                                                    LLVM::is_llvm_struct(asr_tuple->m_type[i]));
                    tuple_hash = builder->CreateAdd(tuple_hash, get_el_hash(capacity, llvm_tuple_i,
                                                                             asr_tuple->m_type[i], module));
                    tuple_hash = builder->CreateSRem(tuple_hash, capacity);
                }
                return tuple_hash;
            }
            case ASR::ttypeType::Logical: {
                return builder->CreateZExt(el, llvm::Type::getInt32Ty(context));
            }
            default: {
                throw LCompilersException("Hashing " + ASRUtils::type_to_str_python(el_asr_type) +
                                          " isn't implemented yet.");
            }
        }
    }

    void LLVMSetLinearProbing::resolve_collision(
        llvm::Value* capacity, llvm::Value* el_hash,
        llvm::Value* el, llvm::Value* el_list,
        llvm::Value* el_mask, llvm::Module& module,
        ASR::ttype_t* el_asr_type, bool for_read) {

        /**
         * C++ equivalent:
         *
         * pos = el_hash;
         *
         * while( true ) {
         *     is_el_skip = el_mask_value == 3;     // tombstone
         *     is_el_set = el_mask_value != 0;
         *     is_el_matching = 0;
         *
         *     compare_elems = is_el_set && !is_el_skip;
         *     if( compare_elems ) {
         *         original_el = el_list[pos];
         *         is_el_matching = el == original_el;
         *     }
         *
         *     cond;
         *     if( for_read ) {
         *         // for reading, continue to next pos
         *         // even if current pos is tombstone
         *         cond = (is_el_set && !is_el_matching) || is_el_skip;
         *     }
         *     else {
         *         // for writing, do not continue
         *         // if current pos is tombstone
         *         cond = is_el_set && !is_el_matching && !is_el_skip;
         *     }
         *
         *     if( cond ) {
         *         pos += 1;
         *         pos %= capacity;
         *     }
         *     else {
         *         break;
         *     }
         * }
         *
         */

        get_builder0()
        if( !for_read ) {
            pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        }
        is_el_matching_var = builder0.CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);

        LLVM::CreateStore(*builder, el_hash, pos_ptr);

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            llvm::Value* el_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(el_mask, pos));
            llvm::Value* is_el_skip = builder->CreateICmpEQ(el_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3)));
            llvm::Value* is_el_set = builder->CreateICmpNE(el_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            llvm::Value* is_el_matching = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
                                                                llvm::APInt(1, 0));
            LLVM::CreateStore(*builder, is_el_matching, is_el_matching_var);
            llvm::Value* compare_elems = builder->CreateAnd(is_el_set,
                                            builder->CreateNot(is_el_skip));
            llvm_utils->create_if_else(compare_elems, [&]() {
                llvm::Value* original_el = llvm_utils->list_api->read_item(el_list, pos,
                                false, module, LLVM::is_llvm_struct(el_asr_type));
                is_el_matching = llvm_utils->is_equal_by_value(el, original_el, module,
                                                                el_asr_type);
                LLVM::CreateStore(*builder, is_el_matching, is_el_matching_var);
            }, [=]() {
            });
            // TODO: Allow safe exit if pos becomes el_hash again.
            // Ideally should not happen as set will be resized once
            // load factor touches a threshold (which will always be less than 1)
            // so there will be some el which will not be set. However for safety
            // we can add an exit from the loop with a error message.
            llvm::Value *cond = nullptr;
            if( for_read ) {
                cond = builder->CreateAnd(is_el_set, builder->CreateNot(
                            LLVM::CreateLoad(*builder, is_el_matching_var)));
                cond = builder->CreateOr(is_el_skip, cond);
            } else {
                cond = builder->CreateAnd(is_el_set, builder->CreateNot(is_el_skip));
                cond = builder->CreateAnd(cond, builder->CreateNot(
                            LLVM::CreateLoad(*builder, is_el_matching_var)));
            }
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            pos = builder->CreateAdd(pos, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                llvm::APInt(32, 1)));
            pos = builder->CreateSRem(pos, capacity);
            LLVM::CreateStore(*builder, pos, pos_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMSetSeparateChaining::resolve_collision(
        llvm::Value* el_hash, llvm::Value* el, llvm::Value* el_linked_list,
        llvm::Type* el_struct_type, llvm::Value* el_mask,
        llvm::Module& module, ASR::ttype_t* el_asr_type) {
        /**
         * C++ equivalent:
         *
         * ll_exists = el_mask_value == 1;
         * if( ll_exists ) {
         *     chain_itr = ll_head;
         * }
         * else {
         *     chain_itr = nullptr;
         * }
         * is_el_matching = 0;
         *
         * while( chain_itr != nullptr && !is_el_matching ) {
         *     chain_itr_prev = chain_itr;
         *     is_el_matching = (el == el_struct_el);
         *     if( !is_el_matching ) {
         *         chain_itr = next_el_struct;  // (*chain_itr)[1]
         *     }
         * }
         *
         * // now, chain_itr either points to element or is nullptr
         *
         */

        get_builder0()
        chain_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        chain_itr_prev = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        is_el_matching_var = builder0.CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);

        LLVM::CreateStore(*builder,
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)), chain_itr_prev);
        llvm::Value* el_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(el_mask, el_hash));
        llvm_utils->create_if_else(builder->CreateICmpEQ(el_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1))), [&]() {
            llvm::Value* el_ll_i8 = builder->CreateBitCast(el_linked_list, llvm::Type::getInt8PtrTy(context));
            LLVM::CreateStore(*builder, el_ll_i8, chain_itr);
        }, [&]() {
            LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)), chain_itr);
        });
        LLVM::CreateStore(*builder,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(1, 0)),
            is_el_matching_var
        );
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpNE(
                LLVM::CreateLoad(*builder, chain_itr),
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))
            );
            cond = builder->CreateAnd(cond, builder->CreateNot(LLVM::CreateLoad(
                                      *builder, is_el_matching_var)));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* el_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);
            LLVM::CreateStore(*builder, el_struct_i8, chain_itr_prev);
            llvm::Value* el_struct = builder->CreateBitCast(el_struct_i8, el_struct_type->getPointerTo());
            llvm::Value* el_struct_el = llvm_utils->create_gep(el_struct, 0);
            if( !LLVM::is_llvm_struct(el_asr_type) ) {
                el_struct_el = LLVM::CreateLoad(*builder, el_struct_el);
            }
            LLVM::CreateStore(*builder, llvm_utils->is_equal_by_value(el, el_struct_el,
                                module, el_asr_type), is_el_matching_var);
            llvm_utils->create_if_else(builder->CreateNot(LLVM::CreateLoad(*builder, is_el_matching_var)), [&]() {
                llvm::Value* next_el_struct = LLVM::CreateLoad(*builder, llvm_utils->create_gep(el_struct, 1));
                LLVM::CreateStore(*builder, next_el_struct, chain_itr);
            }, []() {});
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

    }

    void LLVMSetLinearProbing::resolve_collision_for_write(
        llvm::Value* set, llvm::Value* el_hash, llvm::Value* el,
        llvm::Module* module, ASR::ttype_t* el_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {

        /**
         * C++ equivalent:
         *
         * resolve_collision();     // modifies pos
         * el_list[pos] = el;
         * el_mask_value = el_mask[pos];
         * is_slot_empty = el_mask_value == 0 || el_mask_value == 3;
         * occupancy += is_slot_empty;
         * linear_prob_happened = (el_hash != pos) || (el_mask[el_hash] == 2);
         * set_max_2 = linear_prob_happened ? 2 : 1;
         * el_mask[el_hash] = set_max_2;
         * el_mask[pos] = set_max_2;
         *
         */

        llvm::Value* el_list = get_el_list(set);
        llvm::Value* el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        this->resolve_collision(capacity, el_hash, el, el_list, el_mask, *module, el_asr_type);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm_utils->list_api->write_item(el_list, pos, el,
                                         el_asr_type, false, module, name2memidx);

        llvm::Value* el_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(el_mask, pos));
        llvm::Value* is_slot_empty = builder->CreateICmpEQ(el_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
        is_slot_empty = builder->CreateOr(is_slot_empty, builder->CreateICmpEQ(el_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3))));
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(set);
        is_slot_empty = builder->CreateZExt(is_slot_empty, llvm::Type::getInt32Ty(context));
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        LLVM::CreateStore(*builder, builder->CreateAdd(occupancy, is_slot_empty),
                          occupancy_ptr);

        llvm::Value* linear_prob_happened = builder->CreateICmpNE(el_hash, pos);
        linear_prob_happened = builder->CreateOr(linear_prob_happened,
            builder->CreateICmpEQ(
                LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(el_mask, el_hash)),
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 2)
            ))
        );
        llvm::Value* set_max_2 = builder->CreateSelect(linear_prob_happened,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 2)),
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(el_mask, el_hash));
        LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(el_mask, pos));
    }

    void LLVMSetSeparateChaining::resolve_collision_for_write(
        llvm::Value* set, llvm::Value* el_hash, llvm::Value* el,
        llvm::Module* module, ASR::ttype_t* el_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        /**
         * C++ equivalent:
         *
         * el_linked_list = elems[el_hash];
         * resolve_collision(el);   // modifies chain_itr
         * do_insert = chain_itr == nullptr;
         *
         * if( do_insert ) {
         *     if( chain_itr_prev != nullptr ) {
         *         new_el_struct = malloc(el_struct_size);
         *         new_el_struct[0] = el;
         *         new_el_struct[1] = nullptr;
         *         chain_itr_prev[1] = new_el_struct;
         *     }
         *     else {
         *         el_linked_list[0] = el;
         *         el_linked_list[1] = nullptr;
         *     }
         *     occupancy += 1;
         * }
         * else {
         *     el_struct[0] = el;
         * }
         *
         * buckets_filled_delta = el_mask[el_hash] == 0;
         * buckets_filled += buckets_filled_delta;
         * el_mask[el_hash] = 1;
         *
         */

        llvm::Value* elems = LLVM::CreateLoad(*builder, get_pointer_to_elems(set));
        llvm::Value* el_linked_list = llvm_utils->create_ptr_gep(elems, el_hash);
        llvm::Value* el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
        llvm::Type* el_struct_type = typecode2elstruct[ASRUtils::get_type_code(el_asr_type)];
        this->resolve_collision(el_hash, el, el_linked_list, el_struct_type,
                                el_mask, *module, el_asr_type);
        llvm::Value* el_struct_i8 = LLVM::CreateLoad(*builder, chain_itr);

        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* do_insert = builder->CreateICmpEQ(el_struct_i8,
            llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)));
        builder->CreateCondBr(do_insert, thenBB, elseBB);

        builder->SetInsertPoint(thenBB);
        {
            llvm_utils->create_if_else(builder->CreateICmpNE(
                    LLVM::CreateLoad(*builder, chain_itr_prev),
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))), [&]() {
                llvm::DataLayout data_layout(module);
                size_t el_struct_size = data_layout.getTypeAllocSize(el_struct_type);
                llvm::Value* malloc_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), el_struct_size);
                llvm::Value* new_el_struct_i8 = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
                llvm::Value* new_el_struct = builder->CreateBitCast(new_el_struct_i8, el_struct_type->getPointerTo());
                llvm_utils->deepcopy(el, llvm_utils->create_gep(new_el_struct, 0), el_asr_type, module, name2memidx);
                LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)),
                    llvm_utils->create_gep(new_el_struct, 1));
                llvm::Value* el_struct_prev_i8 = LLVM::CreateLoad(*builder, chain_itr_prev);
                llvm::Value* el_struct_prev = builder->CreateBitCast(el_struct_prev_i8, el_struct_type->getPointerTo());
                LLVM::CreateStore(*builder, new_el_struct_i8, llvm_utils->create_gep(el_struct_prev, 1));
            }, [&]() {
                llvm_utils->deepcopy(el, llvm_utils->create_gep(el_linked_list, 0), el_asr_type, module, name2memidx);
                LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)),
                    llvm_utils->create_gep(el_linked_list, 1));
            });

            llvm::Value* occupancy_ptr = get_pointer_to_occupancy(set);
            llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
            occupancy = builder->CreateAdd(occupancy,
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1));
            LLVM::CreateStore(*builder, occupancy, occupancy_ptr);
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            llvm::Value* el_struct = builder->CreateBitCast(el_struct_i8, el_struct_type->getPointerTo());
            llvm_utils->deepcopy(el, llvm_utils->create_gep(el_struct, 0), el_asr_type, module, name2memidx);
        }
        llvm_utils->start_new_block(mergeBB);
        llvm::Value* buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(set);
        llvm::Value* el_mask_value_ptr = llvm_utils->create_ptr_gep(el_mask, el_hash);
        llvm::Value* el_mask_value = LLVM::CreateLoad(*builder, el_mask_value_ptr);
        llvm::Value* buckets_filled_delta = builder->CreateICmpEQ(el_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
        llvm::Value* buckets_filled = LLVM::CreateLoad(*builder, buckets_filled_ptr);
        buckets_filled = builder->CreateAdd(
            buckets_filled,
            builder->CreateZExt(buckets_filled_delta, llvm::Type::getInt32Ty(context))
        );
        LLVM::CreateStore(*builder, buckets_filled, buckets_filled_ptr);
        LLVM::CreateStore(*builder,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)),
            el_mask_value_ptr);
    }

    void LLVMSetLinearProbing::rehash(
        llvm::Value* set, llvm::Module* module, ASR::ttype_t* el_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {

        /**
         * C++ equivalent:
         *
         * old_capacity = capacity;
         * capacity = 2 * capacity + 1;
         *
         * idx = 0;
         * while( old_capacity > idx ) {
         *     is_el_set = el_mask[idx] != 0;
         *     if( is_el_set ) {
         *         el = el_list[idx];
         *         el_hash = get_el_hash(); // with new capacity
         *         resolve_collision();     // with new_el_list; modifies pos
         *         new_el_list[pos] = el;
         *         linear_prob_happened = el_hash != pos;
         *         set_max_2 = linear_prob_happened ? 2 : 1;
         *         new_el_mask[el_hash] = set_max_2;
         *         new_el_mask[pos] = set_max_2;
         *     }
         *     idx += 1;
         * }
         *
         * free(el_list);
         * free(el_mask);
         * el_list = new_el_list;
         * el_mask = new_el_mask;
         *
         */

        get_builder0()
        llvm::Value* capacity_ptr = get_pointer_to_capacity(set);
        llvm::Value* old_capacity = LLVM::CreateLoad(*builder, capacity_ptr);
        llvm::Value* capacity = builder->CreateMul(old_capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 2)));
        capacity = builder->CreateAdd(capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 1)));
        LLVM::CreateStore(*builder, capacity, capacity_ptr);

        std::string el_type_code = ASRUtils::get_type_code(el_asr_type);
        llvm::Type* el_llvm_type = std::get<2>(typecode2settype[el_type_code]);
        int32_t el_type_size = std::get<1>(typecode2settype[el_type_code]);

        llvm::Value* el_list = get_el_list(set);
        llvm::Value* new_el_list = builder0.CreateAlloca(llvm_utils->list_api->get_list_type(el_llvm_type,
                                                          el_type_code, el_type_size), nullptr);
        llvm_utils->list_api->list_init(el_type_code, new_el_list, *module, capacity, capacity);

        llvm::Value* el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* new_el_mask = LLVM::lfortran_calloc(context, *module, *builder, capacity,
                                                          llvm_mask_size);

        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        idx_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
            llvm::APInt(32, 0)), idx_ptr);

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(old_capacity, LLVM::CreateLoad(*builder, idx_ptr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* idx = LLVM::CreateLoad(*builder, idx_ptr);
            llvm::Function *fn = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
            llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
            llvm::Value* is_el_set = LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(el_mask, idx));
            is_el_set = builder->CreateICmpNE(is_el_set,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            builder->CreateCondBr(is_el_set, thenBB, elseBB);
            builder->SetInsertPoint(thenBB);
            {
                llvm::Value* el = llvm_utils->list_api->read_item(el_list, idx,
                        false, *module, LLVM::is_llvm_struct(el_asr_type));
                llvm::Value* el_hash = get_el_hash(current_capacity, el, el_asr_type, *module);
                this->resolve_collision(current_capacity, el_hash, el, new_el_list,
                               new_el_mask, *module, el_asr_type);
                llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
                llvm::Value* el_dest = llvm_utils->list_api->read_item(
                                    new_el_list, pos, false, *module, true);
                llvm_utils->deepcopy(el, el_dest, el_asr_type, module, name2memidx);

                llvm::Value* linear_prob_happened = builder->CreateICmpNE(el_hash, pos);
                llvm::Value* set_max_2 = builder->CreateSelect(linear_prob_happened,
                    llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 2)),
                    llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
                LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(new_el_mask, el_hash));
                LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(new_el_mask, pos));
            }
            builder->CreateBr(mergeBB);

            llvm_utils->start_new_block(elseBB);
            llvm_utils->start_new_block(mergeBB);
            idx = builder->CreateAdd(idx, llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, idx, idx_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        llvm_utils->list_api->free_data(el_list, *module);
        LLVM::lfortran_free(context, *module, *builder, el_mask);
        LLVM::CreateStore(*builder, LLVM::CreateLoad(*builder, new_el_list), el_list);
        LLVM::CreateStore(*builder, new_el_mask, get_pointer_to_mask(set));
    }

    void LLVMSetSeparateChaining::rehash(
        llvm::Value* set, llvm::Module* module, ASR::ttype_t* el_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        /**
         * C++ equivalent:
         *
         * capacity = 3 * capacity + 1;
         *
         * if( rehash_flag ) {
         *     while( old_capacity > idx ) {
         *         if( el_mask[el_hash] == 1 ) {
         *             write_el_linked_list(old_elems_value[idx]);
         *         }
         *         idx++;
         *     }
         * }
         * else {
         *     // set to old values
         * }
         *
         */

        get_builder0()
        old_capacity = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        old_occupancy = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        old_number_of_buckets_filled = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        idx_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        old_elems = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        old_el_mask = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);

        llvm::Value* capacity_ptr = get_pointer_to_capacity(set);
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(set);
        llvm::Value* number_of_buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(set);
        llvm::Value* old_capacity_value = LLVM::CreateLoad(*builder, capacity_ptr);
        LLVM::CreateStore(*builder, old_capacity_value, old_capacity);
        LLVM::CreateStore(*builder,
            LLVM::CreateLoad(*builder, occupancy_ptr),
            old_occupancy
        );
        LLVM::CreateStore(*builder,
            LLVM::CreateLoad(*builder, number_of_buckets_filled_ptr),
            old_number_of_buckets_filled
        );
        llvm::Value* old_el_mask_value = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
        llvm::Value* old_elems_value = LLVM::CreateLoad(*builder, get_pointer_to_elems(set));
        old_elems_value = builder->CreateBitCast(old_elems_value, llvm::Type::getInt8PtrTy(context));
        LLVM::CreateStore(*builder, old_el_mask_value, old_el_mask);
        LLVM::CreateStore(*builder, old_elems_value, old_elems);

        llvm::Value* capacity = builder->CreateMul(old_capacity_value, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 3)));
        capacity = builder->CreateAdd(capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                       llvm::APInt(32, 1)));
        set_init_given_initial_capacity(ASRUtils::get_type_code(el_asr_type),
                                         set, module, capacity);

        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB_rehash = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB_rehash = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB_rehash = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* rehash_flag = LLVM::CreateLoad(*builder, get_pointer_to_rehash_flag(set));
        builder->CreateCondBr(rehash_flag, thenBB_rehash, elseBB_rehash);

        builder->SetInsertPoint(thenBB_rehash);
        old_elems_value = LLVM::CreateLoad(*builder, old_elems);
        old_elems_value = builder->CreateBitCast(old_elems_value,
            typecode2elstruct[ASRUtils::get_type_code(el_asr_type)]->getPointerTo());
        old_el_mask_value = LLVM::CreateLoad(*builder, old_el_mask);
        old_capacity_value = LLVM::CreateLoad(*builder, old_capacity);
        capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0)), idx_ptr);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(
                                        old_capacity_value,
                                        LLVM::CreateLoad(*builder, idx_ptr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* itr = LLVM::CreateLoad(*builder, idx_ptr);
            llvm::Value* el_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(old_el_mask_value, itr));
            llvm::Value* is_el_set = builder->CreateICmpEQ(el_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));

            llvm_utils->create_if_else(is_el_set, [&]() {
                llvm::Value* srci = llvm_utils->create_ptr_gep(old_elems_value, itr);
                write_el_linked_list(srci, set, capacity, el_asr_type, module, name2memidx);
            }, [=]() {
            });
            llvm::Value* tmp = builder->CreateAdd(
                        itr,
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, idx_ptr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
        builder->CreateBr(mergeBB_rehash);
        llvm_utils->start_new_block(elseBB_rehash);
        {
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_capacity),
                get_pointer_to_capacity(set)
            );
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_occupancy),
                get_pointer_to_occupancy(set)
            );
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_number_of_buckets_filled),
                get_pointer_to_number_of_filled_buckets(set)
            );
            LLVM::CreateStore(*builder,
                builder->CreateBitCast(
                    LLVM::CreateLoad(*builder, old_elems),
                    typecode2elstruct[ASRUtils::get_type_code(el_asr_type)]->getPointerTo()
                ),
                get_pointer_to_elems(set)
            );
            LLVM::CreateStore(*builder,
                LLVM::CreateLoad(*builder, old_el_mask),
                get_pointer_to_mask(set)
            );
        }
        llvm_utils->start_new_block(mergeBB_rehash);
    }

    void LLVMSetSeparateChaining::write_el_linked_list(
        llvm::Value* el_ll, llvm::Value* set, llvm::Value* capacity,
        ASR::ttype_t* m_el_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        /**
         * C++ equivalent:
         *
         * while( src_itr != nullptr ) {
         *     resolve_collision_for_write(el_struct[0]);
         *     src_itr = el_struct[1];
         * }
         *
         */

        get_builder0()
        src_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);

        llvm::Type* el_struct_type = typecode2elstruct[ASRUtils::get_type_code(m_el_type)]->getPointerTo();
        LLVM::CreateStore(*builder,
            builder->CreateBitCast(el_ll, llvm::Type::getInt8PtrTy(context)),
            src_itr);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpNE(
                LLVM::CreateLoad(*builder, src_itr),
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))
            );
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* curr_src = builder->CreateBitCast(LLVM::CreateLoad(*builder, src_itr),
                el_struct_type);
            llvm::Value* src_el_ptr = llvm_utils->create_gep(curr_src, 0);
            llvm::Value* src_el = src_el_ptr;
            if( !LLVM::is_llvm_struct(m_el_type) ) {
                src_el = LLVM::CreateLoad(*builder, src_el_ptr);
            }
            llvm::Value* el_hash = get_el_hash(capacity, src_el, m_el_type, *module);
            resolve_collision_for_write(
                set, el_hash, src_el, module,
                m_el_type, name2memidx);

            llvm::Value* src_next_ptr = LLVM::CreateLoad(*builder, llvm_utils->create_gep(curr_src, 1));
            LLVM::CreateStore(*builder, src_next_ptr, src_itr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    void LLVMSetLinearProbing::rehash_all_at_once_if_needed(
        llvm::Value* set, llvm::Module* module, ASR::ttype_t* el_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {

        /**
         * C++ equivalent:
         *
         * // this condition will be true with 0 capacity too
         * rehash_condition = 5 * occupancy >= 3 * capacity;
         * if( rehash_condition ) {
         *     rehash();
         * }
         *
         */

        llvm::Value* occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(set));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        // Threshold hash is chosen from https://en.wikipedia.org/wiki/Hash_table#Load_factor
        // occupancy / capacity >= 0.6 is same as 5 * occupancy >= 3 * capacity
        llvm::Value* occupancy_times_5 = builder->CreateMul(occupancy, llvm::ConstantInt::get(
                                llvm::Type::getInt32Ty(context), llvm::APInt(32, 5)));
        llvm::Value* capacity_times_3 = builder->CreateMul(capacity, llvm::ConstantInt::get(
                                llvm::Type::getInt32Ty(context), llvm::APInt(32, 3)));
        llvm_utils->create_if_else(builder->CreateICmpSGE(occupancy_times_5,
                                    capacity_times_3), [&]() {
            rehash(set, module, el_asr_type, name2memidx);
        }, []() {});
    }

    void LLVMSetSeparateChaining::rehash_all_at_once_if_needed(
        llvm::Value* set, llvm::Module* module, ASR::ttype_t* el_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        /**
         * C++ equivalent:
         *
         * rehash_condition = rehash_flag && occupancy >= 2 * buckets_filled;
         * if( rehash_condition ) {
         *     rehash();
         * }
         *
         */
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(set));
        llvm::Value* buckets_filled = LLVM::CreateLoad(*builder, get_pointer_to_number_of_filled_buckets(set));
        llvm::Value* rehash_condition = LLVM::CreateLoad(*builder, get_pointer_to_rehash_flag(set));
        llvm::Value* buckets_filled_times_2 = builder->CreateMul(buckets_filled,
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 2)));
        rehash_condition = builder->CreateAnd(rehash_condition,
            builder->CreateICmpSGE(occupancy, buckets_filled_times_2));
        llvm_utils->create_if_else(rehash_condition, [&]() {
            rehash(set, module, el_asr_type, name2memidx);
        }, []() {});
    }

    void LLVMSetInterface::write_item(
        llvm::Value* set, llvm::Value* el,
        llvm::Module* module, ASR::ttype_t* el_asr_type,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        rehash_all_at_once_if_needed(set, module, el_asr_type, name2memidx);
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        llvm::Value* el_hash = get_el_hash(current_capacity, el, el_asr_type, *module);
        this->resolve_collision_for_write(set, el_hash, el, module,
                                          el_asr_type, name2memidx);
    }

    void LLVMSetLinearProbing::resolve_collision_for_read_with_bound_check(
        llvm::Value* set, llvm::Value* el_hash, llvm::Value* el,
        llvm::Module& module, ASR::ttype_t* el_asr_type, bool throw_key_error) {

        /**
         * C++ equivalent:
         *
         * el_mask_value = el_mask[el_hash];
         * is_prob_needed = el_mask_value == 1;
         * if( is_prob_needed ) {
         *     is_el_matching = el == el_list[el_hash];
         *     if( is_el_matching ) {
         *         pos = el_hash;
         *     }
         *     else {
         *         exit(1); // el not present
         *     }
         * }
         * else {
         *     resolve_collision(el, for_read=true);  // modifies pos
         * }
         *
         * is_el_matching = el == el_list[pos];
         * if( !is_el_matching ) {
         *     exit(1); // el not present
         * }
         *
         */

        get_builder0()
        llvm::Value* el_list = get_el_list(set);
        llvm::Value* el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        pos_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        llvm::Value* el_mask_value = LLVM::CreateLoad(*builder,
                                        llvm_utils->create_ptr_gep(el_mask, el_hash));
        llvm::Value* is_prob_not_needed = builder->CreateICmpEQ(el_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        builder->CreateCondBr(is_prob_not_needed, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        {
            // reasoning for this check explained in
            // LLVMDictOptimizedLinearProbing::resolve_collision_for_read_with_bound_check
            llvm::Value* is_el_matching = llvm_utils->is_equal_by_value(el,
                llvm_utils->list_api->read_item(el_list, el_hash, false, module,
                    LLVM::is_llvm_struct(el_asr_type)), module, el_asr_type);

            llvm_utils->create_if_else(is_el_matching, [=]() {
                LLVM::CreateStore(*builder, el_hash, pos_ptr);
            }, [&]() {
                if (throw_key_error) {
                    std::string message = "The set does not contain the specified element";
                    llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("KeyError: %s\n");
                    llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
                    print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
                    int exit_code_int = 1;
                    llvm::Value *exit_code = llvm::ConstantInt::get(context,
                            llvm::APInt(32, exit_code_int));
                    exit(context, module, *builder, exit_code);
                }
            });
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            this->resolve_collision(capacity, el_hash, el, el_list, el_mask,
                                    module, el_asr_type, true);
        }
        llvm_utils->start_new_block(mergeBB);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        // Check if the actual element is present or not
        llvm::Value* is_el_matching = llvm_utils->is_equal_by_value(el,
                llvm_utils->list_api->read_item(el_list, pos, false, module,
                    LLVM::is_llvm_struct(el_asr_type)), module, el_asr_type);

        llvm_utils->create_if_else(is_el_matching, []() {}, [&]() {
            if (throw_key_error) {
                std::string message = "The set does not contain the specified element";
                llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("KeyError: %s\n");
                llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
                print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
                int exit_code_int = 1;
                llvm::Value *exit_code = llvm::ConstantInt::get(context,
                        llvm::APInt(32, exit_code_int));
                exit(context, module, *builder, exit_code);
            }
        });
    }

    void LLVMSetSeparateChaining::resolve_collision_for_read_with_bound_check(
        llvm::Value* set, llvm::Value* el_hash, llvm::Value* el,
        llvm::Module& module, ASR::ttype_t* el_asr_type, bool throw_key_error) {
        /**
         * C++ equivalent:
         *
         * resolve_collision(el);   // modified chain_itr
         * does_el_exist = el_mask[el_hash] == 1 && chain_itr != nullptr;
         * if( !does_el_exist ) {
         *     exit(1); // KeyError
         * }
         *
         */
        llvm::Value* elems = LLVM::CreateLoad(*builder, get_pointer_to_elems(set));
        llvm::Value* el_linked_list = llvm_utils->create_ptr_gep(elems, el_hash);
        llvm::Value* el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
        std::string el_type_code = ASRUtils::get_type_code(el_asr_type);
        llvm::Type* el_struct_type = typecode2elstruct[el_type_code];
        this->resolve_collision(el_hash, el, el_linked_list,
                                el_struct_type, el_mask, module, el_asr_type);
        llvm::Value* el_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(el_mask, el_hash));
        llvm::Value* does_el_exist = builder->CreateICmpEQ(el_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        does_el_exist = builder->CreateAnd(does_el_exist,
            builder->CreateICmpNE(LLVM::CreateLoad(*builder, chain_itr),
            llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)))
        );

        llvm_utils->create_if_else(does_el_exist, []() {}, [&]() {
            if (throw_key_error) {
                std::string message = "The set does not contain the specified element";
                llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("KeyError: %s\n");
                llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
                print_error(context, module, *builder, {fmt_ptr, fmt_ptr2});
                int exit_code_int = 1;
                llvm::Value *exit_code = llvm::ConstantInt::get(context,
                        llvm::APInt(32, exit_code_int));
                exit(context, module, *builder, exit_code);
            }
        });
    }

    void LLVMSetLinearProbing::remove_item(
        llvm::Value* set, llvm::Value* el,
        llvm::Module& module, ASR::ttype_t* el_asr_type, bool throw_key_error) {
        /**
         * C++ equivalent:
         *
         * resolve_collision_for_read(el);  // modifies pos
         * el_mask[pos] = 3;    // tombstone marker
         * occupancy -= 1;
         */
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        llvm::Value* el_hash = get_el_hash(current_capacity, el, el_asr_type, module);
        this->resolve_collision_for_read_with_bound_check(set, el_hash, el, module, el_asr_type, throw_key_error);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm::Value* el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
        llvm::Value* el_mask_i = llvm_utils->create_ptr_gep(el_mask, pos);
        llvm::Value* tombstone_marker = llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 3));
        LLVM::CreateStore(*builder, tombstone_marker, el_mask_i);

        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(set);
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        occupancy = builder->CreateSub(occupancy, llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
        LLVM::CreateStore(*builder, occupancy, occupancy_ptr);
    }

    void LLVMSetSeparateChaining::remove_item(
        llvm::Value* set, llvm::Value* el,
        llvm::Module& module, ASR::ttype_t* el_asr_type, bool throw_key_error) {
        /**
         * C++ equivalent:
         *
         * // modifies chain_itr and chain_itr_prev
         * resolve_collision_for_read_with_bound_check(el);
         *
         * if(chain_itr_prev != nullptr) {
         *     chain_itr_prev[1] = chain_itr[1]; // next
         * }
         * else {
         *     // this linked list is now empty
         *     el_mask[el_hash] = 0;
         *     num_buckets_filled--;
         * }
         *
         * occupancy--;
         *
         */

        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(set));
        llvm::Value* el_hash = get_el_hash(current_capacity, el, el_asr_type, module);
        this->resolve_collision_for_read_with_bound_check(set, el_hash, el, module, el_asr_type, throw_key_error);
        llvm::Value* prev = LLVM::CreateLoad(*builder, chain_itr_prev);
        llvm::Value* found = LLVM::CreateLoad(*builder, chain_itr);

        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");

        builder->CreateCondBr(
            builder->CreateICmpNE(prev, llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))),
            thenBB, elseBB
        );
        builder->SetInsertPoint(thenBB);
        {
            llvm::Type* el_struct_type = typecode2elstruct[ASRUtils::get_type_code(el_asr_type)];
            found = builder->CreateBitCast(found, el_struct_type->getPointerTo());
            llvm::Value* found_next = LLVM::CreateLoad(*builder, llvm_utils->create_gep(found, 1));
            prev = builder->CreateBitCast(prev, el_struct_type->getPointerTo());
            LLVM::CreateStore(*builder, found_next, llvm_utils->create_gep(prev, 1));
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            llvm::Value* el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(set));
            LLVM::CreateStore(
                *builder,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)),
                llvm_utils->create_ptr_gep(el_mask, el_hash)
            );
            llvm::Value* num_buckets_filled_ptr = get_pointer_to_number_of_filled_buckets(set);
            llvm::Value* num_buckets_filled = LLVM::CreateLoad(*builder, num_buckets_filled_ptr);
            num_buckets_filled = builder->CreateSub(num_buckets_filled, llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, num_buckets_filled, num_buckets_filled_ptr);
        }
        llvm_utils->start_new_block(mergeBB);

        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(set);
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        occupancy = builder->CreateSub(occupancy, llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)));
        LLVM::CreateStore(*builder, occupancy, occupancy_ptr);
    }

    void LLVMSetLinearProbing::set_deepcopy(
        llvm::Value* src, llvm::Value* dest,
        ASR::Set_t* set_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        LCOMPILERS_ASSERT(src->getType() == dest->getType());
        llvm::Value* src_occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(src));
        llvm::Value* dest_occupancy_ptr = get_pointer_to_occupancy(dest);
        LLVM::CreateStore(*builder, src_occupancy, dest_occupancy_ptr);

        llvm::Value* src_el_list = get_el_list(src);
        llvm::Value* dest_el_list = get_el_list(dest);
        llvm_utils->list_api->list_deepcopy(src_el_list, dest_el_list,
                                            set_type->m_type, module,
                                            name2memidx);

        llvm::Value* src_el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(src));
        llvm::Value* dest_el_mask_ptr = get_pointer_to_mask(dest);
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* src_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(src));
        llvm::Value* dest_el_mask = LLVM::lfortran_calloc(context, *module, *builder, src_capacity,
                                                      llvm_mask_size);
        builder->CreateMemCpy(dest_el_mask, llvm::MaybeAlign(), src_el_mask,
                              llvm::MaybeAlign(), builder->CreateMul(src_capacity, llvm_mask_size));
        LLVM::CreateStore(*builder, dest_el_mask, dest_el_mask_ptr);
    }

    void LLVMSetSeparateChaining::set_deepcopy(
        llvm::Value* src, llvm::Value* dest,
        ASR::Set_t* set_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        llvm::Value* src_occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(src));
        llvm::Value* src_filled_buckets = LLVM::CreateLoad(*builder, get_pointer_to_number_of_filled_buckets(src));
        llvm::Value* src_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(src));
        llvm::Value* src_el_mask = LLVM::CreateLoad(*builder, get_pointer_to_mask(src));
        llvm::Value* src_rehash_flag = LLVM::CreateLoad(*builder, get_pointer_to_rehash_flag(src));
        LLVM::CreateStore(*builder, src_occupancy, get_pointer_to_occupancy(dest));
        LLVM::CreateStore(*builder, src_filled_buckets, get_pointer_to_number_of_filled_buckets(dest));
        LLVM::CreateStore(*builder, src_capacity, get_pointer_to_capacity(dest));
        LLVM::CreateStore(*builder, src_rehash_flag, get_pointer_to_rehash_flag(dest));
        llvm::DataLayout data_layout(module);
        size_t mask_size = data_layout.getTypeAllocSize(llvm::Type::getInt8Ty(context));
        llvm::Value* llvm_mask_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                            llvm::APInt(32, mask_size));
        llvm::Value* malloc_size = builder->CreateMul(src_capacity, llvm_mask_size);
        llvm::Value* dest_el_mask = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
        LLVM::CreateStore(*builder, dest_el_mask, get_pointer_to_mask(dest));

        // number of elements to be copied = capacity + (occupancy - filled_buckets)
        malloc_size = builder->CreateSub(src_occupancy, src_filled_buckets);
        malloc_size = builder->CreateAdd(src_capacity, malloc_size);
        llvm::Type* el_struct_type = typecode2elstruct[ASRUtils::get_type_code(set_type->m_type)];
        size_t el_struct_size = data_layout.getTypeAllocSize(el_struct_type);
        llvm::Value* llvm_el_struct_size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, el_struct_size));
        malloc_size = builder->CreateMul(malloc_size, llvm_el_struct_size);
        llvm::Value* dest_elems = LLVM::lfortran_malloc(context, *module, *builder, malloc_size);
        dest_elems = builder->CreateBitCast(dest_elems, el_struct_type->getPointerTo());
        get_builder0()
        copy_itr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        next_ptr = builder0.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        llvm::Value* llvm_zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0));
        LLVM::CreateStore(*builder, llvm_zero, copy_itr);
        LLVM::CreateStore(*builder, src_capacity, next_ptr);

        llvm::Value* src_elems = LLVM::CreateLoad(*builder, get_pointer_to_elems(src));
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpSGT(
                                        src_capacity,
                                        LLVM::CreateLoad(*builder, copy_itr));
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* itr = LLVM::CreateLoad(*builder, copy_itr);
            llvm::Value* el_mask_value = LLVM::CreateLoad(*builder,
                llvm_utils->create_ptr_gep(src_el_mask, itr));
            LLVM::CreateStore(*builder, el_mask_value,
                llvm_utils->create_ptr_gep(dest_el_mask, itr));
            llvm::Value* is_el_set = builder->CreateICmpEQ(el_mask_value,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));

            llvm_utils->create_if_else(is_el_set, [&]() {
                llvm::Value* srci = llvm_utils->create_ptr_gep(src_elems, itr);
                llvm::Value* desti = llvm_utils->create_ptr_gep(dest_elems, itr);
                deepcopy_el_linked_list(srci, desti, dest_elems,
                    set_type, module, name2memidx);
            }, []() {});
            llvm::Value* tmp = builder->CreateAdd(
                        itr,
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, copy_itr);
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
        LLVM::CreateStore(*builder, dest_elems, get_pointer_to_elems(dest));
    }

    void LLVMSetSeparateChaining::deepcopy_el_linked_list(
        llvm::Value* srci, llvm::Value* desti, llvm::Value* dest_elems,
        ASR::Set_t* set_type, llvm::Module* module,
        std::map<std::string, std::map<std::string, int>>& name2memidx) {
        /**
         * C++ equivalent:
         *
         * // memory allocation done before calling this function
         *
         * while( src_itr != nullptr ) {
         *     deepcopy(src_el, curr_dest_ptr);
         *     src_itr = src_itr_next;
         *     if( src_next_exists ) {
         *         *next_ptr = *next_ptr + 1;
         *         curr_dest[1] = &dest_elems[*next_ptr];
         *         curr_dest = *curr_dest[1];
         *     }
         *     else {
         *         curr_dest[1] = nullptr;
         *     }
         * }
         *
         */
        get_builder0()
        src_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);
        dest_itr = builder0.CreateAlloca(llvm::Type::getInt8PtrTy(context), nullptr);

        llvm::Type* el_struct_type = typecode2elstruct[ASRUtils::get_type_code(set_type->m_type)]->getPointerTo();
        LLVM::CreateStore(*builder,
            builder->CreateBitCast(srci, llvm::Type::getInt8PtrTy(context)),
            src_itr);
        LLVM::CreateStore(*builder,
            builder->CreateBitCast(desti, llvm::Type::getInt8PtrTy(context)),
            dest_itr);
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value *cond = builder->CreateICmpNE(
                LLVM::CreateLoad(*builder, src_itr),
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context))
            );
            builder->CreateCondBr(cond, loopbody, loopend);
        }

        // body
        llvm_utils->start_new_block(loopbody);
        {
            llvm::Value* curr_src = builder->CreateBitCast(LLVM::CreateLoad(*builder, src_itr),
                el_struct_type);
            llvm::Value* curr_dest = builder->CreateBitCast(LLVM::CreateLoad(*builder, dest_itr),
                el_struct_type);
            llvm::Value* src_el_ptr = llvm_utils->create_gep(curr_src, 0);
            llvm::Value *src_el = src_el_ptr;
            if( !LLVM::is_llvm_struct(set_type->m_type) ) {
                src_el = LLVM::CreateLoad(*builder, src_el_ptr);
            }
            llvm::Value* dest_el_ptr = llvm_utils->create_gep(curr_dest, 0);
            llvm_utils->deepcopy(src_el, dest_el_ptr, set_type->m_type, module, name2memidx);

            llvm::Value* src_next_ptr = LLVM::CreateLoad(*builder, llvm_utils->create_gep(curr_src, 1));
            llvm::Value* curr_dest_next_ptr = llvm_utils->create_gep(curr_dest, 1);
            LLVM::CreateStore(*builder, src_next_ptr, src_itr);

            llvm::Value* src_next_exists = builder->CreateICmpNE(src_next_ptr,
                llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)));
            llvm_utils->create_if_else(src_next_exists, [&]() {
                llvm::Value* next_idx = LLVM::CreateLoad(*builder, next_ptr);
                llvm::Value* dest_next_ptr = llvm_utils->create_ptr_gep(dest_elems, next_idx);
                dest_next_ptr = builder->CreateBitCast(dest_next_ptr, llvm::Type::getInt8PtrTy(context));
                LLVM::CreateStore(*builder, dest_next_ptr, curr_dest_next_ptr);
                LLVM::CreateStore(*builder, dest_next_ptr, dest_itr);
                next_idx = builder->CreateAdd(next_idx, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                              llvm::APInt(32, 1)));
                LLVM::CreateStore(*builder, next_idx, next_ptr);
            }, [&]() {
                LLVM::CreateStore(*builder,
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)),
                    curr_dest_next_ptr
                );
            });
        }

        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);
    }

    llvm::Value* LLVMSetInterface::len(llvm::Value* set) {
        return LLVM::CreateLoad(*builder, get_pointer_to_occupancy(set));
    }

} // namespace LCompilers
