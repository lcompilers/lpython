#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_symbolic.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>

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
    std::set<ASR::symbol_t*> symbolic_vars_to_free;
    std::set<ASR::symbol_t*> symbolic_vars_to_omit;
    SymEngine_Stack symengine_stack;

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = this->current_scope;
        this->current_scope = xx.m_symtab;
        SymbolTable* module_scope = this->current_scope->parent;

        ASR::ttype_t* f_signature= xx.m_function_signature;
        ASR::FunctionType_t *f_type = ASR::down_cast<ASR::FunctionType_t>(f_signature);
        ASR::ttype_t *type1 = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
        for (size_t i = 0; i < f_type->n_arg_types; ++i) {
            if (f_type->m_arg_types[i]->type == ASR::ttypeType::SymbolicExpression) {
                f_type->m_arg_types[i] = type1;
            }
        }

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

        // freeing out variables
        std::string new_name = "basic_free_stack";
        ASR::symbol_t* basic_free_stack_sym = module_scope->get_symbol(new_name);
        Vec<ASR::stmt_t*> func_body;
        func_body.from_pointer_n_copy(al, xx.m_body, xx.n_body);

        for (ASR::symbol_t* symbol : symbolic_vars_to_free) {
            if (symbolic_vars_to_omit.find(symbol) != symbolic_vars_to_omit.end()) continue;
            Vec<ASR::call_arg_t> call_args;
            call_args.reserve(al, 1);
            ASR::call_arg_t call_arg;
            call_arg.loc = xx.base.base.loc;
            call_arg.m_value = ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, symbol));
            call_args.push_back(al, call_arg);
            ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, xx.base.base.loc, basic_free_stack_sym,
                basic_free_stack_sym, call_args.p, call_args.n, nullptr));
            func_body.push_back(al, stmt);
        }

        xx.n_body = func_body.size();
        xx.m_body = func_body.p;
        symbolic_vars_to_free.clear();
    }

    void visit_Variable(const ASR::Variable_t& x) {
        ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
        if (xx.m_type->type == ASR::ttypeType::SymbolicExpression) {
            SymbolTable* module_scope = current_scope->parent;
            std::string var_name = xx.m_name;
            std::string placeholder = "_" + std::string(var_name);

            ASR::ttype_t *type1 = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
            xx.m_type = type1;
            symbolic_vars_to_free.insert(ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&xx));
            if(xx.m_intent == ASR::intentType::In){
                symbolic_vars_to_omit.insert(ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&xx));
            }

            if(xx.m_intent == ASR::intentType::Local){
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

                new_name = "basic_free_stack";
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
                ASR::symbol_t* basic_new_stack_sym = module_scope->get_symbol("basic_new_stack");
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

    ASR::expr_t* handle_argument(Allocator &al, const Location &loc, ASR::expr_t* arg) {
        if (ASR::is_a<ASR::Var_t>(*arg)) {
            return arg;
        } else if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*arg)) {
            this->visit_IntrinsicFunction(*ASR::down_cast<ASR::IntrinsicScalarFunction_t>(arg));
        } else if (ASR::is_a<ASR::Cast_t>(*arg)) {
            this->visit_Cast(*ASR::down_cast<ASR::Cast_t>(arg));
        } else {
            LCOMPILERS_ASSERT(false);
        }
        ASR::symbol_t* var_sym = current_scope->get_symbol(symengine_stack.pop());
        return ASRUtils::EXPR(ASR::make_Var_t(al, loc, var_sym));
    }

    void process_binary_operator(Allocator &al,  const Location &loc, ASR::IntrinsicScalarFunction_t* x, SymbolTable* module_scope,
        const std::string& new_name, ASR::expr_t* target) {
            ASR::expr_t* value1 = handle_argument(al, loc, x->m_args[0]);
            ASR::expr_t* value2 = handle_argument(al, loc, x->m_args[1]);
            perform_symbolic_binary_operation(al, loc, module_scope, new_name, target, value1, value2);
    }

    void process_unary_operator(Allocator &al,  const Location &loc, ASR::IntrinsicScalarFunction_t* x, SymbolTable* module_scope,
        const std::string& new_name, ASR::expr_t* target) {
            ASR::expr_t* value1 = handle_argument(al, loc, x->m_args[0]);
            perform_symbolic_unary_operation(al, loc, module_scope, new_name, target, value1);
    }

    void process_intrinsic_function(Allocator &al,  const Location &loc, ASR::IntrinsicScalarFunction_t* x, SymbolTable* module_scope,
        ASR::expr_t* target){
        int64_t intrinsic_id = x->m_intrinsic_id;
        switch (static_cast<LCompilers::ASRUtils::IntrinsicScalarFunctions>(intrinsic_id)) {
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicPi: {
                std::string new_name = "basic_const_pi";
                symbolic_dependencies.push_back(new_name);
                if (!module_scope->get_symbol(new_name)) {
                    std::string header = "symengine/cwrapper.h";
                    SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 1);
                    ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg)));

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

                // Create the function call statement for basic_const_pi
                ASR::symbol_t* basic_const_pi_sym = module_scope->get_symbol(new_name);
                Vec<ASR::call_arg_t> call_args;
                call_args.reserve(al, 1);
                ASR::call_arg_t call_arg;
                call_arg.loc = loc;
                call_arg.m_value = target;
                call_args.push_back(al, call_arg);

                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, loc, basic_const_pi_sym,
                    basic_const_pi_sym, call_args.p, call_args.n, nullptr));
                pass_result.push_back(al, stmt);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicSymbol: {
                std::string new_name = "symbol_set";
                symbolic_dependencies.push_back(new_name);
                if (!module_scope->get_symbol(new_name)) {
                    std::string header = "symengine/cwrapper.h";
                    SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 1);
                    ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg1);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg1)));
                    ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, loc, fn_symtab, s2c(al, "s"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr)),
                        nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "s"), arg2);
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

                ASR::symbol_t* symbol_set_sym = module_scope->get_symbol(new_name);
                Vec<ASR::call_arg_t> call_args;
                call_args.reserve(al, 2);
                ASR::call_arg_t call_arg1, call_arg2;
                call_arg1.loc = loc;
                call_arg1.m_value = target;
                call_arg2.loc = loc;
                call_arg2.m_value = x->m_args[0];
                call_args.push_back(al, call_arg1);
                call_args.push_back(al, call_arg2);

                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, loc, symbol_set_sym,
                    symbol_set_sym, call_args.p, call_args.n, nullptr));
                pass_result.push_back(al, stmt);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicAdd: {
                process_binary_operator(al, loc, x, module_scope, "basic_add", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicSub: {
                process_binary_operator(al, loc, x, module_scope, "basic_sub", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicMul: {
                process_binary_operator(al, loc, x, module_scope, "basic_mul", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicDiv: {
                process_binary_operator(al, loc, x, module_scope, "basic_div", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicPow: {
                process_binary_operator(al, loc, x, module_scope, "basic_pow", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicDiff: {
                process_binary_operator(al, loc, x, module_scope, "basic_diff", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicSin: {
                process_unary_operator(al, loc, x, module_scope, "basic_sin", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicCos: {
                process_unary_operator(al, loc, x, module_scope, "basic_cos", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicLog: {
                process_unary_operator(al, loc, x, module_scope, "basic_log", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicExp: {
                process_unary_operator(al, loc, x, module_scope, "basic_exp", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicAbs: {
                process_unary_operator(al, loc, x, module_scope, "basic_abs", target);
                break;
            }
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicExpand: {
                process_unary_operator(al, loc, x, module_scope, "basic_expand", target);
                break;
            }
            default: {
                throw LCompilersException("IntrinsicFunction: `"
                    + ASRUtils::get_intrinsic_name(intrinsic_id)
                    + "` is not implemented");
            }
        }
    }

    ASR::symbol_t* declare_basic_str_function(Allocator& al, const Location& loc, SymbolTable* module_scope) {
        std::string name = "basic_str";
        symbolic_dependencies.push_back(name);
        if (!module_scope->get_symbol(name)) {
            std::string header = "symengine/cwrapper.h";
            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

            Vec<ASR::expr_t*> args;
            args.reserve(al, 1);
            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "_lpython_return_variable"), nullptr, 0, ASR::intentType::ReturnVar,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
            fn_symtab->add_symbol(s2c(al, "_lpython_return_variable"), arg1);
            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "x"), arg2);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg2)));

            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);

            Vec<char*> dep;
            dep.reserve(al, 1);

            ASR::expr_t* return_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, fn_symtab->get_symbol("_lpython_return_variable")));
            ASR::asr_t* subrout = ASRUtils::make_Function_t_util(al, loc,
                fn_symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                return_var, ASR::abiType::BindC, ASR::accessType::Public,
                ASR::deftypeType::Interface, s2c(al, name), false, false, false,
                false, false, nullptr, 0, false, false, false, s2c(al, header));
            ASR::symbol_t* symbol = ASR::down_cast<ASR::symbol_t>(subrout);
            module_scope->add_symbol(s2c(al, name), symbol);
        }
        return module_scope->get_symbol(name);
    }

    ASR::symbol_t* declare_integer_set_si_function(Allocator& al, const Location& loc, SymbolTable* module_scope) {
        std::string name = "integer_set_si";
        symbolic_dependencies.push_back(name);
        if (!module_scope->get_symbol(name)) {
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
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "y"), arg2);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg2)));

            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);

            Vec<char*> dep;
            dep.reserve(al, 1);

            ASR::asr_t* subrout = ASRUtils::make_Function_t_util(al, loc,
                fn_symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                ASR::deftypeType::Interface, s2c(al, name), false, false, false,
                false, false, nullptr, 0, false, false, false, s2c(al, header));
            ASR::symbol_t* symbol = ASR::down_cast<ASR::symbol_t>(subrout);
            module_scope->add_symbol(s2c(al, name), symbol);
        }
        return module_scope->get_symbol(name);
    }

    ASR::symbol_t* declare_basic_get_type_function(Allocator& al, const Location& loc, SymbolTable* module_scope) {
        std::string name = "basic_get_type";
        symbolic_dependencies.push_back(name);
        if (!module_scope->get_symbol(name)) {
            std::string header = "symengine/cwrapper.h";
            SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

            Vec<ASR::expr_t*> args;
            args.reserve(al, 1);
            ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "_lpython_return_variable"), nullptr, 0, ASR::intentType::ReturnVar,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
            fn_symtab->add_symbol(s2c(al, "_lpython_return_variable"), arg1);
            ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab->add_symbol(s2c(al, "x"), arg2);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg2)));

            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);

            Vec<char*> dep;
            dep.reserve(al, 1);

            ASR::expr_t* return_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, fn_symtab->get_symbol("_lpython_return_variable")));
            ASR::asr_t* subrout = ASRUtils::make_Function_t_util(al, loc,
                fn_symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                return_var, ASR::abiType::BindC, ASR::accessType::Public,
                ASR::deftypeType::Interface, s2c(al, name), false, false, false,
                false, false, nullptr, 0, false, false, false, s2c(al, header));
            ASR::symbol_t* symbol = ASR::down_cast<ASR::symbol_t>(subrout);
            module_scope->add_symbol(s2c(al, name), symbol);
        }
        return module_scope->get_symbol(name);
    }

    ASR::expr_t* process_attributes(Allocator &al, const Location &loc, ASR::expr_t* expr,
        SymbolTable* module_scope) {
        if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*expr)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(expr);
            int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
            switch (static_cast<LCompilers::ASRUtils::IntrinsicScalarFunctions>(intrinsic_id)) {
                case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicHasSymbolQ: {
                    std::string name = "basic_has_symbol";
                    symbolic_dependencies.push_back(name);
                    if (!module_scope->get_symbol(name)) {
                        std::string header = "symengine/cwrapper.h";
                        SymbolTable* fn_symtab = al.make_new<SymbolTable>(module_scope);

                        Vec<ASR::expr_t*> args;
                        args.reserve(al, 1);
                        ASR::symbol_t* arg1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                            al, loc, fn_symtab, s2c(al, "_lpython_return_variable"), nullptr, 0, ASR::intentType::ReturnVar,
                            nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)),
                            nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
                        fn_symtab->add_symbol(s2c(al, "_lpython_return_variable"), arg1);
                        ASR::symbol_t* arg2 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                            al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                            nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                            nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                        fn_symtab->add_symbol(s2c(al, "x"), arg2);
                        args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg2)));
                        ASR::symbol_t* arg3 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                            al, loc, fn_symtab, s2c(al, "y"), nullptr, 0, ASR::intentType::In,
                            nullptr, nullptr, ASR::storage_typeType::Default, ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
                            nullptr, ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                        fn_symtab->add_symbol(s2c(al, "y"), arg3);
                        args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg3)));

                        Vec<ASR::stmt_t*> body;
                        body.reserve(al, 1);

                        Vec<char*> dep;
                        dep.reserve(al, 1);

                        ASR::expr_t* return_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, fn_symtab->get_symbol("_lpython_return_variable")));
                        ASR::asr_t* subrout = ASRUtils::make_Function_t_util(al, loc,
                            fn_symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                            return_var, ASR::abiType::BindC, ASR::accessType::Public,
                            ASR::deftypeType::Interface, s2c(al, name), false, false, false,
                            false, false, nullptr, 0, false, false, false, s2c(al, header));
                        ASR::symbol_t* symbol = ASR::down_cast<ASR::symbol_t>(subrout);
                        module_scope->add_symbol(s2c(al, name), symbol);
                    }

                    ASR::symbol_t* basic_has_symbol = module_scope->get_symbol(name);
                    ASR::expr_t* value1 = handle_argument(al, loc, intrinsic_func->m_args[0]);
                    ASR::expr_t* value2 = handle_argument(al, loc, intrinsic_func->m_args[1]);
                    Vec<ASR::call_arg_t> call_args;
                    call_args.reserve(al, 1);
                    ASR::call_arg_t call_arg1, call_arg2;
                    call_arg1.loc = loc;
                    call_arg1.m_value = value1;
                    call_args.push_back(al, call_arg1);
                    call_arg2.loc = loc;
                    call_arg2.m_value = value2;
                    call_args.push_back(al, call_arg2);
                    return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                        basic_has_symbol, basic_has_symbol, call_args.p, call_args.n,
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr, nullptr));
                    break;
                }
                case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicFuncQ: {
                    ASR::symbol_t* basic_get_type_sym = declare_basic_get_type_function(al, loc, module_scope);
                    ASR::expr_t* value1 = handle_argument(al, loc, intrinsic_func->m_args[0]);
                    Vec<ASR::call_arg_t> call_args;
                    call_args.reserve(al, 1);
                    ASR::call_arg_t call_arg;
                    call_arg.loc = loc;
                    call_arg.m_value = value1;
                    call_args.push_back(al, call_arg);
                    ASR::expr_t* function_call = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                        basic_get_type_sym, basic_get_type_sym, call_args.p, call_args.n,
                        ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)), nullptr, nullptr));

                    // Declare a temporary integer variable
                    ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
                    ASR::symbol_t* int_sym = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(al, loc, current_scope,
                                        s2c(al, "temp_integer"), nullptr, 0,
                                        ASR::intentType::Local, nullptr,
                                        nullptr, ASR::storage_typeType::Default, int_type, nullptr,
                                        ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false));

                    if (!current_scope->get_symbol(s2c(al, "temp_integer"))) {
                        current_scope->add_symbol(s2c(al, "temp_integer"), int_sym);
                    }
                    ASR::symbol_t* temp_int_sym = current_scope->get_symbol("temp_integer");
                    ASR::expr_t* target_int = ASRUtils::EXPR(ASR::make_Var_t(al, loc, temp_int_sym));
                    ASR::stmt_t* stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target_int, function_call, nullptr));
                    pass_result.push_back(al, stmt1);

                    // Declare a temporary string variable
                    ASR::ttype_t* char_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr));
                    ASR::symbol_t* str_sym = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(al, loc, current_scope,
                                        s2c(al, "temp_string"), nullptr, 0,
                                        ASR::intentType::Local, nullptr,
                                        nullptr, ASR::storage_typeType::Default, char_type, nullptr,
                                        ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false));

                    if (!current_scope->get_symbol(s2c(al, "temp_string"))) {
                        current_scope->add_symbol(s2c(al, "temp_string"), str_sym);
                    }
                    ASR::symbol_t* temp_str_sym = current_scope->get_symbol("temp_string");
                    ASR::expr_t* target_str = ASRUtils::EXPR(ASR::make_Var_t(al, loc, temp_str_sym));
                    ASR::stmt_t* stmt2 = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target_str,
                        ASRUtils::EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, ""),
                        ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, 0, nullptr)))), nullptr));
                    pass_result.push_back(al, stmt2);

                    // If statement 1
                    // Using 17 as the right value of the IntegerCompare node as it represents SYMENGINE_POW through SYMENGINE_ENUM
                    ASR::expr_t* int_cmp_with_17 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc, target_int, ASR::cmpopType::Eq,
                        ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 17, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)))),
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr));
                    ASR::ttype_t *str_type_len_3 = ASRUtils::TYPE(ASR::make_Character_t(
                        al, loc, 1, 3, nullptr));
                    Vec<ASR::stmt_t*> if_body1;
                    if_body1.reserve(al, 1);
                    ASR::stmt_t* stmt3 = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target_str,
                        ASRUtils::EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, "Pow"), str_type_len_3)), nullptr));
                    if_body1.push_back(al, stmt3);
                    ASR::stmt_t* stmt4 = ASRUtils::STMT(ASR::make_If_t(al, loc, int_cmp_with_17, if_body1.p, if_body1.n, nullptr, 0));
                    pass_result.push_back(al, stmt4);

                    // If statement 2
                    // Using 15 as the right value of the IntegerCompare node as it represents SYMENGINE_MUL through SYMENGINE_ENUM
                    ASR::expr_t* int_cmp_with_15 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc, target_int, ASR::cmpopType::Eq,
                        ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 15, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)))),
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr));
                    Vec<ASR::stmt_t*> if_body2;
                    if_body2.reserve(al, 1);
                    ASR::stmt_t* stmt5 = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target_str,
                        ASRUtils::EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, "MUL"), str_type_len_3)), nullptr));
                    if_body2.push_back(al, stmt5);
                    ASR::stmt_t* stmt6 = ASRUtils::STMT(ASR::make_If_t(al, loc, int_cmp_with_15, if_body2.p, if_body2.n, nullptr, 0));
                    pass_result.push_back(al, stmt6);

                    // If statement 3
                    // Using 16 as the right value of the IntegerCompare node as it represents SYMENGINE_ADD through SYMENGINE_ENUM
                    ASR::expr_t* int_cmp_with_16 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc, target_int, ASR::cmpopType::Eq,
                        ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 16, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)))),
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr));
                    Vec<ASR::stmt_t*> if_body3;
                    if_body3.reserve(al, 1);
                    ASR::stmt_t* stmt7 = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target_str,
                        ASRUtils::EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, "Add"), str_type_len_3)), nullptr));
                    if_body3.push_back(al, stmt7);
                    ASR::stmt_t* stmt8 = ASRUtils::STMT(ASR::make_If_t(al, loc, int_cmp_with_16, if_body3.p, if_body3.n, nullptr, 0));
                    pass_result.push_back(al, stmt8);

                    return target_str;
                    break;
                }
                default: {
                    throw LCompilersException("IntrinsicFunction: `"
                        + ASRUtils::get_intrinsic_name(intrinsic_id)
                        + "` is not implemented");
                }
            }
        }
        return expr;
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        SymbolTable* module_scope = current_scope->parent;
        if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*x.m_value)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(x.m_value);
            if (intrinsic_func->m_type->type == ASR::ttypeType::SymbolicExpression) {
                process_intrinsic_function(al, x.base.base.loc, intrinsic_func, module_scope, x.m_target);
            } else if ((intrinsic_func->m_type->type == ASR::ttypeType::Logical) ||
                       (intrinsic_func->m_type->type == ASR::ttypeType::Character)) {
                ASR::expr_t* function_call = process_attributes(al, x.base.base.loc, x.m_value, module_scope);
                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, x.m_target, function_call, nullptr));
                pass_result.push_back(al, stmt);
            }
        } else if (ASR::is_a<ASR::Cast_t>(*x.m_value)) {
            ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(x.m_value);
            if (cast_t->m_kind == ASR::cast_kindType::IntegerToSymbolicExpression) {
                ASR::expr_t* cast_arg = cast_t->m_arg;
                ASR::expr_t* cast_value = cast_t->m_value;
                if (ASR::is_a<ASR::Var_t>(*cast_arg)) {
                    ASR::symbol_t* integer_set_sym = declare_integer_set_si_function(al, x.base.base.loc, module_scope);
                    ASR::ttype_t* cast_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                    ASR::expr_t* value = ASRUtils::EXPR(ASR::make_Cast_t(al, x.base.base.loc, cast_arg,
                        (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, cast_type, nullptr));
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
                } else if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*cast_value)) {
                    ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(cast_value);
                    int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
                    if (static_cast<LCompilers::ASRUtils::IntrinsicScalarFunctions>(intrinsic_id) ==
                        LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicInteger) {
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

                        ASR::symbol_t* integer_set_sym = declare_integer_set_si_function(al, x.base.base.loc, module_scope);
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

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        SymbolTable* module_scope = current_scope->parent;
        Vec<ASR::call_arg_t> call_args;
        call_args.reserve(al, 1);

        for (size_t i=0; i<x.n_args; i++) {
            ASR::expr_t* val = x.m_args[i].m_value;
            if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*val) && ASR::is_a<ASR::SymbolicExpression_t>(*ASRUtils::expr_type(val))) {
                ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(val);
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
                process_intrinsic_function(al, x.base.base.loc, intrinsic_func, module_scope, target);

                ASR::call_arg_t call_arg;
                call_arg.loc = x.base.base.loc;
                call_arg.m_value = target;
                call_args.push_back(al, call_arg);
            } else if (ASR::is_a<ASR::Cast_t>(*val)) {
                ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(val);
                if(cast_t->m_kind != ASR::cast_kindType::IntegerToSymbolicExpression) return;
                this->visit_Cast(*cast_t);
                ASR::symbol_t *var_sym = current_scope->get_symbol(symengine_stack.pop());
                ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));

                ASR::call_arg_t call_arg;
                call_arg.loc = x.base.base.loc;
                call_arg.m_value = target;
                call_args.push_back(al, call_arg);
            } else {
                call_args.push_back(al, x.m_args[i]);
            }
        }
        ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc, x.m_name,
            x.m_name, call_args.p, call_args.n, nullptr));
        pass_result.push_back(al, stmt);
    }

    void visit_Print(const ASR::Print_t &x) {
        std::vector<ASR::expr_t*> print_tmp;
        SymbolTable* module_scope = current_scope->parent;
        for (size_t i=0; i<x.n_values; i++) {
            ASR::expr_t* val = x.m_values[i];
            if (ASR::is_a<ASR::Var_t>(*val) && ASR::is_a<ASR::CPtr_t>(*ASRUtils::expr_type(val))) {
                ASR::symbol_t *v = ASR::down_cast<ASR::Var_t>(val)->m_v;
                if (symbolic_vars_to_free.find(v) == symbolic_vars_to_free.end()) return;
                ASR::symbol_t* basic_str_sym = declare_basic_str_function(al, x.base.base.loc, module_scope);

                // Extract the symbol from value (Var)
                ASR::symbol_t* var_sym = ASR::down_cast<ASR::Var_t>(val)->m_v;
                ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));

                // Now create the FunctionCall node for basic_str
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
            } else if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*val)) {
                ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(val);
                if (ASR::is_a<ASR::SymbolicExpression_t>(*ASRUtils::expr_type(val))) {
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
                    process_intrinsic_function(al, x.base.base.loc, intrinsic_func, module_scope, target);

                    // Now create the FunctionCall node for basic_str
                    ASR::symbol_t* basic_str_sym = declare_basic_str_function(al, x.base.base.loc, module_scope);
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
                } else if (ASR::is_a<ASR::Logical_t>(*ASRUtils::expr_type(val)) ||
                           ASR::is_a<ASR::Character_t>(*ASRUtils::expr_type(val))) {
                    ASR::expr_t* function_call = process_attributes(al, x.base.base.loc, val, module_scope);
                    print_tmp.push_back(function_call);
                }
            } else if (ASR::is_a<ASR::Cast_t>(*val)) {
                ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(val);
                if(cast_t->m_kind != ASR::cast_kindType::IntegerToSymbolicExpression) return;
                this->visit_Cast(*cast_t);
                ASR::symbol_t *var_sym = current_scope->get_symbol(symengine_stack.pop());
                ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));

                // Now create the FunctionCall node for basic_str
                ASR::symbol_t* basic_str_sym = declare_basic_str_function(al, x.base.base.loc, module_scope);
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

    void visit_IntrinsicFunction(const ASR::IntrinsicScalarFunction_t &x) {
        if(x.m_type && x.m_type->type == ASR::ttypeType::SymbolicExpression) {
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

            ASR::IntrinsicScalarFunction_t &xx = const_cast<ASR::IntrinsicScalarFunction_t&>(x);
            ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg));
            process_intrinsic_function(al, x.base.base.loc, &xx, module_scope, target);
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
        if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*cast_value)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(cast_value);
            int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
            if (static_cast<LCompilers::ASRUtils::IntrinsicScalarFunctions>(intrinsic_id) ==
                LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicInteger) {
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

                ASR::symbol_t* integer_set_sym = declare_integer_set_si_function(al, x.base.base.loc, module_scope);
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

    ASR::expr_t* process_with_basic_str(Allocator &al, const Location &loc, const ASR::expr_t* expr,
        ASR::symbol_t* basic_str_sym) {
            ASR::symbol_t *var_sym = nullptr;
            if (ASR::is_a<ASR::Var_t>(*expr)) {
                var_sym = ASR::down_cast<ASR::Var_t>(expr)->m_v;
            } else if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*expr)) {
                ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(expr);
                this->visit_IntrinsicFunction(*intrinsic_func);
                var_sym = current_scope->get_symbol(symengine_stack.pop());
            } else if (ASR::is_a<ASR::Cast_t>(*expr)) {
                ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(expr);
                this->visit_Cast(*cast_t);
                var_sym = current_scope->get_symbol(symengine_stack.pop());
            }

            ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, loc, var_sym));
            // Now create the FunctionCall node for basic_str
            Vec<ASR::call_arg_t> call_args;
            call_args.reserve(al, 1);
            ASR::call_arg_t call_arg;
            call_arg.loc = loc;
            call_arg.m_value = target;
            call_args.push_back(al, call_arg);
            ASR::expr_t* function_call = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                basic_str_sym, basic_str_sym, call_args.p, call_args.n,
                ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr)), nullptr, nullptr));
            return function_call;
    }

    void visit_Assert(const ASR::Assert_t &x) {
        SymbolTable* module_scope = current_scope->parent;
        ASR::expr_t* left_tmp = nullptr;
        ASR::expr_t* right_tmp = nullptr;
        if (ASR::is_a<ASR::LogicalCompare_t>(*x.m_test)) {
            ASR::LogicalCompare_t *l = ASR::down_cast<ASR::LogicalCompare_t>(x.m_test);

            left_tmp = process_attributes(al, x.base.base.loc, l->m_left, module_scope);
            right_tmp = process_attributes(al, x.base.base.loc, l->m_right, module_scope);
            ASR::expr_t* test =  ASRUtils::EXPR(ASR::make_LogicalCompare_t(al, x.base.base.loc, left_tmp,
                l->m_op, right_tmp, l->m_type, l->m_value));

            ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, test, x.m_msg));
            pass_result.push_back(al, assert_stmt);
        } else if (ASR::is_a<ASR::StringCompare_t>(*x.m_test)) {
            ASR::StringCompare_t *st = ASR::down_cast<ASR::StringCompare_t>(x.m_test);

            left_tmp = process_attributes(al, x.base.base.loc, st->m_left, module_scope);
            right_tmp = process_attributes(al, x.base.base.loc, st->m_right, module_scope);
            ASR::expr_t* test =  ASRUtils::EXPR(ASR::make_StringCompare_t(al, x.base.base.loc, left_tmp,
                st->m_op, right_tmp, st->m_type, st->m_value));

            ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, test, x.m_msg));
            pass_result.push_back(al, assert_stmt);
        } else if (ASR::is_a<ASR::SymbolicCompare_t>(*x.m_test)) {
            ASR::SymbolicCompare_t *s = ASR::down_cast<ASR::SymbolicCompare_t>(x.m_test);
            SymbolTable* module_scope = current_scope->parent;
            ASR::expr_t* left_tmp = nullptr;
            ASR::expr_t* right_tmp = nullptr;

            ASR::symbol_t* basic_str_sym = declare_basic_str_function(al, x.base.base.loc, module_scope);
            left_tmp = process_with_basic_str(al, x.base.base.loc, s->m_left, basic_str_sym);
            right_tmp = process_with_basic_str(al, x.base.base.loc, s->m_right, basic_str_sym);
            ASR::expr_t* test =  ASRUtils::EXPR(ASR::make_StringCompare_t(al, x.base.base.loc, left_tmp,
                s->m_op, right_tmp, s->m_type, s->m_value));

            ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, test, x.m_msg));
            pass_result.push_back(al, assert_stmt);
        }
    }
};

void pass_replace_symbolic(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceSymbolicVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LCompilers
