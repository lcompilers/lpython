#include <lfortran/codegen/llvm_arr_utils.h>
#include <lfortran/codegen/llvm_utils.h>

namespace LFortran {

    namespace LLVMArrUtils {

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
                    throw CodeGenError("Explicit shape checking supported only for integer, real, complex, logical and derived types.");
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

        llvm::Type* Descriptor::get_array_type
        (ASR::ttype_t*, int, int, ASR::dimension_t*,
         llvm::Type*, bool) {
            return nullptr;
        }

        llvm::Type* Descriptor::get_malloc_array_type
        (ASR::ttype_t*, int, int, llvm::Type*, bool) {
            return nullptr;
        }

        void Descriptor::fill_array_details(
        llvm::Value*, ASR::dimension_t*, int,
        std::vector<std::pair<llvm::Value*, llvm::Value*>>&) {
        }

        void Descriptor::fill_malloc_array_details(
        llvm::Value*, ASR::dimension_t*, int,
        std::vector<std::pair<llvm::Value*, llvm::Value*>>&,
        llvm::Module*) {
        }

        llvm::Type* Descriptor::get_dimension_descriptor(bool) {
            return nullptr;
        }

        bool Descriptor::is_matching_dimension_descriptor(llvm::ArrayType*, int) {
            return false;
        }

        llvm::Value* Descriptor::get_pointer_to_memory_block() {
            return nullptr;
        }

        llvm::Value* Descriptor::get_offset() {
            return nullptr;
        }

        llvm::Value* Descriptor::get_lower_bound(const int) {
            return nullptr;
        }

        llvm::Value* Descriptor::get_upper_bound(const int) {
            return nullptr;
        }

        llvm::Value* Descriptor::get_stride(const int) {
            return nullptr;
        }

        llvm::Value* Descriptor::get_single_element(llvm::Value*,
            std::vector<llvm::Value*>&, int) {
            return nullptr;
        }

        llvm::ArrayType* Descriptor::get_dim_des_array(int) {
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

        llvm::Type* SimpleCMODescriptor::get_array_type
        (ASR::ttype_t* m_type_, int a_kind,
        int rank, ASR::dimension_t* m_dims,
        llvm::Type* el_type,
        bool get_pointer) {
            ASR::ttypeType type_ = m_type_->type;
            int size = 0;
            if( compile_time_dimensions_t(m_dims, rank) ) {
                size = 1;
                for( int r = 0; r < rank; r++ ) {
                    ASR::dimension_t m_dim = m_dims[r];
                    int start = ASR::down_cast<ASR::ConstantInteger_t>(m_dim.m_start)->m_n;
                    int end = ASR::down_cast<ASR::ConstantInteger_t>(m_dim.m_end)->m_n;
                    size *= (end - start + 1);
                }
            }
            std::pair<std::pair<int, int>, std::pair<int, int>> array_key = std::make_pair(std::make_pair((int)type_, a_kind), std::make_pair(rank, size));
            if( tkr2array.find(array_key) != tkr2array.end() ) {
                if( get_pointer ) {
                    return tkr2array[array_key]->getPointerTo();
                }
                return tkr2array[array_key];
            }
            llvm::ArrayType* dim_des_array = get_dim_des_array(rank);
            std::vector<llvm::Type*> array_type_vec;
            if( size > 0 ) {
                array_type_vec = {  llvm::ArrayType::get(el_type, size), 
                                    llvm::Type::getInt32Ty(context),
                                    dim_des_array  };
            } else {
                array_type_vec = {  el_type->getPointerTo(),
                                    llvm::Type::getInt32Ty(context),
                                    dim_des_array  };
            }
            tkr2array[array_key] = llvm::StructType::create(context, array_type_vec, "array");
            if( get_pointer ) {
                return tkr2array[array_key]->getPointerTo();
            }
            return (llvm::Type*) tkr2array[array_key];
        }

        llvm::ArrayType* SimpleCMODescriptor::get_dim_des_array(int rank) {
            if( rank2desc.find(rank) != rank2desc.end() ) {
                return rank2desc[rank];
            } 
            rank2desc[rank] = llvm::ArrayType::get(dim_des, rank);
            return rank2desc[rank];
        }

        llvm::Type* SimpleCMODescriptor::get_malloc_array_type
        (ASR::ttype_t* m_type_, int a_kind, int rank, llvm::Type* el_type, bool get_pointer) {
            ASR::ttypeType type_ = m_type_->type;
            std::pair<std::pair<int, int>, int> array_key = std::make_pair(std::make_pair((int)type_, a_kind), rank);
            if( tkr2mallocarray.find(array_key) != tkr2mallocarray.end() ) {
                if( get_pointer ) {
                    return tkr2mallocarray[array_key]->getPointerTo();
                }
                return tkr2mallocarray[array_key];
            }
            llvm::ArrayType* dim_des_array = get_dim_des_array(rank);
            std::vector<llvm::Type*> array_type_vec = {
                el_type->getPointerTo(), 
                llvm::Type::getInt32Ty(context),
                dim_des_array};
            tkr2mallocarray[array_key] = llvm::StructType::create(context, array_type_vec, "array");
            if( get_pointer ) {
                return tkr2mallocarray[array_key]->getPointerTo();
            }
            return (llvm::Type*) tkr2mallocarray[array_key];
        }

        llvm::Type* SimpleCMODescriptor::get_dimension_descriptor
        (bool get_pointer) {
            if( !get_pointer ) {
                return dim_des;
            }
            return dim_des->getPointerTo();
        }

        bool SimpleCMODescriptor::is_matching_dimension_descriptor
        (llvm::ArrayType* des, int rank) {
            return (rank2desc.find(rank) != rank2desc.end() && 
                    rank2desc[rank] == des);
        }

        void SimpleCMODescriptor::fill_array_details(
        llvm::Value* arr, ASR::dimension_t* m_dims, int n_dims,
        std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims) {
            bool run_time_size = !LLVMArrUtils::compile_time_dimensions_t(m_dims, n_dims);
            llvm::Value* offset_val = llvm_utils->create_gep(arr, 1);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), offset_val);
            llvm::Value* dim_des_val = llvm_utils->create_gep(arr, 2);
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value* dim_val = llvm_utils->create_gep(dim_des_val, r);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* l_val = llvm_utils->create_gep(dim_val, 1);
                llvm::Value* u_val = llvm_utils->create_gep(dim_val, 2);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 3);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 1)), s_val);
                builder->CreateStore(llvm_dims[r].first, l_val);
                builder->CreateStore(llvm_dims[r].second, u_val);
                u_val = builder->CreateLoad(u_val);
                l_val = builder->CreateLoad(l_val);
                llvm::Value* dim_size = builder->CreateAdd(builder->CreateSub(u_val, l_val), 
                                                        llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                builder->CreateStore(dim_size, dim_size_ptr);
            }
            if( run_time_size ) {
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
                llvm::Value* first_ptr = llvm_utils->create_gep(arr, 0);
                llvm::PointerType* first_ptr2ptr_type = static_cast<llvm::PointerType*>(first_ptr->getType());
                llvm::PointerType* first_ptr_type = static_cast<llvm::PointerType*>(first_ptr2ptr_type->getElementType());
                llvm::Value* arr_first = builder->CreateAlloca(first_ptr_type->getElementType(), 
                                                                builder->CreateLoad(llvm_size));
                builder->CreateStore(arr_first, first_ptr);
            }
        }

        void SimpleCMODescriptor::fill_malloc_array_details(
        llvm::Value* arr, ASR::dimension_t* m_dims, int n_dims,
        std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
        llvm::Module* module) {
            llvm::Value* num_elements = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            llvm::Value* offset_val = llvm_utils->create_gep(arr, 1);
            builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 0)), offset_val);
            llvm::Value* dim_des_val = llvm_utils->create_gep(arr, 2);
            for( int r = 0; r < n_dims; r++ ) {
                llvm::Value* dim_val = llvm_utils->create_gep(dim_des_val, r);
                llvm::Value* s_val = llvm_utils->create_gep(dim_val, 0);
                llvm::Value* l_val = llvm_utils->create_gep(dim_val, 1);
                llvm::Value* u_val = llvm_utils->create_gep(dim_val, 2);
                llvm::Value* dim_size_ptr = llvm_utils->create_gep(dim_val, 3);
                builder->CreateStore(llvm::ConstantInt::get(context, llvm::APInt(32, 1)), s_val);
                builder->CreateStore(llvm_dims[r].first, l_val);
                builder->CreateStore(llvm_dims[r].second, u_val);
                u_val = builder->CreateLoad(u_val);
                l_val = builder->CreateLoad(l_val);
                llvm::Value* dim_size = builder->CreateAdd(builder->CreateSub(u_val, l_val), 
                                                            llvm::ConstantInt::get(context, llvm::APInt(32, 1)));
                num_elements = builder->CreateMul(num_elements, dim_size);
                builder->CreateStore(dim_size, dim_size_ptr);
            }
            std::string func_name = "_lfortran_malloc";
            llvm::Function *fn = module->getFunction(func_name);
            llvm::Value* ptr2firstptr = llvm_utils->create_gep(arr, 0);
            if (!fn) {
                llvm::FunctionType *function_type = llvm::FunctionType::get(
                        llvm::Type::getInt8PtrTy(context), {
                            llvm::Type::getInt32Ty(context)
                        }, true);
                fn = llvm::Function::Create(function_type,
                        llvm::Function::ExternalLinkage, func_name, *module);
            }
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
            std::vector<llvm::Value*> args = {builder->CreateLoad(arg_size)};
            llvm::Value* ptr_as_char_ptr = builder->CreateCall(fn, args);
            llvm::Value* first_ptr = builder->CreateBitCast(ptr_as_char_ptr, ptr_type);
            builder->CreateStore(first_ptr, ptr2firstptr);
        }

        llvm::Value* SimpleCMODescriptor::get_pointer_to_memory_block() {
            return nullptr;
        }

        llvm::Value* SimpleCMODescriptor::get_offset() {
            return nullptr;
        }

        llvm::Value* SimpleCMODescriptor::get_lower_bound(const int) {
            return nullptr;
        }

        llvm::Value* SimpleCMODescriptor::get_upper_bound(const int) {
            return nullptr;
        }

        llvm::Value* SimpleCMODescriptor::get_stride(const int) {
            return nullptr;
        }

        // TODO: Uncomment and implement later
        // void check_single_element(llvm::Value* curr_idx, llvm::Value* arr) {
        // }

        llvm::Value* SimpleCMODescriptor::cmo_convertor_single_element(
            llvm::Value* arr, std::vector<llvm::Value*>& m_args, 
            int n_args, bool check_for_bounds) {
            llvm::Value* dim_des_arr_ptr = llvm_utils->create_gep(arr, 2);
            llvm::Value* prod = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
            llvm::Value* idx = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            for( int r = 0; r < n_args; r++ ) {
                llvm::Value* curr_llvm_idx = m_args[r];
                llvm::Value* dim_des_ptr = llvm_utils->create_gep(dim_des_arr_ptr, r);
                llvm::Value* lval = builder->CreateLoad(llvm_utils->create_gep(dim_des_ptr, 1));
                curr_llvm_idx = builder->CreateSub(curr_llvm_idx, lval);
                if( check_for_bounds ) {
                    // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
                }
                idx = builder->CreateAdd(idx, builder->CreateMul(prod, curr_llvm_idx));
                llvm::Value* dim_size = builder->CreateLoad(llvm_utils->create_gep(dim_des_ptr, 3));
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
            llvm::Value* full_array = llvm_utils->create_gep(array, 0);
            if( static_cast<llvm::PointerType*>(full_array->getType())
                ->getElementType()->isArrayTy() ) {
                tmp = llvm_utils->create_gep(full_array, idx);
            } else {
                tmp = llvm_utils->create_ptr_gep(builder->CreateLoad(full_array), idx);
            }
            return tmp;
        }

    } // LLVMArrUtils

} // LFortran
