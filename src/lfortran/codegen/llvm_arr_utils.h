#ifndef LFORTRAN_LLVM_ARR_UTILS_H
#define LFORTRAN_LLVM_ARR_UTILS_H

#include <memory>

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>

#include <lfortran/parser/alloc.h>
#include <lfortran/asr.h>
#include <lfortran/codegen/llvm_utils.h>

#include <string>
#include <map>

namespace LFortran {

    namespace LLVMArrUtils {

        bool compile_time_dimensions_t(
        ASR::dimension_t* m_dims, int n_dims);

        bool is_explicit_shape(ASR::Variable_t* v);

        enum DESCR_TYPE
        {
            _SimpleCMODescriptor
        };

        class Descriptor {

            public:

                static
                std::unique_ptr<Descriptor>
                get_descriptor(
                    llvm::LLVMContext& context,
                    llvm::IRBuilder<>* builder,
                    LLVMUtils* llvm_utils,
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
                void fill_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, int n_dims, 
                std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims);

                virtual
                void fill_malloc_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, int n_dims,
                std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
                llvm::Module* module);

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
                llvm::Value* get_single_element(llvm::Value* array,
                    std::vector<llvm::Value*>& m_args, int n_args);

        };

        class SimpleCMODescriptor: public Descriptor {

            private:

                llvm::LLVMContext& context;
                LLVMUtils* llvm_utils;
                llvm::IRBuilder<>* builder;

                llvm::StructType* dim_des;
                std::map<int, llvm::ArrayType*> rank2desc;

                std::map<std::pair<std::pair<int, int>, std::pair<int, int>>, llvm::StructType*> tkr2array;

                std::map<std::pair<std::pair<int, int>, int>, llvm::StructType*> tkr2mallocarray;

                llvm::Value* cmo_convertor_single_element(
                llvm::Value* arr, std::vector<llvm::Value*>& m_args, 
                int n_args, bool check_for_bounds);

            public:

                SimpleCMODescriptor(llvm::LLVMContext& _context,
                    llvm::IRBuilder<>* _builder,
                    LLVMUtils* _llvm_utils);

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
                void fill_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, int n_dims, 
                std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims);

                virtual
                void fill_malloc_array_details(
                llvm::Value* arr, ASR::dimension_t* m_dims, int n_dims,
                std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
                llvm::Module* module);

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
                llvm::Value* get_single_element(llvm::Value* array,
                    std::vector<llvm::Value*>& m_args, int n_args);

        };

    } // LLVMArrUtils

} // LFortran

#endif // LFORTRAN_LLVM_ARR_UTILS_H
