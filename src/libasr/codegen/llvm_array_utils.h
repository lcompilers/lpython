#ifndef LFORTRAN_LLVM_ARR_UTILS_H
#define LFORTRAN_LLVM_ARR_UTILS_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>

#include <libasr/alloc.h>
#include <libasr/asr.h>
#include <libasr/codegen/llvm_utils.h>

namespace LFortran {

    namespace LLVMArrUtils {

        llvm::Value* lfortran_malloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* arg_size);

        /*
        * This function checks whether the
        * dimensions are available at compile time.
        * Returns true if all the dimensions reduce
        * to constant integers and false otherwise.
        */
        bool compile_time_dimensions_t(
        ASR::dimension_t* m_dims, int n_dims);

        /*
        * This function checks if the given
        * an variable is an array and all the
        * dimensions are available at compile time.
        */
        bool is_explicit_shape(ASR::Variable_t* v);

        /*
        * Available descriptors are listed
        * under this enum.
        */
        enum DESCR_TYPE
        {
            _SimpleCMODescriptor
        };

        /*
        * Abstract class which defines the interface
        * to be followed by any subclass intending
        * to implement a specific array descriptor.
        */
        class Descriptor {

            public:

                virtual ~Descriptor() {}

                /*
                * Factory method which creates
                * new descriptors and returns a
                * pointer to it. It accepts one of
                * the members DESCR_TYPE enum
                * to create a new descriptor.
                */
                static
                std::unique_ptr<Descriptor>
                get_descriptor(
                    llvm::LLVMContext& context,
                    llvm::IRBuilder<>* builder,
                    LLVMUtils* llvm_utils,
                    DESCR_TYPE descr_type);

                /*
                * Checks whether the given llvm::Value* is an
                * array and follows the same structure as
                * the current descriptor.
                */
                virtual
                bool is_array(llvm::Value* tmp) = 0;

                /*
                * Converts a given array llvm::Value*
                * into an argument of the specified type.
                */
                virtual
                llvm::Value* convert_to_argument(llvm::Value* tmp,
                    llvm::Type* arg_type, bool data_only=false) = 0;

                /*
                * Returns the type of the argument to be
                * used in LLVM functions for passing an array
                * following the current descriptor structure.
                */
                virtual
                llvm::Type* get_argument_type(llvm::Type* type,
                    std::uint32_t, std::string,
                    std::unordered_map
                    <std::uint32_t, std::unordered_map
                                    <std::string, llvm::Type*>>&
                                    arr_arg_type_cache) = 0;

                /*
                * Creates an array llvm::Type* following
                * the same structure as the current descriptor.
                * Uses element type, kind, rank and dimensions
                * to create the array llvm::Type*.
                */
                virtual
                llvm::Type* get_array_type(
                    ASR::ttype_t* m_type_, int a_kind,
                    int rank, llvm::Type* el_type,
                    bool get_pointer=false) = 0;

                /*
                * Same as get_array_type but for allocatable
                * arrays. It doesn't require dimensions for
                * creating array llvm::Type*.
                */
                virtual
                llvm::Type* get_malloc_array_type(
                    ASR::ttype_t* m_type_, int a_kind,
                    int rank, llvm::Type* el_type,
                    bool get_pointer=false) = 0;

                /*
                * Creates an array of dimension descriptors
                * whose each element describes structure
                * of a dimension's information.
                */
                virtual
                llvm::Type* create_dimension_descriptor_array_type() = 0;

                /*
                * Fills the elements of the input array descriptor
                * for arrays on stack memory.
                */
                virtual
                void fill_array_details(
                    llvm::Value* arr, int n_dims,
                    std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims) = 0;

                /*
                * Fills the elements of the input array descriptor
                * for allocatable arrays.
                */
                virtual
                void fill_malloc_array_details(
                    llvm::Value* arr, int n_dims,
                    std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
                    llvm::Module* module) = 0;

                virtual
                void fill_dimension_descriptor(
                    llvm::Value* arr, int n_dims) = 0;

                /*
                * Returns the llvm::Type* associated with the
                * dimension descriptor used by the current class.
                */
                virtual
                llvm::Type* get_dimension_descriptor_type(bool get_pointer=false) = 0;

                /*
                * Returns pointer to data in the input
                * array descriptor according to the rules
                * implemented by current class.
                */
                virtual
                llvm::Value* get_pointer_to_data(llvm::Value* arr) = 0;

                /*
                * Returns offset in the input
                * array descriptor according to the rules
                * implemented by current class).
                */
                virtual
                llvm::Value* get_offset(llvm::Value* dim_des) = 0;

                /*
                * Returns lower bound in the input
                * dimension descriptor according to the rules
                * implemented by current class).
                */
                virtual
                llvm::Value* get_lower_bound(llvm::Value* dim_des, bool load=true) = 0;

                /*
                * Returns upper bound in the input
                * dimension descriptor according to the rules
                * implemented by current class.
                */
                virtual
                llvm::Value* get_upper_bound(llvm::Value* dim_des) = 0;

                /*
                * Returns stride in the input
                * dimension descriptor according to the rules
                * implemented by current class.
                */
                virtual
                llvm::Value* get_stride(llvm::Value* dim_des) = 0;

                /*
                * Returns dimension size in the input
                * dimension descriptor according to the rules
                * implemented by current class.
                */
                virtual
                llvm::Value* get_dimension_size(llvm::Value* dim_des_arr,
                    llvm::Value* dim, bool load=true) = 0;

                virtual
                llvm::Value* get_rank(llvm::Value* arr, bool get_pointer=false) = 0;

                virtual
                void set_rank(llvm::Value* arr, llvm::Value* rank) = 0;

                /*
                * Returns pointer to dimension descriptor array
                * in the input array descriptor according to the rules
                * implemented by current class.
                */
                virtual
                llvm::Value* get_pointer_to_dimension_descriptor_array(llvm::Value* arr, bool load=true) = 0;

                /*
                * Returns pointer to the dimension descriptor
                * in the input dimension descriptor array according
                * to the rules implemented by current class.
                */
                virtual
                llvm::Value* get_pointer_to_dimension_descriptor(llvm::Value* dim_des_arr,
                    llvm::Value* dim) = 0;

                /*
                * Returns the indexed element
                * in the input dimension descriptor array according
                * to the rules implemented by current class.
                */
                virtual
                llvm::Value* get_single_element(llvm::Value* array,
                    std::vector<llvm::Value*>& m_args, int n_args,
                    bool data_only=false, llvm::Value** llvm_diminfo=nullptr) = 0;

                virtual
                llvm::Value* get_is_allocated_flag(llvm::Value* array) = 0;

                virtual
                void set_is_allocated_flag(llvm::Value* array, uint64_t status) = 0;

                virtual
                llvm::Value* reshape(llvm::Value* array, llvm::Value* shape,
                                    llvm::Module* module) = 0;

                virtual
                void copy_array(llvm::Value* src, llvm::Value* dest) = 0;

                virtual
                llvm::Value* get_array_size(llvm::Value* array, llvm::Value* dim,
                                            int output_kind, int dim_kind=4) = 0;

        };

        class SimpleCMODescriptor: public Descriptor {

            private:

                llvm::LLVMContext& context;
                LLVMUtils* llvm_utils;
                llvm::IRBuilder<>* builder;

                llvm::StructType* dim_des;

                std::map<std::string, llvm::StructType*> tkr2array;

                llvm::Value* cmo_convertor_single_element(
                    llvm::Value* arr, std::vector<llvm::Value*>& m_args,
                    int n_args, bool check_for_bounds);

                llvm::Value* cmo_convertor_single_element_data_only(
                    llvm::Value** llvm_diminfo, std::vector<llvm::Value*>& m_args,
                    int n_args, bool check_for_bounds);

            public:

                SimpleCMODescriptor(llvm::LLVMContext& _context,
                    llvm::IRBuilder<>* _builder,
                    LLVMUtils* _llvm_utils);

                virtual
                bool is_array(llvm::Value* tmp);

                virtual
                llvm::Value* convert_to_argument(llvm::Value* tmp,
                    llvm::Type* arg_type, bool data_only=false);

                virtual
                llvm::Type* get_argument_type(llvm::Type* type,
                    std::uint32_t m_h, std::string arg_name,
                    std::unordered_map
                    <std::uint32_t, std::unordered_map
                                    <std::string, llvm::Type*>>&
                        arr_arg_type_cache);

                virtual
                llvm::Type* get_array_type(
                    ASR::ttype_t* m_type_, int a_kind,
                    int rank, llvm::Type* el_type,
                    bool get_pointer=false);

                virtual
                llvm::Type* get_malloc_array_type(
                    ASR::ttype_t* m_type_, int a_kind,
                    int rank, llvm::Type* el_type,
                    bool get_pointer=false);

                virtual
                llvm::Type* create_dimension_descriptor_array_type();

                virtual
                void fill_array_details(
                    llvm::Value* arr, int n_dims,
                    std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims);

                virtual
                void fill_malloc_array_details(
                    llvm::Value* arr, int n_dims,
                    std::vector<std::pair<llvm::Value*, llvm::Value*>>& llvm_dims,
                    llvm::Module* module);

                virtual
                void fill_dimension_descriptor(
                    llvm::Value* arr, int n_dims);

                virtual
                llvm::Type* get_dimension_descriptor_type(bool get_pointer=false);

                virtual
                llvm::Value* get_pointer_to_data(llvm::Value* arr);

                virtual
                llvm::Value* get_rank(llvm::Value* arr, bool get_pointer=false);

                virtual
                void set_rank(llvm::Value* arr, llvm::Value* rank);

                virtual
                llvm::Value* get_offset(llvm::Value* dim_des);

                virtual
                llvm::Value* get_lower_bound(llvm::Value* dim_des, bool load=true);

                virtual
                llvm::Value* get_upper_bound(llvm::Value* dim_des);

                virtual
                llvm::Value* get_dimension_size(llvm::Value* dim_des_arr,
                    llvm::Value* dim, bool load=true);

                virtual
                llvm::Value* get_pointer_to_dimension_descriptor_array(llvm::Value* arr, bool load=true);

                virtual
                llvm::Value* get_pointer_to_dimension_descriptor(llvm::Value* dim_des_arr,
                    llvm::Value* dim);

                virtual
                llvm::Value* get_stride(llvm::Value* dim_des);

                virtual
                llvm::Value* get_single_element(llvm::Value* array,
                    std::vector<llvm::Value*>& m_args, int n_args,
                    bool data_only=false, llvm::Value** llvm_diminfo=nullptr);

                virtual
                llvm::Value* get_is_allocated_flag(llvm::Value* array);

                virtual
                void set_is_allocated_flag(llvm::Value* array, uint64_t status);

                virtual
                llvm::Value* reshape(llvm::Value* array, llvm::Value* shape,
                                    llvm::Module* module);

                virtual
                void copy_array(llvm::Value* src, llvm::Value* dest);

                virtual
                llvm::Value* get_array_size(llvm::Value* array, llvm::Value* dim,
                                            int output_kind, int dim_kind=4);

        };

    } // LLVMArrUtils

} // LFortran

#endif // LFORTRAN_LLVM_ARR_UTILS_H
