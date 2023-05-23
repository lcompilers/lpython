#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/nested_vars.h>
#include <libasr/pass/pass_utils.h>
#include <unordered_map>

namespace LCompilers {

using ASR::down_cast;

/*

This pass captures the global variables that are used by the
nested functions. This is handled in the following way:

Consider,

subroutine A()
    variables declared: x, y, w, k, t

    subroutine B(l)
        variables declared: p, l
        x += y
        w += l
        p += k
    end

    call_subroutine B(k)
end

Then using this pass, we first capture the variables that are used by
nesdted functions. In our example, they are x, y and w from function A.
Then we create a __lcompilers_created__nested_context__A which contains all those
variables with same types but different names, say, global_context_module_for_A_x,
global_context_module_for_A_y, global_context_module_for_A_w, and
global_context_module_for_A_k.

This pass will then transform the above code to:

module __lcompilers_created__nested_context__A
    variables declared: global_context_module_for_A_x,
                        global_context_module_for_A_y,
                        global_context_module_for_A_w,
                        global_context_module_for_A_k
end

subroutine A()
    use __lcompilers_created__nested_context__A
    variables declared: x, y, w, k, t

    subroutine B(l)
        variables declared: p, l
        global_context_module_for_A_x += global_context_module_for_A_y
        global_context_module_for_A_w += l
        p += global_context_module_for_A_k
    end

    global_context_module_for_A_x = x
    global_context_module_for_A_y = y
    global_context_module_for_A_w = w
    global_context_module_for_A_k = k

    call_subroutine B(global_context_module_for_A_k)

    x = global_context_module_for_A_x
    y = global_context_module_for_A_y
    w = global_context_module_for_A_w
    k = global_context_module_for_A_k
end

This assignment and re-assignment to the variables is done only before and
after a function call. The same applies when a global variable from a program
is used by a nested function.

Note: We do change any variables inside the parent function A ** except **
the captured variables inside the function call arguments. The reason is to
preserve the assignments in case of intent out. This highlighted in the line
where we change to `call_subroutine B(global_context_module_for_A_k)`.


This Pass is designed using three classes:
1. NestedVarVisitor - This captures the variables for each function that
                      are used by nested functions and creates a map of
                      function_syms -> {variables_syms}.

2. ReplaceNestedVisitor - This class replaces all the variables inside the
                          nested functions with external module variables.

3. AssignNestedVars - This class add assignments stmts before and after
                      each function call stmts in the parent function.

*/

class NestedVarVisitor : public ASR::BaseWalkVisitor<NestedVarVisitor>
{
public:
    Allocator &al;
    size_t nesting_depth = 0;
    SymbolTable* current_scope;
    std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> nesting_map;

    NestedVarVisitor(Allocator& al_): al(al_) {
        current_scope = nullptr;
    };

    ASR::symbol_t *cur_func_sym = nullptr;
    ASR::symbol_t *par_func_sym = nullptr;

    template<typename T>
    void visit_procedure(const T &x) {
        nesting_depth++;
        bool is_func_visited = false;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                par_func_sym = cur_func_sym;
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
                    item.second);
                if (!is_func_visited) {
                    is_func_visited = true;
                    for (size_t i = 0; i < x.n_body; i++) {
                        visit_stmt(*x.m_body[i]);
                    }
                }
                visit_Function(*s);
            }
        }
        if (!is_func_visited) {
            is_func_visited = true;
            for (size_t i = 0; i < x.n_body; i++) {
                visit_stmt(*x.m_body[i]);
            }
        }
        nesting_depth--;
    }


    void visit_Program(const ASR::Program_t &x) {
        ASR::symbol_t *cur_func_sym_copy = cur_func_sym;
        cur_func_sym = (ASR::symbol_t*)(&x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        visit_procedure(x);
        current_scope = current_scope_copy;
        cur_func_sym = cur_func_sym_copy;
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::symbol_t *cur_func_sym_copy = cur_func_sym;
        cur_func_sym = (ASR::symbol_t*)(&x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        visit_procedure(x);
        current_scope = current_scope_copy;
        cur_func_sym = cur_func_sym_copy;
    }

    void visit_Var(const ASR::Var_t &x) {
        // Only attempt if we are actually in a nested function
        if (nesting_depth > 1) {
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(
                    ASRUtils::symbol_get_past_external(x.m_v));
            // If the variable is not defined in the current scope, it is a
            // "needed global" since we need to be able to access it from the
            // nested procedure.
            if ( current_scope &&
                 v->m_parent_symtab->get_counter() != current_scope->get_counter() &&
                 (v->m_storage != ASR::storage_typeType::Parameter ||
                  ASRUtils::is_array(v->m_type)) ) {
                nesting_map[par_func_sym].insert(x.m_v);
            }
        }
    }
};


class ReplacerNestedVars: public ASR::BaseExprReplacer<ReplacerNestedVars> {
private:
    Allocator &al;
public:
    SymbolTable *current_scope;
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> nested_var_to_ext_var;
    bool skip_replace=false;
    ReplacerNestedVars(Allocator &_al) : al(_al) {}

    void replace_Var(ASR::Var_t* x) {
        if (nested_var_to_ext_var.find(x->m_v) != nested_var_to_ext_var.end()) {
            std::string m_name = nested_var_to_ext_var[x->m_v].first;
            ASR::symbol_t *t = nested_var_to_ext_var[x->m_v].second;
            char *fn_name = ASRUtils::symbol_name(t);
            std::string sym_name = fn_name;
            if (current_scope->get_symbol(sym_name) != nullptr) {
                if (!skip_replace) {
                    x->m_v = current_scope->get_symbol(sym_name);
                }
                return;
            }
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                al, t->base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ fn_name,
                t,
                s2c(al, m_name), nullptr, 0, fn_name,
                ASR::accessType::Public
                );
            ASR::symbol_t *ext_sym = ASR::down_cast<ASR::symbol_t>(fn);
            current_scope->add_symbol(sym_name, ext_sym);
            if (!skip_replace) {
                x->m_v = ext_sym;
            }
        }
    }
};

class ReplaceNestedVisitor: public ASR::CallReplacerOnExpressionsVisitor<ReplaceNestedVisitor> {
    private:

    Allocator& al;
    ReplacerNestedVars replacer;

    public:
    size_t nesting_depth = 0;
    bool is_in_call = false;
    std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> &nesting_map;
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> nested_var_to_ext_var;

    ReplaceNestedVisitor(Allocator& al_,
        std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> &n_map) : al(al_),
        replacer(al_), nesting_map(n_map) {}


    void call_replacer() {
        // Skip replacing original variables with context variables
        // in the parent function except for function/subroutine calls
        bool skip_replace = false;
        if (nesting_depth==1 && !is_in_call) skip_replace = true;
        replacer.current_expr = current_expr;
        replacer.current_scope = current_scope;
        replacer.skip_replace = skip_replace;
        replacer.replace_expr(*current_expr);
    }


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        current_scope = x.m_global_scope;
        SymbolTable* current_scope_copy = current_scope;

        // Add the nested vars by creating a new module

        for (auto &it: nesting_map) {
            // Iterate on each function with nested vars and create a context in
            // a new module.
            current_scope = al.make_new<SymbolTable>(current_scope_copy);
            std::string module_name = "__lcompilers_created__nested_context__" + std::string(
                                    ASRUtils::symbol_name(it.first)) + "_";
            std::map<ASR::symbol_t*, std::string> sym_to_name;
            module_name = current_scope->get_unique_name(module_name);
            for (auto &it2: it.second) {
                std::string new_ext_var = module_name + std::string(ASRUtils::symbol_name(it2));
                ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(
                            ASRUtils::symbol_get_past_external(it2));
                new_ext_var = current_scope->get_unique_name(new_ext_var);
                ASR::ttype_t* var_type = ASRUtils::type_get_past_pointer(var->m_type);
                if( ASR::is_a<ASR::Struct_t>(*var_type) ) {
                    ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(var_type);
                    if( current_scope->get_counter() != ASRUtils::symbol_parent_symtab(
                            struct_t->m_derived_type)->get_counter() ) {
                        ASR::symbol_t* m_derived_type = current_scope->get_symbol(
                            ASRUtils::symbol_name(struct_t->m_derived_type));
                        if( m_derived_type == nullptr ) {
                            char* fn_name = ASRUtils::symbol_name(struct_t->m_derived_type);
                            ASR::symbol_t* original_symbol = ASRUtils::symbol_get_past_external(struct_t->m_derived_type);
                            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                                al, struct_t->m_derived_type->base.loc,
                                /* a_symtab */ current_scope,
                                /* a_name */ fn_name,
                                original_symbol,
                                ASRUtils::symbol_name(ASRUtils::get_asr_owner(original_symbol)),
                                nullptr, 0, fn_name, ASR::accessType::Public
                            );
                            m_derived_type = ASR::down_cast<ASR::symbol_t>(fn);
                            current_scope->add_symbol(fn_name, m_derived_type);
                        }
                        var_type = ASRUtils::TYPE(ASR::make_Struct_t(al, struct_t->base.base.loc,
                                    m_derived_type, struct_t->m_dims, struct_t->n_dims));
                        if( ASR::is_a<ASR::Pointer_t>(*var->m_type) ) {
                            var_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, var_type->base.loc, var_type));
                        }
                    }
                }
                if( ASRUtils::is_array(var_type) &&
                    !ASR::is_a<ASR::Pointer_t>(*var_type) ) {
                    var_type = ASRUtils::duplicate_type_with_empty_dims(al, var_type);
                    var_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, var_type->base.loc, var_type));
                }
                ASR::expr_t *sym_expr = PassUtils::create_auxiliary_variable(
                        it2->base.loc, new_ext_var,
                        al, current_scope, var_type, ASR::intentType::Unspecified);
                ASR::symbol_t* sym = ASR::down_cast<ASR::Var_t>(sym_expr)->m_v;
                nested_var_to_ext_var[it2] = std::make_pair(module_name, sym);
            }
            ASR::asr_t *tmp = ASR::make_Module_t(al, x.base.base.loc,
                                            /* a_symtab */ current_scope,
                                            /* a_name */ s2c(al, module_name),
                                            nullptr,
                                            0,
                                            false, false);
            ASR::symbol_t* mod_sym = ASR::down_cast<ASR::symbol_t>(tmp);
            x.m_global_scope->add_symbol(module_name, mod_sym);
        }
        replacer.nested_var_to_ext_var = nested_var_to_ext_var;

        current_scope = x.m_global_scope;
        for (auto &a : x.m_global_scope->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
    }

    void visit_Program(const ASR::Program_t &x) {
        nesting_depth++;
        ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        transform_stmts(xx.m_body, xx.n_body);
        current_scope = current_scope_copy;
        nesting_depth--;
    }

    void visit_Function(const ASR::Function_t &x) {
        nesting_depth++;
        ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        visit_ttype(*x.m_function_signature);
        for (size_t i=0; i<x.n_args; i++) {
            ASR::expr_t** current_expr_copy_0 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_args[i]));
            call_replacer();
            current_expr = current_expr_copy_0;
            if( x.m_args[i] )
            visit_expr(*x.m_args[i]);
        }
        transform_stmts(xx.m_body, xx.n_body);
        if (x.m_return_var) {
            ASR::expr_t** current_expr_copy_1 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_return_var));
            call_replacer();
            current_expr = current_expr_copy_1;
            if( x.m_return_var )
            visit_expr(*x.m_return_var);
        }
        current_scope = current_scope_copy;
        nesting_depth--;
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        bool is_in_call_copy = is_in_call;
        is_in_call = true;
        for (size_t i=0; i<x.n_args; i++) {
            visit_call_arg(x.m_args[i]);
        }
        is_in_call = is_in_call_copy;
        visit_ttype(*x.m_type);
        if (x.m_value) {
            ASR::expr_t** current_expr_copy_118 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            call_replacer();
            current_expr = current_expr_copy_118;
            if( x.m_value )
            visit_expr(*x.m_value);
        }
        if (x.m_dt) {
            ASR::expr_t** current_expr_copy_119 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_dt));
            call_replacer();
            current_expr = current_expr_copy_119;
            if( x.m_dt )
            visit_expr(*x.m_dt);
        }
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        bool is_in_call_copy = is_in_call;
        is_in_call = true;
        for (size_t i=0; i<x.n_args; i++) {
            visit_call_arg(x.m_args[i]);
        }
        is_in_call = is_in_call_copy;
        if (x.m_dt) {
            ASR::expr_t** current_expr_copy_83 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_dt));
            call_replacer();
            current_expr = current_expr_copy_83;
            if( x.m_dt )
            visit_expr(*x.m_dt);
        }
    }

};

class AssignNestedVars: public PassUtils::PassVisitor<AssignNestedVars> {
public:
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> &nested_var_to_ext_var;
    std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> &nesting_map;

    ASR::symbol_t *cur_func_sym = nullptr;
    bool calls_present = false;

    AssignNestedVars(Allocator &al_,
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> &nv,
    std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> &nm) :
    PassVisitor(al_, nullptr), nested_var_to_ext_var(nv), nesting_map(nm)
    {
        pass_result.reserve(al, 1);
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        std::vector<ASR::stmt_t*> assigns_at_end;
        if (pass_result.size() > 0) {
            asr_changed = true;
            for (size_t j=0; j < pass_result.size(); j++) {
                body.push_back(al, pass_result[j]);
            }
            pass_result.n = 0;
        }
        for (size_t i=0; i<n_body; i++) {
            pass_result.n = 0;
            retain_original_stmt = false;
            remove_original_stmt = false;
            calls_present = false;
            assigns_at_end.clear();
            visit_stmt(*m_body[i]);
            if (cur_func_sym != nullptr && calls_present) {
                if (nesting_map.find(cur_func_sym) != nesting_map.end()) {
                    for (auto &sym: nesting_map[cur_func_sym]) {
                        std::string m_name = nested_var_to_ext_var[sym].first;
                        ASR::symbol_t *t = nested_var_to_ext_var[sym].second;
                        char *fn_name = ASRUtils::symbol_name(t);
                        std::string sym_name_ext = fn_name;
                        ASR::symbol_t *ext_sym = current_scope->get_symbol(sym_name_ext);
                        if (ext_sym == nullptr) {
                            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                                al, t->base.loc,
                                /* a_symtab */ current_scope,
                                /* a_name */ fn_name,
                                t,
                                s2c(al, m_name), nullptr, 0, fn_name,
                                ASR::accessType::Public
                            );
                            ext_sym = ASR::down_cast<ASR::symbol_t>(fn);
                            current_scope->add_symbol(sym_name_ext, ext_sym);
                        }
                        ASR::symbol_t* sym_ = sym;
                        if( current_scope->get_counter() != ASRUtils::symbol_parent_symtab(sym_)->get_counter() ) {
                            std::string sym_name = ASRUtils::symbol_name(sym_);
                            sym_ = current_scope->get_symbol(sym_name);
                            if( !sym_ ) {
                                ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                                    al, t->base.loc,
                                    /* a_symtab */ current_scope,
                                    /* a_name */ s2c(al, current_scope->get_unique_name(sym_name)),
                                    ASRUtils::symbol_get_past_external(sym),
                                    ASRUtils::symbol_name(ASRUtils::get_asr_owner(ASRUtils::symbol_get_past_external(sym))),
                                    nullptr, 0, s2c(al, sym_name), ASR::accessType::Public
                                );
                                sym_ = ASR::down_cast<ASR::symbol_t>(fn);
                                current_scope->add_symbol(sym_name, sym_);
                            }
                        }
                        LCOMPILERS_ASSERT(ext_sym != nullptr);
                        LCOMPILERS_ASSERT(sym_ != nullptr);
                        ASR::expr_t *target = ASRUtils::EXPR(ASR::make_Var_t(al, t->base.loc, ext_sym));
                        ASR::expr_t *val = ASRUtils::EXPR(ASR::make_Var_t(al, t->base.loc, sym_));
                        if( ASRUtils::is_array(ASRUtils::symbol_type(sym)) ||
                            ASR::is_a<ASR::Pointer_t>(*ASRUtils::symbol_type(sym)) ) {
                            ASR::stmt_t *associate = ASRUtils::STMT(ASR::make_Associate_t(al, t->base.loc,
                                                        target, val));
                            body.push_back(al, associate);
                        } else {
                            ASR::stmt_t *assignment = ASRUtils::STMT(ASR::make_Assignment_t(al, t->base.loc,
                                                        target, val, nullptr));
                            body.push_back(al, assignment);
                            assignment = ASRUtils::STMT(ASR::make_Assignment_t(al, t->base.loc,
                                            val, target, nullptr));
                            assigns_at_end.push_back(assignment);
                        }
                    }
                }
            }
            if (pass_result.size() > 0) {
                asr_changed = true;
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                if( retain_original_stmt ) {
                    body.push_back(al, m_body[i]);
                    retain_original_stmt = false;
                }
                pass_result.n = 0;
            } else if(!remove_original_stmt) {
                body.push_back(al, m_body[i]);
            }
            if (!assigns_at_end.empty()) {
                for (auto &stm: assigns_at_end) {
                    body.push_back(al, stm);
                }
            }
        }
        m_body = body.p;
        n_body = body.size();
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        ASR::symbol_t *sym_copy = cur_func_sym;
        cur_func_sym = (ASR::symbol_t*)&xx;
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);

        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
            if (ASR::is_a<ASR::Block_t>(*item.second)) {
                ASR::Block_t *s = ASR::down_cast<ASR::Block_t>(item.second);
                visit_Block(*s);
            }
            if (ASR::is_a<ASR::AssociateBlock_t>(*item.second)) {
                ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                visit_AssociateBlock(*s);
            }
        }
        cur_func_sym = sym_copy;
        current_scope = current_scope_copy;
    }

    void visit_Program(const ASR::Program_t &x) {
        ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = xx.m_symtab;
        ASR::symbol_t *sym_copy = cur_func_sym;
        cur_func_sym = (ASR::symbol_t*)&xx;
        transform_stmts(xx.m_body, xx.n_body);

        // Transform nested functions and subroutines
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
            if (ASR::is_a<ASR::AssociateBlock_t>(*item.second)) {
                ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                visit_AssociateBlock(*s);
            }
            if (ASR::is_a<ASR::Block_t>(*item.second)) {
                ASR::Block_t *s = ASR::down_cast<ASR::Block_t>(item.second);
                visit_Block(*s);
            }
        }
        current_scope = current_scope_copy;
        cur_func_sym = sym_copy;
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        calls_present = true;
        for (size_t i=0; i<x.n_args; i++) {
            visit_call_arg(x.m_args[i]);
        }
        visit_ttype(*x.m_type);
        if (x.m_value)
            visit_expr(*x.m_value);
        if (x.m_dt)
            visit_expr(*x.m_dt);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        calls_present = true;
        for (size_t i=0; i<x.n_args; i++) {
            visit_call_arg(x.m_args[i]);
        }
        if (x.m_dt)
            visit_expr(*x.m_dt);
    }
};

void pass_nested_vars(Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    NestedVarVisitor v(al);
    v.visit_TranslationUnit(unit);
    ReplaceNestedVisitor w(al, v.nesting_map);
    w.visit_TranslationUnit(unit);
    AssignNestedVars z(al, w.nested_var_to_ext_var, w.nesting_map);
    z.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LCompilers
