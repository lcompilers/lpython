#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_symbolic.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>

#include <vector>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class SymEngine_Stack {
public:
    std::vector<std::string> stack;
    int stack_top = -1;
    int count = 0;

    SymEngine_Stack() {}

    std::string push() {
        std::string var;
        var = "stack" + std::to_string(count);
        stack.push_back(var);
        stack_top++;
        count++;
        return stack[stack_top];
    }

    std::string pop() {
        std::string top = stack[stack_top];
        stack_top--;
        stack.pop_back();
        if (stack_top == -1) stack.clear();
        return top;
    }
};

class ReplaceSymbolicVisitor : public PassUtils::PassVisitor<ReplaceSymbolicVisitor>
{
public:
    ReplaceSymbolicVisitor(Allocator &al_) :
    PassVisitor(al_, nullptr) {
        pass_result.reserve(al, 1);
    }
    std::vector<std::string> symbolic_dependencies;
    std::set<ASR::symbol_t*> symbolic_vars;
    SymEngine_Stack symengine_stack;

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = this->current_scope;
        this->current_scope = xx.m_symtab;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *s = ASR::down_cast<ASR::Variable_t>(item.second);
                this->visit_Variable(*s);
            }
        }
        transform_stmts(xx.m_body, xx.n_body);

        SetChar function_dependencies;
        function_dependencies.n = 0;
        function_dependencies.reserve(al, 1);
        for( size_t i = 0; i < xx.n_dependencies; i++ ) {
            function_dependencies.push_back(al, xx.m_dependencies[i]);
        }
        for( size_t i = 0; i < symbolic_dependencies.size(); i++ ) {
            function_dependencies.push_back(al, s2c(al, symbolic_dependencies[i]));
        }
        symbolic_dependencies.clear();
        xx.n_dependencies = function_dependencies.size();
        xx.m_dependencies = function_dependencies.p;
        this->current_scope = current_scope_copy;
    }

    void visit_Variable(const ASR::Variable_t& x) {
        ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
        if (xx.m_type->type == ASR::ttypeType::SymbolicExpression) {
            SymbolTable* module_scope = current_scope->parent;
            std::string var_name = xx.m_name;
            std::string placeholder = "_" + std::string(var_name);

            ASR::ttype_t *type1 = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
            xx.m_type = type1;
            symbolic_vars.insert(ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&xx));

            ASR::ttype_t *type2 = ASRUtils::TYPE(ASR::make_Integer_t(al, xx.base.base.loc, 8));
            ASR::symbol_t* sym2 = ASR::down_cast<ASR::symbol_t>(
                ASR::make_Variable_t(al, xx.base.base.loc, current_scope,
                                    s2c(al, placeholder), nullptr, 0,
                                    xx.m_intent, nullptr,
                                    nullptr, xx.m_storage,
                                    type2, nullptr, xx.m_abi,
                                    xx.m_access, xx.m_presence,
                                    xx.m_value_attr));

            current_scope->add_symbol(s2c(al, placeholder), sym2);

            std::string new_name = "basic_new_stack";
            symbolic_dependencies.push_back(new_name);
            if (!module_scope->get_symbol(new_name)) {
                std::string header = "symengine/cwrapper.h";
                SymbolTable *fn_symtab = al.make_new<SymbolTable>(module_scope);

                Vec<ASR::expr_t*> args;
                {
                    args.reserve(al, 1);
                    ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, xx.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, type1, nullptr,
                        ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, arg)));
                }

                Vec<ASR::stmt_t*> body;
                body.reserve(al, 1);

                Vec<char *> dep;
                dep.reserve(al, 1);

                ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, xx.base.base.loc,
                    fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                    nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                    ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                    false, false, nullptr, 0, false, false, false, s2c(al, header));
                ASR::symbol_t *new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                module_scope->add_symbol(new_name, new_symbol);
            }

            ASR::symbol_t* var_sym = current_scope->get_symbol(var_name);
            ASR::symbol_t* placeholder_sym = current_scope->get_symbol(placeholder);
            ASR::expr_t* target1 = ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, placeholder_sym));
            ASR::expr_t* target2 = ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, var_sym));

            // statement 1
            ASR::expr_t* value1 = ASRUtils::EXPR(ASR::make_Cast_t(al, xx.base.base.loc,
                ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, xx.base.base.loc, 0,
                ASRUtils::TYPE(ASR::make_Integer_t(al, xx.base.base.loc, 4)))),
                (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, type2,
                ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, xx.base.base.loc, 0, type2))));

            // statement 2
            ASR::expr_t* value2 = ASRUtils::EXPR(ASR::make_PointerNullConstant_t(al, xx.base.base.loc, type1));

            // statement 3
            ASR::expr_t* get_pointer_node = ASRUtils::EXPR(ASR::make_GetPointer_t(al, xx.base.base.loc,
                target1, ASRUtils::TYPE(ASR::make_Pointer_t(al, xx.base.base.loc, type2)), nullptr));
            ASR::expr_t* value3 = ASRUtils::EXPR(ASR::make_PointerToCPtr_t(al, xx.base.base.loc, get_pointer_node,
                type1, nullptr));

            // statement 4
            ASR::symbol_t* basic_new_stack_sym = module_scope->get_symbol(new_name);
            Vec<ASR::call_arg_t> call_args;
            call_args.reserve(al, 1);
            ASR::call_arg_t call_arg;
            call_arg.loc = xx.base.base.loc;
            call_arg.m_value = target2;
            call_args.push_back(al, call_arg);

            // defining the assignment statement
            ASR::stmt_t* stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target1, value1, nullptr));
            ASR::stmt_t* stmt2 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value2, nullptr));
            ASR::stmt_t* stmt3 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value3, nullptr));
            ASR::stmt_t* stmt4 = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, xx.base.base.loc, basic_new_stack_sym,
                basic_new_stack_sym, call_args.p, call_args.n, nullptr));

            pass_result.push_back(al, stmt1);
            pass_result.push_back(al, stmt2);
            pass_result.push_back(al, stmt3);
            pass_result.push_back(al, stmt4);
        }
    }

    void perform_symbolic_binary_operation(Allocator &al, const Location &loc, SymbolTable* module_scope,
        const std::string& new_name, ASR::expr_t* value1, ASR::expr_t* value2, ASR::expr_t* value3) {
        symbolic_dependencies.push_back(new_name);
        if (!module_scope->get_symbol(new_name)) {
            std::string header = "symengine/cwrapper.h";
            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

            Vec<ASR::expr_t*> args;
            args.reserve(al, 3);
            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "x"), arg1);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg1)));
            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "y"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "y"), arg2);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg2)));
            ASR::symbol_t* arg3 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "z"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "z"), arg3);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg3)));

            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);

            Vec<char*> dep;
            dep.reserve(al, 1);

            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, loc,
                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                false, false, nullptr, 0, false, false, false, s2c(al, header));
            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
            module_scope->add_symbol(s2c(al, new_name), new_symbol);
        }

        ASR::symbol_t* func_sym = module_scope->get_symbol(new_name);
        Vec<ASR::call_arg_t> call_args;
        call_args.reserve(al, 3);
        ASR::call_arg_t call_arg1, call_arg2, call_arg3;
        call_arg1.loc = loc;
        call_arg1.m_value = value1;
        call_arg2.loc = loc;
        call_arg2.m_value = value2;
        call_arg3.loc = loc;
        call_arg3.m_value = value3;
        call_args.push_back(al, call_arg1);
        call_args.push_back(al, call_arg2);
        call_args.push_back(al, call_arg3);

        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, loc, func_sym,
            func_sym, call_args.p, call_args.n, nullptr));
        pass_result.push_back(al, stmt);
    }

    void perform_symbolic_unary_operation(Allocator &al, const Location &loc, SymbolTable* module_scope,
        const std::string& new_name, ASR::expr_t* value1, ASR::expr_t* value2) {
        symbolic_dependencies.push_back(new_name);
        if (!module_scope->get_symbol(new_name)) {
            std::string header = "symengine/cwrapper.h";
            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

            Vec<ASR::expr_t*> args;
            args.reserve(al, 2);
            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "x"), arg1);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg1)));
            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "y"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "y"), arg2);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg2)));

            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);

            Vec<char*> dep;
            dep.reserve(al, 1);

            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, loc,
                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                false, false, nullptr, 0, false, false, false, s2c(al, header));
            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
            module_scope->add_symbol(s2c(al, new_name), new_symbol);
        }

        ASR::symbol_t* func_sym = module_scope->get_symbol(new_name);
        Vec<ASR::call_arg_t> call_args;
        call_args.reserve(al, 2);
        ASR::call_arg_t call_arg1, call_arg2;
        call_arg1.loc = loc;
        call_arg1.m_value = value1;
        call_arg2.loc = loc;
        call_arg2.m_value = value2;
        call_args.push_back(al, call_arg1);
        call_args.push_back(al, call_arg2);

        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, loc, func_sym,
            func_sym, call_args.p, call_args.n, nullptr));
        pass_result.push_back(al, stmt);
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        SymbolTable* module_scope = current_scope->parent;
        if (ASR::is_a<ASR::IntrinsicFunction_t>(*x.m_value)) {
            ASR::IntrinsicFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicFunction_t>(x.m_value);
            int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
            if (intrinsic_func->m_type->type == ASR::ttypeType::SymbolicExpression) {
                switch (static_cast<LCompilers::ASRUtils::IntrinsicFunctions>(intrinsic_id)) {
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicPi: {
                        std::string new_name = "basic_const_pi";
                        symbolic_dependencies.push_back(new_name);
                        if (!module_scope->get_symbol(new_name)) {
                            std::string header = "symengine/cwrapper.h";
                            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                            Vec<ASR::expr_t*> args;
                            args.reserve(al, 1);
                            ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "x"), arg);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg)));

                            Vec<ASR::stmt_t*> body;
                            body.reserve(al, 1);

                            Vec<char*> dep;
                            dep.reserve(al, 1);

                            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                                false, false, nullptr, 0, false, false, false, s2c(al, header));
                            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                            module_scope->add_symbol(s2c(al, new_name), new_symbol);
                        }

                        // Create the function call statement for basic_const_pi
                        ASR::symbol_t* basic_const_pi_sym = module_scope->get_symbol(new_name);
                        Vec<ASR::call_arg_t> call_args;
                        call_args.reserve(al, 1);
                        ASR::call_arg_t call_arg;
                        call_arg.loc = x.base.base.loc;
                        call_arg.m_value = x.m_target;
                        call_args.push_back(al, call_arg);

                        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, basic_const_pi_sym,
                            basic_const_pi_sym, call_args.p, call_args.n, nullptr));
                        pass_result.push_back(al, stmt);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSymbol: {
                        std::string new_name = "symbol_set";
                        symbolic_dependencies.push_back(new_name);
                        if (!module_scope->get_symbol(new_name)) {
                            std::string header = "symengine/cwrapper.h";
                            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                            Vec<ASR::expr_t*> args;
                            args.reserve(al, 1);
                            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "x"), arg1);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg1)));
                            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "s"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "s"), arg2);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

                            Vec<ASR::stmt_t*> body;
                            body.reserve(al, 1);

                            Vec<char*> dep;
                            dep.reserve(al, 1);

                            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                                false, false, nullptr, 0, false, false, false, s2c(al, header));
                            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                            module_scope->add_symbol(s2c(al, new_name), new_symbol);
                        }

                        ASR::symbol_t* symbol_set_sym = module_scope->get_symbol(new_name);
                        Vec<ASR::call_arg_t> call_args;
                        call_args.reserve(al, 2);
                        ASR::call_arg_t call_arg1, call_arg2;
                        call_arg1.loc = x.base.base.loc;
                        call_arg1.m_value = x.m_target;
                        call_arg2.loc = x.base.base.loc;
                        call_arg2.m_value = intrinsic_func->m_args[0];
                        call_args.push_back(al, call_arg1);
                        call_args.push_back(al, call_arg2);

                        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, symbol_set_sym,
                            symbol_set_sym, call_args.p, call_args.n, nullptr));
                        pass_result.push_back(al, stmt);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicAdd: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_add",
                            x.m_target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSub: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_sub",
                            x.m_target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicMul: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_mul",
                            x.m_target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicDiv: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_div",
                            x.m_target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicPow: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_pow",
                            x.m_target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicDiff: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_diff",
                            x.m_target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSin: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_sin",
                            x.m_target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicCos: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_cos",
                            x.m_target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicLog: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_log",
                            x.m_target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicExp: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_exp",
                            x.m_target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicAbs: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_abs",
                            x.m_target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicExpand: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_expand",
                            x.m_target, value);
                        break;
                    }
                    default: {
                        throw LCompilersException("IntrinsicFunction: `"
                            + ASRUtils::get_intrinsic_name(intrinsic_id)
                            + "` is not implemented");
                    }
                }
            }
        } else if (ASR::is_a<ASR::Cast_t>(*x.m_value)) {
            ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(x.m_value);
            if (cast_t->m_kind == ASR::cast_kindType::IntegerToSymbolicExpression) {
                ASR::expr_t* cast_arg = cast_t->m_arg;
                ASR::expr_t* cast_value = cast_t->m_value;
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*cast_value)) {
                    ASR::IntrinsicFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicFunction_t>(cast_value);
                    int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
                    if (static_cast<LCompilers::ASRUtils::IntrinsicFunctions>(intrinsic_id) ==
                        LCompilers::ASRUtils::IntrinsicFunctions::SymbolicInteger) {
                        int const_value = 0;
                        if (ASR::is_a<ASR::IntegerConstant_t>(*cast_arg)){
                            ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(cast_arg);
                            const_value = const_int->m_n;
                        }
                        if (ASR::is_a<ASR::IntegerUnaryMinus_t>(*cast_arg)){
                            ASR::IntegerUnaryMinus_t *const_int_minus = ASR::down_cast<ASR::IntegerUnaryMinus_t>(cast_arg);
                            ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(const_int_minus->m_value);
                            const_value = const_int->m_n;
                        }
                        std::string new_name = "integer_set_si";
                        symbolic_dependencies.push_back(new_name);
                        if (!module_scope->get_symbol(new_name)) {
                            std::string header = "symengine/cwrapper.h";
                            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                            Vec<ASR::expr_t*> args;
                            args.reserve(al, 2);
                            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "x"), arg1);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg1)));
                            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "y"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "y"), arg2);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

                            Vec<ASR::stmt_t*> body;
                            body.reserve(al, 1);

                            Vec<char*> dep;
                            dep.reserve(al, 1);

                            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                                false, false, nullptr, 0, false, false, false, s2c(al, header));
                            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                            module_scope->add_symbol(s2c(al, new_name), new_symbol);
                        }

                        ASR::symbol_t* integer_set_sym = module_scope->get_symbol(new_name);
                        ASR::ttype_t* cast_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                        ASR::expr_t* value = ASRUtils::EXPR(ASR::make_Cast_t(al, x.base.base.loc, cast_arg,
                            (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, cast_type,
                            ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, const_value, cast_type))));
                        Vec<ASR::call_arg_t> call_args;
                        call_args.reserve(al, 2);
                        ASR::call_arg_t call_arg1, call_arg2;
                        call_arg1.loc = x.base.base.loc;
                        call_arg1.m_value = x.m_target;
                        call_arg2.loc = x.base.base.loc;
                        call_arg2.m_value = value;
                        call_args.push_back(al, call_arg1);
                        call_args.push_back(al, call_arg2);

                        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, integer_set_sym,
                            integer_set_sym, call_args.p, call_args.n, nullptr));
                        pass_result.push_back(al, stmt);
                    }
                }
            }
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        std::vector<ASR::expr_t*> print_tmp;
        SymbolTable* module_scope = current_scope->parent;
        for (size_t i=0; i<x.n_values; i++) {
            ASR::expr_t* val = x.m_values[i];
            if (ASR::is_a<ASR::Var_t>(*val) && ASR::is_a<ASR::CPtr_t>(*ASRUtils::expr_type(val))) {
                ASR::symbol_t *v = ASR::down_cast<ASR::Var_t>(val)->m_v;
                if (symbolic_vars.find(v) == symbolic_vars.end()) return;
                std::string new_name = "basic_str";
                symbolic_dependencies.push_back(new_name);
                if (!module_scope->get_symbol(new_name)) {
                    std::string header = "symengine/cwrapper.h";
                    SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 1);
                    ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "_lpython_return_variable"), nullptr, 0, ASR::intentType::ReturnVar,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
                    fn_symtab->add_symbol(s2c(al, "_lpython_return_variable"), arg1);
                    ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg2);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

                    Vec<ASR::stmt_t*> body;
                    body.reserve(al, 1);

                    Vec<char*> dep;
                    dep.reserve(al, 1);

                    ASR::expr_t* return_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, fn_symtab->get_symbol("_lpython_return_variable")));
                    ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                        fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                        return_var, ASR::abiType::BindC, ASR::accessType::Public,
                        ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                        false, false, nullptr, 0, false, false, false, s2c(al, header));
                    ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                    module_scope->add_symbol(s2c(al, new_name), new_symbol);
                }

                // Extract the symbol from value (Var)
                ASR::symbol_t* var_sym = ASR::down_cast<ASR::Var_t>(val)->m_v;
                ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));

                // Now create the FunctionCall node for basic_str
                ASR::symbol_t* basic_str_sym = module_scope->get_symbol(new_name);
                Vec<ASR::call_arg_t> call_args;
                call_args.reserve(al, 1);
                ASR::call_arg_t call_arg;
                call_arg.loc = x.base.base.loc;
                call_arg.m_value = target;
                call_args.push_back(al, call_arg);
                ASR::expr_t* function_call = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x.base.base.loc,
                    basic_str_sym, basic_str_sym, call_args.p, call_args.n,
                    ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)), nullptr, nullptr));
                print_tmp.push_back(function_call);
            } else if (ASR::is_a<ASR::IntrinsicFunction_t>(*val) && ASR::is_a<ASR::SymbolicExpression_t>(*ASRUtils::expr_type(val))) {
                ASR::IntrinsicFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicFunction_t>(val);
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, x.base.base.loc));
                std::string symengine_var = symengine_stack.push();
                ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                    al, x.base.base.loc, current_scope, s2c(al, symengine_var), nullptr, 0, ASR::intentType::Local,
                    nullptr, nullptr, ASR::storage_typeType::Default, type, nullptr,
                    ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
                current_scope->add_symbol(s2c(al, symengine_var), arg);
                for (auto &item : current_scope->get_scope()) {
                    if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                        ASR::Variable_t *s = ASR::down_cast<ASR::Variable_t>(item.second);
                        this->visit_Variable(*s);
                    }
                }

                ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg));
                int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
                switch (static_cast<LCompilers::ASRUtils::IntrinsicFunctions>(intrinsic_id)) {
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicPi: {
                        std::string new_name = "basic_const_pi";
                        symbolic_dependencies.push_back(new_name);
                        if (!module_scope->get_symbol(new_name)) {
                            std::string header = "symengine/cwrapper.h";
                            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                            Vec<ASR::expr_t*> args;
                            args.reserve(al, 1);
                            ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "x"), arg);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg)));

                            Vec<ASR::stmt_t*> body;
                            body.reserve(al, 1);

                            Vec<char*> dep;
                            dep.reserve(al, 1);

                            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                                false, false, nullptr, 0, false, false, false, s2c(al, header));
                            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                            module_scope->add_symbol(s2c(al, new_name), new_symbol);
                        }

                        // Create the function call statement for basic_const_pi
                        ASR::symbol_t* basic_const_pi_sym = module_scope->get_symbol(new_name);
                        Vec<ASR::call_arg_t> call_args;
                        call_args.reserve(al, 1);
                        ASR::call_arg_t call_arg;
                        call_arg.loc = x.base.base.loc;
                        call_arg.m_value = target;
                        call_args.push_back(al, call_arg);

                        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, basic_const_pi_sym,
                            basic_const_pi_sym, call_args.p, call_args.n, nullptr));
                        pass_result.push_back(al, stmt);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSymbol: {
                        std::string new_name = "symbol_set";
                        symbolic_dependencies.push_back(new_name);
                        if (!module_scope->get_symbol(new_name)) {
                            std::string header = "symengine/cwrapper.h";
                            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                            Vec<ASR::expr_t*> args;
                            args.reserve(al, 1);
                            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "x"), arg1);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg1)));
                            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                                al, x.base.base.loc, fn_symtab, s2c(al, "s"), nullptr, 0, ASR::intentType::In,
                                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)),
                                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                            fn_symtab->add_symbol(s2c(al, "s"), arg2);
                            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

                            Vec<ASR::stmt_t*> body;
                            body.reserve(al, 1);

                            Vec<char*> dep;
                            dep.reserve(al, 1);

                            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                                false, false, nullptr, 0, false, false, false, s2c(al, header));
                            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                            module_scope->add_symbol(s2c(al, new_name), new_symbol);
                        }

                        ASR::symbol_t* symbol_set_sym = module_scope->get_symbol(new_name);
                        Vec<ASR::call_arg_t> call_args;
                        call_args.reserve(al, 2);
                        ASR::call_arg_t call_arg1, call_arg2;
                        call_arg1.loc = x.base.base.loc;
                        call_arg1.m_value = target;
                        call_arg2.loc = x.base.base.loc;
                        call_arg2.m_value = intrinsic_func->m_args[0];
                        call_args.push_back(al, call_arg1);
                        call_args.push_back(al, call_arg2);

                        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, symbol_set_sym,
                            symbol_set_sym, call_args.p, call_args.n, nullptr));
                        pass_result.push_back(al, stmt);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicAdd: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_add",
                            target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSub: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_sub",
                            target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicMul: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_mul",
                            target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicDiv: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_div",
                            target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicPow: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_pow",
                            target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicDiff: {
                        ASR::expr_t* value1 = intrinsic_func->m_args[0];
                        ASR::expr_t* value2 = intrinsic_func->m_args[1];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                            value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                        }

                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                            value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                        }
                        perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_diff",
                            target, value1, value2);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSin: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_sin",
                            target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicCos: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_cos",
                            target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicLog: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_log",
                            target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicExp: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_exp",
                            target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicAbs: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_abs",
                            target, value);
                        break;
                    }
                    case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicExpand: {
                        ASR::expr_t* value = intrinsic_func->m_args[0];
                        if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                            ASR::IntrinsicFunction_t* s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                            this->visit_IntrinsicFunction(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                            ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                            this->visit_Cast(*s);
                            ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                            value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                        }
                        perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_expand",
                            target, value);
                        break;
                    }
                    default: {
                        throw LCompilersException("IntrinsicFunction: `"
                            + ASRUtils::get_intrinsic_name(intrinsic_id)
                            + "` is not implemented");
                    }
                }
                std::string new_name = "basic_str";
                symbolic_dependencies.push_back(new_name);
                if (!module_scope->get_symbol(new_name)) {
                    std::string header = "symengine/cwrapper.h";
                    SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 1);
                    ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "_lpython_return_variable"), nullptr, 0, ASR::intentType::ReturnVar,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
                    fn_symtab->add_symbol(s2c(al, "_lpython_return_variable"), arg1);
                    ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg2);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

                    Vec<ASR::stmt_t*> body;
                    body.reserve(al, 1);

                    Vec<char*> dep;
                    dep.reserve(al, 1);

                    ASR::expr_t* return_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, fn_symtab->get_symbol("_lpython_return_variable")));
                    ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                        fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                        return_var, ASR::abiType::BindC, ASR::accessType::Public,
                        ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                        false, false, nullptr, 0, false, false, false, s2c(al, header));
                    ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                    module_scope->add_symbol(s2c(al, new_name), new_symbol);
                }
                // Now create the FunctionCall node for basic_str
                ASR::symbol_t* basic_str_sym = module_scope->get_symbol(new_name);
                Vec<ASR::call_arg_t> call_args;
                call_args.reserve(al, 1);
                ASR::call_arg_t call_arg;
                call_arg.loc = x.base.base.loc;
                call_arg.m_value = target;
                call_args.push_back(al, call_arg);
                ASR::expr_t* function_call = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x.base.base.loc,
                    basic_str_sym, basic_str_sym, call_args.p, call_args.n,
                    ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)), nullptr, nullptr));
                print_tmp.push_back(function_call);
            } else {
                print_tmp.push_back(x.m_values[i]);
            }
        }
        if (!print_tmp.empty()) {
            Vec<ASR::expr_t*> tmp_vec;
            tmp_vec.reserve(al, print_tmp.size());
            for (auto &e: print_tmp) {
                tmp_vec.push_back(al, e);
            }
            ASR::stmt_t *print_stmt = ASRUtils::STMT(
                ASR::make_Print_t(al, x.base.base.loc, nullptr, tmp_vec.p, tmp_vec.size(),
                            x.m_separator, x.m_end));
            print_tmp.clear();
            pass_result.push_back(al, print_stmt);
        }
    }

    void visit_IntrinsicFunction(const ASR::IntrinsicFunction_t &x) {
        if(x.m_type->type != ASR::ttypeType::SymbolicExpression) return;
        SymbolTable* module_scope = current_scope->parent;

        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, x.base.base.loc));
        std::string symengine_var = symengine_stack.push();
        ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
            al, x.base.base.loc, current_scope, s2c(al, symengine_var), nullptr, 0, ASR::intentType::Local,
            nullptr, nullptr, ASR::storage_typeType::Default, type, nullptr,
            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
        current_scope->add_symbol(s2c(al, symengine_var), arg);
        for (auto &item : current_scope->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *s = ASR::down_cast<ASR::Variable_t>(item.second);
                this->visit_Variable(*s);
            }
        }

        int64_t intrinsic_id = x.m_intrinsic_id;
        ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg));
        switch (static_cast<LCompilers::ASRUtils::IntrinsicFunctions>(intrinsic_id)) {
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicPi: {
                std::string new_name = "basic_const_pi";
                symbolic_dependencies.push_back(new_name);
                if (!module_scope->get_symbol(new_name)) {
                    std::string header = "symengine/cwrapper.h";
                    SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 1);
                    ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg)));

                    Vec<ASR::stmt_t*> body;
                    body.reserve(al, 1);

                    Vec<char*> dep;
                    dep.reserve(al, 1);

                    ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                        fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                        nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                        ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                        false, false, nullptr, 0, false, false, false, s2c(al, header));
                    ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                    module_scope->add_symbol(s2c(al, new_name), new_symbol);
                }

                // Create the function call statement for basic_const_pi
                ASR::symbol_t* basic_const_pi_sym = module_scope->get_symbol(new_name);
                Vec<ASR::call_arg_t> call_args;
                call_args.reserve(al, 1);
                ASR::call_arg_t call_arg;
                call_arg.loc = x.base.base.loc;
                call_arg.m_value = target;
                call_args.push_back(al, call_arg);

                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, basic_const_pi_sym,
                    basic_const_pi_sym, call_args.p, call_args.n, nullptr));
                pass_result.push_back(al, stmt);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSymbol: {
                std::string new_name = "symbol_set";
                symbolic_dependencies.push_back(new_name);
                if (!module_scope->get_symbol(new_name)) {
                    std::string header = "symengine/cwrapper.h";
                    SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 1);
                    ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg1);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg1)));
                    ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "s"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "s"), arg2);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

                    Vec<ASR::stmt_t*> body;
                    body.reserve(al, 1);

                    Vec<char*> dep;
                    dep.reserve(al, 1);

                    ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                        fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                        nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                        ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                        false, false, nullptr, 0, false, false, false, s2c(al, header));
                    ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                    module_scope->add_symbol(s2c(al, new_name), new_symbol);
                }

                ASR::symbol_t* symbol_set_sym = module_scope->get_symbol(new_name);
                Vec<ASR::call_arg_t> call_args;
                call_args.reserve(al, 2);
                ASR::call_arg_t call_arg1, call_arg2;
                call_arg1.loc = x.base.base.loc;
                call_arg1.m_value = target;
                call_arg2.loc = x.base.base.loc;
                call_arg2.m_value = x.m_args[0];
                call_args.push_back(al, call_arg1);
                call_args.push_back(al, call_arg2);

                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, symbol_set_sym,
                    symbol_set_sym, call_args.p, call_args.n, nullptr));
                pass_result.push_back(al, stmt);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicAdd: {
                ASR::expr_t* value1 = x.m_args[0];
                ASR::expr_t* value2 = x.m_args[1];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                }

                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                }
                perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_add",
                    target, value1, value2);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSub: {
                ASR::expr_t* value1 = x.m_args[0];
                ASR::expr_t* value2 = x.m_args[1];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                }

                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                }
                perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_sub",
                    target, value1, value2);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicMul: {
                ASR::expr_t* value1 = x.m_args[0];
                ASR::expr_t* value2 = x.m_args[1];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                }

                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                }
                perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_mul",
                    target, value1, value2);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicDiv: {
                ASR::expr_t* value1 = x.m_args[0];
                ASR::expr_t* value2 = x.m_args[1];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                }

                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                }
                perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_div",
                    target, value1, value2);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicPow: {
                ASR::expr_t* value1 = x.m_args[0];
                ASR::expr_t* value2 = x.m_args[1];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                }

                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                }
                perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_pow",
                    target, value1, value2);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicDiff: {
                ASR::expr_t* value1 = x.m_args[0];
                ASR::expr_t* value2 = x.m_args[1];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value1)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value1);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                } else if (ASR::is_a<ASR::Cast_t>(*value1)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value1);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
                    value1 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));
                }

                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value2)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value2);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                } else if (ASR::is_a<ASR::Cast_t>(*value2)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value2);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym2 = current_scope->get_symbol(symengine_stack.pop());
                    value2 = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym2));
                }
                perform_symbolic_binary_operation(al, x.base.base.loc, module_scope, "basic_diff",
                    target, value1, value2);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicSin: {
                ASR::expr_t* value = x.m_args[0];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                }
                perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_sin",
                    target, value);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicCos: {
                ASR::expr_t* value = x.m_args[0];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                }
                perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_cos",
                    target, value);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicLog: {
                ASR::expr_t* value = x.m_args[0];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                }
                perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_log",
                    target, value);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicExp: {
                ASR::expr_t* value = x.m_args[0];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                }
                perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_exp",
                    target, value);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicAbs: {
                ASR::expr_t* value = x.m_args[0];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                }
                perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_abs",
                    target, value);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicFunctions::SymbolicExpand: {
                ASR::expr_t* value = x.m_args[0];
                if (ASR::is_a<ASR::IntrinsicFunction_t>(*value)) {
                    ASR::IntrinsicFunction_t *s = ASR::down_cast<ASR::IntrinsicFunction_t>(value);
                    this->visit_IntrinsicFunction(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                } else if (ASR::is_a<ASR::Cast_t>(*value)) {
                    ASR::Cast_t* s = ASR::down_cast<ASR::Cast_t>(value);
                    this->visit_Cast(*s);
                    ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
                    value = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));
                }
                perform_symbolic_unary_operation(al, x.base.base.loc, module_scope, "basic_expand",
                    target, value);
                break;
            }
            default: {
                throw LCompilersException("IntrinsicFunction: `"
                    + ASRUtils::get_intrinsic_name(intrinsic_id)
                    + "` is not implemented");
            }
        }
    }

    void visit_Cast(const ASR::Cast_t &x) {
        if(x.m_kind != ASR::cast_kindType::IntegerToSymbolicExpression) return;
        SymbolTable* module_scope = current_scope->parent;

        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, x.base.base.loc));
        std::string symengine_var = symengine_stack.push();
        ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
            al, x.base.base.loc, current_scope, s2c(al, symengine_var), nullptr, 0, ASR::intentType::Local,
            nullptr, nullptr, ASR::storage_typeType::Default, type, nullptr,
            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
        current_scope->add_symbol(s2c(al, symengine_var), arg);
        for (auto &item : current_scope->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *s = ASR::down_cast<ASR::Variable_t>(item.second);
                this->visit_Variable(*s);
            }
        }

        ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg));
        ASR::expr_t* cast_arg = x.m_arg;
        ASR::expr_t* cast_value = x.m_value;
        if (ASR::is_a<ASR::IntrinsicFunction_t>(*cast_value)) {
            ASR::IntrinsicFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicFunction_t>(cast_value);
            int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
            if (static_cast<LCompilers::ASRUtils::IntrinsicFunctions>(intrinsic_id) ==
                LCompilers::ASRUtils::IntrinsicFunctions::SymbolicInteger) {
                int const_value = 0;
                if (ASR::is_a<ASR::IntegerConstant_t>(*cast_arg)){
                    ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(cast_arg);
                    const_value = const_int->m_n;
                }
                if (ASR::is_a<ASR::IntegerUnaryMinus_t>(*cast_arg)){
                    ASR::IntegerUnaryMinus_t *const_int_minus = ASR::down_cast<ASR::IntegerUnaryMinus_t>(cast_arg);
                    ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(const_int_minus->m_value);
                    const_value = const_int->m_n;
                }
                std::string new_name = "integer_set_si";
                symbolic_dependencies.push_back(new_name);
                if (!module_scope->get_symbol(new_name)) {
                    std::string header = "symengine/cwrapper.h";
                    SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 2);
                    ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg1);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg1)));
                    ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "y"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "y"), arg2);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

                    Vec<ASR::stmt_t*> body;
                    body.reserve(al, 1);

                    Vec<char*> dep;
                    dep.reserve(al, 1);

                    ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                        fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                        nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                        ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                        false, false, nullptr, 0, false, false, false, s2c(al, header));
                    ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                    module_scope->add_symbol(s2c(al, new_name), new_symbol);
                }

                ASR::symbol_t* integer_set_sym = module_scope->get_symbol(new_name);
                ASR::ttype_t* cast_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                ASR::expr_t* value = ASRUtils::EXPR(ASR::make_Cast_t(al, x.base.base.loc, cast_arg,
                    (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, cast_type,
                    ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, const_value, cast_type))));
                Vec<ASR::call_arg_t> call_args;
                call_args.reserve(al, 2);
                ASR::call_arg_t call_arg1, call_arg2;
                call_arg1.loc = x.base.base.loc;
                call_arg1.m_value = target;
                call_arg2.loc = x.base.base.loc;
                call_arg2.m_value = value;
                call_args.push_back(al, call_arg1);
                call_args.push_back(al, call_arg2);

                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, integer_set_sym,
                    integer_set_sym, call_args.p, call_args.n, nullptr));
                pass_result.push_back(al, stmt);
            }
        }
    }

    void visit_Assert(const ASR::Assert_t &x) {
        if (!ASR::is_a<ASR::SymbolicCompare_t>(*x.m_test)) return;
        ASR::SymbolicCompare_t *s = ASR::down_cast<ASR::SymbolicCompare_t>(x.m_test);
        SymbolTable* module_scope = current_scope->parent;
        ASR::expr_t* left_tmp;
        ASR::expr_t* right_tmp;

        std::string new_name = "basic_str";
        symbolic_dependencies.push_back(new_name);
        if (!module_scope->get_symbol(new_name)) {
            std::string header = "symengine/cwrapper.h";
            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

            Vec<ASR::expr_t*> args;
            args.reserve(al, 1);
            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, x.base.base.loc, fn_symtab, s2c(al, "_lpython_return_variable"), nullptr, 0, ASR::intentType::ReturnVar,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
            fn_symtab->add_symbol(s2c(al, "_lpython_return_variable"), arg1);
            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "x"), arg2);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg2)));

            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);

            Vec<char*> dep;
            dep.reserve(al, 1);

            ASR::expr_t* return_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, fn_symtab->get_symbol("_lpython_return_variable")));
            ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                return_var, ASR::abiType::BindC, ASR::accessType::Public,
                ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                false, false, nullptr, 0, false, false, false, s2c(al, header));
            ASR::symbol_t* new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
            module_scope->add_symbol(s2c(al, new_name), new_symbol);
        }
        ASR::symbol_t* basic_str_sym = module_scope->get_symbol(new_name);

        if(ASR::is_a<ASR::Var_t>(*s->m_left)) {
            ASR::symbol_t *var_sym = ASR::down_cast<ASR::Var_t>(s->m_left)->m_v;
            ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));

            // Now create the FunctionCall node for basic_str
            Vec<ASR::call_arg_t> call_args1;
            call_args1.reserve(al, 1);
            ASR::call_arg_t call_arg1;
            call_arg1.loc = x.base.base.loc;
            call_arg1.m_value = target;
            call_args1.push_back(al, call_arg1);
            ASR::expr_t* function_call1 = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x.base.base.loc,
                basic_str_sym, basic_str_sym, call_args1.p, call_args1.n,
                ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)), nullptr, nullptr));
            left_tmp = function_call1;
        } else if(ASR::is_a<ASR::IntrinsicFunction_t>(*s->m_left)) {
            ASR::IntrinsicFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicFunction_t>(s->m_left);
            this->visit_IntrinsicFunction(*intrinsic_func);
            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
            ASR::expr_t* left_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));

            // Now create the FunctionCall node for basic_str
            Vec<ASR::call_arg_t> call_args1;
            call_args1.reserve(al, 1);
            ASR::call_arg_t call_arg1;
            call_arg1.loc = x.base.base.loc;
            call_arg1.m_value = left_var;
            call_args1.push_back(al, call_arg1);
            ASR::expr_t* function_call1 = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x.base.base.loc,
                basic_str_sym, basic_str_sym, call_args1.p, call_args1.n,
                ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)), nullptr, nullptr));
            left_tmp = function_call1;
        }

        if(ASR::is_a<ASR::IntrinsicFunction_t>(*s->m_right)) {
            ASR::IntrinsicFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicFunction_t>(s->m_right);
            this->visit_IntrinsicFunction(*intrinsic_func);
            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
            ASR::expr_t* right_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));

            // Now create the FunctionCall node for basic_str
            Vec<ASR::call_arg_t> call_args2;
            call_args2.reserve(al, 1);
            ASR::call_arg_t call_arg2;
            call_arg2.loc = x.base.base.loc;
            call_arg2.m_value = right_var;
            call_args2.push_back(al, call_arg2);
            ASR::expr_t* function_call2 = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x.base.base.loc,
                basic_str_sym, basic_str_sym, call_args2.p, call_args2.n,
                ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)), nullptr, nullptr));
            right_tmp = function_call2;
        } else if (ASR::is_a<ASR::Cast_t>(*s->m_right)) {
            ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(s->m_right);
            this->visit_Cast(*cast_t);
            ASR::symbol_t* var_sym1 = current_scope->get_symbol(symengine_stack.pop());
            ASR::expr_t* right_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym1));

            // Now create the FunctionCall node for basic_str
            Vec<ASR::call_arg_t> call_args2;
            call_args2.reserve(al, 1);
            ASR::call_arg_t call_arg2;
            call_arg2.loc = x.base.base.loc;
            call_arg2.m_value = right_var;
            call_args2.push_back(al, call_arg2);
            ASR::expr_t* function_call2 = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x.base.base.loc,
                basic_str_sym, basic_str_sym, call_args2.p, call_args2.n,
                ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, -2, nullptr)), nullptr, nullptr));
            right_tmp = function_call2;
        }
        ASR::expr_t* test =  ASRUtils::EXPR(ASR::make_StringCompare_t(al, x.base.base.loc, left_tmp,
            s->m_op, right_tmp, s->m_type, s->m_value));

        ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, test, x.m_msg));
        pass_result.push_back(al, assert_stmt);
    }
};

void pass_replace_symbolic(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceSymbolicVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LCompilers