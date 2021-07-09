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
                bool is_array(llvm::Value* tmp);

                virtual
                llvm::Value* convert_to_argument(llvm::Value* tmp,
                llvm::Type* arg_type);

                virtual
                llvm::Type* get_argument_type(llvm::Type* type,
                std::uint32_t, std::string,
                std::unordered_map<std::uint32_t, std::unordered_map<std::string, llvm::Type*>>& arr_arg_type_cache);

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
                llvm::ArrayType* create_dimension_descriptor_array_type(int rank);

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
                llvm::Type* get_dimension_descriptor_type(bool get_pointer=false);

                virtual
                bool is_matching_dimension_descriptor(llvm::ArrayType* des, int rank);

                virtual
                llvm::Value* get_pointer_to_memory_block(llvm::Value* arr);

                virtual
                llvm::Value* get_offset(llvm::Value* dim_des);

                virtual
                llvm::Value* get_lower_bound(llvm::Value* dim_des);

                virtual
                llvm::Value* get_upper_bound(llvm::Value* dim_des);

                virtual
                llvm::Value* get_stride(llvm::Value* dim_des);

                virtual
                llvm::Value* get_dimension_size(llvm::Value* dim_des_arr,
                    llvm::Value* dim);
                
                virtual
                llvm::Value* get_pointer_to_dimension_descriptor_array(llvm::Value* arr);

                virtual
                llvm::Value* get_pointer_to_dimension_descriptor(llvm::Value* dim_des_arr, 
                    llvm::Value* dim);

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
                bool is_array(llvm::Value* tmp);

                virtual
                llvm::Value* convert_to_argument(llvm::Value* tmp,
                llvm::Type* arg_type);

                virtual
                llvm::Type* get_argument_type(llvm::Type* type,
                std::uint32_t m_h, std::string arg_name,
                std::unordered_map<std::uint32_t, std::unordered_map<std::string, llvm::Type*>>& arr_arg_type_cache);

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
                llvm::ArrayType* create_dimension_descriptor_array_type(int rank);

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
                llvm::Type* get_dimension_descriptor_type(bool get_pointer=false);

                virtual
                bool is_matching_dimension_descriptor(llvm::ArrayType* des, int rank);

                virtual
                llvm::Value* get_pointer_to_memory_block(llvm::Value* arr);

                virtual
                llvm::Value* get_offset(llvm::Value* dim_des);

                virtual
                llvm::Value* get_lower_bound(llvm::Value* dim_des);

                virtual
                llvm::Value* get_upper_bound(llvm::Value* dim_des);

                virtual
                llvm::Value* get_dimension_size(llvm::Value* dim_des_arr,
                    llvm::Value* dim);
                
                virtual
                llvm::Value* get_pointer_to_dimension_descriptor_array(llvm::Value* arr);

                virtual
                llvm::Value* get_pointer_to_dimension_descriptor(llvm::Value* dim_des_arr, 
                    llvm::Value* dim);

                virtual
                llvm::Value* get_stride(llvm::Value* dim_des);

                virtual
                llvm::Value* get_single_element(llvm::Value* array,
                    std::vector<llvm::Value*>& m_args, int n_args);

        };

    } // LLVMArrUtils

} // LFortran

#endif // LFORTRAN_LLVM_ARR_UTILS_H
