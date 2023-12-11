#include <libasr/codegen/llvm_array_utils.h>
#include <libasr/codegen/llvm_utils.h>
#include <libasr/asr_utils.h>

namespace LCompilers {

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
                builder.CreateBitCast(ptr, llvm::Type::getInt8PtrTy(context)), arg_size};
            return builder.CreateCall(fn, args);
        }

        bool compile_time_dimensions_t(ASR::dimension_t* m_dims, int n_dims) {
            if( n_dims <= 0 ) {
                return false;
            }
            bool is_ok = true;
            for( int r = 0; r < n_dims; r++ ) {
                if( m_dims[r].m_length == nullptr &&
                    m_dims[r].m_start == nullptr ) {
                    is_ok = false;
                    break;
                }
                if( m_dims[r].m_length == nullptr ) {
                    is_ok = false;
                    break;
                }
            }
            return is_ok;
        }

        bool is_explicit_shape(ASR::Variable_t* v) {
            ASR::dimension_t* m_dims = nullptr;
            int n_dims = 0;
            switch( v->m_type->type ) {
                case ASR::ttypeType::Array: {
                    ASR::Array_t* v_type = ASR::down_cast<ASR::Array_t>(v->m_type);
                    m_dims = v_type->m_dims;
                    n_dims = v_type->n_dims;
                    break;
                }
                default: {
                    throw LCompilersException("Explicit shape checking supported only for integer, real, complex, logical and derived types.");
                }
            }
            return compile_time_dimensions_t(m_dims, n_dims);
        }

        std::unique_ptr<Descriptor>
        Descriptor::get_descriptor
        (llvm::LLVMContext& context, llvm::IRBuilder<>* builder,
         LLVMUtils* llvm_utils, DESCR_TYPE descr_type,
         CompilerOptions& co, std::vector<llvm::Value*>& heap_arrays_) {
            switch( descr_type ) {
                case DESCR_TYPE::_SimpleCMODescriptor: {
                    return std::make_unique<SimpleCMODescriptor>(context, builder, llvm_utils, co, heap_arrays_);
                }
            }
            return nullptr;
        }

        SimpleCMODescriptor::SimpleCMODescriptor(llvm::LLVMContext& _context,
            llvm::IRBuilder<>* _builder, LLVMUtils* _llvm_utils, CompilerOptions& co_,
            std::vector<llvm::Value*>& heap_arrays_):
        context(_context),
        llvm_utils(std::move(_llvm_utils)),
        builder(std::move(_builder)),
        dim_des(llvm::StructType::create(
            context,
            std::vector<llvm::Type*>(
                {llvm::Type::getInt32Ty(context),
                 llvm::Type::getInt32Ty(context),
                 llvm::Type::getInt32Ty(context)}),
                 "dimension_descriptor")
        ), co(co_), heap_arrays(heap_arrays_) {
        }

        bool SimpleCMODescriptor::is_array(ASR::ttype_t* asr_type) {
            std::string asr_type_code = ASRUtils::get_type_code(
                ASRUtils::type_get_past_allocatable(asr_type), false, false);
            return (tkr2array.find(asr_type_code) != tkr2array.end() &&
                    ASRUtils::is_array(asr_type));
        }

        llvm::Value* SimpleCMODescriptor::
        convert_to_argument(llvm::Value* tmp, ASR::ttype_t* asr_arg_type,
                            llvm::Type* arg_type, bool data_only) {
            if( data_only ) {
                return LLVM::CreateLoad(*builder, get_pointer_to_data(tmp));
            }
            llvm::Value* arg_struct = builder->CreateAlloca(arg_type, nullptr);
            llvm::Value* first_ele_ptr = nullptr;
            std::string asr_arg_type_code = ASRUtils::get_type_code(ASRUtils::get_contained_type(asr_arg_type), false, false);
            llvm::StructType* tmp_struct_type = tkr2array[asr_arg_type_code].first;
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
        (ASR::ttype_t* m_type_, llvm::Type* el_type,
        bool get_pointer) {
            std::string array_key = ASRUtils::get_type_code(m_type_, false, false);
            if( tkr2array.find(array_key) != tkr2array.end() ) {
                if( get_pointer ) {
                    return tkr2array[array_key].first->getPointerTo();
                }
                return tkr2array[array_key].first;
            }
            llvm::Type* dim_des_array = create_dimension_descriptor_array_type();
            std::vector<llvm::Type*> array_type_vec;
            array_type_vec = {  el_type->getPointerTo(),
                                llvm::Type::getInt32Ty(context),
                                dim_des_array,
                                llvm::Type::getInt1Ty(context),
                                llvm::Type::getInt32Ty(context)  };
            llvm::StructType* new_array_type = llvm::StructType::create(context, array_type_vec, "array");
            tkr2array[array_key] = std::make_pair(new_array_type, el_type);
            if( get_pointer ) {
                return tkr2array[array_key].first->getPointerTo();
            }
            return (llvm::Type*) tkr2array[array_key].first;
        }

        llvm::Type* SimpleCMODescriptor::create_dimension_descriptor_array_type() {
            return dim_des->getPointerTo();
        }

        llvm::Type* SimpleCMODescriptor::get_dimension_descriptor_type
        (bool get_pointer) {
            if( !get_pointer ) {
                return dim_des;
            }
            return dim_des->getPointerTo();
        }

        llvm::Value* SimpleCMODescriptor::
        get_pointer_to_dimension_descriptor_array(llvm::Value* arr, bool load) {
            llvm::Value* dim_des_arr_ptr = llvm_utils->create_gep(arr, 2);
            if( !load ) {
                return dim_des_arr_ptr;
            }
            return LLVM::CreateLoad(*builder, dim_des_arr_ptr);
        }

        llvm::Value* SimpleCMODescriptor::
        get_rank(llvm::Value* arr, bool get_pointer) {
            llvm::Value* rank_ptr = llvm_utils->create_gep(arr, 4);
            if( get_pointer ) {
                return rank_ptr;
            }
            return LLVM::CreateLoad(*builder, rank_ptr);
        }

        void SimpleCMODescriptor::
        set_rank(llvm::Value* arr, llvm::Value* rank) {
            llvm::Value* rank_ptr = llvm_utils->create_gep(arr, 4);
            LLVM::CreateStore(*builder, rank, rank_ptr);
        }

        llvm::Value* SimpleCMODescriptor::
        get_dimension_size(llvm::Value* dim_des_arr, llvm::Value* dim, bool load) {
            llvm::Value* dim_size = llvm_utils->create_gep(llvm_utils->create_ptr_gep(dim_des_arr, dim), 2);
            if( !load ) {
                return dim_size;
            }
            return LLVM::CreateLoad(*builder, dim_size);
        }

        llvm::Value* SimpleCMODescriptor::
        get_dimension_size(llvm::Value* dim_des, bool load) {
            llvm::Value* dim_size = llvm_utils->create_gep(dim_des, 2);
            if( !load ) {
                return dim_size;
            }
            return LLVM::CreateLoad(*builder, dim_size);
        }

        void SimpleCMODescriptor::fill_array_details(
        llvm::Value* arr, llvm::Type* llvm_data_type, int n_dims,
        std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
        llvm::Module* module, bool reserve_data_memory) {
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
            llvm::Value* prod = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value* dim_val = llvm_utils->create_ptr_gep(dim_des_val, r);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* l_val = llvm_utils->create_gep(dim_val, 1);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 2);
                builder->CreateStore(prod, s_val);
                builder->CreateStore(llvm_dims[r].first, l_val);
                llvm::Value* dim_size = llvm_dims[r].second;
                prod = builder->CreateMul(prod, dim_size);
                builder->CreateStore(dim_size, dim_size_ptr);
            }

            if( !reserve_data_memory ) {
                return ;
            }

            llvm::Value* llvm_size = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            builder->CreateStore(prod, llvm_size);
            llvm::Value* first_ptr = get_pointer_to_data(arr);
            llvm::Value* arr_first = nullptr;

            if( !co.stack_arrays ) {
                llvm::DataLayout data_layout(module);
                uint64_t size = data_layout.getTypeAllocSize(llvm_data_type);
                builder->CreateStore(builder->CreateMul(
                    LLVM::CreateLoad(*builder, llvm_size),
                    llvm::ConstantInt::get(context, llvm::APInt(32, size))), llvm_size);
                llvm::Value* arr_first_i8 = lfortran_malloc(
                    context, *module, *builder, LLVM::CreateLoad(*builder, llvm_size));
                heap_arrays.push_back(arr_first_i8);
                arr_first = builder->CreateBitCast(
                    arr_first_i8, llvm_data_type->getPointerTo());
            } else {
                arr_first = builder->CreateAlloca(
                    llvm_data_type, LLVM::CreateLoad(*builder, llvm_size));
            }
            builder->CreateStore(arr_first, first_ptr);
        }

        void SimpleCMODescriptor::fill_array_details(
            llvm::Value* source, llvm::Value* destination,
            ASR::ttype_t* /*asr_shape_type*/, bool ignore_data) {
            if( !ignore_data ) {
                // TODO: Implement data filling to destination array
                LCOMPILERS_ASSERT(false);
            }


            llvm::Value* source_offset_val = LLVM::CreateLoad(*builder, llvm_utils->create_gep(source, 1));
            llvm::Value* dest_offset = llvm_utils->create_gep(destination, 1);
            builder->CreateStore(source_offset_val, dest_offset);


            llvm::Value* source_dim_des_val = LLVM::CreateLoad(*builder, llvm_utils->create_gep(source, 2));
            llvm::Value* dest_dim_des_ptr = llvm_utils->create_gep(destination, 2);
            builder->CreateStore(source_dim_des_val, dest_dim_des_ptr);


            llvm::Value* source_rank = this->get_rank(source, false);
            this->set_rank(destination, source_rank);
        };

        void SimpleCMODescriptor::fill_malloc_array_details(
            llvm::Value* arr, llvm::Type* llvm_data_type, int n_dims,
            std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
            llvm::Module* module, bool realloc) {
            arr = LLVM::CreateLoad(*builder, arr);
            llvm::Value* offset_val = llvm_utils->create_gep(arr, 1);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                                    offset_val);
            llvm::Value* dim_des_val = LLVM::CreateLoad(*builder, llvm_utils->create_gep(arr, 2));
            llvm::Value* prod = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value* dim_val = llvm_utils->create_ptr_gep(dim_des_val, r);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* l_val = llvm_utils->create_gep(dim_val, 1);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 2);
                builder->CreateStore(prod, s_val);
                builder->CreateStore(llvm_dims[r].first, l_val);
                llvm::Value* dim_size = llvm_dims[r].second;
                builder->CreateStore(dim_size, dim_size_ptr);
                prod = builder->CreateMul(prod, dim_size);
            }
            llvm::Value* ptr2firstptr = get_pointer_to_data(arr);
            llvm::AllocaInst *arg_size = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr);
            llvm::DataLayout data_layout(module);
            llvm::Type* ptr_type = llvm_data_type->getPointerTo();
            uint64_t size = data_layout.getTypeAllocSize(llvm_data_type);
            llvm::Value* llvm_size = llvm::ConstantInt::get(context, llvm::APInt(32, size));
            prod = builder->CreateMul(prod, llvm_size);
            builder->CreateStore(prod, arg_size);
            llvm::Value* ptr_as_char_ptr = nullptr;
            if( realloc ) {
                ptr_as_char_ptr = lfortran_realloc(context, *module,
                    *builder, LLVM::CreateLoad(*builder, ptr2firstptr),
                    LLVM::CreateLoad(*builder, arg_size));
            } else {
                ptr_as_char_ptr = lfortran_malloc(context, *module,
                    *builder, LLVM::CreateLoad(*builder, arg_size));
            }
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

        void SimpleCMODescriptor::reset_array_details(llvm::Value* arr, llvm::Value* source_arr, int n_dims) {
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
            llvm::Value* source_dim_des_arr = this->get_pointer_to_dimension_descriptor_array(source_arr);
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value* dim_val = llvm_utils->create_ptr_gep(dim_des_val, r);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* stride = this->get_stride(
                    this->get_pointer_to_dimension_descriptor(source_dim_des_arr,
                    llvm::ConstantInt::get(context, llvm::APInt(32, r))));
                builder->CreateStore(stride, s_val);
                llvm::Value* l_val = llvm_utils->create_gep(dim_val, 1);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 2);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 1)), l_val);
                llvm::Value* dim_size = this->get_dimension_size(
                   this->get_pointer_to_dimension_descriptor(source_dim_des_arr,
                    llvm::ConstantInt::get(context, llvm::APInt(32, r))));
                builder->CreateStore(dim_size, dim_size_ptr);
            }
        }

        void SimpleCMODescriptor::fill_descriptor_for_array_section(
            llvm::Value* value_desc, llvm::Value* target,
            llvm::Value** lbs, llvm::Value** ubs,
            llvm::Value** ds, llvm::Value** non_sliced_indices,
            int value_rank, int target_rank) {
            llvm::Value* value_desc_data = LLVM::CreateLoad(*builder, get_pointer_to_data(value_desc));
            std::vector<llvm::Value*> section_first_indices;
            for( int i = 0; i < value_rank; i++ ) {
                if( ds[i] != nullptr ) {
                    LCOMPILERS_ASSERT(lbs[i] != nullptr);
                    section_first_indices.push_back(lbs[i]);
                } else {
                    LCOMPILERS_ASSERT(non_sliced_indices[i] != nullptr);
                    section_first_indices.push_back(non_sliced_indices[i]);
                }
            }
            llvm::Value* target_offset = cmo_convertor_single_element(
                value_desc, section_first_indices, value_rank, false);
            value_desc_data = llvm_utils->create_ptr_gep(value_desc_data, target_offset);
            llvm::Value* target_data = get_pointer_to_data(target);
            builder->CreateStore(value_desc_data, target_data);

            builder->CreateStore(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
                get_offset(target, false));

            llvm::Value* value_dim_des_array = get_pointer_to_dimension_descriptor_array(value_desc);
            llvm::Value* target_dim_des_array = get_pointer_to_dimension_descriptor_array(target);
            int j = 0;
            for( int i = 0; i < value_rank; i++ ) {
                if( ds[i] != nullptr ) {
                    llvm::Value* dim_length = builder->CreateAdd(
                                                builder->CreateSDiv(
                                                    builder->CreateSub(ubs[i], lbs[i]),
                                                    ds[i]),
                                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                        llvm::APInt(32, 1))
                                              );
                    llvm::Value* value_dim_des = llvm_utils->create_ptr_gep(value_dim_des_array, i);
                    llvm::Value* target_dim_des = llvm_utils->create_ptr_gep(target_dim_des_array, j);
                    llvm::Value* value_stride = get_stride(value_dim_des, true);
                    llvm::Value* target_stride = get_stride(target_dim_des, false);
                    builder->CreateStore(value_stride, target_stride);
                    // Diverges from LPython, 0 should be stored there.
                    builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)),
                                         get_lower_bound(target_dim_des, false));
                    builder->CreateStore(dim_length,
                                         get_dimension_size(target_dim_des, false));
                    j++;
                }
            }
            LCOMPILERS_ASSERT(j == target_rank);
            builder->CreateStore(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                       llvm::APInt(32, target_rank)),
                get_rank(target, true));
        }

        void SimpleCMODescriptor::fill_descriptor_for_array_section_data_only(
            llvm::Value* value_desc, llvm::Value* target,
            llvm::Value** lbs, llvm::Value** ubs,
            llvm::Value** ds, llvm::Value** non_sliced_indices,
            llvm::Value** llvm_diminfo, int value_rank, int target_rank) {
            std::vector<llvm::Value*> section_first_indices;
            for( int i = 0; i < value_rank; i++ ) {
                if( ds[i] != nullptr ) {
                    LCOMPILERS_ASSERT(lbs[i] != nullptr);
                    section_first_indices.push_back(lbs[i]);
                } else {
                    LCOMPILERS_ASSERT(non_sliced_indices[i] != nullptr);
                    section_first_indices.push_back(non_sliced_indices[i]);
                }
            }
            llvm::Value* target_offset = cmo_convertor_single_element_data_only(
                llvm_diminfo, section_first_indices, value_rank, false);
            value_desc = llvm_utils->create_ptr_gep(value_desc, target_offset);
            builder->CreateStore(value_desc, get_pointer_to_data(target));

            builder->CreateStore(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
                get_offset(target, false));

            llvm::Value* target_dim_des_array = get_pointer_to_dimension_descriptor_array(target);
            int j = 0, r = 1;
            llvm::Value* stride = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            for( int i = 0; i < value_rank; i++ ) {
                if( ds[i] != nullptr ) {
                    llvm::Value* dim_length = builder->CreateAdd(
                                                builder->CreateSDiv(
                                                    builder->CreateSub(ubs[i], lbs[i]),
                                                    ds[i]),
                                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                                        llvm::APInt(32, 1))
                                              );
                    llvm::Value* target_dim_des = llvm_utils->create_ptr_gep(target_dim_des_array, j);
                    builder->CreateStore(stride,
                                         get_stride(target_dim_des, false));
                    builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), llvm::APInt(32, 1)),
                                         get_lower_bound(target_dim_des, false));
                    builder->CreateStore(dim_length,
                                         get_dimension_size(target_dim_des, false));
                    j++;
                }
                stride = builder->CreateMul(stride, llvm_diminfo[r]);
                r += 2;
            }
            LCOMPILERS_ASSERT(j == target_rank);
            builder->CreateStore(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                       llvm::APInt(32, target_rank)),
                get_rank(target, true));
        }

        llvm::Value* SimpleCMODescriptor::get_pointer_to_dimension_descriptor(llvm::Value* dim_des_arr,
            llvm::Value* dim) {
            return llvm_utils->create_ptr_gep(dim_des_arr, dim);
        }

        llvm::Value* SimpleCMODescriptor::get_pointer_to_data(llvm::Value* arr) {
            return llvm_utils->create_gep(arr, 0);
        }

        llvm::Value* SimpleCMODescriptor::get_offset(llvm::Value* arr, bool load) {
            llvm::Value* offset = llvm_utils->create_gep(arr, 1);
            if( !load ) {
                return offset;
            }
            return LLVM::CreateLoad(*builder, offset);
        }

        llvm::Value* SimpleCMODescriptor::get_lower_bound(llvm::Value* dim_des, bool load) {
            llvm::Value* lb = llvm_utils->create_gep(dim_des, 1);
            if( !load ) {
                return lb;
            }
            return LLVM::CreateLoad(*builder, lb);
        }

        llvm::Value* SimpleCMODescriptor::get_upper_bound(llvm::Value* dim_des) {
            llvm::Value* lb = LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des, 1));
            llvm::Value* dim_size = LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des, 2));
            return builder->CreateSub(builder->CreateAdd(dim_size, lb),
                                      llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
        }

        llvm::Value* SimpleCMODescriptor::get_stride(llvm::Value* dim_des, bool load) {
            llvm::Value* stride = llvm_utils->create_gep(dim_des, 0);
            if( !load ) {
                return stride;
            }
            return LLVM::CreateLoad(*builder, stride);
        }

        // TODO: Uncomment and implement later
        // void check_single_element(llvm::Value* curr_idx, llvm::Value* arr) {
        // }

        llvm::Value* SimpleCMODescriptor::cmo_convertor_single_element(
            llvm::Value* arr, std::vector<llvm::Value*>& m_args,
            int n_args, bool check_for_bounds) {
            llvm::Value* dim_des_arr_ptr = LLVM::CreateLoad(*builder, llvm_utils->create_gep(arr, 2));
            llvm::Value* idx = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            for( int r = 0; r < n_args; r++ ) {
                llvm::Value* curr_llvm_idx = m_args[r];
                llvm::Value* dim_des_ptr = llvm_utils->create_ptr_gep(dim_des_arr_ptr, r);
                llvm::Value* lval = LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des_ptr, 1));
                curr_llvm_idx = builder->CreateSub(curr_llvm_idx, lval);
                if( check_for_bounds ) {
                    // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
                }
                llvm::Value* stride = LLVM::CreateLoad(*builder, llvm_utils->create_gep(dim_des_ptr, 0));
                idx = builder->CreateAdd(idx, builder->CreateMul(stride, curr_llvm_idx));
            }
            llvm::Value* offset_val = LLVM::CreateLoad(*builder, llvm_utils->create_gep(arr, 1));
            return builder->CreateAdd(idx, offset_val);
        }

        llvm::Value* SimpleCMODescriptor::cmo_convertor_single_element_data_only(
            llvm::Value** llvm_diminfo, std::vector<llvm::Value*>& m_args,
            int n_args, bool check_for_bounds, bool is_unbounded_pointer_to_data) {
            llvm::Value* prod = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            llvm::Value* idx = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            for( int r = 0, r1 = 0; r < n_args; r++ ) {
                llvm::Value* curr_llvm_idx = m_args[r];
                llvm::Value* lval = llvm_diminfo[r1];
                curr_llvm_idx = builder->CreateSub(curr_llvm_idx, lval);
                if( check_for_bounds ) {
                    // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
                }
                idx = builder->CreateAdd(idx, builder->CreateMul(prod, curr_llvm_idx));
                if (is_unbounded_pointer_to_data) {
                    r1 += 1;
                } else {
                    llvm::Value* dim_size = llvm_diminfo[r1 + 1];
                    r1 += 2;
                    prod = builder->CreateMul(prod, dim_size);
                }
            }
            return idx;
        }

        llvm::Value* SimpleCMODescriptor::get_single_element(llvm::Value* array,
            std::vector<llvm::Value*>& m_args, int n_args, bool data_only,
            bool is_fixed_size, llvm::Value** llvm_diminfo, bool polymorphic,
            llvm::Type* polymorphic_type, bool is_unbounded_pointer_to_data) {
            llvm::Value* tmp = nullptr;
            // TODO: Uncomment later
            // bool check_for_bounds = is_explicit_shape(v);
            bool check_for_bounds = false;
            llvm::Value* idx = nullptr;
            if( data_only || is_fixed_size ) {
                LCOMPILERS_ASSERT(llvm_diminfo);
                idx = cmo_convertor_single_element_data_only(llvm_diminfo, m_args, n_args, check_for_bounds, is_unbounded_pointer_to_data);
                if( is_fixed_size ) {
                    tmp = llvm_utils->create_gep(array, idx);
                } else {
                    tmp = llvm_utils->create_ptr_gep(array, idx);
                }
            } else {
                idx = cmo_convertor_single_element(array, m_args, n_args, check_for_bounds);
                llvm::Value* full_array = get_pointer_to_data(array);
                if( polymorphic ) {
                    full_array = llvm_utils->create_gep(LLVM::CreateLoad(*builder, full_array), 1);
                    full_array = builder->CreateBitCast(LLVM::CreateLoad(*builder, full_array), polymorphic_type);
                    tmp = llvm_utils->create_ptr_gep(full_array, idx);
                } else {
                    tmp = llvm_utils->create_ptr_gep(LLVM::CreateLoad(*builder, full_array), idx);
                }
            }
            return tmp;
        }

        llvm::Value* SimpleCMODescriptor::get_is_allocated_flag(llvm::Value* array,
            llvm::Type* llvm_data_type) {
            return builder->CreateICmpNE(
                builder->CreatePtrToInt(LLVM::CreateLoad(*builder, get_pointer_to_data(array)),
                    llvm::Type::getInt64Ty(context)),
                builder->CreatePtrToInt(llvm::ConstantPointerNull::get(llvm_data_type->getPointerTo()),
                    llvm::Type::getInt64Ty(context))
            );
        }

        void SimpleCMODescriptor::reset_is_allocated_flag(llvm::Value* array,
            llvm::Type* llvm_data_type) {
            builder->CreateStore(
                llvm::ConstantPointerNull::get(llvm_data_type->getPointerTo()),
                get_pointer_to_data(array)
            );
        }

        llvm::Value* SimpleCMODescriptor::get_array_size(llvm::Value* array, llvm::Value* dim, int kind, int dim_kind) {
            llvm::Value* dim_des_val = this->get_pointer_to_dimension_descriptor_array(array);
            llvm::Value* tmp = nullptr;
            if( dim ) {
                tmp = builder->CreateSub(dim, llvm::ConstantInt::get(context, llvm::APInt(dim_kind * 8, 1)));
                tmp = this->get_dimension_size(dim_des_val, tmp);
                tmp = builder->CreateSExt(tmp, llvm_utils->getIntType(kind));
                return tmp;
            }
            llvm::BasicBlock &entry_block = builder->GetInsertBlock()->getParent()->getEntryBlock();
            llvm::IRBuilder<> builder0(context);
            builder0.SetInsertPoint(&entry_block, entry_block.getFirstInsertionPt());
            llvm::Value* rank = this->get_rank(array);
            llvm::Value* llvm_size = builder0.CreateAlloca(llvm_utils->getIntType(kind), nullptr);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(kind * 8, 1)), llvm_size);

            llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
            llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
            llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

            llvm::Value* r = builder0.CreateAlloca(llvm_utils->getIntType(4), nullptr);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), r);
            // head
            llvm_utils->start_new_block(loophead);
            llvm::Value *cond = builder->CreateICmpSLT(LLVM::CreateLoad(*builder, r), rank);
            builder->CreateCondBr(cond, loopbody, loopend);

            // body
            llvm_utils->start_new_block(loopbody);
            llvm::Value* r_val = LLVM::CreateLoad(*builder, r);
            llvm::Value* ret_val = LLVM::CreateLoad(*builder, llvm_size);
            llvm::Value* dim_size = this->get_dimension_size(dim_des_val, r_val);
            dim_size = builder->CreateSExt(dim_size, llvm_utils->getIntType(kind));
            ret_val = builder->CreateMul(ret_val, dim_size);
            builder->CreateStore(ret_val, llvm_size);
            r_val = builder->CreateAdd(r_val, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            builder->CreateStore(r_val, r);
            builder->CreateBr(loophead);

            // end
            llvm_utils->start_new_block(loopend);

            tmp = LLVM::CreateLoad(*builder, llvm_size);
            return tmp;
        }

        llvm::Value* SimpleCMODescriptor::reshape(llvm::Value* array, llvm::Type* llvm_data_type,
                                                  llvm::Value* shape, ASR::ttype_t* asr_shape_type,
                                                  llvm::Module* module) {
            llvm::Value* reshaped = builder->CreateAlloca(array->getType()->getContainedType(0), nullptr, "reshaped");

            // Deep copy data from array to reshaped.
            llvm::Value* num_elements = this->get_array_size(array, nullptr, 4);

            llvm::Value* first_ptr = this->get_pointer_to_data(reshaped);
            llvm::Value* arr_first = builder->CreateAlloca(llvm_data_type, num_elements);
            builder->CreateStore(arr_first, first_ptr);

            llvm::Value* ptr2firstptr = this->get_pointer_to_data(array);
            llvm::DataLayout data_layout(module);
            uint64_t size = data_layout.getTypeAllocSize(llvm_data_type);
            llvm::Value* llvm_size = llvm::ConstantInt::get(context, llvm::APInt(32, size));
            num_elements = builder->CreateMul(num_elements, llvm_size);
            builder->CreateMemCpy(LLVM::CreateLoad(*builder, first_ptr), llvm::MaybeAlign(),
                                  LLVM::CreateLoad(*builder, ptr2firstptr), llvm::MaybeAlign(),
                                  num_elements);

            if( this->is_array(asr_shape_type) ) {
                builder->CreateStore(LLVM::CreateLoad(*builder, llvm_utils->create_gep(array, 1)),
                            llvm_utils->create_gep(reshaped, 1));
                llvm::Value* n_dims = this->get_array_size(shape, nullptr, 4);
                llvm::Value* shape_data = LLVM::CreateLoad(*builder, this->get_pointer_to_data(shape));
                llvm::Value* dim_des_val = llvm_utils->create_gep(reshaped, 2);
                llvm::Value* dim_des_first = builder->CreateAlloca(dim_des, n_dims);
                builder->CreateStore(n_dims, this->get_rank(reshaped, true));
                builder->CreateStore(dim_des_first, dim_des_val);
                llvm::Value* prod = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
                dim_des_val = LLVM::CreateLoad(*builder, dim_des_val);
                llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
                llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
                llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

                llvm::Value* r = builder->CreateAlloca(llvm_utils->getIntType(4), nullptr);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), r);
                // head
                llvm_utils->start_new_block(loophead);
                llvm::Value *cond = builder->CreateICmpSLT(LLVM::CreateLoad(*builder, r), n_dims);
                builder->CreateCondBr(cond, loopbody, loopend);

                // body
                llvm_utils->start_new_block(loopbody);
                llvm::Value* r_val = LLVM::CreateLoad(*builder, r);
                llvm::Value* dim_val = llvm_utils->create_ptr_gep(dim_des_val, r_val);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 2);
                builder->CreateStore(prod, s_val);
                llvm::Value* dim_size = LLVM::CreateLoad(*builder, llvm_utils->create_ptr_gep(shape_data, r_val));
                prod = builder->CreateMul(prod, dim_size);
                builder->CreateStore(dim_size, dim_size_ptr);
                r_val = builder->CreateAdd(r_val, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                builder->CreateStore(r_val, r);
                builder->CreateBr(loophead);

                // end
                llvm_utils->start_new_block(loopend);
            }
            return reshaped;
        }

        // Shallow copies source array descriptor to destination descriptor
        void SimpleCMODescriptor::copy_array(llvm::Value* src, llvm::Value* dest,
            llvm::Module* module, ASR::ttype_t* asr_data_type, bool create_dim_des_array,
            bool reserve_memory) {
            llvm::Value* num_elements = this->get_array_size(src, nullptr, 4);

            llvm::Value* first_ptr = this->get_pointer_to_data(dest);
            llvm::Type* llvm_data_type = tkr2array[ASRUtils::get_type_code(ASRUtils::type_get_past_pointer(
                ASRUtils::type_get_past_allocatable(asr_data_type)), false, false)].second;
            if( reserve_memory ) {
                llvm::Value* arr_first = builder->CreateAlloca(llvm_data_type, num_elements);
                builder->CreateStore(arr_first, first_ptr);
            }

            llvm::Value* ptr2firstptr = this->get_pointer_to_data(src);
            llvm::DataLayout data_layout(module);
            uint64_t size = data_layout.getTypeAllocSize(llvm_data_type);
            llvm::Value* llvm_size = llvm::ConstantInt::get(context, llvm::APInt(32, size));
            num_elements = builder->CreateMul(num_elements, llvm_size);
            builder->CreateMemCpy(LLVM::CreateLoad(*builder, first_ptr), llvm::MaybeAlign(),
                                  LLVM::CreateLoad(*builder, ptr2firstptr), llvm::MaybeAlign(),
                                  num_elements);

            llvm::Value* src_dim_des_val = this->get_pointer_to_dimension_descriptor_array(src, true);
            llvm::Value* n_dims = this->get_rank(src, false);
            llvm::Value* dest_dim_des_val = nullptr;
            if( !create_dim_des_array ) {
                dest_dim_des_val = this->get_pointer_to_dimension_descriptor_array(dest, true);
            } else {
                llvm::Value* src_offset_ptr = LLVM::CreateLoad(*builder, llvm_utils->create_gep(src, 1));
                builder->CreateStore(src_offset_ptr, llvm_utils->create_gep(dest, 1));
                llvm::Value* dest_dim_des_ptr = this->get_pointer_to_dimension_descriptor_array(dest, false);
                dest_dim_des_val = builder->CreateAlloca(dim_des, n_dims);
                builder->CreateStore(dest_dim_des_val, dest_dim_des_ptr);
            }
            llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head");
            llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
            llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");

            llvm::Value* r = builder->CreateAlloca(llvm_utils->getIntType(4), nullptr);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), r);
            // head
            llvm_utils->start_new_block(loophead);
            llvm::Value *cond = builder->CreateICmpSLT(LLVM::CreateLoad(*builder, r), n_dims);
            builder->CreateCondBr(cond, loopbody, loopend);

            // body
            llvm_utils->start_new_block(loopbody);
            llvm::Value* r_val = LLVM::CreateLoad(*builder, r);
            llvm::Value* src_dim_val = llvm_utils->create_ptr_gep(src_dim_des_val, r_val);
            llvm::Value* src_l_val = nullptr;
            llvm::Value* src_s_val = nullptr;
            if( create_dim_des_array ) {
                src_s_val = llvm_utils->create_gep(src_dim_val, 0);
                src_l_val = llvm_utils->create_gep(src_dim_val, 1);
            }
            llvm::Value* src_dim_size_ptr = llvm_utils->create_gep(src_dim_val, 2);
            llvm::Value* dest_dim_val = llvm_utils->create_ptr_gep(dest_dim_des_val, r_val);
            llvm::Value* dest_s_val = nullptr;
            llvm::Value* dest_l_val = nullptr;
            llvm::Value* dest_dim_size_ptr = nullptr;
            if( create_dim_des_array ) {
                dest_s_val = llvm_utils->create_gep(dest_dim_val, 0);
                dest_l_val = llvm_utils->create_gep(dest_dim_val, 1);
                dest_dim_size_ptr = llvm_utils->create_gep(dest_dim_val, 2);
            }

            if( create_dim_des_array ) {
                builder->CreateStore(LLVM::CreateLoad(*builder, src_l_val), dest_l_val);
                builder->CreateStore(LLVM::CreateLoad(*builder, src_s_val), dest_s_val);
                builder->CreateStore(LLVM::CreateLoad(*builder, src_dim_size_ptr), dest_dim_size_ptr);
            }
            r_val = builder->CreateAdd(r_val, llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
            builder->CreateStore(r_val, r);
            builder->CreateBr(loophead);

            // end
            llvm_utils->start_new_block(loopend);


            builder->CreateStore(n_dims, this->get_rank(dest, true));
        }

        void SimpleCMODescriptor::copy_array_data_only(llvm::Value* src, llvm::Value* dest,
            llvm::Module* module, llvm::Type* llvm_data_type, llvm::Value* num_elements) {
            llvm::DataLayout data_layout(module);
            uint64_t size = data_layout.getTypeAllocSize(llvm_data_type);
            llvm::Value* llvm_size = llvm::ConstantInt::get(context, llvm::APInt(32, size));
            num_elements = builder->CreateMul(num_elements, llvm_size);
            builder->CreateMemCpy(src, llvm::MaybeAlign(), dest, llvm::MaybeAlign(), num_elements);
        }

    } // LLVMArrUtils

} // namespace LCompilers
