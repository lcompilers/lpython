#include <lfortran/codegen/llvm_arr_utils.h>

namespace LFortran {

    namespace LLVMArrUtils {

        Descriptor*
        Descriptor::get_descriptor
        (Allocator& al,
         llvm::LLVMContext& context,
         DESCR_TYPE descr_type) {
            switch( descr_type ) {
                case DESCR_TYPE::_SimpleCMODescriptor: {
                    return al.make_new<SimpleCMODescriptor>(context);
                }
            }
            return nullptr;
        }

        llvm::Type* Descriptor::get_array_type
        (ASR::ttype_t*, int, int, ASR::dimension_t*,
         bool) {
            return nullptr;
        }

        llvm::Type* Descriptor::get_malloc_array_type
        (ASR::ttype_t*, int, int, bool) {
            return nullptr;
        }

        void Descriptor::fill_array_details(
        llvm::Value*, ASR::dimension_t*, int) {
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

        SimpleCMODescriptor::SimpleCMODescriptor(llvm::LLVMContext &context):
        context(context),
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
        bool get_pointer=false) {
            ASR::ttypeType type_ = m_type_->type;
            int size = 0;
            if( compile_time_dimensions_t(m_dims, rank) ) {
                size = 1;
                for( int r = 0; r < rank; r++ ) {
                    ASR::dimension_t m_dim = m_dims[r];
                    int start = down_cast<ASR::ConstantInteger_t>(m_dim.m_start)->m_n;
                    int end = down_cast<ASR::ConstantInteger_t>(m_dim.m_end)->m_n;
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
                                    getIntType(4),
                                    dim_des_array  };
            } else {
                array_type_vec = {  el_type->getPointerTo(),
                                    getIntType(4),
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
        (ASR::ttype_t* m_type_, int a_kind, int rank, llvm::Type* el_type, bool get_pointer=false) {
            ASR::ttypeType type_ = m_type_->type;
            std::pair<std::pair<int, int>, int> array_key = std::make_pair(std::make_pair((int)type_, a_kind), rank);
            if( tkr2mallocarray.find(array_key) != tkr2mallocarray.end() ) {
                if( get_pointer ) {
                    return tkr2mallocarray[array_key]->getPointerTo();
                }
                return tkr2mallocarray[array_key];
            }
            llvm::ArrayType* dim_des_array = arr_descr->get_dim_des_array(rank);
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

    } // LLVMArrUtils

} // LFortran
