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

    /********************************** Utils *********************************/
    #define BASIC_CONST(SYM, name)                                              \
        case LCompilers::ASRUtils::IntrinsicScalarFunctions::Symbolic##SYM: {   \
            pass_result.push_back(al, basic_const(loc,                          \
                "basic_const_"#name, target));                                  \
            break; }

    #define BASIC_BINOP(SYM, name)                                              \
        case LCompilers::ASRUtils::IntrinsicScalarFunctions::Symbolic##SYM: {   \
            pass_result.push_back(al, basic_binop(loc, "basic_"#name, target,   \
                    x->m_args[0], x->m_args[1]));                               \
            break; }

    #define BASIC_UNARYOP(SYM, name)                                            \
        case LCompilers::ASRUtils::IntrinsicScalarFunctions::Symbolic##SYM: {   \
            pass_result.push_back(al, basic_unaryop(loc, "basic_"#name,         \
                    target, x->m_args[0]));                                     \
            break; }

    #define BASIC_ATTR(SYM, N)                                                  \
        case LCompilers::ASRUtils::IntrinsicScalarFunctions::Symbolic##SYM: {   \
            ASR::expr_t* function_call = basic_get_type(loc,                    \
                intrinsic_func->m_args[0]);                                     \
            return iEq(function_call, i32(N)); }

    ASR::stmt_t *SubroutineCall(const Location &loc, ASR::symbol_t *sym,
            std::vector<ASR::expr_t *> args) {
        Vec<ASR::call_arg_t> call_args; call_args.reserve(al, args.size());
        for (auto &x: args) {
            ASR::call_arg_t call_arg;
            call_arg.loc = loc;
            call_arg.m_value = x;
            call_args.push_back(al, call_arg);
        }
        return ASRUtils::STMT(ASR::make_SubroutineCall_t(al, loc, sym,
            sym, call_args.p, call_args.n, nullptr));
    }

    ASR::expr_t *FunctionCall(const Location &loc, ASR::symbol_t *sym,
            std::vector<ASR::expr_t *> args, ASR::ttype_t *return_type) {
        Vec<ASR::call_arg_t> call_args; call_args.reserve(al, args.size());
        for (auto &x: args) {
            ASR::call_arg_t call_arg;
            call_arg.loc = loc;
            call_arg.m_value = x;
            call_args.push_back(al, call_arg);
        }
        return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
            sym, sym, call_args.p, call_args.n, return_type, nullptr, nullptr));
    }

    ASR::symbol_t *create_bindc_function(const Location &loc,
            const std::string &fn_name, std::vector<ASR::ttype_t *> args_type,
            ASR::ttype_t *return_type=nullptr) {
        ASRUtils::ASRBuilder b(al, loc);
        symbolic_dependencies.push_back(fn_name);
        ASR::symbol_t* fn_sym = current_scope->resolve_symbol(fn_name);
        if ( !fn_sym ) {
            std::string header = "symengine/cwrapper.h";
            SymbolTable *fn_symtab = al.make_new<SymbolTable>(current_scope->parent);

            Vec<ASR::expr_t*> args; args.reserve(al, 1); int i = 1;
            for (auto &type: args_type) {
                std::string arg_name = "x_0" + std::to_string(i); i++;
                args.push_back(al, b.Variable(fn_symtab, arg_name, type,
                    ASR::intentType::In, ASR::abiType::BindC, true));
            }
            ASR::expr_t *return_var = nullptr;
            if ( return_type ) {
                char *return_var_name = s2c(al, "_lpython_return_variable");
                return_var = b.Variable(fn_symtab, return_var_name, return_type,
                    ASR::intentType::ReturnVar, ASR::abiType::BindC, false);
            }

            Vec<ASR::stmt_t *> body; body.reserve(al, 1);
            SetChar dep; dep.reserve(al, 1);
            fn_sym = ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(
                al, loc, fn_symtab, s2c(al, fn_name), dep.p, dep.n, args.p, args.n,
                body.p, body.n, return_var, ASR::abiType::BindC, ASR::accessType::Public,
                ASR::deftypeType::Interface, s2c(al, fn_name), false, false, false,
                false, false, nullptr, 0, false, false, false, s2c(al, header)));
            current_scope->parent->add_symbol(fn_name, fn_sym);
        }
        return fn_sym;
    }

    ASR::stmt_t *basic_new_stack(const Location &loc, ASR::expr_t *x) {
        ASR::symbol_t* basic_new_stack_sym = create_bindc_function(loc, "basic_new_stack",
            {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))});
        return SubroutineCall(loc, basic_new_stack_sym, {x});
    }

    ASR::stmt_t *basic_free_stack(const Location &loc, ASR::expr_t *x) {
        ASR::symbol_t* basic_free_stack_sym = create_bindc_function(loc, "basic_free_stack",
            {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))});
        return SubroutineCall(loc, basic_free_stack_sym, {x});
    }

    ASR::expr_t *basic_new_heap(const Location& loc) {
        ASR::symbol_t* basic_new_heap_sym = create_bindc_function(loc,
            "basic_new_heap", {}, ASRUtils::TYPE((ASR::make_CPtr_t(al, loc))));
        Vec<ASR::call_arg_t> call_args; call_args.reserve(al, 1);
        return FunctionCall(loc, basic_new_heap_sym, {},
            ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)));
    }

    ASR::stmt_t* basic_get_args(const Location& loc, ASR::expr_t *x, ASR::expr_t *y) {
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_CPtr_t(al, loc));
        ASR::symbol_t* basic_get_args_sym = create_bindc_function(loc,
            "basic_get_args", {type, type});
        return SubroutineCall(loc, basic_get_args_sym, {x, y});
    }

    ASR::expr_t *vecbasic_new(const Location& loc) {
        ASR::symbol_t* vecbasic_new_sym = create_bindc_function(loc,
            "vecbasic_new", {}, ASRUtils::TYPE((ASR::make_CPtr_t(al, loc))));
        Vec<ASR::call_arg_t> call_args; call_args.reserve(al, 1);
        return FunctionCall(loc, vecbasic_new_sym, {},
            ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)));
    }

    ASR::stmt_t* vecbasic_get(const Location& loc, ASR::expr_t *x, ASR::expr_t *y, ASR::expr_t *z) {
        ASR::ttype_t *cptr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, loc));
        ASR::symbol_t* vecbasic_get_sym = create_bindc_function(loc, "vecbasic_get",
            {cptr_type, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)), cptr_type});
        return SubroutineCall(loc, vecbasic_get_sym, {x, y, z});
    }

    ASR::expr_t *vecbasic_size(const Location& loc, ASR::expr_t *x) {
        ASR::symbol_t* vecbasic_size_sym = create_bindc_function(loc,
            "vecbasic_size", {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))},
            ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)));
        return FunctionCall(loc, vecbasic_size_sym, {x},
            ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)));
    }

    ASR::stmt_t* basic_assign(const Location& loc,
            ASR::expr_t *target, ASR::expr_t *value) {
        ASR::ttype_t *cptr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, loc));
        ASR::symbol_t* basic_assign_sym = create_bindc_function(loc, "basic_assign",
            {cptr_type, cptr_type});
        return SubroutineCall(loc, basic_assign_sym, {target, value});
    }

    ASR::expr_t* basic_str(const Location& loc, ASR::expr_t *x) {
        ASR::symbol_t* basic_str_sym = create_bindc_function(loc,
            "basic_str", {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))},
            ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr)));
        return FunctionCall(loc, basic_str_sym, {x},
            ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr)));
    }

    ASR::expr_t* basic_get_type(const Location& loc, ASR::expr_t* value) {
        ASR::symbol_t* basic_get_type_sym = create_bindc_function(loc,
            "basic_get_type", {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))},
            ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)));
        return FunctionCall(loc, basic_get_type_sym, {handle_argument(al, loc, value)},
            ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)));
    }

    ASR::expr_t* basic_compare(const Location& loc,
            std::string fn_name, ASR::expr_t *left, ASR::expr_t *right) {
        ASR::symbol_t* basic_compare_sym = create_bindc_function(loc,
            fn_name, {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)), ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))},
            ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
        return FunctionCall(loc, basic_compare_sym, {handle_argument(al, loc, left),
            handle_argument(al, loc, right)}, ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
    }

    ASR::stmt_t* integer_set_si(const Location& loc, ASR::expr_t *target,
            ASR::expr_t *value) {
        ASR::symbol_t* integer_set_si_sym = create_bindc_function(loc,
            "integer_set_si", {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)),
            ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8))});
        return SubroutineCall(loc, integer_set_si_sym, {target, value});
    }

    ASR::stmt_t *symbol_set(const Location &loc, ASR::expr_t *target, ASR::expr_t *value) {
        ASR::symbol_t* symbol_set_sym = create_bindc_function(loc, "symbol_set",
            {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)), ASRUtils::TYPE(
            ASR::make_Character_t(al, loc, 1, -2, nullptr))});
        return SubroutineCall(loc, symbol_set_sym, {target, value});
    }

    ASR::stmt_t *basic_const(const Location &loc,
            const std::string &fn_name, ASR::expr_t* value) {
        ASR::symbol_t* basic_const_sym = create_bindc_function(loc, fn_name,
            {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))});
        return SubroutineCall(loc, basic_const_sym, {value});
    }

    ASR::stmt_t *basic_binop(const Location &loc, const std::string &fn_name,
            ASR::expr_t* target, ASR::expr_t* op_01, ASR::expr_t* op_02) {
        ASR::ttype_t *cptr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, loc));
        ASR::symbol_t* basic_binop_sym = create_bindc_function(loc, fn_name,
            {cptr_type, cptr_type, cptr_type});
        return SubroutineCall(loc, basic_binop_sym, {target,
            handle_argument(al, loc, op_01), handle_argument(al, loc, op_02)});
    }

    ASR::stmt_t *basic_unaryop(const Location &loc, const std::string &fn_name,
            ASR::expr_t* target, ASR::expr_t* op_01) {
        ASR::symbol_t* basic_unaryop_sym = create_bindc_function(loc, fn_name,
            {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)), ASRUtils::TYPE(
            ASR::make_CPtr_t(al, loc))});
        return SubroutineCall(loc, basic_unaryop_sym, {target,
            handle_argument(al, loc, op_01)});
    }

    ASR::expr_t *basic_has_symbol(const Location &loc, ASR::expr_t *value_01, ASR::expr_t *value_02) {
        ASR::symbol_t* basic_has_symbol_sym = create_bindc_function(loc,
            "basic_has_symbol", {ASRUtils::TYPE(ASR::make_CPtr_t(al, loc)), ASRUtils::TYPE(ASR::make_CPtr_t(al, loc))},
            ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
        return FunctionCall(loc, basic_has_symbol_sym,
            {handle_argument(al, loc, value_01), handle_argument(al, loc, value_02)},
            ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
    }
    /********************************** Utils *********************************/

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = this->current_scope;
        this->current_scope = xx.m_symtab;

        ASR::ttype_t* f_signature= xx.m_function_signature;
        ASR::FunctionType_t *f_type = ASR::down_cast<ASR::FunctionType_t>(f_signature);
        ASR::ttype_t *CPtr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
        for (size_t i = 0; i < f_type->n_arg_types; ++i) {
            if (f_type->m_arg_types[i]->type == ASR::ttypeType::SymbolicExpression) {
                f_type->m_arg_types[i] = CPtr_type;
            } else if (f_type->m_arg_types[i]->type == ASR::ttypeType::List) {
                ASR::List_t* list = ASR::down_cast<ASR::List_t>(f_type->m_arg_types[i]);
                if (list->m_type->type == ASR::ttypeType::SymbolicExpression){
                    ASR::ttype_t* list_type = ASRUtils::TYPE(ASR::make_List_t(al, xx.base.base.loc, CPtr_type));
                    f_type->m_arg_types[i] = list_type;
                }
            }
        }

        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *s = ASR::down_cast<ASR::Variable_t>(item.second);
                this->visit_Variable(*s);
            }
        }
        transform_stmts(xx.m_body, xx.n_body);

        // freeing out variables
        if (!symbolic_vars_to_free.empty()) {
            Vec<ASR::stmt_t*> func_body;
            func_body.from_pointer_n_copy(al, xx.m_body, xx.n_body);

            for (ASR::symbol_t* symbol : symbolic_vars_to_free) {
                if (symbolic_vars_to_omit.find(symbol) != symbolic_vars_to_omit.end()) continue;
                func_body.push_back(al, basic_free_stack(x.base.base.loc,
                    ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, symbol))));
            }

            xx.n_body = func_body.size();
            xx.m_body = func_body.p;
            symbolic_vars_to_free.clear();
        }

        SetChar function_dependencies;
        function_dependencies.from_pointer_n_copy(al, xx.m_dependencies, xx.n_dependencies);
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
            std::string var_name = xx.m_name;
            std::string placeholder = "_" + std::string(var_name);

            ASR::ttype_t *CPtr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
            xx.m_type = CPtr_type;
            if (var_name != "_lpython_return_variable" && xx.m_intent != ASR::intentType::Out) {
                symbolic_vars_to_free.insert(ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&xx));
            }
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
                ASR::expr_t* value2 = ASRUtils::EXPR(ASR::make_PointerNullConstant_t(al, xx.base.base.loc, CPtr_type));

                // statement 3
                ASR::expr_t* get_pointer_node = ASRUtils::EXPR(ASR::make_GetPointer_t(al, xx.base.base.loc,
                    target1, ASRUtils::TYPE(ASR::make_Pointer_t(al, xx.base.base.loc, type2)), nullptr));
                ASR::expr_t* value3 = ASRUtils::EXPR(ASR::make_PointerToCPtr_t(al, xx.base.base.loc, get_pointer_node,
                    CPtr_type, nullptr));

                // defining the assignment statement
                ASR::stmt_t* stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target1, value1, nullptr));
                ASR::stmt_t* stmt2 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value2, nullptr));
                ASR::stmt_t* stmt3 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value3, nullptr));
                // statement 4
                ASR::stmt_t* stmt4 = basic_new_stack(x.base.base.loc, target2);

                pass_result.push_back(al, stmt1);
                pass_result.push_back(al, stmt2);
                pass_result.push_back(al, stmt3);
                pass_result.push_back(al, stmt4);
            }
        } else if (xx.m_type->type == ASR::ttypeType::List) {
            ASR::List_t* list = ASR::down_cast<ASR::List_t>(xx.m_type);
            if (list->m_type->type == ASR::ttypeType::SymbolicExpression){
                ASR::ttype_t *CPtr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
                ASR::ttype_t* list_type = ASRUtils::TYPE(ASR::make_List_t(al, xx.base.base.loc, CPtr_type));
                xx.m_type = list_type;
            }
        }
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

    void process_intrinsic_function(const Location &loc,
            ASR::IntrinsicScalarFunction_t* x, ASR::expr_t* target) {
        int64_t intrinsic_id = x->m_intrinsic_id;
        switch (static_cast<LCompilers::ASRUtils::IntrinsicScalarFunctions>(intrinsic_id)) {
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicSymbol: {
                pass_result.push_back(al, symbol_set(loc, target, x->m_args[0]));
                break;
            }
            BASIC_CONST(Pi, pi)
            BASIC_CONST(E, E)
            BASIC_BINOP(Add, add)
            BASIC_BINOP(Sub, sub)
            BASIC_BINOP(Mul, mul)
            BASIC_BINOP(Div, div)
            BASIC_BINOP(Pow, pow)
            BASIC_BINOP(Diff, diff)
            BASIC_UNARYOP(Sin, sin)
            BASIC_UNARYOP(Cos, cos)
            BASIC_UNARYOP(Log, log)
            BASIC_UNARYOP(Exp, exp)
            BASIC_UNARYOP(Abs, abs)
            BASIC_UNARYOP(Expand, expand)
            case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicGetArgument: {
                // Define necessary function symbols
                ASR::expr_t* value1 = handle_argument(al, loc, x->m_args[0]);

                // Define necessary variables
                ASR::ttype_t* CPtr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, loc));
                std::string args_str = current_scope->get_unique_name("_lcompilers_symbolic_argument_container");
                ASR::symbol_t* args_sym = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                    al, loc, current_scope, s2c(al, args_str), nullptr, 0, ASR::intentType::Local,
                    nullptr, nullptr, ASR::storage_typeType::Default, CPtr_type, nullptr,
                    ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
                current_scope->add_symbol(args_str, args_sym);

                // Statement 1
                ASR::expr_t* args = ASRUtils::EXPR(ASR::make_Var_t(al, loc, args_sym));
                ASR::expr_t* function_call1 = vecbasic_new(loc);
                ASR::stmt_t* stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, args, function_call1, nullptr));
                pass_result.push_back(al, stmt1);

                // Statement 2
                pass_result.push_back(al, basic_get_args(loc, value1, args));

                // Statement 3
                ASR::expr_t* function_call2 = vecbasic_size(loc, args);
                ASR::expr_t* test = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc, function_call2, ASR::cmpopType::Gt,
                        x->m_args[1], ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr));
                std::string error_str = "tuple index out of range";
                ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                        1, error_str.size(), nullptr));
                ASR::expr_t* error = ASRUtils::EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, error_str), str_type));
                ASR::stmt_t *stmt3 = ASRUtils::STMT(ASR::make_Assert_t(al, loc, test, error));
                pass_result.push_back(al, stmt3);

                // Statement 4
                pass_result.push_back(al, vecbasic_get(loc, args, x->m_args[1], target));
                break;
            }
            default: {
                throw LCompilersException("IntrinsicFunction: `"
                    + ASRUtils::get_intrinsic_name(intrinsic_id)
                    + "` is not implemented");
            }
        }
    }

    ASR::expr_t* process_attributes(const Location &loc, ASR::expr_t* expr) {
        if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*expr)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(expr);
            int64_t intrinsic_id = intrinsic_func->m_intrinsic_id;
            switch (static_cast<LCompilers::ASRUtils::IntrinsicScalarFunctions>(intrinsic_id)) {
                case LCompilers::ASRUtils::IntrinsicScalarFunctions::SymbolicHasSymbolQ: {
                    return basic_has_symbol(loc, intrinsic_func->m_args[0],
                        intrinsic_func->m_args[1]);
                }
                // (sym_name, n) where n = 16, 15, ... as the right value of the
                // IntegerCompare node as it represents SYMENGINE_ADD through SYMENGINE_ENUM
                BASIC_ATTR(AddQ, 16)
                BASIC_ATTR(MulQ, 15)
                BASIC_ATTR(PowQ, 17)
                BASIC_ATTR(LogQ, 29)
                BASIC_ATTR(SinQ, 35)
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
        if (ASR::is_a<ASR::Var_t>(*x.m_value) && ASR::is_a<ASR::CPtr_t>(*ASRUtils::expr_type(x.m_value))) {
            ASR::symbol_t *v = ASR::down_cast<ASR::Var_t>(x.m_value)->m_v;
            if (symbolic_vars_to_free.find(v) == symbolic_vars_to_free.end()) return;
            ASR::symbol_t* var_sym = ASR::down_cast<ASR::Var_t>(x.m_value)->m_v;
            pass_result.push_back(al, basic_assign(x.base.base.loc, x.m_target,
                ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym))));
        } else if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*x.m_value)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(x.m_value);
            if (intrinsic_func->m_type->type == ASR::ttypeType::SymbolicExpression) {
                process_intrinsic_function(x.base.base.loc, intrinsic_func, x.m_target);
            } else if (intrinsic_func->m_type->type == ASR::ttypeType::Logical) {
                ASR::expr_t* function_call = process_attributes(x.base.base.loc, x.m_value);
                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, x.m_target, function_call, nullptr));
                pass_result.push_back(al, stmt);
            }
        } else if (ASR::is_a<ASR::Cast_t>(*x.m_value)) {
            ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(x.m_value);
            if (cast_t->m_kind == ASR::cast_kindType::IntegerToSymbolicExpression) {
                ASR::expr_t* cast_arg = cast_t->m_arg;
                ASR::expr_t* cast_value = cast_t->m_value;
                if (ASR::is_a<ASR::Var_t>(*cast_arg)) {
                    ASR::ttype_t* cast_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                    ASR::expr_t* value = ASRUtils::EXPR(ASR::make_Cast_t(al, x.base.base.loc, cast_arg,
                        (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, cast_type, nullptr));
                    pass_result.push_back(al, integer_set_si(x.base.base.loc, x.m_target, value));
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

                        ASR::ttype_t* cast_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                        ASR::expr_t* value = ASRUtils::EXPR(ASR::make_Cast_t(al, x.base.base.loc, cast_arg,
                            (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, cast_type,
                            ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, const_value, cast_type))));
                        pass_result.push_back(al, integer_set_si(x.base.base.loc, x.m_target, value));
                    }
                }
            }
        } else if (ASR::is_a<ASR::ListConstant_t>(*x.m_value)) {
            ASR::ListConstant_t* list_constant = ASR::down_cast<ASR::ListConstant_t>(x.m_value);
            if (list_constant->m_type->type == ASR::ttypeType::List) {
                ASR::List_t* list = ASR::down_cast<ASR::List_t>(list_constant->m_type);

                if (list->m_type->type == ASR::ttypeType::SymbolicExpression){
                    if(ASR::is_a<ASR::Var_t>(*x.m_target)) {
                        ASR::symbol_t *v = ASR::down_cast<ASR::Var_t>(x.m_target)->m_v;
                        if (ASR::is_a<ASR::Variable_t>(*v)) {
                            // Step1: Add the placeholder for the list variable to the scope
                            ASRUtils::ASRBuilder b(al, x.base.base.loc);
                            ASR::ttype_t* CPtr_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc));
                            ASR::ttype_t* list_type = ASRUtils::TYPE(ASR::make_List_t(al, x.base.base.loc, CPtr_type));
                            ASR::Variable_t *list_variable = ASR::down_cast<ASR::Variable_t>(v);
                            std::string list_name = list_variable->m_name;
                            std::string placeholder = "_" + std::string(list_name);

                            ASR::symbol_t* placeholder_sym = ASR::down_cast<ASR::symbol_t>(
                                ASR::make_Variable_t(al, list_variable->base.base.loc, current_scope,
                                                    s2c(al, placeholder), nullptr, 0,
                                                    list_variable->m_intent, nullptr,
                                                    nullptr, list_variable->m_storage,
                                                    list_type, nullptr, list_variable->m_abi,
                                                    list_variable->m_access, list_variable->m_presence,
                                                    list_variable->m_value_attr));

                            current_scope->add_symbol(s2c(al, placeholder), placeholder_sym);
                            ASR::expr_t* placeholder_target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, placeholder_sym));

                            Vec<ASR::expr_t*> temp_list1, temp_list2;
                            temp_list1.reserve(al, list_constant->n_args + 1);
                            temp_list2.reserve(al, list_constant->n_args + 1);

                            for (size_t i = 0; i < list_constant->n_args; ++i) {
                                ASR::expr_t* value = handle_argument(al, x.base.base.loc,  list_constant->m_args[i]);
                                temp_list1.push_back(al, value);
                            }

                            ASR::expr_t* temp_list_const1 = ASRUtils::EXPR(ASR::make_ListConstant_t(al, x.base.base.loc, temp_list1.p,
                                            temp_list1.size(), list_type));
                            ASR::stmt_t* stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, placeholder_target, temp_list_const1, nullptr));
                            pass_result.push_back(al, stmt1);

                            // Step2: Add the empty list variable
                            ASR::expr_t* temp_list_const2 = ASRUtils::EXPR(ASR::make_ListConstant_t(al, x.base.base.loc, temp_list2.p,
                                            temp_list2.size(), list_type));
                            ASR::stmt_t* stmt2 = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, x.m_target, temp_list_const2, nullptr));
                            pass_result.push_back(al, stmt2);

                            // Step3: Add the list index to the function scope
                            std::string symbolic_list_index = current_scope->get_unique_name("symbolic_list_index");
                            ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4));
                            ASR::symbol_t* index_sym = ASR::down_cast<ASR::symbol_t>(
                                ASR::make_Variable_t(al, x.base.base.loc, current_scope, s2c(al, symbolic_list_index),
                                nullptr, 0, ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                int32_type, nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false));
                            current_scope->add_symbol(symbolic_list_index, index_sym);
                            ASR::expr_t* index = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, index_sym));
                            ASR::stmt_t* stmt3 = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, index,
                                ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 0, int32_type)), nullptr));
                            pass_result.push_back(al, stmt3);

                            // Step4: Add the DoLoop for appending elements into the list
                            std::string block_name = current_scope->get_unique_name("block");
                            SymbolTable* block_symtab = al.make_new<SymbolTable>(current_scope);
                            char *tmp_var_name = s2c(al, "tmp");
                            ASR::expr_t* tmp_var = b.Variable(block_symtab, tmp_var_name, CPtr_type,
                                ASR::intentType::Local, ASR::abiType::Source, false);
                            Vec<ASR::stmt_t*> block_body; block_body.reserve(al, 1);
                            ASR::stmt_t* block_stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, tmp_var,
                                basic_new_heap(x.base.base.loc), nullptr));
                            block_body.push_back(al, block_stmt1);
                            ASR::stmt_t* block_stmt2 = ASRUtils::STMT(ASR::make_ListAppend_t(al, x.base.base.loc, x.m_target, tmp_var));
                            block_body.push_back(al, block_stmt2);
                            block_body.push_back(al, basic_assign(x.base.base.loc, ASRUtils::EXPR(ASR::make_ListItem_t(al,
                                x.base.base.loc, x.m_target, index, CPtr_type, nullptr)), ASRUtils::EXPR(ASR::make_ListItem_t(al,
                                x.base.base.loc, placeholder_target, index, CPtr_type, nullptr))));
                            ASR::symbol_t* block = ASR::down_cast<ASR::symbol_t>(ASR::make_Block_t(al, x.base.base.loc,
                                                block_symtab, s2c(al, block_name), block_body.p, block_body.n));
                            current_scope->add_symbol(block_name, block);
                            ASR::stmt_t* block_call = ASRUtils::STMT(ASR::make_BlockCall_t(
                                al, x.base.base.loc, -1, block));
                            std::vector<ASR::stmt_t*> do_loop_body;
                            do_loop_body.push_back(block_call);
                            ASR::stmt_t* stmt4 = b.DoLoop(index, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 0, int32_type)),
                                ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x.base.base.loc,
                                ASRUtils::EXPR(ASR::make_ListLen_t(al, x.base.base.loc, placeholder_target, int32_type, nullptr)), ASR::binopType::Sub,
                                ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int32_type)), int32_type, nullptr)),
                                do_loop_body, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int32_type)));
                            pass_result.push_back(al, stmt4);
                        }
                    }
                }
            }
        } else if (ASR::is_a<ASR::ListItem_t>(*x.m_value)) {
            ASR::ListItem_t* list_item = ASR::down_cast<ASR::ListItem_t>(x.m_value);
            if (list_item->m_type->type == ASR::ttypeType::SymbolicExpression) {
                ASR::expr_t *value = ASRUtils::EXPR(ASR::make_ListItem_t(al,
                    x.base.base.loc, list_item->m_a, list_item->m_pos,
                    ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)), nullptr));
                pass_result.push_back(al, basic_assign(x.base.base.loc, x.m_target, value));
            }
        } else if (ASR::is_a<ASR::SymbolicCompare_t>(*x.m_value)) {
            ASR::SymbolicCompare_t *s = ASR::down_cast<ASR::SymbolicCompare_t>(x.m_value);
            if (s->m_op == ASR::cmpopType::Eq || s->m_op == ASR::cmpopType::NotEq) {
                ASR::expr_t* function_call = nullptr;
                if (s->m_op == ASR::cmpopType::Eq) {
                    function_call = basic_compare(x.base.base.loc, "basic_eq", s->m_left, s->m_right);
                } else {
                    function_call = basic_compare(x.base.base.loc, "basic_neq", s->m_left, s->m_right);
                }
                ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, x.m_target, function_call, nullptr));
                pass_result.push_back(al, stmt);
            }
        }
    }

    void visit_If(const ASR::If_t& x) {
        ASR::If_t& xx = const_cast<ASR::If_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
        transform_stmts(xx.m_orelse, xx.n_orelse);
        if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*xx.m_test)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(xx.m_test);
            if (intrinsic_func->m_type->type == ASR::ttypeType::Logical) {
                ASR::expr_t* function_call = process_attributes(xx.base.base.loc, xx.m_test);
                xx.m_test = function_call;
            }
        } else if (ASR::is_a<ASR::LogicalNot_t>(*xx.m_test)) {
            ASR::LogicalNot_t* logical_not = ASR::down_cast<ASR::LogicalNot_t>(xx.m_test);
            if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*logical_not->m_arg)) {
                ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(logical_not->m_arg);
                if (intrinsic_func->m_type->type == ASR::ttypeType::Logical) {
                    ASR::expr_t* function_call = process_attributes(xx.base.base.loc, logical_not->m_arg);
                    ASR::expr_t* new_logical_not = ASRUtils::EXPR(ASR::make_LogicalNot_t(al, xx.base.base.loc, function_call,
                        logical_not->m_type, logical_not->m_value));
                    xx.m_test = new_logical_not;
                }
            }
        } else if (ASR::is_a<ASR::SymbolicCompare_t>(*xx.m_test)) {
            ASR::SymbolicCompare_t *s = ASR::down_cast<ASR::SymbolicCompare_t>(xx.m_test);
            ASR::expr_t* function_call = nullptr;
            if (s->m_op == ASR::cmpopType::Eq) {
                function_call = basic_compare(xx.base.base.loc, "basic_eq", s->m_left, s->m_right);
            } else {
                function_call = basic_compare(xx.base.base.loc, "basic_neq", s->m_left, s->m_right);
            }
            ASR::stmt_t* stmt = ASRUtils::STMT(ASR::make_If_t(al, xx.base.base.loc, function_call,
                xx.m_body, xx.n_body, xx.m_orelse, xx.n_orelse));
            pass_result.push_back(al, stmt);
        }
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        Vec<ASR::call_arg_t> call_args;
        call_args.reserve(al, 1);

        for (size_t i=0; i<x.n_args; i++) {
            ASR::expr_t* val = x.m_args[i].m_value;
            if (val && ASR::is_a<ASR::IntrinsicScalarFunction_t>(*val) && ASR::is_a<ASR::SymbolicExpression_t>(*ASRUtils::expr_type(val))) {
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
                process_intrinsic_function(x.base.base.loc, intrinsic_func, target);

                ASR::call_arg_t call_arg;
                call_arg.loc = x.base.base.loc;
                call_arg.m_value = target;
                call_args.push_back(al, call_arg);
            } else if (val && ASR::is_a<ASR::Cast_t>(*val)) {
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
            x.m_name, call_args.p, call_args.n, x.m_dt));
        pass_result.push_back(al, stmt);
    }

    void visit_Print(const ASR::Print_t &x) {
        std::vector<ASR::expr_t*> print_tmp;
        for (size_t i=0; i<x.n_values; i++) {
            ASR::expr_t* val = x.m_values[i];
            if (ASR::is_a<ASR::Var_t>(*val) && ASR::is_a<ASR::CPtr_t>(*ASRUtils::expr_type(val))) {
                ASR::symbol_t *v = ASR::down_cast<ASR::Var_t>(val)->m_v;
                if (symbolic_vars_to_free.find(v) == symbolic_vars_to_free.end()) return;
                print_tmp.push_back(basic_str(x.base.base.loc, val));
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
                    process_intrinsic_function(x.base.base.loc, intrinsic_func, target);

                    // Now create the FunctionCall node for basic_str
                    print_tmp.push_back(basic_str(x.base.base.loc, target));
                } else if (ASR::is_a<ASR::Logical_t>(*ASRUtils::expr_type(val))) {
                    ASR::expr_t* function_call = process_attributes(x.base.base.loc, val);
                    print_tmp.push_back(function_call);
                }
            } else if (ASR::is_a<ASR::Cast_t>(*val)) {
                ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(val);
                if(cast_t->m_kind != ASR::cast_kindType::IntegerToSymbolicExpression) return;
                this->visit_Cast(*cast_t);
                ASR::symbol_t *var_sym = current_scope->get_symbol(symengine_stack.pop());
                ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var_sym));

                // Now create the FunctionCall node for basic_str
                print_tmp.push_back(basic_str(x.base.base.loc, target));
            } else if (ASR::is_a<ASR::SymbolicCompare_t>(*val)) {
                ASR::SymbolicCompare_t *s = ASR::down_cast<ASR::SymbolicCompare_t>(val);
                if (s->m_op == ASR::cmpopType::Eq || s->m_op == ASR::cmpopType::NotEq) {
                    ASR::expr_t* function_call = nullptr;
                    if (s->m_op == ASR::cmpopType::Eq) {
                        function_call = basic_compare(x.base.base.loc, "basic_eq", s->m_left, s->m_right);
                    } else {
                        function_call = basic_compare(x.base.base.loc, "basic_neq", s->m_left, s->m_right);
                    }
                    print_tmp.push_back(function_call);
                }
            } else if (ASR::is_a<ASR::ListItem_t>(*val)) {
                ASR::ListItem_t* list_item = ASR::down_cast<ASR::ListItem_t>(val);
                if (list_item->m_type->type == ASR::ttypeType::SymbolicExpression) {
                    ASR::expr_t *value = ASRUtils::EXPR(ASR::make_ListItem_t(al,
                        x.base.base.loc, list_item->m_a, list_item->m_pos,
                        ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc)), nullptr));
                    print_tmp.push_back(basic_str(x.base.base.loc, value));
                }
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
                ASR::make_Print_t(al, x.base.base.loc, tmp_vec.p, tmp_vec.size(),
                            x.m_separator, x.m_end));
            print_tmp.clear();
            pass_result.push_back(al, print_stmt);
        }
    }

    void visit_IntrinsicFunction(const ASR::IntrinsicScalarFunction_t &x) {
        if(x.m_type && x.m_type->type == ASR::ttypeType::SymbolicExpression) {
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
            process_intrinsic_function(x.base.base.loc, &xx, target);
        }
    }

    void visit_Cast(const ASR::Cast_t &x) {
        if(x.m_kind != ASR::cast_kindType::IntegerToSymbolicExpression) return;

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

                ASR::ttype_t* cast_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                ASR::expr_t* value = ASRUtils::EXPR(ASR::make_Cast_t(al, x.base.base.loc, cast_arg,
                    (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, cast_type,
                    ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, const_value, cast_type))));
                pass_result.push_back(al, integer_set_si(x.base.base.loc, target, value));
            }
        }
    }

    ASR::expr_t* process_with_basic_str(const Location &loc, const ASR::expr_t *expr) {
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
            } else {
                LCOMPILERS_ASSERT(false);
            }

            ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(al, loc, var_sym));
            // Now create the FunctionCall node for basic_str and return
            return basic_str(loc, target);
    }

    void visit_Assert(const ASR::Assert_t &x) {
        ASR::expr_t* left_tmp = nullptr;
        ASR::expr_t* right_tmp = nullptr;
        if (ASR::is_a<ASR::LogicalCompare_t>(*x.m_test)) {
            ASR::LogicalCompare_t *l = ASR::down_cast<ASR::LogicalCompare_t>(x.m_test);

            left_tmp = process_attributes(x.base.base.loc, l->m_left);
            right_tmp = process_attributes(x.base.base.loc, l->m_right);
            ASR::expr_t* test =  ASRUtils::EXPR(ASR::make_LogicalCompare_t(al, x.base.base.loc, left_tmp,
                l->m_op, right_tmp, l->m_type, l->m_value));

            ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, test, x.m_msg));
            pass_result.push_back(al, assert_stmt);
        } else if (ASR::is_a<ASR::SymbolicCompare_t>(*x.m_test)) {
            ASR::SymbolicCompare_t* s = ASR::down_cast<ASR::SymbolicCompare_t>(x.m_test);
            if (s->m_op == ASR::cmpopType::Eq || s->m_op == ASR::cmpopType::NotEq) {
                ASR::expr_t* function_call = nullptr;
                if (s->m_op == ASR::cmpopType::Eq) {
                    function_call = basic_compare(x.base.base.loc, "basic_eq", s->m_left, s->m_right);
                } else {
                    function_call = basic_compare(x.base.base.loc, "basic_neq", s->m_left, s->m_right);
                }
                ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, function_call, x.m_msg));
                pass_result.push_back(al, assert_stmt);
            }
        } else if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*x.m_test)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(x.m_test);
            if (intrinsic_func->m_type->type == ASR::ttypeType::Logical) {
                ASR::expr_t* test = process_attributes(x.base.base.loc, x.m_test);
                ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, test, x.m_msg));
                pass_result.push_back(al, assert_stmt);
            }
        } else if (ASR::is_a<ASR::LogicalBinOp_t>(*x.m_test)) {
            ASR::LogicalBinOp_t* binop = ASR::down_cast<ASR::LogicalBinOp_t>(x.m_test);
            if (ASR::is_a<ASR::SymbolicCompare_t>(*binop->m_left) && ASR::is_a<ASR::SymbolicCompare_t>(*binop->m_right)) {
                ASR::SymbolicCompare_t *s1 = ASR::down_cast<ASR::SymbolicCompare_t>(binop->m_left);
                left_tmp = process_with_basic_str(x.base.base.loc, s1->m_left);
                right_tmp = process_with_basic_str(x.base.base.loc, s1->m_right);
                ASR::expr_t* test1 =  ASRUtils::EXPR(ASR::make_StringCompare_t(al, x.base.base.loc, left_tmp,
                    s1->m_op, right_tmp, s1->m_type, s1->m_value));

                ASR::SymbolicCompare_t *s2 = ASR::down_cast<ASR::SymbolicCompare_t>(binop->m_right);
                left_tmp = process_with_basic_str(x.base.base.loc, s2->m_left);
                right_tmp = process_with_basic_str(x.base.base.loc, s2->m_right);
                ASR::expr_t* test2 =  ASRUtils::EXPR(ASR::make_StringCompare_t(al, x.base.base.loc, left_tmp,
                    s2->m_op, right_tmp, s2->m_type, s2->m_value));

                ASR::expr_t *cond = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, x.base.base.loc,
                    test1, ASR::logicalbinopType::Or, test2, binop->m_type, binop->m_value));
                ASR::stmt_t *assert_stmt = ASRUtils::STMT(ASR::make_Assert_t(al, x.base.base.loc, cond, x.m_msg));
                pass_result.push_back(al, assert_stmt);
            }
        }
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
        if (ASR::is_a<ASR::IntrinsicScalarFunction_t>(*xx.m_test)) {
            ASR::IntrinsicScalarFunction_t* intrinsic_func = ASR::down_cast<ASR::IntrinsicScalarFunction_t>(xx.m_test);
            if (intrinsic_func->m_type->type == ASR::ttypeType::Logical) {
                ASR::expr_t* function_call = process_attributes(xx.base.base.loc, xx.m_test);
                xx.m_test = function_call;
            }
        }
    }

    void visit_Return(const ASR::Return_t &x) {
        if (!symbolic_vars_to_free.empty()){
            for (ASR::symbol_t* symbol : symbolic_vars_to_free) {
                if (symbolic_vars_to_omit.find(symbol) != symbolic_vars_to_omit.end()) continue;
                // freeing out variables
                pass_result.push_back(al, basic_free_stack(x.base.base.loc,
                    ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, symbol))));
            }
            symbolic_vars_to_free.clear();
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_Return_t(al, x.base.base.loc)));
        }
    }
};

void pass_replace_symbolic(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceSymbolicVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LCompilers
