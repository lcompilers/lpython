#include <libasr/codegen/llvm_array_utils.h>
#include <libasr/codegen/llvm_utils.h>

namespace LFortran {

    namespace LLVMArrUtils {

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

        bool compile_time_dimensions_t(ASR::dimension_t* m_dims, int n_dims) {
            if( n_dims <= 0 ) {
                return false;
            }
            bool is_ok = true;
            for( int r = 0; r < n_dims; r++ ) {
                if( m_dims[r].m_end == nullptr &&
                    m_dims[r].m_start == nullptr ) {
                    is_ok = false;
                    break;
                }
                if( (m_dims[r].m_end != nullptr &&
                    m_dims[r].m_end->type != ASR::exprType::ConstantInteger) ||
                    (m_dims[r].m_start != nullptr &&
                    m_dims[r].m_start->type != ASR::exprType::ConstantInteger) ) {
                    is_ok = false;
                    break;
                }
            }
            return is_ok;
        }

        bool is_explicit_shape(ASR::Variable_t* v) {
            ASR::dimension_t* m_dims;
            int n_dims;
            switch( v->m_type->type ) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* v_type = ASR::down_cast<ASR::Integer_t>(v->m_type);
                    m_dims = v_type->m_dims;
                    n_dims = v_type->n_dims;
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* v_type = ASR::down_cast<ASR::Real_t>(v->m_type);
                    m_dims = v_type->m_dims;
                    n_dims = v_type->n_dims;
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* v_type = ASR::down_cast<ASR::Complex_t>(v->m_type);
                    m_dims = v_type->m_dims;
                    n_dims = v_type->n_dims;
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* v_type = ASR::down_cast<ASR::Logical_t>(v->m_type);
                    m_dims = v_type->m_dims;
                    n_dims = v_type->n_dims;
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* v_type = ASR::down_cast<ASR::Derived_t>(v->m_type);
                    m_dims = v_type->m_dims;
                    n_dims = v_type->n_dims;
                    break;
                }
                default: {
                    throw LFortranException("Explicit shape checking supported only for integer, real, complex, logical and derived types.");
                }
            }
            return compile_time_dimensions_t(m_dims, n_dims);
        }

        std::unique_ptr<Descriptor>
        Descriptor::get_descriptor
        (llvm::LLVMContext& context,
         llvm::IRBuilder<>* builder,
         LLVMUtils* llvm_utils,
         DESCR_TYPE descr_type) {
            switch( descr_type ) {
                case DESCR_TYPE::_SimpleCMODescriptor: {
                    return std::make_unique<SimpleCMODescriptor>(context, builder, llvm_utils);
                }
            }
            return nullptr;
        }

        SimpleCMODescriptor::SimpleCMODescriptor(llvm::LLVMContext& _context,
            llvm::IRBuilder<>* _builder,
            LLVMUtils* _llvm_utils):
        context(_context),
        llvm_utils(std::move(_llvm_utils)),
        builder(std::move(_builder)),
        dim_des(llvm::StructType::create(
            context,
            std::vector<llvm::Type*>(
                {llvm::Type::getInt32Ty(context),
                 llvm::Type::getInt32Ty(context),
                 llvm::Type::getInt32Ty(context),
                 llvm::Type::getInt32Ty(context)}),
                 "dimension_descriptor")
        ) {
        }

        bool SimpleCMODescriptor::is_array(llvm::Value* tmp) {
            llvm::Type* tmp_type = nullptr;
            if( tmp->getType()->isPointerTy() ) {
                tmp_type = static_cast<llvm::PointerType*>(tmp->getType())->getElementType();
            } else {
                tmp_type = tmp->getType();
            }
            if( tmp_type->isStructTy() ) {
                llvm::StructType* tmp_struct_type = static_cast<llvm::StructType*>(tmp_type);
                if( tmp_struct_type->getNumElements() > 2 &&
                    tmp_struct_type->getElementType(2) == dim_des->getPointerTo() ) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }

        llvm::Value* SimpleCMODescriptor::
        convert_to_argument(llvm::Value* tmp, llvm::Type* arg_type) {
            llvm::Value* arg_struct = builder->CreateAlloca(arg_type, nullptr);
            llvm::Value* first_ele_ptr = nullptr;
            llvm::Type* tmp_type = static_cast<llvm::PointerType*>(tmp->getType())->getElementType();
            llvm::StructType* tmp_struct_type = static_cast<llvm::StructType*>(tmp_type);
            if( tmp_struct_type->getElementType(0)->isArrayTy() ) {
                first_ele_ptr = llvm_utils->create_gep(get_pointer_to_data(tmp), 0);
            } else if( tmp_struct_type->getNumElements() < 5 ) {
                first_ele_ptr = LLVM::CreateLoad(*builder, get_pointer_to_data(tmp));
            } else if( tmp_struct_type->getNumElements() == 5 ) {
                return tmp;
            }
            llvm::Value* first_arg_ptr = llvm_utils->create_gep(arg_struct, 0);
            builder->CreateStore(first_ele_ptr, first_arg_ptr);
            llvm::Value* sec_ele_ptr = get_offset(tmp);
            llvm::Value* sec_arg_ptr = llvm_utils->create_gep(arg_struct, 1);
            builder->CreateStore(sec_ele_ptr, sec_arg_ptr);
            llvm::Value* third_ele_ptr = LLVM::CreateLoad(*builder,
                get_pointer_to_dimension_descriptor_array(tmp));
            llvm::Value* third_arg_ptr = llvm_utils->create_gep(arg_struct, 2);
            builder->CreateStore(third_ele_ptr, third_arg_ptr);
            return arg_struct;
        }

        llvm::Type* SimpleCMODescriptor::get_argument_type(llvm::Type* type,
        std::uint32_t m_h, std::string arg_name,
        std::unordered_map<std::uint32_t, std::unordered_map<std::string, llvm::Type*>>& arr_arg_type_cache) {
            llvm::StructType* type_struct = static_cast<llvm::StructType*>(type);
            llvm::Type* first_ele_ptr_type = nullptr;
            if( type_struct->getElementType(0)->isArrayTy() ) {
                llvm::ArrayType* arr_type = static_cast<llvm::ArrayType*>(type_struct->getElementType(0));
                llvm::Type* ele_type = arr_type->getElementType();
                first_ele_ptr_type = ele_type->getPointerTo();
            } else if( type_struct->getElementType(0)->isPointerTy() &&
                       type_struct->getNumElements() < 5 ) {
                first_ele_ptr_type = type_struct->getElementType(0);
            } else if( type_struct->getElementType(0)->isPointerTy() &&
                       type_struct->getNumElements() == 5 ) {
                arr_arg_type_cache[m_h][std::string(arg_name)] = type;
                return type->getPointerTo();
            }
            llvm::Type* new_arr_type = nullptr;

            if( arr_arg_type_cache.find(m_h) == arr_arg_type_cache.end() || (
                arr_arg_type_cache.find(m_h) != arr_arg_type_cache.end() &&
                arr_arg_type_cache[m_h].find(std::string(arg_name)) == arr_arg_type_cache[m_h].end() ) ) {
                std::vector<llvm::Type*> arg_des = {first_ele_ptr_type};
                for( size_t i = 1; i < type_struct->getNumElements(); i++ ) {
                    arg_des.push_back(static_cast<llvm::StructType*>(type)->getElementType(i));
                }
                new_arr_type = llvm::StructType::create(context, arg_des, "array_call");
                arr_arg_type_cache[m_h][std::string(arg_name)] = new_arr_type;
            } else {
                new_arr_type = arr_arg_type_cache[m_h][std::string(arg_name)];
            }
            return new_arr_type->getPointerTo();
        }

        llvm::Type* SimpleCMODescriptor::get_array_type
        (ASR::ttype_t* m_type_, int a_kind,
        int rank, llvm::Type* el_type,
        bool get_pointer) {
            ASR::ttypeType type_ = m_type_->type;
            std::pair<std::pair<int, int>, int> array_key = std::make_pair(std::make_pair((int)type_, a_kind), rank);
            if( tkr2array.find(array_key) != tkr2array.end() ) {
                if( get_pointer ) {
                    return tkr2array[array_key]->getPointerTo();
                }
                return tkr2array[array_key];
            }
            llvm::Type* dim_des_array = create_dimension_descriptor_array_type();
            std::vector<llvm::Type*> array_type_vec;
            array_type_vec = {  el_type->getPointerTo(),
                                llvm::Type::getInt32Ty(context),
                                dim_des_array,
                                llvm::Type::getInt1Ty(context),
                                llvm::Type::getInt32Ty(context)  };
            llvm::StructType* new_array_type = llvm::StructType::create(context, array_type_vec, "array");
            tkr2array[array_key] = new_array_type;
            if( get_pointer ) {
                return tkr2array[array_key]->getPointerTo();
            }
            return (llvm::Type*) tkr2array[array_key];
        }

        llvm::Type* SimpleCMODescriptor::create_dimension_descriptor_array_type() {
            return dim_des->getPointerTo();
        }

        llvm::Type* SimpleCMODescriptor::get_malloc_array_type
        (ASR::ttype_t* m_type_, int a_kind, int rank, llvm::Type* el_type, bool get_pointer) {
            ASR::ttypeType type_ = m_type_->type;
            std::pair<std::pair<int, int>, int> array_key = std::make_pair(std::make_pair((int)type_, a_kind), rank);
            if( tkr2array.find(array_key) != tkr2array.end() ) {
                if( get_pointer ) {
                    return tkr2array[array_key]->getPointerTo();
                }
                return tkr2array[array_key];
            }
            llvm::Type* dim_des_array = create_dimension_descriptor_array_type();
            std::vector<llvm::Type*> array_type_vec = {
                el_type->getPointerTo(),
                llvm::Type::getInt32Ty(context),
                dim_des_array,
                llvm::Type::getInt1Ty(context),
                llvm::Type::getInt32Ty(context)};
            llvm::StructType* new_array_type = llvm::StructType::create(context, array_type_vec, "array");
            tkr2array[array_key] = new_array_type;
            if( get_pointer ) {
                return tkr2array[array_key]->getPointerTo();
            }
            return (llvm::Type*) tkr2array[array_key];
        }

        llvm::Type* SimpleCMODescriptor::get_dimension_descriptor_type
        (bool get_pointer) {
            if( !get_pointer ) {
                return dim_des;
            }
            return dim_des->getPointerTo();
        }

        llvm::Value* SimpleCMODescriptor::
        get_pointer_to_dimension_descriptor_array(llvm::Value* arr) {
            return LLVM::CreateLoad(*builder, llvm_utils->create_gep(arr, 2));
        }

        llvm::Value* SimpleCMODescriptor::
        get_rank(llvm::Value* arr, bool get_pointer) {
            llvm::Value* rank_ptr = llvm_utils->create_gep(arr, 4);
            if( get_pointer ) {
                return rank_ptr;
            }
            return LLVM::CreateLoad(*builder, rank_ptr);
        }

        llvm::Value* SimpleCMODescriptor::
        get_dimension_size(llvm::Value* dim_des_arr, llvm::Value* dim) {
            return LLVM::CreateLoad(*builder,
                        llvm_utils->create_gep(llvm_utils->create_ptr_gep(dim_des_arr, dim), 3));
        }

        void SimpleCMODescriptor::fill_array_details(
        llvm::Value* arr, ASR::dimension_t* m_dims, int n_dims,
        std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims) {
            llvm::Value* offset_val = llvm_utils->create_gep(arr, 1);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), offset_val);
            llvm::Value* dim_des_val = llvm_utils->create_gep(arr, 2);
            llvm::Value* llvm_ndims = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, n_dims)), llvm_ndims);
            llvm::Value* dim_des_first = builder->CreateAlloca(dim_des,
                                                               LLVM::CreateLoad(*builder, llvm_ndims));
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, n_dims)), get_rank(arr, true));
            builder->CreateStore(dim_des_first, dim_des_val);
            dim_des_val = LLVM::CreateLoad(*builder, dim_des_val);
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value* dim_val = llvm_utils->create_ptr_gep(dim_des_val, r);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* l_val = llvm_utils->create_gep(dim_val, 1);
                llvm::Value* u_val = llvm_utils->create_gep(dim_val, 2);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 3);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 1)), s_val);
                builder->CreateStore(llvm_dims[r].first, l_val);
                builder->CreateStore(llvm_dims[r].second, u_val);
                u_val = LLVM::CreateLoad(*builder, u_val);
                l_val = LLVM::CreateLoad(*builder, l_val);
                llvm::Value* dim_size = builder->CreateAdd(builder->CreateSub(u_val, l_val),
                                                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                builder->CreateStore(dim_size, dim_size_ptr);
            }

            llvm::Value* llvm_size = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            llvm::Value* const_1 = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            llvm::Value* prod = const_1;
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value *m_start, *m_end;
                ASR::dimension_t m_dim = m_dims[r];
                if( m_dim.m_start != nullptr ) {
                    m_start = llvm_dims[r].first;
                } else {
                    m_start = const_1;
                }
                m_end = llvm_dims[r].second;
                llvm::Value* dim_size_1 = builder->CreateSub(m_end, m_start);
                llvm::Value* dim_size = builder->CreateAdd(dim_size_1, const_1);
                prod = builder->CreateMul(prod, dim_size);
            }
            builder->CreateStore(prod, llvm_size);
            llvm::Value* first_ptr = get_pointer_to_data(arr);
            llvm::PointerType* first_ptr2ptr_type = static_cast<llvm::PointerType*>(first_ptr->getType());
            llvm::PointerType* first_ptr_type = static_cast<llvm::PointerType*>(first_ptr2ptr_type->getElementType());
            llvm::Value* arr_first = builder->CreateAlloca(first_ptr_type->getElementType(),
                                                            LLVM::CreateLoad(*builder, llvm_size));
            builder->CreateStore(arr_first, first_ptr);
        }

        void SimpleCMODescriptor::fill_malloc_array_details(
        llvm::Value* arr, int n_dims,
        std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
        llvm::Module* module) {
            llvm::Value* num_elements = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            llvm::Value* offset_val = llvm_utils->create_gep(arr, 1);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                                    offset_val);
            set_is_allocated_flag(arr, 1);
            llvm::Value* dim_des_val = LLVM::CreateLoad(*builder, llvm_utils->create_gep(arr, 2));
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value* dim_val = llvm_utils->create_ptr_gep(dim_des_val, r);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* l_val = llvm_utils->create_gep(dim_val, 1);
                llvm::Value* u_val = llvm_utils->create_gep(dim_val, 2);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 3);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 1)), s_val);
                builder->CreateStore(llvm_dims[r].first, l_val);
                builder->CreateStore(llvm_dims[r].second, u_val);
                u_val = LLVM::CreateLoad(*builder, u_val);
                l_val = LLVM::CreateLoad(*builder, l_val);
                llvm::Value* dim_size = builder->CreateAdd(builder->CreateSub(u_val, l_val),
                                                            llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                num_elements = builder->CreateMul(num_elements, dim_size);
                builder->CreateStore(dim_size, dim_size_ptr);
            }
            llvm::Value* ptr2firstptr = get_pointer_to_data(arr);
            llvm::AllocaInst *arg_size = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            llvm::DataLayout data_layout(module);
            llvm::Type* ptr2firstptr_type = ptr2firstptr->getType();
            llvm::Type* ptr_type = static_cast<llvm::PointerType*>(ptr2firstptr_type)->getElementType();
            uint64_t size = data_layout.getTypeAllocSize(
                                static_cast<llvm::PointerType*>(ptr_type)->
                                getElementType());
            llvm::Value* llvm_size = llvm::ConstantInt::get(context, llvm::APInt(32, size));
            num_elements = builder->CreateMul(num_elements, llvm_size);
            builder->CreateStore(num_elements, arg_size);
            llvm::Value* ptr_as_char_ptr = lfortran_malloc(context, *module, *builder, LLVM::CreateLoad(*builder, arg_size));
            llvm::Value* first_ptr = builder->CreateBitCast(ptr_as_char_ptr, ptr_type);
            builder->CreateStore(first_ptr, ptr2firstptr);
        }

        void SimpleCMODescriptor::fill_dimension_descriptor(
            llvm::Value* arr, int n_dims) {
            llvm::Value* dim_des_val = llvm_utils->create_gep(arr, 2);
            llvm::Value* llvm_ndims = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, n_dims)), llvm_ndims);
            llvm::Value* dim_des_first = builder->CreateAlloca(dim_des,
                                                               LLVM::CreateLoad(*builder, llvm_ndims));
            builder->CreateStore(dim_des_first, dim_des_val);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, n_dims)), get_rank(arr, true));
        }

        llvm::Value* SimpleCMODescriptor::get_pointer_to_dimension_descriptor(llvm::Value* dim_des_arr,
            llvm::Value* dim) {
            return llvm_utils->create_ptr_gep(dim_des_arr, dim);
        }

        llvm::Value* SimpleCMODescriptor::get_pointer_to_data(llvm::Value* arr) {
            return llvm_utils->create_gep(arr, 0);
        }

        llvm::Value* SimpleCMODescriptor::get_offset(llvm::Value* arr) {
            return LLVM::CreateLoad(*builder, llvm_utils->create_gep(arr, 1));
        }

        llvm::Value* SimpleCMODescriptor::get_lower_bound(llvm::Value* dim_des) {
            return LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des, 1));
        }

        llvm::Value* SimpleCMODescriptor::get_upper_bound(llvm::Value* dim_des) {
            return LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des, 2));
        }

        llvm::Value* SimpleCMODescriptor::get_stride(llvm::Value*) {
            return nullptr;
        }

        // TODO: Uncomment and implement later
        // void check_single_element(llvm::Value* curr_idx, llvm::Value* arr) {
        // }

        llvm::Value* SimpleCMODescriptor::cmo_convertor_single_element(
            llvm::Value* arr, std::vector<llvm::Value*>& m_args,
            int n_args, bool check_for_bounds) {
            llvm::Value* dim_des_arr_ptr = LLVM::CreateLoad(*builder, llvm_utils->create_gep(arr, 2));
            llvm::Value* prod = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            llvm::Value* idx = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            for( int r = 0; r < n_args; r++ ) {
                llvm::Value* curr_llvm_idx = m_args[r];
                llvm::Value* dim_des_ptr = llvm_utils->create_ptr_gep(dim_des_arr_ptr, r);
                llvm::Value* lval = LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des_ptr, 1));
                curr_llvm_idx = builder->CreateSub(curr_llvm_idx, lval);
                if( check_for_bounds ) {
                    // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
                }
                idx = builder->CreateAdd(idx, builder->CreateMul(prod, curr_llvm_idx));
                llvm::Value* dim_size = LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des_ptr, 3));
                prod = builder->CreateMul(prod, dim_size);
            }
            return idx;
        }

        llvm::Value* SimpleCMODescriptor::get_single_element(llvm::Value* array,
            std::vector<llvm::Value*>& m_args, int n_args) {
            llvm::Value* tmp = nullptr;
            // TODO: Uncomment later
            // bool check_for_bounds = is_explicit_shape(v);
            bool check_for_bounds = false;
            llvm::Value* idx = cmo_convertor_single_element(array, m_args, n_args, check_for_bounds);
            llvm::Value* full_array = get_pointer_to_data(array);
            tmp = llvm_utils->create_ptr_gep(LLVM::CreateLoad(*builder, full_array), idx);
            return tmp;
        }

        llvm::Value* SimpleCMODescriptor::get_is_allocated_flag(llvm::Value* array) {
            return LLVM::CreateLoad(*builder, llvm_utils->create_gep(array, 3));
        }

        void SimpleCMODescriptor::set_is_allocated_flag(llvm::Value* array, uint64_t status) {
            llvm::Value* is_allocated_flag = llvm_utils->create_gep(array, 3);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(1, status)),
                                    is_allocated_flag);
        }

    } // LLVMArrUtils

} // LFortran
