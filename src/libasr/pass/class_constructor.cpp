#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/class_constructor.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>
#include <queue>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class ReplaceStructTypeConstructor: public ASR::BaseExprReplacer<ReplaceStructTypeConstructor> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;
    bool& remove_original_statement;
    bool& inside_symtab;
    std::map<SymbolTable*, Vec<ASR::stmt_t*>>& symtab2decls;

    public:

    SymbolTable* current_scope;
    ASR::expr_t* result_var;

    ReplaceStructTypeConstructor(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
        bool& remove_original_statement_, bool& inside_symtab_,
        std::map<SymbolTable*, Vec<ASR::stmt_t*>>& symtab2decls_) :
    al(al_), pass_result(pass_result_),
    remove_original_statement(remove_original_statement_),
    inside_symtab(inside_symtab_), symtab2decls(symtab2decls_),
    current_scope(nullptr), result_var(nullptr) {}

    void replace_StructTypeConstructor(ASR::StructTypeConstructor_t* x) {
        if( x->n_args == 0 ) {
            remove_original_statement = true;
            return ;
        }
        if( result_var == nullptr ) {
            std::string result_var_name = current_scope->get_unique_name("temp_struct_var__");
            result_var = PassUtils::create_auxiliary_variable(x->base.base.loc,
                                result_var_name, al, current_scope, x->m_type);
            *current_expr = result_var;
        } else {
            if( inside_symtab ) {
                *current_expr = nullptr;
            } else {
                remove_original_statement = true;
            }
        }

        std::deque<ASR::symbol_t*> constructor_arg_syms;
        ASR::Struct_t* dt_der = ASR::down_cast<ASR::Struct_t>(x->m_type);
        ASR::StructType_t* dt_dertype = ASR::down_cast<ASR::StructType_t>(
                                         ASRUtils::symbol_get_past_external(dt_der->m_derived_type));
        while( dt_dertype ) {
            for( int i = (int) dt_dertype->n_members - 1; i >= 0; i-- ) {
                constructor_arg_syms.push_front(
                    dt_dertype->m_symtab->get_symbol(
                        dt_dertype->m_members[i]));
            }
            if( dt_dertype->m_parent != nullptr ) {
                ASR::symbol_t* dt_der_sym = ASRUtils::symbol_get_past_external(
                                        dt_dertype->m_parent);
                LCOMPILERS_ASSERT(ASR::is_a<ASR::StructType_t>(*dt_der_sym));
                dt_dertype = ASR::down_cast<ASR::StructType_t>(dt_der_sym);
            } else {
                dt_dertype = nullptr;
            }
        }
        LCOMPILERS_ASSERT(constructor_arg_syms.size() == x->n_args);

        for( size_t i = 0; i < x->n_args; i++ ) {
            if( x->m_args[i].m_value == nullptr ) {
                 continue ;
            }
            ASR::symbol_t* member = constructor_arg_syms[i];
            if( ASR::is_a<ASR::StructTypeConstructor_t>(*x->m_args[i].m_value) ) {
                ASR::expr_t* result_var_copy = result_var;
                ASR::symbol_t *v = nullptr;
                if (ASR::is_a<ASR::Var_t>(*result_var_copy)) {
                    v = ASR::down_cast<ASR::Var_t>(result_var_copy)->m_v;
                }
                result_var = ASRUtils::EXPR(ASRUtils::getStructInstanceMember_t(al,
                                            x->base.base.loc, (ASR::asr_t*) result_var_copy, v,
                                            member, current_scope));
                ASR::expr_t** current_expr_copy = current_expr;
                current_expr = &(x->m_args[i].m_value);
                replace_expr(x->m_args[i].m_value);
                current_expr = current_expr_copy;
                result_var = result_var_copy;
            } else {
                Vec<ASR::stmt_t*>* result_vec = nullptr;
                if( inside_symtab ) {
                    if( symtab2decls.find(current_scope) == symtab2decls.end() ) {
                        Vec<ASR::stmt_t*> result_vec_;
                        result_vec_.reserve(al, 0);
                        symtab2decls[current_scope] = result_vec_;
                    }
                    result_vec = &symtab2decls[current_scope];
                } else {
                    result_vec = &pass_result;
                }
                ASR::symbol_t *v = nullptr;
                if (ASR::is_a<ASR::Var_t>(*result_var)) {
                    v = ASR::down_cast<ASR::Var_t>(result_var)->m_v;
                }
                ASR::expr_t* derived_ref = ASRUtils::EXPR(ASRUtils::getStructInstanceMember_t(al,
                                                x->base.base.loc, (ASR::asr_t*) result_var, v,
                                                member, current_scope));
                ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al,
                                            x->base.base.loc, derived_ref,
                                            x->m_args[i].m_value, nullptr));
                result_vec->push_back(al, assign);
            }
        }
    }
};

class StructTypeConstructorVisitor : public ASR::CallReplacerOnExpressionsVisitor<StructTypeConstructorVisitor>
{
    private:

        Allocator& al;
        bool remove_original_statement;
        bool inside_symtab;
        ReplaceStructTypeConstructor replacer;
        Vec<ASR::stmt_t*> pass_result;
        std::map<SymbolTable*, Vec<ASR::stmt_t*>> symtab2decls;

    public:

        StructTypeConstructorVisitor(Allocator& al_) :
        al(al_), remove_original_statement(false),
        inside_symtab(true), replacer(al_, pass_result,
            remove_original_statement, inside_symtab,
            symtab2decls) {
            pass_result.n = 0;
            pass_result.reserve(al, 0);
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            bool inside_symtab_copy = inside_symtab;
            inside_symtab = false;
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);

            if( symtab2decls.find(current_scope) != symtab2decls.end() ) {
                Vec<ASR::stmt_t*>& decls = symtab2decls[current_scope];
                for (size_t j = 0; j < decls.size(); j++) {
                    body.push_back(al, decls[j]);
                }
                symtab2decls.erase(current_scope);
            }

            for (size_t i = 0; i < n_body; i++) {
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                remove_original_statement = false;
                replacer.result_var = nullptr;
                visit_stmt(*m_body[i]);
                for (size_t j = 0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                if( !remove_original_statement ) {
                    body.push_back(al, m_body[i]);
                }
                remove_original_statement = false;
            }
            m_body = body.p;
            n_body = body.size();
            replacer.result_var = nullptr;
            pass_result.n = 0;
            pass_result.reserve(al, 0);
            inside_symtab = inside_symtab_copy;
        }

        void visit_Variable(const ASR::Variable_t &x) {
            ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
            replacer.result_var = ASRUtils::EXPR(ASR::make_Var_t(al,
                                    x.base.base.loc, &(xx.base)));
            ASR::CallReplacerOnExpressionsVisitor<
                StructTypeConstructorVisitor>::visit_Variable(x);
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
                remove_original_statement = false;
                return ;
            }

            replacer.result_var = x.m_target;
            ASR::expr_t** current_expr_copy_9 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            this->call_replacer();
            current_expr = current_expr_copy_9;
            if( !remove_original_statement ) {
                this->visit_expr(*x.m_value);
            }
        }

};

void pass_replace_class_constructor(Allocator &al,
    ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    StructTypeConstructorVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}


} // namespace LCompilers
