#include <iostream>
#include <memory>

#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/exception.h>


namespace LFortran {

static inline ASR::TranslationUnit_t* TRANSLATION_UNIT(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::unit);
    ASR::unit_t *t = (ASR::unit_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::unitType::TranslationUnit);
    return (ASR::TranslationUnit_t*)t;
}

static inline ASR::ttype_t* TYPE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    return (ASR::ttype_t*)f;
}

static inline ASR::var_t* VAR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::var);
    return (ASR::var_t*)f;
}

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    return (ASR::expr_t*)f;
}

static inline ASR::stmt_t* STMT(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::stmt);
    return (ASR::stmt_t*)f;
}

class IRBuilder : public llvm::IRBuilder<>
{
};

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
public:
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;

    IRBuilder *builder;
    llvm::Value *tmp;

    ASRToLLVMVisitor(llvm::LLVMContext &context) : context{context} {}

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
        for (auto &item : x.m_global_scope->scope) {
            visit_asr(*item.second);
        }
    }

    void visit_Function(const ASR::Function_t &x) {
        module = std::make_unique<llvm::Module>("LFortran", context);
        module->setDataLayout("");
        std::vector<llvm::Type *> args;
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(context), args, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, x.m_name, module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                "EntryBlock", F);
        llvm::IRBuilder<> _builder = llvm::IRBuilder<>(BB);
        builder = reinterpret_cast<IRBuilder *>(&_builder);
        builder->SetInsertPoint(BB);

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        llvm::verifyFunction(*F);
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        //this->visit_expr(*x.m_target);
        //LFORTRAN_ASSERT(x.m_target.m_id == "f");
        this->visit_expr(*x.m_value);
        llvm::Value *ret_val = tmp;
        builder->CreateRet(ret_val);
    }
    void visit_Num(const ASR::Num_t &x) {
        tmp = llvm::ConstantInt::get(context, llvm::APInt(64, x.m_n));
    }

};

std::unique_ptr<LLVMModule> asr_to_llvm(ASR::asr_t &asr,
        llvm::LLVMContext &context, Allocator &al)
{
    ASRToLLVMVisitor v(context);
    LFORTRAN_ASSERT(asr.type == ASR::asrType::unit);
    ASR::asr_t *asr2 = &asr;
    ASR::TranslationUnit_t *unit=TRANSLATION_UNIT(asr2);
    if (unit->n_items > 0) {
        LFORTRAN_ASSERT(unit->n_items == 1);

        // Must wrap the items into a function
        SymbolTable *new_scope = al.make_new<SymbolTable>();
        // Copy the old scope
        new_scope->scope = unit->m_global_scope->scope;

        // Add an anonymous function
        const char* fn_name_orig = "f";
        char *fn_name = (char*)fn_name_orig;
        SymbolTable *fn_scope = al.make_new<SymbolTable>();

        ASR::ttype_t *type;
        Location loc;
        type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        ASR::asr_t *return_var = ASR::make_Variable_t(al, loc,
            fn_name, intent_return_var, type);
        fn_scope->scope[std::string(fn_name)] = return_var;

        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, loc,
            fn_scope, VAR(return_var));

        ASR::expr_t *target = EXPR(return_var_ref);
        ASR::expr_t *value = EXPR(unit->m_items[0]);
        Vec<ASR::stmt_t*> body;
        ASR::stmt_t* asr_stmt= STMT(ASR::make_Assignment_t(al, loc, target, value));
        body.reserve(al, 1);
        body.push_back(al, asr_stmt);


        ASR::asr_t *fn = ASR::make_Function_t(
            al, loc,
            /* a_name */ fn_name,
            /* a_args */ nullptr,
            /* n_args */ 0,
            /* a_body */ body.p,
            /* n_body */ body.size(),
            /* a_bind */ nullptr,
            /* a_return_var */ EXPR(return_var_ref),
            /* a_module */ nullptr,
            /* a_symtab */ fn_scope);
        std::string sym_name = fn_name;
        if (new_scope->scope.find(sym_name) != new_scope->scope.end()) {
            throw SemanticError("Function already defined", fn->loc);
        }
        new_scope->scope[sym_name] = fn;

        asr2 = ASR::make_TranslationUnit_t(al, asr.loc,
            new_scope, nullptr, 0);
    }
    v.visit_asr(*asr2);
    return std::make_unique<LLVMModule>(std::move(v.module));
}

} // namespace LFortran
