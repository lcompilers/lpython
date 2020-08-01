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

static inline ASR::Variable_t* VARIABLE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::var);
    ASR::var_t *t = (ASR::var_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::varType::Variable);
    return (ASR::Variable_t*)t;
}

static inline ASR::Var_t* EXPR_VAR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    ASR::expr_t *t = (ASR::expr_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::exprType::Var);
    return (ASR::Var_t*)t;
}

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
public:
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::Value *tmp;

    // TODO: This is not scoped, should lookup by hashes instead:
    std::map<std::string, llvm::AllocaInst*> llvm_symtab;

    ASRToLLVMVisitor(llvm::LLVMContext &context) : context{context} {}

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        module = std::make_unique<llvm::Module>("LFortran", context);
        module->setDataLayout("");
        builder = std::make_unique<llvm::IRBuilder<>>(context);


        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
        for (auto &item : x.m_global_scope->scope) {
            visit_asr(*item.second);
        }
    }
    void visit_Program(const ASR::Program_t &x) {
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(context), {}, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, "main", module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);

        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;

                // TODO: we are assuming integer here:
                llvm::AllocaInst *ptr = builder->CreateAlloca(
                    llvm::Type::getInt64Ty(context), nullptr, v->m_name);
                llvm_symtab[std::string(v->m_name)] = ptr;
            }
        }

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        llvm::Value *ret_val2 = llvm::ConstantInt::get(context,
            llvm::APInt(64, 0));
        builder->CreateRet(ret_val2);
    }

    void visit_Function(const ASR::Function_t &x) {
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(context), {}, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, x.m_name, module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);

        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;

                // TODO: we are assuming integer here:
                llvm::AllocaInst *ptr = builder->CreateAlloca(
                    llvm::Type::getInt64Ty(context), nullptr, v->m_name);
                llvm_symtab[std::string(v->m_name)] = ptr;
            }
        }

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        llvm::Value *ret_val = llvm_symtab[std::string(x.m_name)];
        llvm::Value *ret_val2 = builder->CreateLoad(ret_val);
        builder->CreateRet(ret_val2);
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        //this->visit_expr(*x.m_target);
        ASR::var_t *t1 = EXPR_VAR((ASR::asr_t*)(x.m_target))->m_v;
        llvm::Value *target= llvm_symtab[std::string(VARIABLE((ASR::asr_t*)t1)->m_name)];
        this->visit_expr(*x.m_value);
        llvm::Value *value=tmp;
        builder->CreateStore(value, target);

    }

    void visit_Compare(const ASR::Compare_t &x) {
        this->visit_expr(*x.m_left);
        llvm::Value *left = tmp;
        this->visit_expr(*x.m_right);
        llvm::Value *right = tmp;
        switch (x.m_op) {
            case (ASR::cmpopType::Eq) : {
                tmp = builder->CreateICmpEQ(left, right);
                break;
            }
            default : {
                throw SemanticError("Comparison operator not implemented",
                        x.base.base.loc);
            }
        }
    }

    void visit_If(const ASR::If_t &x) {
        this->visit_expr(*x.m_test);
        llvm::Value *cond=tmp;

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        for (size_t i=0; i<x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
        }

    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        llvm::Value *left_val = tmp;
        this->visit_expr(*x.m_right);
        llvm::Value *right_val = tmp;
        switch (x.m_op) {
            case ASR::operatorType::Add: { 
                tmp = builder->CreateAdd(left_val, right_val);
                break;
            };
            case ASR::operatorType::Sub: { 
                tmp = builder->CreateSub(left_val, right_val);
                break;
            };
            case ASR::operatorType::Mul: { 
                tmp = builder->CreateMul(left_val, right_val);
                break;
            };
            case ASR::operatorType::Div: { 
                tmp = builder->CreateUDiv(left_val, right_val);
                break;
            };
            case ASR::operatorType::Pow: { 
                throw CodeGenError("Pow not implemented yet");
                break;
            };
        }
    }

    void visit_Num(const ASR::Num_t &x) {
        tmp = llvm::ConstantInt::get(context, llvm::APInt(64, x.m_n));
    }

    void visit_Var(const ASR::Var_t &x) {
        llvm::Value *ptr = llvm_symtab[std::string(VARIABLE((ASR::asr_t*)(x.m_v))->m_name)];
        tmp = builder->CreateLoad(ptr);
    }

    void visit_Print(const ASR::Print_t &x) {
        llvm::Function *fn_printf = module->getFunction("_lfortran_printf");
        if (!fn_printf) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {llvm::Type::getInt8PtrTy(context)}, true);
            fn_printf = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, "_lfortran_printf", module.get());
        }
        LFORTRAN_ASSERT(x.n_values == 1);
        this->visit_expr(*x.m_values[0]);
        llvm::Value *arg1 = tmp;
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("%d\n");
        builder->CreateCall(fn_printf, {fmt_ptr, arg1});
    }

    void visit_ErrorStop(const ASR::ErrorStop_t &x) {
        llvm::Function *fn_printf = module->getFunction("_lfortran_printf");
        if (!fn_printf) {
            llvm::FunctionType *function_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(context), {llvm::Type::getInt8PtrTy(context)}, true);
            fn_printf = llvm::Function::Create(function_type,
                    llvm::Function::ExternalLinkage, "_lfortran_printf", module.get());
        }
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("ERROR STOP\n");
        builder->CreateCall(fn_printf, {fmt_ptr});
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
    std::string msg;
    llvm::raw_string_ostream err(msg);
    if (llvm::verifyModule(*v.module, &err)) {
        std::string buf;
        llvm::raw_string_ostream os(buf);
        v.module->print(os, nullptr);
        std::cout << os.str();
        throw CodeGenError("asr_to_llvm: module failed verification. Error:\n"
            + err.str());
    };
    return std::make_unique<LLVMModule>(std::move(v.module));
}

} // namespace LFortran
