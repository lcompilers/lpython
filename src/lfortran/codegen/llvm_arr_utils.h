#ifndef LFORTRAN_LLVM_ARR_UTILS_H
#define LFORTRAN_LLVM_ARR_UTILS_H

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>

#include <lfortran/parser/alloc.h>
#include <lfortran/asr.h>
#include <lfortran/codegen/asr_to_llvm.h>

#include <string>
#include <map>

namespace LFortran {

    namespace LLVMArrUtils {

        enum DESCR_TYPE
        {
            _SimpleCMODescriptor
        };

        class Descriptor {

            public:

                static
                Descriptor*
                get_descriptor(
                    Allocator& al,
                    llvm::LLVMContext& context,
                    DESCR_TYPE descr_type);

                virtual
                llvm::Type* get_array_type(
                ASR::ttype_t* m_type_, int a_kind,
                int rank, ASR::dimension_t* m_dims,
                llvm::Type* el_type,
                bool get_pointer=false);

                virtual
                llvm::Type* get_malloc_array_type(
                ASR::ttype_t* m_type_, int a_kind,
                int rank, llvm::Type* el_type,
                bool get_pointer=false);

                virtual
                llvm::ArrayType* get_dim_des_array(int rank);

                virtual
                llvm::Type* get_malloc_array_type
                (ASR::ttype_t* m_type_, int a_kind,
                int rank, bool get_pointer=false);

                virtual
                void fill_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, 
                int n_dims);

                virtual
                void fill_malloc_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, 
                int n_dims);

                virtual
                llvm::Type* get_dimension_descriptor(bool get_pointer=false);

                virtual
                bool is_matching_dimension_descriptor(llvm::ArrayType* des, int rank);

                virtual
                llvm::Value* get_pointer_to_memory_block();

                virtual
                llvm::Value* get_offset();

                virtual
                llvm::Value* get_lower_bound(const int dim);

                virtual
                llvm::Value* get_upper_bound(const int dim);

                virtual
                llvm::Value* get_stride(const int dim);

                virtual
                llvm::Value* get_single_element(ASR::ArrayRef_t& x);

        };

        class SimpleCMODescriptor: public Descriptor {

            private:

                llvm::LLVMContext& context;

                llvm::StructType* dim_des;
                std::map<int, llvm::ArrayType*> rank2desc;

                std::map<std::pair<std::pair<int, int>, std::pair<int, int>>, llvm::StructType*> tkr2array;

                std::map<std::pair<std::pair<int, int>, int>, llvm::StructType*> tkr2mallocarray;

            public:

                SimpleCMODescriptor();

                virtual
                llvm::Type* get_el_type(ASR::ttype_t* m_type_, int a_kind);

                virtual
                llvm::Type* get_array_type(
                ASR::ttype_t* m_type_, int a_kind,
                int rank, ASR::dimension_t* m_dims,
                llvm::Type* el_type,
                bool get_pointer=false);

                virtual
                llvm::Type* get_malloc_array_type(
                ASR::ttype_t* m_type_, int a_kind,
                int rank, llvm::Type* el_type,
                bool get_pointer=false);

                virtual
                llvm::ArrayType* get_dim_des_array(int rank);

                virtual
                llvm::Type* get_malloc_array_type
                (ASR::ttype_t* m_type_, int a_kind,
                int rank, bool get_pointer=false);

                virtual
                void fill_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, 
                int n_dims);

                virtual
                void fill_malloc_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, 
                int n_dims);

                virtual
                llvm::Type* get_dimension_descriptor(bool get_pointer=false);

                virtual
                bool is_matching_dimension_descriptor(llvm::ArrayType* des, int rank);

                virtual
                llvm::Value* get_pointer_to_memory_block();

                virtual
                llvm::Value* get_offset();

                virtual
                llvm::Value* get_lower_bound(const int dim);

                virtual
                llvm::Value* get_upper_bound(const int dim);

                virtual
                llvm::Value* get_stride(const int dim);

                virtual
                llvm::Value* get_single_element(ASR::ArrayRef_t& x);

        };

    } // LLVMArrUtils

} // LFortran

#endif // LFORTRAN_LLVM_ARR_UTILS_H
