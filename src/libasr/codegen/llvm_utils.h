#ifndef LFORTRAN_LLVM_UTILS_H
#define LFORTRAN_LLVM_UTILS_H

#include <memory>

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>

#include <map>
#include <tuple>

namespace LFortran {

    namespace LLVM {

        llvm::Value* CreateLoad(llvm::IRBuilder<> &builder, llvm::Value *x);
        llvm::Value* CreateStore(llvm::IRBuilder<> &builder, llvm::Value *x, llvm::Value *y);
        llvm::Value* CreateGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx);
        llvm::Value* CreateInBoundsGEP(llvm::IRBuilder<> &builder, llvm::Value *x, std::vector<llvm::Value *> &idx);
        llvm::Value* lfortran_malloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* arg_size);
        llvm::Value* lfortran_realloc(llvm::LLVMContext &context, llvm::Module &module,
                llvm::IRBuilder<> &builder, llvm::Value* ptr, llvm::Value* arg_size);
    }

    class LLVMUtils {

        private:

            llvm::LLVMContext& context;
            llvm::IRBuilder<>* builder;

        public:

            LLVMUtils(llvm::LLVMContext& context,
                llvm::IRBuilder<>* _builder);

            llvm::Value* create_gep(llvm::Value* ds, int idx);

            llvm::Value* create_gep(llvm::Value* ds, llvm::Value* idx);

            llvm::Value* create_ptr_gep(llvm::Value* ptr, int idx);

            llvm::Value* create_ptr_gep(llvm::Value* ptr, llvm::Value* idx);

            llvm::Type* getIntType(int a_kind, bool get_pointer=false);

            void start_new_block(llvm::BasicBlock *bb);

    }; // LLVMUtils

    class LLVMList {
        private:

            llvm::LLVMContext& context;
            LLVMUtils* llvm_utils;
            llvm::IRBuilder<>* builder;

            std::map<std::string, std::tuple<llvm::Type*, int32_t, llvm::Type*>> typecode2listtype;

            void resize_if_needed(llvm::Value* list, llvm::Value* n,
                                  llvm::Value* capacity, int32_t type_size,
                                  llvm::Type* el_type, llvm::Module& module);

            void shift_end_point_by_one(llvm::Value* list);

        public:

            LLVMList(llvm::LLVMContext& context_, LLVMUtils* llvm_utils,
                        llvm::IRBuilder<>* builder);

            llvm::Type* get_list_type(llvm::Type* el_type, std::string& type_code,
                                        int32_t type_size);

            void list_init(std::string& type_code, llvm::Value* list,
                            llvm::Module& module, int32_t initial_capacity=1,
                            int32_t n=0);

            llvm::Value* get_pointer_to_list_data(llvm::Value* list);

            llvm::Value* get_pointer_to_current_end_point(llvm::Value* list);

            llvm::Value* get_pointer_to_current_capacity(llvm::Value* list);

            void list_deepcopy(llvm::Value* src, llvm::Value* dest,
                                std::string& src_type_code,
                                llvm::Module& module);

            llvm::Value* read_item(llvm::Value* list, llvm::Value* pos,
                                   bool get_pointer=false);

            void write_item(llvm::Value* list, llvm::Value* pos,
                            llvm::Value* item);

            void append(llvm::Value* list, llvm::Value* item,
                        llvm::Module& module, std::string& type_code);
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

            void tuple_init(llvm::Value* llvm_tuple, std::vector<llvm::Value*>& values);

            llvm::Value* read_item(llvm::Value* llvm_tuple, llvm::Value* pos,
                                   bool get_pointer=false);

            llvm::Value* read_item(llvm::Value* llvm_tuple, size_t pos,
                                   bool get_pointer=false);

            void tuple_deepcopy(llvm::Value* src, llvm::Value* dest,
                                std::string& type_code);
    };

} // LFortran

#endif // LFORTRAN_LLVM_UTILS_H
