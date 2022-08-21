#include <libasr/assert.h>
#include <libasr/codegen/llvm_utils.h>
#include <libasr/asr_utils.h>

namespace LFortran {

    namespace LLVM {

        llvm::Value* CreateLoad(llvm::IRBuilder<> &builder, llvm::Value *x) {
            llvm::Type *t = x->getType();
            LFORTRAN_ASSERT(t->isPointerTy());
            llvm::Type *t2 = t->getContainedType(0);
            return builder.CreateLoad(t2, x);
        }

        llvm::Value* CreateStore(llvm::IRBuilder<> &builder, llvm::Value *x, llvm::Value *y) {
            LFORTRAN_ASSERT(y->getType()->isPointerTy());
            return builder.CreateStore(x, y);
        }


        llvm::Value* CreateGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx) {
            llvm::Type *t = x->getType();
            LFORTRAN_ASSERT(t->isPointerTy());
            llvm::Type *t2 = t->getContainedType(0);
            return builder.CreateGEP(t2, x, idx);
        }

        llvm::Value* CreateInBoundsGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx) {
            llvm::Type *t = x->getType();
            LFORTRAN_ASSERT(t->isPointerTy());
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
        llvm::IRBuilder<>* _builder):
        context(context),
        builder(std::move(_builder)) {
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
                    LFORTRAN_ASSERT(false);
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
                    LFORTRAN_ASSERT(false);
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
        fn->getBasicBlockList().push_back(bb);
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
        llvm::AllocaInst *pleft_arg = builder->CreateAlloca(character_type,
            nullptr);
        builder->CreateStore(left_arg, pleft_arg);
        llvm::AllocaInst *pright_arg = builder->CreateAlloca(character_type,
            nullptr);
        builder->CreateStore(right_arg, pright_arg);
        std::vector<llvm::Value*> args = {pleft_arg, pright_arg};
        return builder->CreateCall(fn, args);
    }

    llvm::Value* LLVMUtils::is_equal_by_value(llvm::Value* left, llvm::Value* right,
                                              llvm::Module& module, ASR::ttype_t* asr_type) {
        switch( asr_type->type ) {
            case ASR::ttypeType::Integer: {
                return builder->CreateICmpEQ(left, right);
            };
            case ASR::ttypeType::Real: {
                return builder->CreateFCmpOEQ(left, right);
            }
            case ASR::ttypeType::Character: {
                return lfortran_str_cmp(left, right, "_lpython_str_compare_eq",
                                        module);
            }
            case ASR::ttypeType::Tuple: {
                ASR::Tuple_t* tuple_type = ASR::down_cast<ASR::Tuple_t>(asr_type);
                return tuple_api->check_tuple_equality(left, right, tuple_type, context,
                                                       builder, module);
            }
            default: {
                throw LCompilersException("LLVMUtils::is_equal_by_value isn't implemented for " +
                                          ASRUtils::type_to_str_python(asr_type));
            }
        }
    }

    void LLVMUtils::deepcopy(llvm::Value* src, llvm::Value* dest,
                             ASR::ttype_t* asr_type, llvm::Module& module) {
        switch( asr_type->type ) {
            case ASR::ttypeType::Integer:
            case ASR::ttypeType::Real:
            case ASR::ttypeType::Character:
            case ASR::ttypeType::Logical:
            case ASR::ttypeType::Complex: {
                LLVM::CreateStore(*builder, src, dest);
                break ;
            };
            case ASR::ttypeType::Tuple: {
                ASR::Tuple_t* tuple_type = ASR::down_cast<ASR::Tuple_t>(asr_type);
                tuple_api->tuple_deepcopy(src, dest, tuple_type, module);
                break ;
            }
            case ASR::ttypeType::List: {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(asr_type);
                list_api->list_deepcopy(src, dest, list_type, module);
                break ;
            }
            default: {
                throw LCompilersException("LLVMUtils::deepcopy isn't implemented for " +
                                          ASRUtils::type_to_str_python(asr_type));
            }
        }
    }

    LLVMList::LLVMList(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        context(context_),
        llvm_utils(std::move(llvm_utils_)),
        builder(std::move(builder_)) {}

    LLVMDict::LLVMDict(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        context(context_),
        llvm_utils(std::move(llvm_utils_)),
        builder(std::move(builder_)),
        pos_ptr(nullptr), is_key_matching_var(nullptr),
        idx_ptr(nullptr), are_iterators_set(false),
        is_dict_present(false) {
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
        is_dict_present = true;
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

    llvm::Value* LLVMDict::get_value_list(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 2);
    }

    llvm::Value* LLVMDict::get_pointer_to_occupancy(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 0);
    }

    llvm::Value* LLVMDict::get_pointer_to_capacity(llvm::Value* dict) {
        return llvm_utils->list_api->get_pointer_to_current_capacity(
                    get_value_list(dict));
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

    void LLVMList::list_deepcopy(llvm::Value* src, llvm::Value* dest,
                                 ASR::List_t* list_type, llvm::Module& module) {
        list_deepcopy(src, dest, list_type->m_type, module);
    }

    void LLVMList::list_deepcopy(llvm::Value* src, llvm::Value* dest,
                                 ASR::ttype_t* element_type, llvm::Module& module) {
        LFORTRAN_ASSERT(src->getType() == dest->getType());
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
        llvm::Value* copy_data = LLVM::lfortran_malloc(context, module, *builder,
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
            llvm::AllocaInst *pos_ptr = builder->CreateAlloca(llvm::Type::getInt32Ty(context),
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
                llvm::Value* srci = read_item(src, pos, true);
                llvm::Value* desti = read_item(dest, pos, true);
                llvm_utils->deepcopy(srci, desti, element_type, module);
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
                                 ASR::Dict_t* dict_type, llvm::Module* module) {
        LFORTRAN_ASSERT(src->getType() == dest->getType());
        llvm::Value* src_occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(src));
        llvm::Value* dest_occupancy_ptr = get_pointer_to_occupancy(dest);
        LLVM::CreateStore(*builder, src_occupancy, dest_occupancy_ptr);

        llvm::Value* src_key_list = get_key_list(src);
        llvm::Value* dest_key_list = get_key_list(dest);
        llvm_utils->list_api->list_deepcopy(src_key_list, dest_key_list,
                                            dict_type->m_key_type, *module);

        llvm::Value* src_value_list = get_value_list(src);
        llvm::Value* dest_value_list = get_value_list(dest);
        llvm_utils->list_api->list_deepcopy(src_value_list, dest_value_list,
                                            dict_type->m_value_type, *module);

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

    void LLVMList::check_index_within_bounds(llvm::Value* /*list*/, llvm::Value* /*pos*/) {

    }

    void LLVMList::write_item(llvm::Value* list, llvm::Value* pos,
                              llvm::Value* item, ASR::ttype_t* asr_type,
                              llvm::Module& module, bool check_index_bound) {
        if( check_index_bound ) {
            check_index_within_bounds(list, pos);
        }
        llvm::Value* list_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        llvm::Value* element_ptr = llvm_utils->create_ptr_gep(list_data, pos);
        llvm_utils->deepcopy(item, element_ptr, asr_type, module);
    }

    void LLVMList::write_item(llvm::Value* list, llvm::Value* pos,
                              llvm::Value* item, bool check_index_bound) {
        if( check_index_bound ) {
            check_index_within_bounds(list, pos);
        }
        llvm::Value* list_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        llvm::Value* element_ptr = llvm_utils->create_ptr_gep(list_data, pos);
        LLVM::CreateStore(*builder, item, element_ptr);
    }

    llvm::Value* LLVMDict::get_pointer_to_keymask(llvm::Value* dict) {
        return llvm_utils->create_gep(dict, 3);
    }

    void LLVMDict::set_iterators() {
        if( are_iterators_set || !is_dict_present ) {
            return ;
        }
        pos_ptr = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, "pos_ptr");
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
            llvm::APInt(32, 0)), pos_ptr);
        is_key_matching_var = builder->CreateAlloca(llvm::Type::getInt1Ty(context), nullptr,
                                "is_key_matching_var");
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
            llvm::APInt(1, 0)), is_key_matching_var);
        idx_ptr = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, "idx_ptr");
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
            llvm::APInt(32, 0)), idx_ptr);
        are_iterators_set = true;
    }

    void LLVMDict::reset_iterators() {
        pos_ptr = nullptr;
        is_key_matching_var = nullptr;
        idx_ptr = nullptr;
        are_iterators_set = false;
    }

    void LLVMDict::resolve_collision(
        llvm::Value* capacity, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* key_list,
        llvm::Value* key_mask, llvm::Module& module,
        ASR::ttype_t* key_asr_type, bool /*for_read*/) {
        if( !are_iterators_set ) {
            pos_ptr = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            is_key_matching_var = builder->CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);
        }
        LLVM::CreateStore(*builder, key_hash, pos_ptr);


        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");


        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            llvm::Value* is_key_set = LLVM::CreateLoad(*builder,
                                        llvm_utils->create_ptr_gep(key_mask, pos));
            is_key_set = builder->CreateICmpNE(is_key_set,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            llvm::Value* is_key_matching = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
                                                                  llvm::APInt(1, 0));
            LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            llvm::Function *fn = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
            llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
            builder->CreateCondBr(is_key_set, thenBB, elseBB);
            builder->SetInsertPoint(thenBB);
            {
                llvm::Value* original_key = llvm_utils->list_api->read_item(key_list, pos,
                                                LLVM::is_llvm_struct(key_asr_type), false);
                is_key_matching = llvm_utils->is_equal_by_value(key, original_key, module,
                                                                key_asr_type);
                LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            }
            builder->CreateBr(mergeBB);


            llvm_utils->start_new_block(elseBB);
            llvm_utils->start_new_block(mergeBB);
            // TODO: Allow safe exit if pos becomes key_hash again.
            // Ideally should not happen as dict will be resized once
            // load factor touches a threshold (which will always be less than 1)
            // so there will be some key which will not be set. However for safety
            // we can add an exit from the loop with a error message.
            llvm::Value *cond = builder->CreateAnd(is_key_set, builder->CreateNot(
                                    LLVM::CreateLoad(*builder, is_key_matching_var)));
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
        if( !are_iterators_set ) {
            if( !for_read ) {
                pos_ptr = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            }
            is_key_matching_var = builder->CreateAlloca(llvm::Type::getInt1Ty(context), nullptr);
        }

        LLVM::CreateStore(*builder, key_hash, pos_ptr);

        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

        // head
        llvm_utils->start_new_block(loophead);
        {
            llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
            llvm::Value* is_key_set = LLVM::CreateLoad(*builder,
                                        llvm_utils->create_ptr_gep(key_mask, pos));
            is_key_set = builder->CreateICmpNE(is_key_set,
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            llvm::Value* is_key_matching = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
                                                                llvm::APInt(1, 0));
            LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            llvm::Function *fn = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
            llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
            builder->CreateCondBr(is_key_set, thenBB, elseBB);
            builder->SetInsertPoint(thenBB);
            {
                llvm::Value* original_key = llvm_utils->list_api->read_item(key_list, pos,
                                                LLVM::is_llvm_struct(key_asr_type), false);
                is_key_matching = llvm_utils->is_equal_by_value(key, original_key, module,
                                                                key_asr_type);
                LLVM::CreateStore(*builder, is_key_matching, is_key_matching_var);
            }
            builder->CreateBr(mergeBB);

            llvm_utils->start_new_block(elseBB);
            llvm_utils->start_new_block(mergeBB);
            // TODO: Allow safe exit if pos becomes key_hash again.
            // Ideally should not happen as dict will be resized once
            // load factor touches a threshold (which will always be less than 1)
            // so there will be some key which will not be set. However for safety
            // we can add an exit from the loop with a error message.
            llvm::Value *cond = builder->CreateAnd(is_key_set, builder->CreateNot(
                LLVM::CreateLoad(*builder, is_key_matching_var)));
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

    void LLVMDict::resolve_collision_for_write(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Value* value,
        llvm::Module& module, ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, module, key_asr_type);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm_utils->list_api->write_item(key_list, pos, key,
                                         key_asr_type, module, false);
        llvm_utils->list_api->write_item(value_list, pos, value,
                                         value_asr_type, module, false);
        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(key_mask, pos));
        llvm::Value* is_slot_empty = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
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
        llvm::Module& module, ASR::ttype_t* key_asr_type,
        ASR::ttype_t* value_asr_type) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, module, key_asr_type);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm_utils->list_api->write_item(key_list, pos, key,
                                         key_asr_type, module, false);
        llvm_utils->list_api->write_item(value_list, pos, value,
                                         value_asr_type, module, false);

        llvm::Value* key_mask_value = LLVM::CreateLoad(*builder,
            llvm_utils->create_ptr_gep(key_mask, pos));
        llvm::Value* is_slot_empty = builder->CreateICmpEQ(key_mask_value,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
        llvm::Value* occupancy_ptr = get_pointer_to_occupancy(dict);
        is_slot_empty = builder->CreateZExt(is_slot_empty, llvm::Type::getInt32Ty(context));
        llvm::Value* occupancy = LLVM::CreateLoad(*builder, occupancy_ptr);
        LLVM::CreateStore(*builder, builder->CreateAdd(occupancy, is_slot_empty),
                          occupancy_ptr);

        llvm::Value* linear_prob_happened = builder->CreateICmpNE(key_hash, pos);
        llvm::Value* set_max_2 = builder->CreateSelect(linear_prob_happened,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 2)),
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), llvm::APInt(8, 1)));
        LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(key_mask, key_hash));
        LLVM::CreateStore(*builder, set_max_2, llvm_utils->create_ptr_gep(key_mask, pos));
    }

    llvm::Value* LLVMDict::resolve_collision_for_read(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        this->resolve_collision(capacity, key_hash, key, key_list, key_mask, module, key_asr_type);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm::Value* item = llvm_utils->list_api->read_item(value_list, pos, true, false);
        return item;
    }

    llvm::Value* LLVMDictOptimizedLinearProbing::resolve_collision_for_read(
        llvm::Value* dict, llvm::Value* key_hash,
        llvm::Value* key, llvm::Module& module,
        ASR::ttype_t* key_asr_type) {
        llvm::Value* key_list = get_key_list(dict);
        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* key_mask = LLVM::CreateLoad(*builder, get_pointer_to_keymask(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        if( !are_iterators_set ) {
            pos_ptr = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        }
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
            LLVM::CreateStore(*builder, key_hash, pos_ptr);
        }
        builder->CreateBr(mergeBB);
        llvm_utils->start_new_block(elseBB);
        {
            this->resolve_collision(capacity, key_hash, key, key_list, key_mask,
                           module, key_asr_type, true);
        }
        llvm_utils->start_new_block(mergeBB);
        llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
        llvm::Value* item = llvm_utils->list_api->read_item(value_list, pos, true, false);
        return item;
    }

    llvm::Value* LLVMDict::get_key_hash(llvm::Value* capacity, llvm::Value* key,
                                        ASR::ttype_t* key_asr_type, llvm::Module& /*module*/) {
        // Write specialised hash functions for intrinsic types
        // This is to avoid unnecessary calls to C-runtime and do
        // as much as possible in LLVM directly.
        switch( key_asr_type->type ) {
            case ASR::ttypeType::Integer: {
                // Simple modulo with the capacity of the dict.
                // We can update it later to do a better hash function
                // which produces lesser collisions.
                return builder->CreateSRem(key, capacity);
            }
            default: {
                throw LCompilersException("Hashing " + ASRUtils::type_to_str_python(key_asr_type) +
                                          " isn't implemented yet.");
            }
        }
    }

    void LLVMDict::rehash(llvm::Value* dict, llvm::Module* module,
                          ASR::ttype_t* key_asr_type,
                          ASR::ttype_t* value_asr_type) {
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
        llvm::Value* new_key_list = builder->CreateAlloca(llvm_utils->list_api->get_list_type(key_llvm_type,
                                                          key_type_code, key_type_size), nullptr);
        llvm_utils->list_api->list_init(key_type_code, new_key_list, *module, capacity, capacity);

        llvm::Value* value_list = get_value_list(dict);
        llvm::Value* new_value_list = builder->CreateAlloca(llvm_utils->list_api->get_list_type(value_llvm_type,
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
        if( !are_iterators_set ) {
            idx_ptr = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
        }
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
                                    LLVM::is_llvm_struct(key_asr_type), false);
                llvm::Value* value = llvm_utils->list_api->read_item(value_list, idx,
                                        LLVM::is_llvm_struct(value_asr_type), false);
                llvm::Value* key_hash = get_key_hash(current_capacity, key, key_asr_type, *module);
                this->resolve_collision(current_capacity, key_hash, key, new_key_list,
                               new_key_mask, *module, key_asr_type);
                llvm::Value* pos = LLVM::CreateLoad(*builder, pos_ptr);
                llvm::Value* key_dest = llvm_utils->list_api->read_item(new_key_list, pos,
                                            true, false);
                llvm_utils->deepcopy(key, key_dest, key_asr_type, *module);
                llvm::Value* value_dest = llvm_utils->list_api->read_item(new_value_list, pos,
                                            true, false);
                llvm_utils->deepcopy(value, value_dest, value_asr_type, *module);

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

    void LLVMDict::rehash_all_at_once_if_needed(llvm::Value* dict, llvm::Module* module,
        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");

        llvm::Value* occupancy = LLVM::CreateLoad(*builder, get_pointer_to_occupancy(dict));
        llvm::Value* capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        occupancy = builder->CreateAdd(occupancy, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                         llvm::APInt(32, 1)));
        occupancy = builder->CreateSIToFP(occupancy, llvm::Type::getFloatTy(context));
        capacity = builder->CreateSIToFP(capacity, llvm::Type::getFloatTy(context));
        llvm::Value* load_factor = builder->CreateFDiv(occupancy, capacity);
        // Threshold hash is chosen from https://en.wikipedia.org/wiki/Hash_table#Load_factor
        llvm::Value* load_factor_threshold = llvm::ConstantFP::get(llvm::Type::getFloatTy(context),
                                                                   llvm::APFloat((float) 0.6));
        builder->CreateCondBr(builder->CreateFCmpOGE(load_factor, load_factor_threshold), thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        {
            rehash(dict, module, key_asr_type, value_asr_type);
        }
        builder->CreateBr(mergeBB);

        llvm_utils->start_new_block(elseBB);
        llvm_utils->start_new_block(mergeBB);
    }

    void LLVMDict::write_item(llvm::Value* dict, llvm::Value* key,
                              llvm::Value* value, llvm::Module* module,
                              ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type) {
        rehash_all_at_once_if_needed(dict, module, key_asr_type, value_asr_type);
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, key_asr_type, *module);
        this->resolve_collision_for_write(dict, key_hash, key, value, *module,
                                          key_asr_type, value_asr_type);
    }

    llvm::Value* LLVMDict::read_item(llvm::Value* dict, llvm::Value* key,
                             llvm::Module& module, ASR::ttype_t* key_asr_type,
                             bool get_pointer) {
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_capacity(dict));
        llvm::Value* key_hash = get_key_hash(current_capacity, key, key_asr_type, module);
        llvm::Value* value_ptr = this->resolve_collision_for_read(dict, key_hash, key, module,
                                                                  key_asr_type);
        if( get_pointer ) {
            return value_ptr;
        }
        return LLVM::CreateLoad(*builder, value_ptr);
    }

    llvm::Value* LLVMList::read_item(llvm::Value* list, llvm::Value* pos, bool get_pointer,
                                     bool check_index_bound) {
        if( check_index_bound ) {
            check_index_within_bounds(list, pos);
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

    LLVMDict::~LLVMDict() {
        typecode2dicttype.clear();
    }

    LLVMDictOptimizedLinearProbing::~LLVMDictOptimizedLinearProbing() {}

    void LLVMList::resize_if_needed(llvm::Value* list, llvm::Value* n,
                                    llvm::Value* capacity, int32_t type_size,
                                    llvm::Type* el_type, llvm::Module& module) {
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
        copy_data = LLVM::lfortran_realloc(context, module, *builder,
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
                          ASR::ttype_t* asr_type, llvm::Module& module) {
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder, get_pointer_to_current_end_point(list));
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_current_capacity(list));
        std::string type_code = ASRUtils::get_type_code(asr_type);
        int type_size = std::get<1>(typecode2listtype[type_code]);
        llvm::Type* el_type = std::get<2>(typecode2listtype[type_code]);
        resize_if_needed(list, current_end_point, current_capacity,
                         type_size, el_type, module);
        write_item(list, current_end_point, item, asr_type, module);
        shift_end_point_by_one(list);
    }

    void LLVMList::insert_item(llvm::Value* list, llvm::Value* pos,
                               llvm::Value* item, ASR::ttype_t* asr_type,
                               llvm::Module& module) {
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
        llvm::AllocaInst *tmp_ptr = builder->CreateAlloca(el_type, nullptr);
        LLVM::CreateStore(*builder, read_item(list, pos, false), tmp_ptr);
        llvm::Value* tmp = nullptr;

        // TODO: Should be created outside the user loop and not here.
        // LLVMList should treat them as data members and create them
        // only if they are NULL
        llvm::AllocaInst *pos_ptr = builder->CreateAlloca(
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
            tmp = read_item(list, next_index, false);
            write_item(list, next_index, LLVM::CreateLoad(*builder, tmp_ptr));
            LLVM::CreateStore(*builder, tmp, tmp_ptr);

            tmp = builder->CreateAdd(
                        LLVM::CreateLoad(*builder, pos_ptr),
                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            LLVM::CreateStore(*builder, tmp, pos_ptr);
        }
        builder->CreateBr(loophead);

        // end
        llvm_utils->start_new_block(loopend);

        write_item(list, pos, item, asr_type, module);
        shift_end_point_by_one(list);
    }

    llvm::Value* LLVMList::find_item_position(llvm::Value* list,
        llvm::Value* item, ASR::ttype_t* item_type, llvm::Module& module) {
        llvm::Type* pos_type = llvm::Type::getInt32Ty(context);
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder,
                                        get_pointer_to_current_end_point(list));
        // TODO: Should be created outside the user loop and not here.
        // LLVMList should treat them as data members and create them
        // only if they are NULL
        llvm::AllocaInst *i = builder->CreateAlloca(pos_type, nullptr);
        LLVM::CreateStore(*builder, llvm::ConstantInt::get(
                                    context, llvm::APInt(32, 0)), i);
        llvm::Value* tmp = nullptr;

        /* Equivalent in C++:
         * int i = 0;
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
                                              LLVM::is_llvm_struct(item_type));
            llvm::Value* is_item_not_equal = builder->CreateNot(
                                                llvm_utils->is_equal_by_value(
                                                    left_arg, item,
                                                    module, item_type)
                                            );
            llvm::Value *cond = builder->CreateAnd(is_item_not_equal,
                                                   builder->CreateICmpSGT(current_end_point,
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


        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");

        llvm::Value* cond = builder->CreateICmpEQ(
                              LLVM::CreateLoad(*builder, i), current_end_point);
        builder->CreateCondBr(cond, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        {
            // TODO: Allow runtime information like the exact element which is
            // not found.
            std::string message = "The list does not contain the element";
            llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("ValueError: %s\n");
            llvm::Value *fmt_ptr2 = builder->CreateGlobalStringPtr(message);
            printf(context, module, *builder, {fmt_ptr, fmt_ptr2});
            int exit_code_int = 1;
            llvm::Value *exit_code = llvm::ConstantInt::get(context,
                    llvm::APInt(32, exit_code_int));
            exit(context, module, *builder, exit_code);
        }
        builder->CreateBr(mergeBB);

        llvm_utils->start_new_block(elseBB);
        llvm_utils->start_new_block(mergeBB);

        return LLVM::CreateLoad(*builder, i);
    }

    void LLVMList::remove(llvm::Value* list, llvm::Value* item,
                          ASR::ttype_t* item_type, llvm::Module& module) {
        llvm::Type* pos_type = llvm::Type::getInt32Ty(context);
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder,
                                        get_pointer_to_current_end_point(list));
        // TODO: Should be created outside the user loop and not here.
        // LLVMList should treat them as data members and create them
        // only if they are NULL
        llvm::AllocaInst *item_pos = builder->CreateAlloca(pos_type, nullptr);
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
                       read_item(list, tmp, false));
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

    void LLVMTuple::tuple_init(llvm::Value* llvm_tuple,
                               std::vector<llvm::Value*>& values) {
        for( size_t i = 0; i < values.size(); i++ ) {
            llvm::Value* item_ptr = read_item(llvm_tuple, i, true);
            builder->CreateStore(values[i], item_ptr);
        }
    }

    void LLVMTuple::tuple_deepcopy(llvm::Value* src, llvm::Value* dest,
                                   ASR::Tuple_t* tuple_type, llvm::Module& module) {
        LFORTRAN_ASSERT(src->getType() == dest->getType());
        for( size_t i = 0; i < tuple_type->n_type; i++ ) {
            llvm::Value* src_item = read_item(src, i, LLVM::is_llvm_struct(
                                              tuple_type->m_type[i]));
            llvm::Value* dest_item_ptr = read_item(dest, i, true);
            llvm_utils->deepcopy(src_item, dest_item_ptr,
                                 tuple_type->m_type[i], module);
        }
    }

    llvm::Value* LLVMTuple::check_tuple_equality(llvm::Value* t1, llvm::Value* t2,
                                                 ASR::Tuple_t* tuple_type,
                                                 llvm::LLVMContext& context,
                                                 llvm::IRBuilder<>* builder,
                                                 llvm::Module& module) {
        llvm::Value* is_equal = llvm::ConstantInt::get(context, llvm::APInt(1, 1));
        for( size_t i = 0; i < tuple_type->n_type; i++ ) {
            llvm::Value* t1i = llvm_utils->tuple_api->read_item(t1, i);
            llvm::Value* t2i = llvm_utils->tuple_api->read_item(t2, i);
            llvm::Value* is_t1_eq_t2 = llvm_utils->is_equal_by_value(t1i, t2i, module,
                                        tuple_type->m_type[i]);
            is_equal = builder->CreateAnd(is_equal, is_t1_eq_t2);
        }
        return is_equal;
    }

} // namespace LFortran
