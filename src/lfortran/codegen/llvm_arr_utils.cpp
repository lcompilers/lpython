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

        std::unique_ptr<LLVMArrUtils::Descriptor>
        Descriptor::get_descriptor
        (llvm::LLVMContext& context,
         std::unique_ptr<LLVMUtils>& llvm_utils,
         std::unique_ptr<llvm::IRBuilder<>>& builder;
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
        llvm::Value*, ASR::dimension_t*, int) {
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

        llvm::Value* Descriptor::get_single_element(ASR::ArrayRef_t&) {
            return nullptr;
        }

        llvm::ArrayType* Descriptor::get_dim_des_array(int) {
            return nullptr;
        }

        SimpleCMODescriptor::SimpleCMODescriptor(llvm::LLVMContext &context,
            std::unique_ptr<llvm::IRBuilder<>>& builder,
            std::unique_ptr<LLVMUtils>& llvm_utils):
        context(context),
        llvm_utils(llvm_utils),
        builder(builder),
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
                ASR::dimension_t m_dim = m_dims[r];
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
                llvm::Value* llvm_size = builder->CreateAlloca(getIntType(4), nullptr);
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
        llvm::Value*, ASR::dimension_t*, int) {
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

        llvm::Value* SimpleCMODescriptor::get_single_element(ASR::ArrayRef_t&) {
            return nullptr;
        }

    } // LLVMArrUtils

} // LFortran
