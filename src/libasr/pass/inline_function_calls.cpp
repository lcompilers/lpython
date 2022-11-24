#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/inline_function_calls.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <map>
#include <utility>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces function calls expressions with the body of the function
itself. This helps in avoiding function call overhead in the backend code.

Converts:

    complex(4) function f(a) result(b)
    complex(4), intent(in) :: a
    b = a + 5
    end function

    c = f(a)

to:

    c = a + 5

*/
class InlineFunctionCallVisitor : public PassUtils::PassVisitor<InlineFunctionCallVisitor>
{
private:

    std::string rl_path;

    ASR::expr_t* function_result_var;

    bool from_inline_function_call, inlining_function;
    bool fixed_duplicated_expr_stmt;
    bool is_fast;

    // Stores the local variables corresponding to the ones
    // present in function symbol table.
    std::map<std::string, ASR::symbol_t*> arg2value;

    std::string current_routine;

    bool inline_external_symbol_calls;


    ASRUtils::ExprStmtDuplicator node_duplicator;

    SymbolTable* current_routine_scope;
    ASRUtils::LabelGenerator* label_generator;
    ASR::symbol_t* empty_block;
    ASRUtils::ReplaceReturnWithGotoVisitor return_replacer;

public:

    bool function_inlined;

    InlineFunctionCallVisitor(Allocator &al_, const std::string& rl_path_,
                              bool inline_external_symbol_calls_, bool is_fast_)
    : PassVisitor(al_, nullptr),
    rl_path(rl_path_), function_result_var(nullptr),
    from_inline_function_call(false), inlining_function(false), fixed_duplicated_expr_stmt(false),
    is_fast(is_fast_),
    current_routine(""), inline_external_symbol_calls(inline_external_symbol_calls_),
    node_duplicator(al_), current_routine_scope(nullptr),
    label_generator(ASRUtils::LabelGenerator::get_instance()),
    empty_block(nullptr), return_replacer(al_, 0),
    function_inlined(false)
    {
        pass_result.reserve(al, 1);
    }

    void configure_node_duplicator(bool allow_procedure_calls_) {
        node_duplicator.allow_procedure_calls = allow_procedure_calls_;
    }

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        current_routine = std::string(xx.m_name);
        PassUtils::PassVisitor<InlineFunctionCallVisitor>::visit_Function(x);
        current_routine.clear();
    }

    void visit_Var(const ASR::Var_t& x) {
        ASR::Var_t& xx = const_cast<ASR::Var_t&>(x);
        std::string x_var_name = std::string(ASRUtils::symbol_name(x.m_v));

        // If anything is not local to a function being inlined
        // then do not inline the function by setting
        // fixed_duplicated_expr_stmt to false.
        // To be supported later.
        if( current_routine_scope &&
            current_routine_scope->get_symbol(x_var_name) == nullptr ) {
            fixed_duplicated_expr_stmt = false;
            return ;
        }
        if( x.m_v->type == ASR::symbolType::Variable ) {
            ASR::Variable_t* x_var = ASR::down_cast<ASR::Variable_t>(x.m_v);
            if( arg2value.find(x_var_name) != arg2value.end() ) {
                x_var = ASR::down_cast<ASR::Variable_t>(arg2value[x_var_name]);
                if( current_scope->get_symbol(std::string(x_var->m_name)) != nullptr ) {
                    xx.m_v = arg2value[x_var_name];
                }
                x_var = ASR::down_cast<ASR::Variable_t>(x.m_v);
            }
        } else {
            fixed_duplicated_expr_stmt = false;
        }
    }

    void set_empty_block(SymbolTable* scope, const Location& loc) {
        std::string empty_block_name = scope->get_unique_name("~empty_block");
        if( empty_block_name != "~empty_block" ) {
            empty_block = scope->get_symbol("~empty_block");
        } else {
            SymbolTable* empty_symtab = al.make_new<SymbolTable>(scope);
            empty_block = ASR::down_cast<ASR::symbol_t>(ASR::make_Block_t(al, loc,
                                empty_symtab,
                                s2c(al, empty_block_name), nullptr, 0));
            scope->add_symbol(empty_block_name, empty_block);
        }
    }

    void remove_empty_block(SymbolTable* scope) {
        scope->erase_symbol("~empty_block");
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        // If this node is visited by any other visitor
        // or it is being visited while inlining another function call
        // then return. To ensure that only one function call is inlined
        // at a time.
        if( !from_inline_function_call || inlining_function ) {
            if( !inlining_function ) {
                return ;
            }
            // TODO: Handle type later
            if( ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) ) {
                ASR::ExternalSymbol_t* called_sym_ext = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
                ASR::symbol_t* f_sym = ASRUtils::symbol_get_past_external(called_sym_ext->m_external);
                ASR::Function_t* f = ASR::down_cast<ASR::Function_t>(f_sym);

                // Never inline intrinsic functions
                if( ASRUtils::is_intrinsic_function2(f) ) {
                    return ;
                }

                ASR::symbol_t* called_sym = x.m_name;

                // TODO: Hanlde later
                // ASR::symbol_t* called_sym_original = x.m_original_name;

                ASR::FunctionCall_t& xx = const_cast<ASR::FunctionCall_t&>(x);
                std::string called_sym_name = std::string(called_sym_ext->m_name);
                std::string new_sym_name_str = current_scope->get_unique_name(called_sym_name);
                char* new_sym_name = s2c(al, new_sym_name_str);
                if( current_scope->get_symbol(new_sym_name_str) == nullptr ) {
                    ASR::Module_t *m = ASR::down_cast2<ASR::Module_t>(f->m_symtab->parent->asr_owner);
                    char *modname = m->m_name;
                    ASR::symbol_t* new_sym = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                                                al, called_sym->base.loc, current_scope, new_sym_name,
                                                f_sym, modname, nullptr, 0,
                                                f->m_name, ASR::accessType::Private));
                    current_scope->add_symbol(new_sym_name_str, new_sym);
                }
                xx.m_name = current_scope->get_symbol(new_sym_name_str);
            }

            for( size_t i = 0; i < x.n_args; i++ ) {
                visit_expr(*x.m_args[i].m_value);
            }
            return ;
        }

        // Clear up any local variables present in arg2value map
        // due to inlining other function calls
        arg2value.clear();

        // Stores the result temporarily to avoid corrupting
        // the actual pass result due to failure of inlining function
        // calls
        Vec<ASR::stmt_t*> pass_result_local;
        pass_result_local.reserve(al, 1);

        // Avoid external symbols for now.
        ASR::symbol_t* routine = x.m_name;
        if( !ASR::is_a<ASR::Function_t>(*routine) ) {
            if( ASR::is_a<ASR::ExternalSymbol_t>(*routine) &&
                inline_external_symbol_calls) {
                routine = ASRUtils::symbol_get_past_external(x.m_name);
            } else {
                return ;
            }
        }

        // Avoid inlining current function call if its a recursion.
        ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(routine);
        if( ASRUtils::is_intrinsic_function2(func) ||
            std::string(func->m_name) == current_routine ) {
            return ;
        }

        if( !is_fast && !func->m_inline ) {
            return ;
        }

        current_routine_scope = func->m_symtab;

        ASR::expr_t* return_var = nullptr;
        // The following prepares arg2value map for inlining the
        // current function call. Variables are created in the current
        // scope for the arguments. These local variables are then initialised
        // as well with the argument value.
        for( size_t i = 0; i < func->n_args + 1; i++ ) {
            ASR::expr_t *func_margs_i = nullptr, *x_m_args_i = nullptr;
            if( i < func->n_args ) {
                func_margs_i = func->m_args[i];
                x_m_args_i = x.m_args[i].m_value;
            } else {
                func_margs_i = func->m_return_var;
                x_m_args_i = nullptr;
            }
            if( !ASR::is_a<ASR::Var_t>(*func_margs_i) ) {
                arg2value.clear();
                return ;
            }
            ASR::Var_t* arg_var = ASR::down_cast<ASR::Var_t>(func_margs_i);
            // TODO: Expand to other symbol types, Function, Subroutine, ExternalSymbol
            if( !ASR::is_a<ASR::Variable_t>(*(arg_var->m_v)) ) {
                arg2value.clear();
                return ;
            }
            ASR::Variable_t* arg_variable = ASR::down_cast<ASR::Variable_t>(arg_var->m_v);
            std::string arg_variable_name = std::string(arg_variable->m_name);
            std::string arg_name = current_scope->get_unique_name(arg_variable_name + "_" + std::string(func->m_name));
            ASR::stmt_t* assign_stmt = nullptr;
            ASR::expr_t* call_arg_var = nullptr;
            if( x_m_args_i ) {
                call_arg_var = PassUtils::create_auxiliary_variable_for_expr(x_m_args_i, arg_name, al, current_scope, assign_stmt);
            } else {
                call_arg_var = PassUtils::create_auxiliary_variable(func_margs_i->base.loc, arg_name, al, current_scope, ASRUtils::expr_type(func_margs_i));
                return_var = call_arg_var;
            }
            if( assign_stmt ) {
                pass_result_local.push_back(al, assign_stmt);
            }
            arg2value[arg_variable_name] = ASR::down_cast<ASR::Var_t>(call_arg_var)->m_v;
        }

        bool success = true;
        // Stores the initialisation expression for function's local variables
        // i.e., other than the argument variables.
        std::vector<std::pair<ASR::expr_t*, ASR::symbol_t*>> exprs_to_be_visited;

        // The following loop inserts the function's local symbols i.e.,
        // the ones other than the arguments.
        // exprs_to_be_visited temporarily stores the initilisation expression as well.
        for( auto& itr : func->m_symtab->get_scope() ) {
            if( startswith(itr.first, "~empty_block") ) {
                set_empty_block(current_scope, func->base.base.loc);
                continue;
            }
            ASR::Variable_t* func_var = ASR::down_cast<ASR::Variable_t>(itr.second);
            std::string func_var_name = itr.first;
            if( arg2value.find(func_var_name) == arg2value.end() ) {
                std::string local_var_name = current_scope->get_unique_name(func_var_name + "_" + std::string(func->m_name));
                node_duplicator.success = true;
                ASR::expr_t *m_symbolic_value = node_duplicator.duplicate_expr(func_var->m_symbolic_value);
                if( !node_duplicator.success ) {
                    success = false;
                    break;
                }
                node_duplicator.success = true;
                ASR::expr_t *m_value = node_duplicator.duplicate_expr(func_var->m_value);
                if( !node_duplicator.success ) {
                    success = false;
                    break;
                }
                ASR::ttype_t* local_var_type = func_var->m_type;
                ASR::symbol_t* local_var = (ASR::symbol_t*) ASR::make_Variable_t(
                                                al, func_var->base.base.loc, current_scope,
                                                s2c(al, local_var_name), ASR::intentType::Local,
                                                nullptr, nullptr, ASR::storage_typeType::Default,
                                                local_var_type, ASR::abiType::Source, ASR::accessType::Public,
                                                ASR::presenceType::Required, false);
                current_scope->add_symbol(local_var_name, local_var);
                arg2value[func_var_name] = local_var;
                if( m_symbolic_value && !ASR::is_a<ASR::Const_t>(*local_var_type) ) {
                    exprs_to_be_visited.push_back(std::make_pair(m_symbolic_value, local_var));
                }
                if( m_value && !ASR::is_a<ASR::Const_t>(*local_var_type) ) {
                    exprs_to_be_visited.push_back(std::make_pair(m_value, local_var));
                }
            }
        }

        // At this point arg2value map is ready with all the variables.
        // Now, we visit the initilisation expression of the local variables
        // and replace the variables present in those expressions with the ones
        // in the current scope. See, `visit_Var` to know how replacement occurs.
        for( size_t i = 0; i < exprs_to_be_visited.size() && success; i++ ) {
            ASR::expr_t* value = exprs_to_be_visited[i].first;
            fixed_duplicated_expr_stmt = true;
            visit_expr(*value);
            if( !fixed_duplicated_expr_stmt ) {
                success = false;
                break;
            }
            ASR::symbol_t* variable = exprs_to_be_visited[i].second;
            ASR::expr_t* var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, variable->base.loc, variable));
            ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, var->base.loc, var, value, nullptr));
            pass_result_local.push_back(al, assign_stmt);
        }

        Vec<ASR::stmt_t*> func_copy;
        func_copy.reserve(al, func->n_body);
        // Duplicate each and every statement of the function body.
        for( size_t i = 0; i < func->n_body && success; i++ ) {
            node_duplicator.success = true;
            ASR::stmt_t* m_body_copy = node_duplicator.duplicate_stmt(func->m_body[i]);
            if( node_duplicator.success ) {
                func_copy.push_back(al, m_body_copy);
            } else {
                success = false;
            }
        }

        if( success ) {
            // Set inlining_function to true so that we inline
            // only one function at a time.
            inlining_function = true;
            for( size_t i = 0; i < func->n_body && success; i++ ) {
                fixed_duplicated_expr_stmt = true;
                visit_stmt(*func_copy[i]);
                success = success && fixed_duplicated_expr_stmt;
            }

            if( success ) {
                set_empty_block(current_scope, func->base.base.loc);
                uint64_t block_call_label = label_generator->get_unique_label();
                ASR::stmt_t* block_call = ASRUtils::STMT(ASR::make_BlockCall_t(al, x.base.base.loc,
                                                        block_call_label, empty_block));
                label_generator->add_node_with_unique_label((ASR::asr_t*) block_call,
                                                            block_call_label);
                return_replacer.set_goto_label(block_call_label);
                // If duplication is successfull then fill the
                // pass result with assignment statements
                // (for local variables in the loop just below)
                // and the function body (the next loop).
                for( size_t i = 0; i < pass_result_local.size(); i++ ) {
                    pass_result.push_back(al, pass_result_local[i]);
                }

                bool is_goto_added = false;
                for( size_t i = 0; i < func->n_body; i++ ) {
                    return_replacer.current_stmt = &func_copy.p[i];
                    return_replacer.has_replacement_happened = false;
                    return_replacer.replace_stmt(func_copy[i]);
                    is_goto_added = is_goto_added || return_replacer.has_replacement_happened;
                    pass_result.push_back(al, func_copy[i]);
                }
                if( is_goto_added ) {
                    pass_result.push_back(al, block_call);
                } else {
                    remove_empty_block(current_scope);
                }
            }
            inlining_function = false;
            current_routine_scope = nullptr;
            function_result_var = return_var;
        }

        if (!success) {
            // If not successfull then delete all the local variables
            // created for the purpose of inlining the current function call.
            for( auto& itr : arg2value ) {
                ASR::Variable_t* auxiliary_var = ASR::down_cast<ASR::Variable_t>(itr.second);
                current_scope->erase_symbol(std::string(auxiliary_var->m_name));
            }
            function_result_var = nullptr;
        }
        // At least one function is inlined
        function_inlined = success;
        success = false;
        // Clear up the arg2value to avoid corruption
        // of any kind.
        arg2value.clear();
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_RealBinOp(const ASR::RealBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t& x) {
        handle_BinOp(x);
    }

    template <typename T>
    void handle_BinOp(const T& x) {
        T& xx = const_cast<T&>(x);
        from_inline_function_call = true;
        function_result_var = nullptr;
        visit_expr(*x.m_left);
        if( function_result_var ) {
            xx.m_left = function_result_var;
        }
        function_result_var = nullptr;
        visit_expr(*x.m_right);
        if( function_result_var ) {
            xx.m_right = function_result_var;
        }
        function_result_var = nullptr;
        from_inline_function_call = false;
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        from_inline_function_call = true;
        retain_original_stmt = true;
        ASR::Assignment_t& xx = const_cast<ASR::Assignment_t&>(x);
        function_result_var = nullptr;
        visit_expr(*x.m_target);
        function_result_var = nullptr;
        visit_expr(*x.m_value);
        if( function_result_var ) {
            xx.m_value = function_result_var;
        }
        function_result_var = nullptr;
        from_inline_function_call = false;
    }

};

void pass_inline_function_calls(Allocator &al, ASR::TranslationUnit_t &unit,
                                const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    bool inline_external_symbol_calls = pass_options.inline_external_symbol_calls;
    InlineFunctionCallVisitor v(al, rl_path, inline_external_symbol_calls, pass_options.fast);
    v.configure_node_duplicator(false);
    v.visit_TranslationUnit(unit);
    v.configure_node_duplicator(true);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LFortran
