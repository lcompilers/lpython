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
#include <lfortran/pass/do_loops.h>
#include <lfortran/pass/global_stmts.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/pickle.h>


namespace LFortran {

// Platform dependent fast unique hash:
uint64_t static get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

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

    std::map<uint64_t, llvm::Value*> llvm_symtab;

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
        uint32_t h = get_hash((ASR::asr_t*)&x);
        // This happens at global scope, so the intent can only be either local
        // (global variable declared/initialized in this translation unit), or
        // external (global variable not declared/initialized in this
        // translation unit, just referenced).
        LFORTRAN_ASSERT(x.m_intent == intent_local
            || x.m_intent == intent_external);
        bool external = (x.m_intent == intent_external);
        if (x.m_type->type == ASR::ttypeType::Integer) {
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                llvm::Type::getInt64Ty(context));
            if (!external) {
                module->getNamedGlobal(x.m_name)->setInitializer(
                    llvm::ConstantInt::get(context, llvm::APInt(64, 0)));
            }
            llvm_symtab[h] = ptr;
        } else if (x.m_type->type == ASR::ttypeType::Real) {
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                llvm::Type::getFloatTy(context));
            if (!external) {
                module->getNamedGlobal(x.m_name)->setInitializer(
                    llvm::ConstantFP::get(context, llvm::APFloat((float)0)));
            }
            llvm_symtab[h] = ptr;
        } else if (x.m_type->type == ASR::ttypeType::Logical) {
            llvm::Constant *ptr = module->getOrInsertGlobal(x.m_name,
                llvm::Type::getInt1Ty(context));
            if (!external) {
                module->getNamedGlobal(x.m_name)->setInitializer(
                    llvm::ConstantInt::get(context, llvm::APInt(1, 0)));
            }
            llvm_symtab[h] = ptr;
        } else {
            throw CodeGenError("Variable type not supported");
        }
    }

    void visit_Module(const ASR::Module_t &x) {
        // TODO: mangle all subroutine / function names with a module prefix
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::sub) {
                ASR::Subroutine_t *s = ASR::down_cast2<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (item.second->type == ASR::asrType::fn) {
                ASR::Function_t *s = ASR::down_cast2<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::sub) {
                ASR::Subroutine_t *s = ASR::down_cast2<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (item.second->type == ASR::asrType::fn) {
                ASR::Function_t *s = ASR::down_cast2<ASR::Function_t>(item.second);
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
                uint32_t h = get_hash((ASR::asr_t*)v);

                if (v->m_type->type == ASR::ttypeType::Integer) {
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        llvm::Type::getInt64Ty(context), nullptr, v->m_name);
                    llvm_symtab[h] = ptr;
                } else if (v->m_type->type == ASR::ttypeType::Real) {
                    // TODO: Assuming single precision
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        llvm::Type::getFloatTy(context), nullptr, v->m_name);
                    llvm_symtab[h] = ptr;
                } else if (v->m_type->type == ASR::ttypeType::Logical) {
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        llvm::Type::getInt1Ty(context), nullptr, v->m_name);
                    llvm_symtab[h] = ptr;
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

    template <typename T>
    std::vector<llvm::Type*> convert_args(const T &x) {
        std::vector<llvm::Type*> args;
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
            // TODO: we are assuming integer here:
            LFORTRAN_ASSERT(arg->m_type->type == ASR::ttypeType::Integer);
            // We pass all arguments as pointers for now
            args.push_back(llvm::Type::getInt64PtrTy(context));
        }
        return args;
    }

    template <typename T>
    void declare_args(const T &x, llvm::Function &F) {
        size_t i = 0;
        for (llvm::Argument &llvm_arg : F.args()) {
            ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
            uint32_t h = get_hash((ASR::asr_t*)arg);
            std::string arg_s = arg->m_name;
            llvm_arg.setName(arg_s);
            llvm_symtab[h] = &llvm_arg;
            i++;
        }
    }

    template <typename T>
    void declare_local_vars(const T &x) {
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;
                uint32_t h = get_hash((ASR::asr_t*)v);

                llvm::Type *type;
                if (v->m_intent == intent_local || v->m_intent == intent_return_var) {
                    if (v->m_type->type == ASR::ttypeType::Integer) {
                        type = llvm::Type::getInt64Ty(context);
                    } else if (v->m_type->type == ASR::ttypeType::Real) {
                        type = llvm::Type::getFloatTy(context);
                    } else {
                        throw CodeGenError("Function: type not supported");
                    }
                    llvm::AllocaInst *ptr = builder->CreateAlloca(
                        type, nullptr, v->m_name);
                    llvm_symtab[h] = ptr;
                }
            }
        }
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::ttypeType return_var_type = EXPR2VAR(x.m_return_var)->m_type->type;
        llvm::Type *return_type;
        if (return_var_type == ASR::ttypeType::Integer) {
            return_type = llvm::Type::getInt64Ty(context);
        } else if (return_var_type == ASR::ttypeType::Real) {
            return_type = llvm::Type::getFloatTy(context);
        } else {
            throw CodeGenError("Function: return type not supported");
        }
        std::vector<llvm::Type*> args = convert_args(x);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                return_type, args, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, x.m_name, module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);

        declare_args(x, *F);

        declare_local_vars(x);

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        ASR::Variable_t *asr_retval = EXPR2VAR(x.m_return_var);
        uint32_t h = get_hash((ASR::asr_t*)asr_retval);
        llvm::Value *ret_val = llvm_symtab[h];
        llvm::Value *ret_val2 = builder->CreateLoad(ret_val);
        builder->CreateRet(ret_val2);
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        std::vector<llvm::Type*> args = convert_args(x);
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(context), args, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, x.m_name, module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                ".entry", F);
        builder->SetInsertPoint(BB);

        declare_args(x, *F);

        declare_local_vars(x);

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        builder->CreateRetVoid();
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        //this->visit_expr(*x.m_target);
        ASR::Variable_t *asr_target = EXPR2VAR(x.m_target);
        uint32_t h = get_hash((ASR::asr_t*)asr_target);
        llvm::Value *target = llvm_symtab[h];
        this->visit_expr(*x.m_value);
        llvm::Value *value=tmp;
        builder->CreateStore(value, target);

    }

    void visit_Compare(const ASR::Compare_t &x) {
        this->visit_expr(*x.m_left);
        llvm::Value *left = tmp;
        this->visit_expr(*x.m_right);
        llvm::Value *right = tmp;
        LFORTRAN_ASSERT(expr_type(x.m_left)->type == expr_type(x.m_right)->type);
        ASR::ttypeType optype = expr_type(x.m_left)->type;
        if (optype == ASR::ttypeType::Integer) {
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
        } else if (optype == ASR::ttypeType::Real) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : {
                    tmp = builder->CreateFCmpUEQ(left, right);
                    break;
                }
                case (ASR::cmpopType::Gt) : {
                    tmp = builder->CreateFCmpUGT(left, right);
                    break;
                }
                case (ASR::cmpopType::GtE) : {
                    tmp = builder->CreateFCmpUGE(left, right);
                    break;
                }
                case (ASR::cmpopType::Lt) : {
                    tmp = builder->CreateFCmpULT(left, right);
                    break;
                }
                case (ASR::cmpopType::LtE) : {
                    tmp = builder->CreateFCmpULE(left, right);
                    break;
                }
                case (ASR::cmpopType::NotEq) : {
                    tmp = builder->CreateFCmpUNE(left, right);
                    break;
                }
                default : {
                    throw SemanticError("Comparison operator not implemented",
                            x.base.base.loc);
                }
            }
        } else {
            throw CodeGenError("Only Integer and Real implemented in Compare");
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

    void visit_Exit(const ASR::Exit_t & /* x */) {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *after = llvm::BasicBlock::Create(context, "after", fn);
        builder->CreateBr(current_loopend);
        builder->SetInsertPoint(after);
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
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
        if (x.m_type->type == ASR::ttypeType::Integer) {
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
                    llvm::Value *fleft = builder->CreateSIToFP(left_val,
                            llvm::Type::getFloatTy(context));
                    llvm::Value *fright = builder->CreateSIToFP(right_val,
                            llvm::Type::getFloatTy(context));

                    llvm::Function *fn_pow = module->getFunction("llvm.pow.f32");
                    if (!fn_pow) {
                        llvm::FunctionType *function_type = llvm::FunctionType::get(
                                llvm::Type::getFloatTy(context), {
                                    llvm::Type::getFloatTy(context),
                                    llvm::Type::getFloatTy(context)
                                }, false);
                        fn_pow = llvm::Function::Create(function_type,
                                llvm::Function::ExternalLinkage, "llvm.pow.f32",
                                module.get());
                    }
                    tmp = builder->CreateCall(fn_pow, {fleft, fright});
                    tmp = builder->CreateFPToSI(tmp, llvm::Type::getInt64Ty(context));
                    break;
                };
            }
        } else if (x.m_type->type == ASR::ttypeType::Real) {
            switch (x.m_op) {
                case ASR::operatorType::Add: {
                    tmp = builder->CreateFAdd(left_val, right_val);
                    break;
                };
                case ASR::operatorType::Sub: {
                    tmp = builder->CreateFSub(left_val, right_val);
                    break;
                };
                case ASR::operatorType::Mul: {
                    tmp = builder->CreateFMul(left_val, right_val);
                    break;
                };
                case ASR::operatorType::Div: {
                    tmp = builder->CreateFDiv(left_val, right_val);
                    break;
                };
                case ASR::operatorType::Pow: {
                    llvm::Function *fn_pow = module->getFunction("llvm.pow.f32");
                    if (!fn_pow) {
                        llvm::FunctionType *function_type = llvm::FunctionType::get(
                                llvm::Type::getFloatTy(context), {
                                    llvm::Type::getFloatTy(context),
                                    llvm::Type::getFloatTy(context)
                                }, false);
                        fn_pow = llvm::Function::Create(function_type,
                                llvm::Function::ExternalLinkage, "llvm.pow.f32",
                                module.get());
                    }
                    tmp = builder->CreateCall(fn_pow, {left_val, right_val});
                    break;
                };
            }
        } else {
            throw CodeGenError("Binop: Only Real and Integer types implemented");
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
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(x.m_v);
        uint32_t h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(llvm_symtab.find(h) != llvm_symtab.end());
        llvm::Value *ptr = llvm_symtab[h];
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

    void visit_Stop(const ASR::Stop_t & /* x */) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("STOP\n");
        printf(context, *module, *builder, {fmt_ptr});
        int exit_code_int = 0;
        llvm::Value *exit_code = llvm::ConstantInt::get(context,
                llvm::APInt(64, exit_code_int));
        exit(context, *module, *builder, exit_code);
    }

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        llvm::Value *fmt_ptr = builder->CreateGlobalStringPtr("ERROR STOP\n");
        printf(context, *module, *builder, {fmt_ptr});
        int exit_code_int = 1;
        llvm::Value *exit_code = llvm::ConstantInt::get(context,
                llvm::APInt(64, exit_code_int));
        exit(context, *module, *builder, exit_code);
    }

    template <typename T>
    std::vector<llvm::Value*> convert_call_args(const T &x) {
        std::vector<llvm::Value *> args;
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i]->type == ASR::exprType::Var) {
                ASR::Variable_t *arg = EXPR2VAR(x.m_args[i]);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                tmp = llvm_symtab[h];
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
        return args;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(x.m_name);
        llvm::Function *fn = module->getFunction(s->m_name);
        if (!fn) {
            throw CodeGenError("Subroutine code not generated for '"
                + std::string(s->m_name) + "'");
        }
        std::vector<llvm::Value *> args = convert_call_args(x);
        builder->CreateCall(fn, args);
    }

    void visit_FuncCall(const ASR::FuncCall_t &x) {
        ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(x.m_func);
        llvm::Function *fn = module->getFunction(s->m_name);
        if (!fn) {
            throw CodeGenError("Function code not generated for '"
                + std::string(s->m_name) + "'");
        }
        std::vector<llvm::Value *> args = convert_call_args(x);
        tmp = builder->CreateCall(fn, args);
    }

};



std::unique_ptr<LLVMModule> asr_to_llvm(ASR::TranslationUnit_t &asr,
        llvm::LLVMContext &context, Allocator &al)
{
    ASRToLLVMVisitor v(context);
    pass_wrap_global_stmts_into_function(al, asr, "f");

    // Uncomment for debugging the ASR after the transformation
    // std::cout << pickle(asr) << std::endl;

    pass_replace_do_loops(al, asr);
    v.visit_asr((ASR::asr_t&)asr);
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
