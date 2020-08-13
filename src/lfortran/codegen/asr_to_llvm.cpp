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
#include <lfortran/asr_utils.h>
#include <lfortran/pickle.h>


namespace LFortran {

void printf(llvm::LLVMContext &context, llvm::Module &module,
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

void exit(llvm::LLVMContext &context, llvm::Module &module,
    llvm::IRBuilder<> &builder, llvm::Value* exit_code)
{
    llvm::Function *fn_exit = module.getFunction("exit");
    if (!fn_exit) {
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), {llvm::Type::getInt64Ty(context)},
                false);
        fn_exit = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, "exit", &module);
    }
    builder.CreateCall(fn_exit, {exit_code});
}

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
public:
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::Value *tmp;
    llvm::BasicBlock *current_loophead, *current_loopend;

    // TODO: This is not scoped, should lookup by hashes instead:
    std::map<std::string, llvm::Value*> llvm_symtab;

    ASRToLLVMVisitor(llvm::LLVMContext &context) : context{context} {}

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        module = std::make_unique<llvm::Module>("LFortran", context);
        module->setDataLayout("");
        builder = std::make_unique<llvm::IRBuilder<>>(context);


        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);

        // Process Variables first:
        for (auto &item : x.m_global_scope->scope) {
            if (item.second->type == ASR::asrType::var) {
                visit_asr(*item.second);
            }
        }

        // Then the rest:
        for (auto &item : x.m_global_scope->scope) {
            if (item.second->type != ASR::asrType::var) {
                visit_asr(*item.second);
            }
        }
    }

    void visit_Variable(const ASR::Variable_t &x) {
        // This hapens at global scope
        if (x.m_type->type == ASR::ttypeType::Integer) {
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                llvm::Type::getInt64Ty(context));
            llvm_symtab[std::string(x.m_name)] = ptr;
        } else if (x.m_type->type == ASR::ttypeType::Logical) {
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                llvm::Type::getInt1Ty(context));
            llvm_symtab[std::string(x.m_name)] = ptr;
        } else {
            throw CodeGenError("Variable type not supported");
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::sub) {
                ASR::Subroutine_t *s = SUBROUTINE(item.second);
                visit_Subroutine(*s);
            }
            if (item.second->type == ASR::asrType::fn) {
                ASR::Function_t *s = FUNCTION(item.second);
                visit_Function(*s);
            }
        }

        // Generate code for the main program
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

                if (v->m_type->type == ASR::ttypeType::Integer) {
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        llvm::Type::getInt64Ty(context), nullptr, v->m_name);
                    llvm_symtab[std::string(v->m_name)] = ptr;
                } else if (v->m_type->type == ASR::ttypeType::Real) {
                    // TODO: Assuming single precision
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        llvm::Type::getFloatTy(context), nullptr, v->m_name);
                    llvm_symtab[std::string(v->m_name)] = ptr;
                } else if (v->m_type->type == ASR::ttypeType::Logical) {
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        llvm::Type::getInt1Ty(context), nullptr, v->m_name);
                    llvm_symtab[std::string(v->m_name)] = ptr;
                } else {
                    throw CodeGenError("Variable type not supported");
                }
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

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        std::vector<llvm::Type*> args;
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_args[i])->m_v);
            LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
            // TODO: we are assuming integer here:
            LFORTRAN_ASSERT(arg->m_type->type == ASR::ttypeType::Integer);
            // We pass all arguments as pointers for now
            args.push_back(llvm::Type::getInt64PtrTy(context));
        }

        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), args, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, x.m_name, module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);

        size_t i = 0;
        for (llvm::Argument &llvm_arg : F->args()) {
            ASR::Variable_t *arg = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_args[i])->m_v);
            LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
            std::string arg_s = arg->m_name;
            llvm_arg.setName(arg_s);
            llvm_symtab[arg_s] = &llvm_arg;
            i++;
        }

        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;
                if (!is_arg_dummy(v->m_intent)) {
                    // TODO: we are assuming integer here:
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        llvm::Type::getInt64Ty(context), nullptr, v->m_name);
                    llvm_symtab[std::string(v->m_name)] = ptr;
                }
            }
        }

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        builder->CreateRetVoid();
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
            case (ASR::cmpopType::Gt) : {
                tmp = builder->CreateICmpSGT(left, right);
                break;
            }
            case (ASR::cmpopType::GtE) : {
                tmp = builder->CreateICmpSGE(left, right);
                break;
            }
            case (ASR::cmpopType::Lt) : {
                tmp = builder->CreateICmpSLT(left, right);
                break;
            }
            case (ASR::cmpopType::LtE) : {
                tmp = builder->CreateICmpSLE(left, right);
                break;
            }
            case (ASR::cmpopType::NotEq) : {
                tmp = builder->CreateICmpNE(left, right);
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
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");
        builder->CreateCondBr(cond, thenBB, elseBB);
        builder->SetInsertPoint(thenBB);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        llvm::Value *thenV = llvm::ConstantInt::get(context, llvm::APInt(64, 1));
        builder->CreateBr(mergeBB);
        thenBB = builder->GetInsertBlock();
        fn->getBasicBlockList().push_back(elseBB);
        builder->SetInsertPoint(elseBB);
        for (size_t i=0; i<x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
        }
        llvm::Value *elseV = llvm::ConstantInt::get(context, llvm::APInt(64, 2));
        builder->CreateBr(mergeBB);
        elseBB = builder->GetInsertBlock();
        fn->getBasicBlockList().push_back(mergeBB);
        builder->SetInsertPoint(mergeBB);
        llvm::PHINode *PN = builder->CreatePHI(llvm::Type::getInt64Ty(context), 2,
                                        "iftmp");
        PN->addIncoming(thenV, thenBB);
        PN->addIncoming(elseV, elseBB);
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *loophead = llvm::BasicBlock::Create(context, "loop.head", fn);
        llvm::BasicBlock *loopbody = llvm::BasicBlock::Create(context, "loop.body");
        llvm::BasicBlock *loopend = llvm::BasicBlock::Create(context, "loop.end");
        this->current_loophead = loophead;
        this->current_loopend = loopend;

        // head
        builder->CreateBr(loophead);
        builder->SetInsertPoint(loophead);
        this->visit_expr(*x.m_test);
        llvm::Value *cond = tmp;
        builder->CreateCondBr(cond, loopbody, loopend);

        // body
        fn->getBasicBlockList().push_back(loopbody);
        builder->SetInsertPoint(loopbody);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        builder->CreateBr(loophead);

        // end
        fn->getBasicBlockList().push_back(loopend);
        builder->SetInsertPoint(loopend);
    }

    void visit_Exit(const ASR::Exit_t &x) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *after = llvm::BasicBlock::Create(context, "after", fn);
        builder->CreateBr(current_loopend);
        builder->SetInsertPoint(after);
    }

    void visit_Cycle(const ASR::Cycle_t &x) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *after = llvm::BasicBlock::Create(context, "after", fn);
        builder->CreateBr(current_loophead);
        builder->SetInsertPoint(after);
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

    void visit_UnaryOp(const ASR::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        if (x.m_type->type == ASR::ttypeType::Integer) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // tmp = tmp;
                return;
            } else if (x.m_op == ASR::unaryopType::USub) {
                llvm::Value *zero = llvm::ConstantInt::get(context,
                    llvm::APInt(64, 0));
                tmp = builder ->CreateSub(zero, tmp);
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet");
            }
        } else if (x.m_type->type == ASR::ttypeType::Logical) {
            if (x.m_op == ASR::unaryopType::Not) {
                tmp = builder ->CreateNot(tmp);
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet in Logical");
            }
        } else {
            throw CodeGenError("UnaryOp: type not supported yet");
        }
    }

    void visit_Num(const ASR::Num_t &x) {
        tmp = llvm::ConstantInt::get(context, llvm::APInt(64, x.m_n));
    }

    void visit_ConstantReal(const ASR::ConstantReal_t &x) {
        double val = std::atof(x.m_r);
        // TODO: assuming single precision
        tmp = llvm::ConstantFP::get(context, llvm::APFloat((float)val));
    }

    void visit_Constant(const ASR::Constant_t &x) {
        int val;
        if (x.m_value == true) {
            val = 1;
        } else {
            val = 0;
        }
        tmp = llvm::ConstantInt::get(context, llvm::APInt(1, val));
    }


    void visit_Str(const ASR::Str_t &x) {
        tmp = builder->CreateGlobalStringPtr(x.m_s);
    }

    void visit_Var(const ASR::Var_t &x) {
        llvm::Value *ptr = llvm_symtab[std::string(VARIABLE((ASR::asr_t*)(x.m_v))->m_name)];
        tmp = builder->CreateLoad(ptr);
    }

    void visit_ImplicitCast(const ASR::ImplicitCast_t &x) {
        visit_expr(*x.m_arg);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                tmp = builder->CreateSIToFP(tmp, llvm::Type::getFloatTy(context));
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                tmp = builder->CreateFPToSI(tmp, llvm::Type::getInt64Ty(context));
                break;
            }
            default : throw CodeGenError("Cast kind not implemented");
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        std::vector<llvm::Value *> args;
        std::vector<std::string> fmt;
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            args.push_back(tmp);
            ASR::expr_t *v = x.m_values[i];
            ASR::ttype_t *t = expr_type(v);
            if (t->type == ASR::ttypeType::Integer) {
                fmt.push_back("%d");
            } else if (t->type == ASR::ttypeType::Real) {
                fmt.push_back("%f");
            } else if (t->type == ASR::ttypeType::Character) {
                fmt.push_back("%s");
            } else {
                throw LFortranException("Type not implemented");
            }
        }
        std::string fmt_str;
        for (size_t i=0; i<fmt.size(); i++) {
            fmt_str += fmt[i];
            if (i < fmt.size()-1) fmt_str += " ";
        }
        fmt_str += "\n";
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr(fmt_str);
        std::vector<llvm::Value *> printf_args;
        printf_args.push_back(fmt_ptr);
        printf_args.insert(printf_args.end(), args.begin(), args.end());
        printf(context, *module, *builder, printf_args);
    }

    void visit_Stop(const ASR::Stop_t &x) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("STOP\n");
        printf(context, *module, *builder, {fmt_ptr});
        int exit_code_int = 0;
        llvm::Value *exit_code = llvm::ConstantInt::get(context,
                llvm::APInt(64, exit_code_int));
        exit(context, *module, *builder, exit_code);
    }

    void visit_ErrorStop(const ASR::ErrorStop_t &x) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("ERROR STOP\n");
        printf(context, *module, *builder, {fmt_ptr});
        int exit_code_int = 1;
        llvm::Value *exit_code = llvm::ConstantInt::get(context,
                llvm::APInt(64, exit_code_int));
        exit(context, *module, *builder, exit_code);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Subroutine_t *s = SUBROUTINE((ASR::asr_t*)x.m_name);
        llvm::Function *fn = module->getFunction(s->m_name);
        if (!fn) {
            throw CodeGenError("Subroutine code not generated for '"
                + std::string(s->m_name) + "'");
        }
        std::vector<llvm::Value *> args;
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i]->type == ASR::exprType::Var) {
                ASR::Variable_t *arg = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_args[i])->m_v);
                std::string arg_name = arg->m_name;
                tmp = llvm_symtab[arg_name];
            } else {
                this->visit_expr(*x.m_args[i]);
                llvm::Value *value=tmp;
                llvm::AllocaInst *target = builder->CreateAlloca(
                    llvm::Type::getInt64Ty(context), nullptr);
                builder->CreateStore(value, target);
                tmp = target;
            }
            args.push_back(tmp);
        }
        builder->CreateCall(fn, args);
    }

};

// Edits the ASR inplace.
void wrap_global_stmts_into_function(Allocator &al, ASR::TranslationUnit_t &unit) {
    if (unit.n_items > 0) {
        // Add an anonymous function
        const char* fn_name_orig = "f";
        char *fn_name = (char*)fn_name_orig;
        SymbolTable *fn_scope = al.make_new<SymbolTable>(unit.m_global_scope);

        ASR::ttype_t *type;
        Location loc;
        type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        ASR::asr_t *return_var = ASR::make_Variable_t(al, loc,
            fn_scope, fn_name, intent_return_var, type);
        fn_scope->scope[std::string(fn_name)] = return_var;

        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, loc,
            VAR(return_var));

        Vec<ASR::stmt_t*> body;
        body.reserve(al, unit.n_items);
        for (size_t i=0; i<unit.n_items; i++) {
            if (unit.m_items[i]->type == ASR::asrType::expr) {
                ASR::expr_t *target = EXPR(return_var_ref);
                ASR::expr_t *value = EXPR(unit.m_items[i]);
                ASR::stmt_t* asr_stmt = STMT(ASR::make_Assignment_t(al, loc, target, value));
                body.push_back(al, asr_stmt);
            } else if (unit.m_items[i]->type == ASR::asrType::stmt) {
                ASR::stmt_t* asr_stmt = STMT(unit.m_items[i]);
                body.push_back(al, asr_stmt);
            } else {
                throw CodeGenError("Unsupported type of global scope node");
            }
        }


        ASR::asr_t *fn = ASR::make_Function_t(
            al, loc,
            /* a_symtab */ fn_scope,
            /* a_name */ fn_name,
            /* a_args */ nullptr,
            /* n_args */ 0,
            /* a_body */ body.p,
            /* n_body */ body.size(),
            /* a_bind */ nullptr,
            /* a_return_var */ EXPR(return_var_ref),
            /* a_module */ nullptr);
        std::string sym_name = fn_name;
        if (unit.m_global_scope->scope.find(sym_name) != unit.m_global_scope->scope.end()) {
            throw SemanticError("Function already defined", fn->loc);
        }
        unit.m_global_scope->scope[sym_name] = fn;
        unit.m_items = nullptr;
        unit.n_items = 0;
    }
}

/*
Converts:

    do i = a, b, c
        ...
    end do

to:

    i = a-c
    do while (i+c <= b)
        i = i+c
        ...
    end do

The comparison is >= for c<0.
*/
Vec<ASR::stmt_t*> replace_doloop(Allocator &al, const ASR::DoLoop_t &loop) {
    Location loc = loop.base.base.loc;
    ASR::expr_t *a=loop.m_head.m_start;
    ASR::expr_t *b=loop.m_head.m_end;
    ASR::expr_t *c=loop.m_head.m_increment;
    LFORTRAN_ASSERT(a);
    LFORTRAN_ASSERT(b);
    if (!c) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        c = EXPR(ASR::make_Num_t(al, loc, 1, type));
    }
    LFORTRAN_ASSERT(c);
    int increment;
    if (c->type == ASR::exprType::Num) {
        increment = EXPR_NUM((ASR::asr_t*)c)->m_n;
    } else if (c->type == ASR::exprType::UnaryOp) {
        ASR::UnaryOp_t *u = EXPR_UNARYOP((ASR::asr_t*)c);
        LFORTRAN_ASSERT(u->m_op == ASR::unaryopType::USub);
        LFORTRAN_ASSERT(u->m_operand->type == ASR::exprType::Num);
        increment = - EXPR_NUM((ASR::asr_t*)u->m_operand)->m_n;
    } else {
        throw CodeGenError("Do loop increment type not supported");
    }
    ASR::cmpopType cmp_op;
    if (increment > 0) {
        cmp_op = ASR::cmpopType::LtE;
    } else {
        cmp_op = ASR::cmpopType::GtE;
    }
    ASR::expr_t *target = loop.m_head.m_v;
    ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
    ASR::stmt_t *stmt1 = STMT(ASR::make_Assignment_t(al, loc, target,
        EXPR(ASR::make_BinOp_t(al, loc, a, ASR::operatorType::Sub, c, type))
    ));

    ASR::expr_t *cond = EXPR(ASR::make_Compare_t(al, loc,
        EXPR(ASR::make_BinOp_t(al, loc, target, ASR::operatorType::Add, c, type)),
        cmp_op, b, type));
    Vec<ASR::stmt_t*> body;
    body.reserve(al, loop.n_body+1);
    body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, target,
        EXPR(ASR::make_BinOp_t(al, loc, target, ASR::operatorType::Add, c, type))
    )));
    for (size_t i=0; i<loop.n_body; i++) {
        body.push_back(al, loop.m_body[i]);
    }
    ASR::stmt_t *stmt2 = STMT(ASR::make_WhileLoop_t(al, loc, cond,
        body.p, body.size()));
    Vec<ASR::stmt_t*> result;
    result.reserve(al, 2);
    result.push_back(al, stmt1);
    result.push_back(al, stmt2);

    /*
    std::cout << "Input:" << std::endl;
    std::cout << pickle((ASR::asr_t&)loop);
    std::cout << "Output:" << std::endl;
    std::cout << pickle((ASR::asr_t&)*stmt1);
    std::cout << pickle((ASR::asr_t&)*stmt2);
    std::cout << "--------------" << std::endl;
    */

    return result;
}

class DoLoopVisitor : public ASR::BaseWalkVisitor<DoLoopVisitor>
{
private:
    Allocator &al;
    Vec<ASR::stmt_t*> do_loop_result;
public:
    DoLoopVisitor(Allocator &al) : al{al} {
        do_loop_result.n = 0;

    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        for (auto &a : x.m_global_scope->scope) {
            this->visit_asr(*a.second);
        }
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            do_loop_result.n = 0;
            visit_stmt(*m_body[i]);
            if (do_loop_result.size() > 0) {
                for (size_t j=0; j<do_loop_result.size(); j++) {
                    body.push_back(al, do_loop_result[j]);
                }
                do_loop_result.n = 0;
            } else {
                body.push_back(al, m_body[i]);
            }
        }
        m_body = body.p;
        n_body = body.size();
    }

    // TODO: Only Program and While is processed, we need to process all calls
    // to visit_stmt().

    void visit_Program(const ASR::Program_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        do_loop_result = replace_doloop(al, x);
    }
};

void replace_doloops(Allocator &al, ASR::TranslationUnit_t &unit) {
    DoLoopVisitor v(al);
    // Each call transforms only one layer of nested loops, so we call it twice
    // to transform doubly nested loops:
    v.visit_TranslationUnit(unit);
    v.visit_TranslationUnit(unit);
}

std::unique_ptr<LLVMModule> asr_to_llvm(ASR::asr_t &asr,
        llvm::LLVMContext &context, Allocator &al)
{
    ASRToLLVMVisitor v(context);
    LFORTRAN_ASSERT(asr.type == ASR::asrType::unit);
    wrap_global_stmts_into_function(al, *TRANSLATION_UNIT(&asr));

    // Uncomment for debugging the ASR after the transformation
    // std::cout << pickle(asr) << std::endl;

    replace_doloops(al, *TRANSLATION_UNIT(&asr));
    v.visit_asr(asr);
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
