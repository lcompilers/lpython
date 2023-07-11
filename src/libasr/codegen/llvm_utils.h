#ifndef LFORTRAN_LLVM_UTILS_H
#define LFORTRAN_LLVM_UTILS_H

#include <memory>

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <libasr/asr.h>

#include <map>
#include <unordered_map>
#include <tuple>

#if LLVM_VERSION_MAJOR >= 11
#    define FIXED_VECTOR_TYPE llvm::FixedVectorType
#else
#    define FIXED_VECTOR_TYPE llvm::VectorType
#endif

namespace LCompilers {

    // Platform dependent fast unique hash:
    static inline uint64_t get_hash(ASR::asr_t *node)
    {
        return (uint64_t)node;
    }

    namespace {

    // This exception is used to abort the visitor pattern when an error occurs.
    // This is only used locally in this file, not propagated outside. An error
    // must be already present in ASRToLLVMVisitor::diag before throwing this
    // exception. This is checked with an assert when the CodeGenAbort is
    // caught.
    class CodeGenAbort
    {
    };

    // Local exception that is only used in this file to exit the visitor
    // pattern and caught later (not propagated outside). It accepts an error
    // message that is then appended at the end of ASRToLLVMVisitor::diag.  The
    // `diag` can already contain other errors or warnings.  This is a
    // convenience class. One can also report the error into `diag` directly and
    // call `CodeGenAbort` instead.
    class CodeGenError
    {
        public:
            diag::Diagnostic d;
        public:
            CodeGenError(const std::string &msg)
                : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen)}
            { }

            CodeGenError(const std::string &msg, const Location &loc)
                : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen, {
                    diag::Label("", {loc})
                })}
            { }
        };

    }

    namespace LLVMArrUtils {
        class Descriptor;
    }

    static inline void printf(llvm::LLVMContext &context, llvm::Module &module,
        llvm::IRBuilder<> &builder, const std::vector<llvm::Value*> &args)
    {
        llvm::Function *fn_printf = module.getFunction("_lfortran_printf");
        if (!fn_printf) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {llvm::Type::getInt8PtrTy(context)}, true);
            fn_printf = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, "_lfortran_printf", &module);
        }
        builder.CreateCall(fn_printf, args);
    }

    static inline llvm::Value* string_format_fortran(llvm::LLVMContext &context, llvm::Module &module,
        llvm::IRBuilder<> &builder, const std::vector<llvm::Value*> &args)
    {
        llvm::Function *fn_printf = module.getFunction("_lcompilers_string_format_fortran");
        if (!fn_printf) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getInt8PtrTy(context),
                    {llvm::Type::getInt8PtrTy(context)}, true);
            fn_printf = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, "_lcompilers_string_format_fortran", &module);
        }
        return builder.CreateCall(fn_printf, args);
    }

    static inline void print_error(llvm::LLVMContext &context, llvm::Module &module,
        llvm::IRBuilder<> &builder, const std::vector<llvm::Value*> &args)
    {
        llvm::Function *fn_printf = module.getFunction("_lcompilers_print_error");
        if (!fn_printf) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {llvm::Type::getInt8PtrTy(context)}, true);
            fn_printf = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, "_lcompilers_print_error", &module);
        }
        builder.CreateCall(fn_printf, args);
    }

    static inline void exit(llvm::LLVMContext &context, llvm::Module &module,
        llvm::IRBuilder<> &builder, llvm::Value* exit_code)
    {
        llvm::Function *fn_exit = module.getFunction("exit");
        if (!fn_exit) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {llvm::Type::getInt32Ty(context)},
                    false);
            fn_exit = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, "exit", &module);
        }
        builder.CreateCall(fn_exit, {exit_code});
    }

    // Insert the following anywhere inside the LLVM backend to print
    // addresses at runtime:
    // call_print_stacktrace_addresses(context, *module, *builder, {filename, use_colors});
    static inline void call_print_stacktrace_addresses(llvm::LLVMContext &context,
            llvm::Module &module, llvm::IRBuilder<> &builder,
            const std::vector<llvm::Value*> &args)
    {
        llvm::Function *fn = module.getFunction("print_stacktrace_addresses");
        if (!fn) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), {
                    llvm::Type::getInt8PtrTy(context),
                    llvm::Type::getInt1Ty(context)
                }, true);
            fn = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, "print_stacktrace_addresses",
                &module);
        }
        builder.CreateCall(fn, args);
    }

    namespace LLVM {

        llvm::Value* CreateLoad(llvm::IRBuilder<> &builder, llvm::Value *x);
        llvm::Value* CreateStore(llvm::IRBuilder<> &builder, llvm::Value *x, llvm::Value *y);
        llvm::Value* CreateGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx);
        llvm::Value* CreateInBoundsGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx);
        llvm::Value* lfortran_malloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* arg_size);
        llvm::Value* lfortran_realloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* ptr, llvm::Value* arg_size);
        llvm::Value* lfortran_calloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* count, llvm::Value* type_size);
        llvm::Value* lfortran_free(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* ptr);
        static inline bool is_llvm_struct(ASR::ttype_t* asr_type) {
            return ASR::is_a<ASR::Tuple_t>(*asr_type) ||
                   ASR::is_a<ASR::List_t>(*asr_type) ||
                   ASR::is_a<ASR::Struct_t>(*asr_type) ||
                   ASR::is_a<ASR::Class_t>(*asr_type)||
                   ASR::is_a<ASR::Dict_t>(*asr_type);
        }
        static inline bool is_llvm_pointer(const ASR::ttype_t& asr_type) {
            return ( ASR::is_a<ASR::Pointer_t>(asr_type) ||
                   ASR::is_a<ASR::Allocatable_t>(asr_type) );
        }
    }

    class LLVMList;
    class LLVMTuple;
    class LLVMDictInterface;

    class LLVMUtils {

        private:

            llvm::LLVMContext& context;
            llvm::IRBuilder<>* builder;
            llvm::AllocaInst *str_cmp_itr;

            bool are_iterators_set;

        public:

            LLVMTuple* tuple_api;
            LLVMList* list_api;
            LLVMDictInterface* dict_api;
            LLVMArrUtils::Descriptor* arr_api;
            llvm::Module* module;
            std::string& der_type_name;
            std::map<std::string, llvm::StructType*>& name2dertype;
            std::map<std::string, llvm::StructType*>& name2dercontext;
            std::vector<std::string>& struct_type_stack;
            std::map<std::string, std::string>& dertype2parent;
            std::map<std::string, std::map<std::string, int>>& name2memidx;
            std::unordered_map<std::uint32_t, std::unordered_map<std::string, llvm::Type*>>& arr_arg_type_cache;
            std::map<std::string, std::pair<llvm::Type*, llvm::Type*>>& fname2arg_type;

            LLVMDictInterface* dict_api_lp;
            LLVMDictInterface* dict_api_sc;

            CompilerOptions &compiler_options;

            llvm::StructType *complex_type_4, *complex_type_8;
            llvm::StructType *complex_type_4_ptr, *complex_type_8_ptr;
            llvm::PointerType *character_type;

            LLVMUtils(llvm::LLVMContext& context,
                llvm::IRBuilder<>* _builder, std::string& der_type_name_,
                std::map<std::string, llvm::StructType*>& name2dertype_,
                std::map<std::string, llvm::StructType*>& name2dercontext_,
                std::vector<std::string>& struct_type_stack_,
                std::map<std::string, std::string>& dertype2parent_,
                std::map<std::string, std::map<std::string, int>>& name2memidx_,
                CompilerOptions &compiler_options_,
                std::unordered_map<std::uint32_t, std::unordered_map<std::string, llvm::Type*>>& arr_arg_type_cache_,
                std::map<std::string, std::pair<llvm::Type*, llvm::Type*>>& fname2arg_type_);

            llvm::Value* create_gep(llvm::Value* ds, int idx);

            llvm::Value* create_gep(llvm::Value* ds, llvm::Value* idx);

            llvm::Value* create_ptr_gep(llvm::Value* ptr, int idx);

            llvm::Value* create_ptr_gep(llvm::Value* ptr, llvm::Value* idx);

            llvm::Type* getIntType(int a_kind, bool get_pointer=false);

            void start_new_block(llvm::BasicBlock *bb);

            llvm::Value* lfortran_str_cmp(llvm::Value* left_arg, llvm::Value* right_arg,
                                          std::string runtime_func_name, llvm::Module& module);

            llvm::Value* is_equal_by_value(llvm::Value* left, llvm::Value* right,
                                           llvm::Module& module, ASR::ttype_t* asr_type);

            llvm::Value* is_ineq_by_value(llvm::Value* left, llvm::Value* right,
                                          llvm::Module& module, ASR::ttype_t* asr_type,
                                          int8_t overload_id, ASR::ttype_t* int32_type=nullptr);

            void set_iterators();

            void reset_iterators();

            void set_module(llvm::Module* module_);

            llvm::Type* getMemberType(ASR::ttype_t* mem_type,
                ASR::Variable_t* member, llvm::Module* module);

            void createStructContext(ASR::StructType_t* der_type);

            llvm::Type* getStructType(ASR::StructType_t* der_type, llvm::Module* module, bool is_pointer=false);

            llvm::Type* getStructType(ASR::ttype_t* _type, llvm::Module* module, bool is_pointer=false);

            llvm::Type* getUnionType(ASR::UnionType_t* union_type,
                llvm::Module* module, bool is_pointer=false);

            llvm::Type* getUnionType(ASR::ttype_t* _type,
                llvm::Module* module, bool is_pointer=false);

            llvm::Type* getClassType(ASR::ClassType_t* der_type, bool is_pointer=false);

            llvm::Type* getClassType(ASR::StructType_t* der_type, bool is_pointer=false);

            llvm::Type* getClassType(ASR::ttype_t* _type, bool is_pointer=false);

            llvm::Type* getFPType(int a_kind, bool get_pointer=false);

            llvm::Type* getComplexType(int a_kind, bool get_pointer=false);

            llvm::Type* get_el_type(ASR::ttype_t* m_type_, llvm::Module* module);

            llvm::Type* get_dict_type(ASR::ttype_t* asr_type, llvm::Module* module);

            llvm::FunctionType* get_function_type(const ASR::Function_t &x, llvm::Module* module);

            std::vector<llvm::Type*> convert_args(const ASR::Function_t &x, llvm::Module* module);

            llvm::Type* get_type_from_ttype_t(ASR::ttype_t* asr_type,
                ASR::symbol_t *type_declaration, ASR::storage_typeType m_storage,
                bool& is_array_type, bool& is_malloc_array_type, bool& is_list,
                ASR::dimension_t*& m_dims, int& n_dims, int& a_kind, llvm::Module* module,
                ASR::abiType m_abi=ASR::abiType::Source, bool is_pointer=false);

            llvm::Type* get_type_from_ttype_t_util(ASR::ttype_t* asr_type,
                llvm::Module* module, ASR::abiType asr_abi=ASR::abiType::Source);

            llvm::Type* get_arg_type_from_ttype_t(ASR::ttype_t* asr_type,
                ASR::symbol_t *type_declaration, ASR::abiType m_abi, ASR::abiType arg_m_abi,
                ASR::storage_typeType m_storage, bool arg_m_value_attr, int& n_dims,
                int& a_kind, bool& is_array_type, ASR::intentType arg_intent, llvm::Module* module,
                bool get_pointer=true);

            void set_dict_api(ASR::Dict_t* dict_type);

            void deepcopy(llvm::Value* src, llvm::Value* dest,
                ASR::ttype_t* asr_type, llvm::Module* module,
                std::map<std::string, std::map<std::string, int>>& name2memidx);


            // Note: `llvm_utils->create_if_else` and `create_loop` are optional APIs
            // that do not have to be used. Many times, for more complicated
            // things, it might be more readable to just use the LLVM API
            // without any extra layer on top. In some other cases, it might
            // be more readable to use this abstraction.
            // The `if_block` and `else_block` must generate one or more blocks. In
            // addition, the `if_block` must not be terminated, we terminate it
            // ourselves. The `else_block` can be either terminated or not.
            template <typename IF, typename ELSE>
            void create_if_else(llvm::Value * cond, IF if_block, ELSE else_block) {
                llvm::Function *fn = builder->GetInsertBlock()->getParent();

                llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
                llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
                llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");

                builder->CreateCondBr(cond, thenBB, elseBB);
                builder->SetInsertPoint(thenBB); {
                    if_block();
                }
                builder->CreateBr(mergeBB);

                start_new_block(elseBB); {
                    else_block();
                }
                start_new_block(mergeBB);
            }

    }; // LLVMUtils

    class LLVMList {
        private:

            llvm::LLVMContext& context;
            LLVMUtils* llvm_utils;
            llvm::IRBuilder<>* builder;

            std::map<std::string, std::tuple<llvm::Type*, int32_t, llvm::Type*>> typecode2listtype;

            void resize_if_needed(llvm::Value* list, llvm::Value* n,
                                  llvm::Value* capacity, int32_t type_size,
                                  llvm::Type* el_type, llvm::Module* module);

            void shift_end_point_by_one(llvm::Value* list);

        public:

            LLVMList(llvm::LLVMContext& context_, LLVMUtils* llvm_utils,
                     llvm::IRBuilder<>* builder);

            llvm::Type* get_list_type(llvm::Type* el_type, std::string& type_code,
                                        int32_t type_size);

            void list_init(std::string& type_code, llvm::Value* list,
                           llvm::Module& module, llvm::Value* initial_capacity,
                           llvm::Value* n);

            void list_init(std::string& type_code, llvm::Value* list,
                            llvm::Module& module, int32_t initial_capacity=1,
                            int32_t n=0);

            llvm::Value* get_pointer_to_list_data(llvm::Value* list);

            llvm::Value* get_pointer_to_current_end_point(llvm::Value* list);

            llvm::Value* get_pointer_to_current_capacity(llvm::Value* list);

            void list_deepcopy(llvm::Value* src, llvm::Value* dest,
                ASR::List_t* list_type, llvm::Module* module,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            void list_deepcopy(llvm::Value* src, llvm::Value* dest,
                ASR::ttype_t* element_type, llvm::Module* module,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* read_item(llvm::Value* list, llvm::Value* pos,
                                   bool enable_bounds_checking,
                                   llvm::Module& module, bool get_pointer=false);

            llvm::Value* len(llvm::Value* list);

            void check_index_within_bounds(llvm::Value* list, llvm::Value* pos,
                                           llvm::Module& module);

            void write_item(llvm::Value* list, llvm::Value* pos,
                llvm::Value* item, ASR::ttype_t* asr_type,
                bool enable_bounds_checking, llvm::Module* module,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            void write_item(llvm::Value* list, llvm::Value* pos,
                            llvm::Value* item, bool enable_bounds_checking,
                            llvm::Module& module);

            void append(llvm::Value* list, llvm::Value* item,
                        ASR::ttype_t* asr_type, llvm::Module* module,
                        std::map<std::string, std::map<std::string, int>>& name2memidx);

            void insert_item(llvm::Value* list, llvm::Value* pos,
                            llvm::Value* item, ASR::ttype_t* asr_type,
                            llvm::Module* module,
                            std::map<std::string, std::map<std::string, int>>& name2memidx);

            void remove(llvm::Value* list, llvm::Value* item,
                        ASR::ttype_t* item_type, llvm::Module& module);

            llvm::Value* pop_position(llvm::Value* list, llvm::Value* pos,
                                      ASR::ttype_t* list_type, llvm::Module* module,
                                      std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* pop_last(llvm::Value* list, ASR::ttype_t* list_type, llvm::Module& module);

            void list_clear(llvm::Value* list);

            void reverse(llvm::Value* list, llvm::Module& module);

            llvm::Value* find_item_position(llvm::Value* list,
                llvm::Value* item, ASR::ttype_t* item_type,
                llvm::Module& module, llvm::Value* start=nullptr,
                llvm::Value* end=nullptr);

            llvm::Value* index(llvm::Value* list, llvm::Value* item,
                                llvm::Value* start, llvm::Value* end,
                                ASR::ttype_t* item_type, llvm::Module& module);

            llvm::Value* count(llvm::Value* list, llvm::Value* item,
                                ASR::ttype_t* item_type, llvm::Module& module);

            void free_data(llvm::Value* list, llvm::Module& module);

            llvm::Value* check_list_equality(llvm::Value* l1, llvm::Value* l2, ASR::ttype_t *item_type,
                llvm::LLVMContext& context, llvm::IRBuilder<>* builder, llvm::Module& module);

            llvm::Value* check_list_inequality(llvm::Value* l1, llvm::Value* l2,
                ASR::ttype_t *item_type, llvm::LLVMContext& context,
                llvm::IRBuilder<>* builder, llvm::Module& module,
                int8_t overload_id, ASR::ttype_t* int32_type=nullptr);

            void list_repeat_copy(llvm::Value* repeat_list, llvm::Value* init_list,
                                  llvm::Value* num_times, llvm::Value* init_list_len,
                                  llvm::Module* module);
    };

    class LLVMTuple {
        private:

            llvm::LLVMContext& context;
            LLVMUtils* llvm_utils;
            llvm::IRBuilder<>* builder;

            std::map<std::string, std::pair<llvm::Type*, size_t>> typecode2tupletype;

        public:

            LLVMTuple(llvm::LLVMContext& context_,
                      LLVMUtils* llvm_utils,
                      llvm::IRBuilder<>* builder);

            llvm::Type* get_tuple_type(std::string& type_code,
                                       std::vector<llvm::Type*>& el_types);

            void tuple_init(llvm::Value* llvm_tuple, std::vector<llvm::Value*>& values,
                            ASR::Tuple_t* tuple_type, llvm::Module* module,
                            std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* read_item(llvm::Value* llvm_tuple, llvm::Value* pos,
                                   bool get_pointer=false);

            llvm::Value* read_item(llvm::Value* llvm_tuple, size_t pos,
                                   bool get_pointer=false);

            void tuple_deepcopy(llvm::Value* src, llvm::Value* dest,
                                ASR::Tuple_t* type_code, llvm::Module* module,
                                std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* check_tuple_equality(llvm::Value* t1, llvm::Value* t2,
                ASR::Tuple_t* tuple_type, llvm::LLVMContext& context,
                llvm::IRBuilder<>* builder, llvm::Module& module);

            llvm::Value* check_tuple_inequality(llvm::Value* t1, llvm::Value* t2,
                ASR::Tuple_t* tuple_type, llvm::LLVMContext& context,
                llvm::IRBuilder<>* builder, llvm::Module& module, int8_t overload_id);

            void concat(llvm::Value* t1, llvm::Value* t2, ASR::Tuple_t* tuple_type_1,
                        ASR::Tuple_t* tuple_type_2, llvm::Value* concat_tuple,
                        ASR::Tuple_t* concat_tuple_type, llvm::Module& module,
                        std::map<std::string, std::map<std::string, int>>& name2memidx);
    };

    class LLVMDictInterface {

        protected:

            llvm::LLVMContext& context;
            LLVMUtils* llvm_utils;
            llvm::IRBuilder<>* builder;
            llvm::AllocaInst *pos_ptr, *is_key_matching_var;
            llvm::AllocaInst *idx_ptr, *hash_iter, *hash_value;
            llvm::AllocaInst *polynomial_powers;
            llvm::AllocaInst *chain_itr, *chain_itr_prev;
            llvm::AllocaInst *old_capacity, *old_key_value_pairs, *old_key_mask;
            llvm::AllocaInst *old_occupancy, *old_number_of_buckets_filled;
            llvm::AllocaInst *src_itr, *dest_itr, *next_ptr, *copy_itr;
            llvm::Value *tmp_value_ptr;
            bool are_iterators_set;

            std::map<std::pair<std::string, std::string>,
                     std::tuple<llvm::Type*, std::pair<int32_t, int32_t>,
                                std::pair<llvm::Type*, llvm::Type*>>> typecode2dicttype;

        public:

            bool is_dict_present_;

            LLVMDictInterface(
                llvm::LLVMContext& context_,
                LLVMUtils* llvm_utils,
                llvm::IRBuilder<>* builder);

            virtual
            llvm::Type* get_dict_type(std::string key_type_code, std::string value_type_code,
                int32_t key_type_size, int32_t value_type_size,
                llvm::Type* key_type, llvm::Type* value_type) = 0;

            virtual
            void dict_init(std::string key_type_code, std::string value_type_code,
                llvm::Value* dict, llvm::Module* module, size_t initial_capacity) = 0;

            virtual
            llvm::Value* get_key_list(llvm::Value* dict) = 0;

            virtual
            llvm::Value* get_value_list(llvm::Value* dict) = 0;

            virtual
            llvm::Value* get_pointer_to_occupancy(llvm::Value* dict) = 0;

            virtual
            llvm::Value* get_pointer_to_capacity(llvm::Value* dict) = 0;

            virtual
            llvm::Value* get_key_hash(llvm::Value* capacity, llvm::Value* key,
                ASR::ttype_t* key_asr_type, llvm::Module& module);

            virtual
            void resolve_collision_for_write(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Value* value,
                llvm::Module* module, ASR::ttype_t* key_asr_type,
                ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx) = 0;

            virtual
            llvm::Value* resolve_collision_for_read(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Module& module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type) = 0;

            virtual
            llvm::Value* resolve_collision_for_read_with_bound_check(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Module& module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type) = 0;

            virtual
            llvm::Value* resolve_collision_for_read_with_default(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Module& module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type, llvm::Value* def_value) = 0;

            virtual
            void rehash(llvm::Value* dict, llvm::Module* module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx) = 0;

            virtual
            void rehash_all_at_once_if_needed(llvm::Value* dict,
                llvm::Module* module,
                ASR::ttype_t* key_asr_type,
                ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx) = 0;

            virtual
            void write_item(llvm::Value* dict, llvm::Value* key,
                llvm::Value* value, llvm::Module* module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx) = 0;

            virtual
            llvm::Value* read_item(llvm::Value* dict, llvm::Value* key,
                llvm::Module& module, ASR::Dict_t* dict_type, bool enable_bounds_checking,
                bool get_pointer=false) = 0;

            virtual
            llvm::Value* get_item(llvm::Value* dict, llvm::Value* key,
                llvm::Module& module, ASR::Dict_t* dict_type, llvm::Value* def_value,
                bool get_pointer=false) = 0;

            virtual
            llvm::Value* pop_item(llvm::Value* dict, llvm::Value* key,
                llvm::Module& module, ASR::Dict_t* dict_type,
                bool get_pointer=false) = 0;

            virtual
            void set_iterators();

            virtual
            void reset_iterators();

            virtual
            void dict_deepcopy(llvm::Value* src, llvm::Value* dest,
                ASR::Dict_t* dict_type, llvm::Module* module,
                std::map<std::string, std::map<std::string, int>>& name2memidx) = 0;

            virtual
            llvm::Value* len(llvm::Value* dict) = 0;

            virtual
            bool is_dict_present();

            virtual
            void set_is_dict_present(bool value);

            virtual ~LLVMDictInterface() = 0;

    };

    class LLVMDict: public LLVMDictInterface {

        public:

            LLVMDict(llvm::LLVMContext& context_,
                     LLVMUtils* llvm_utils,
                     llvm::IRBuilder<>* builder);

            llvm::Type* get_dict_type(std::string key_type_code, std::string value_type_code,
                int32_t key_type_size, int32_t value_type_size,
                llvm::Type* key_type, llvm::Type* value_type);

            void dict_init(std::string key_type_code, std::string value_type_code,
                llvm::Value* dict, llvm::Module* module, size_t initial_capacity);

            llvm::Value* get_key_list(llvm::Value* dict);

            llvm::Value* get_value_list(llvm::Value* dict);

            llvm::Value* get_pointer_to_occupancy(llvm::Value* dict);

            llvm::Value* get_pointer_to_capacity(llvm::Value* dict);

            virtual
            void resolve_collision(llvm::Value* capacity, llvm::Value* key_hash,
                                llvm::Value* key, llvm::Value* key_list,
                                llvm::Value* key_mask, llvm::Module& module,
                                ASR::ttype_t* key_asr_type, bool for_read=false);

            void resolve_collision_for_write(llvm::Value* dict, llvm::Value* key_hash,
                                          llvm::Value* key, llvm::Value* value,
                                          llvm::Module* module, ASR::ttype_t* key_asr_type,
                                          ASR::ttype_t* value_asr_type,
                                          std::map<std::string, std::map<std::string, int>>& name2memidx);

            void _check_key_present_or_default(llvm::Module& module, llvm::Value *key, llvm::Value *key_list,
                ASR::ttype_t* key_asr_type, llvm::Value *value_list, llvm::Value *pos,
                llvm::Value *def_value, llvm::Value* &result);

            llvm::Value* resolve_collision_for_read(llvm::Value* dict, llvm::Value* key_hash,
                                                 llvm::Value* key, llvm::Module& module,
                                                 ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type);

            llvm::Value* resolve_collision_for_read_with_bound_check(llvm::Value* dict, llvm::Value* key_hash,
                                                 llvm::Value* key, llvm::Module& module,
                                                 ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type);

            llvm::Value* resolve_collision_for_read_with_default(llvm::Value* dict, llvm::Value* key_hash,
                                                 llvm::Value* key, llvm::Module& module,
                                                 ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                                                 llvm::Value* def_value);

            void rehash(llvm::Value* dict, llvm::Module* module,
                        ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                        std::map<std::string, std::map<std::string, int>>& name2memidx);

            void rehash_all_at_once_if_needed(llvm::Value* dict,
                                              llvm::Module* module,
                                              ASR::ttype_t* key_asr_type,
                                              ASR::ttype_t* value_asr_type,
                                              std::map<std::string, std::map<std::string, int>>& name2memidx);

            void write_item(llvm::Value* dict, llvm::Value* key,
                            llvm::Value* value, llvm::Module* module,
                            ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                            std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* read_item(llvm::Value* dict, llvm::Value* key,
                                   llvm::Module& module, ASR::Dict_t* key_asr_type, bool enable_bounds_checking,
                                   bool get_pointer=false);

            llvm::Value* get_item(llvm::Value* dict, llvm::Value* key,
                                   llvm::Module& module, ASR::Dict_t* key_asr_type, llvm::Value* def_value,
                                   bool get_pointer=false);

            llvm::Value* pop_item(llvm::Value* dict, llvm::Value* key,
                                   llvm::Module& module, ASR::Dict_t* dict_type,
                                   bool get_pointer=false);

            virtual
            llvm::Value* get_pointer_to_keymask(llvm::Value* dict);

            void dict_deepcopy(llvm::Value* src, llvm::Value* dest,
                ASR::Dict_t* dict_type, llvm::Module* module,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* len(llvm::Value* dict);

            virtual ~LLVMDict();
    };

    class LLVMDictOptimizedLinearProbing: public LLVMDict {

        public:

            LLVMDictOptimizedLinearProbing(llvm::LLVMContext& context_,
                                    LLVMUtils* llvm_utils,
                                    llvm::IRBuilder<>* builder);

            void resolve_collision(llvm::Value* capacity, llvm::Value* key_hash,
                                llvm::Value* key, llvm::Value* key_list,
                                llvm::Value* key_mask, llvm::Module& module,
                                ASR::ttype_t* key_asr_type, bool for_read=false);

            void resolve_collision_for_write(llvm::Value* dict, llvm::Value* key_hash,
                                            llvm::Value* key, llvm::Value* value,
                                            llvm::Module* module, ASR::ttype_t* key_asr_type,
                                            ASR::ttype_t* value_asr_type,
                                            std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* resolve_collision_for_read(llvm::Value* dict, llvm::Value* key_hash,
                                                    llvm::Value* key, llvm::Module& module,
                                                    ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type);

            llvm::Value* resolve_collision_for_read_with_bound_check(llvm::Value* dict, llvm::Value* key_hash,
                                                    llvm::Value* key, llvm::Module& module,
                                                    ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type);

            llvm::Value* resolve_collision_for_read_with_default(llvm::Value* dict, llvm::Value* key_hash,
                                                    llvm::Value* key, llvm::Module& module,
                                                    ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                                                    llvm::Value *def_value);

            virtual ~LLVMDictOptimizedLinearProbing();

    };

    class LLVMDictSeparateChaining: public LLVMDictInterface {

        protected:

            std::map<std::pair<std::string, std::string>, llvm::Type*> typecode2kvstruct;

            llvm::Value* get_pointer_to_number_of_filled_buckets(llvm::Value* dict);

            llvm::Value* get_pointer_to_key_value_pairs(llvm::Value* dict);

            llvm::Value* get_pointer_to_rehash_flag(llvm::Value* dict);

            void deepcopy_key_value_pair_linked_list(llvm::Value* srci, llvm::Value* desti,
                llvm::Value* dest_key_value_pairs, ASR::Dict_t* dict_type,
                llvm::Module* module, std::map<std::string, std::map<std::string, int>>& name2memidx);

            void write_key_value_pair_linked_list(llvm::Value* kv_ll, llvm::Value* dict,
                llvm::Value* capacity, ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                llvm::Module* module, std::map<std::string, std::map<std::string, int>>& name2memidx);

            void resolve_collision(llvm::Value* capacity, llvm::Value* key_hash,
                llvm::Value* key, llvm::Value* key_value_pair_linked_list,
                llvm::Type* kv_pair_type, llvm::Value* key_mask,
                llvm::Module& module, ASR::ttype_t* key_asr_type);

            llvm::Type* get_key_value_pair_type(std::string key_type_code, std::string value_type_code);

            llvm::Type* get_key_value_pair_type(ASR::ttype_t* key_asr_type, ASR::ttype_t* value_pair_type);

            void dict_init_given_initial_capacity(std::string key_type_code, std::string value_type_code,
                llvm::Value* dict, llvm::Module* module, llvm::Value* initial_capacity);

        public:

            LLVMDictSeparateChaining(
                llvm::LLVMContext& context_,
                LLVMUtils* llvm_utils_,
                llvm::IRBuilder<>* builder_);

            llvm::Type* get_dict_type(std::string key_type_code, std::string value_type_code,
                int32_t key_type_size, int32_t value_type_size,
                llvm::Type* key_type, llvm::Type* value_type);

            void dict_init(std::string key_type_code, std::string value_type_code,
                llvm::Value* dict, llvm::Module* module, size_t initial_capacity);

            llvm::Value* get_key_list(llvm::Value* dict);

            llvm::Value* get_value_list(llvm::Value* dict);

            llvm::Value* get_pointer_to_occupancy(llvm::Value* dict);

            llvm::Value* get_pointer_to_capacity(llvm::Value* dict);

            void resolve_collision_for_write(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Value* value,
                llvm::Module* module, ASR::ttype_t* key_asr_type,
                ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* resolve_collision_for_read(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Module& module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type);

            llvm::Value* resolve_collision_for_read_with_bound_check(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Module& module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type);

            llvm::Value* resolve_collision_for_read_with_default(llvm::Value* dict, llvm::Value* key_hash,
                llvm::Value* key, llvm::Module& module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                llvm::Value* def_value);

            void rehash(llvm::Value* dict, llvm::Module* module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            void rehash_all_at_once_if_needed(llvm::Value* dict,
                llvm::Module* module,
                ASR::ttype_t* key_asr_type,
                ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            void write_item(llvm::Value* dict, llvm::Value* key,
                llvm::Value* value, llvm::Module* module,
                ASR::ttype_t* key_asr_type, ASR::ttype_t* value_asr_type,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* read_item(llvm::Value* dict, llvm::Value* key,
                llvm::Module& module, ASR::Dict_t* dict_type, bool enable_bounds_checking,
                bool get_pointer=false);

            llvm::Value* get_item(llvm::Value* dict, llvm::Value* key,
                llvm::Module& module, ASR::Dict_t* dict_type, llvm::Value* def_value,
                bool get_pointer=false);

            llvm::Value* pop_item(llvm::Value* dict, llvm::Value* key,
                llvm::Module& module, ASR::Dict_t* dict_type,
                bool get_pointer=false);

            llvm::Value* get_pointer_to_keymask(llvm::Value* dict);

            void dict_deepcopy(llvm::Value* src, llvm::Value* dest,
                ASR::Dict_t* dict_type, llvm::Module* module,
                std::map<std::string, std::map<std::string, int>>& name2memidx);

            llvm::Value* len(llvm::Value* dict);

            virtual ~LLVMDictSeparateChaining();

    };

} // namespace LCompilers

#endif // LFORTRAN_LLVM_UTILS_H
