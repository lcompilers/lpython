#include <libasr/assert.h>
#include <libasr/codegen/llvm_utils.h>

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

    LLVMList::LLVMList(llvm::LLVMContext& context_,
        LLVMUtils* llvm_utils_,
        llvm::IRBuilder<>* builder_):
        context(context_),
        llvm_utils(std::move(llvm_utils_)),
        builder(std::move(builder_)) {}

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
            LCompilersException("list for " + type_code + " not declared yet.");
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

    void LLVMList::list_deepcopy(llvm::Value* src, llvm::Value* dest,
                                 std::string& src_type_code,
                                 llvm::Module& module) {
        LFORTRAN_ASSERT(src->getType() == dest->getType());
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
        builder->CreateMemCpy(copy_data, llvm::MaybeAlign(), src_data,
                              llvm::MaybeAlign(), arg_size);
        builder->CreateStore(copy_data, get_pointer_to_list_data(dest));
    }

    void LLVMList::write_item(llvm::Value* list, llvm::Value* pos, llvm::Value* item) {
        llvm::Value* list_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        llvm::Value* element_ptr = llvm_utils->create_ptr_gep(list_data, pos);
        builder->CreateStore(item, element_ptr);
    }

    llvm::Value* LLVMList::read_item(llvm::Value* list, llvm::Value* pos, bool get_pointer) {
        llvm::Value* list_data = LLVM::CreateLoad(*builder, get_pointer_to_list_data(list));
        llvm::Value* element_ptr = llvm_utils->create_ptr_gep(list_data, pos);
        if( get_pointer ) {
            return element_ptr;
        }
        return LLVM::CreateLoad(*builder, element_ptr);
    }

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
                          llvm::Module& module,
                          std::string& type_code) {
        llvm::Value* current_end_point = LLVM::CreateLoad(*builder, get_pointer_to_current_end_point(list));
        llvm::Value* current_capacity = LLVM::CreateLoad(*builder, get_pointer_to_current_capacity(list));
        int type_size = std::get<1>(typecode2listtype[type_code]);
        llvm::Type* el_type = std::get<2>(typecode2listtype[type_code]);
        resize_if_needed(list, current_end_point, current_capacity,
                         type_size, el_type, module);
        write_item(list, current_end_point, item);
        shift_end_point_by_one(list);
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
                                   std::string& type_code) {
        LFORTRAN_ASSERT(src->getType() == dest->getType());
        size_t n_elements = typecode2tupletype[type_code].second;
        for( size_t i = 0; i < n_elements; i++ ) {
            llvm::Value* src_item = read_item(src, i, false);
            llvm::Value* dest_item_ptr = read_item(dest, i, true);
            builder->CreateStore(src_item, dest_item_ptr);
        }
    }

} // namespace LFortran
