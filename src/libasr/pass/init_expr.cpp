#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/init_expr.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class ReplaceInitExpr: public ASR::BaseExprReplacer<ReplaceInitExpr> {

    public:

    Allocator& al;
    std::map<SymbolTable*, Vec<ASR::stmt_t*>>& symtab2decls;

    SymbolTable* current_scope;
    ASR::expr_t* result_var;

    ReplaceInitExpr(
        Allocator& al_,
        std::map<SymbolTable*, Vec<ASR::stmt_t*>>& symtab2decls_) :
    al(al_), symtab2decls(symtab2decls_),
    current_scope(nullptr), result_var(nullptr) {}

    void replace_ArrayConstant(ASR::ArrayConstant_t* x) {
        if( symtab2decls.find(current_scope) == symtab2decls.end() ) {
            Vec<ASR::stmt_t*> result_vec_;
            result_vec_.reserve(al, 0);
            symtab2decls[current_scope] = result_vec_;
        }
        Vec<ASR::stmt_t*>* result_vec = &symtab2decls[current_scope];
        bool remove_original_statement = false;
        PassUtils::ReplacerUtils::replace_ArrayConstant(x, this,
            remove_original_statement, result_vec);
    }

    void replace_StructTypeConstructor(ASR::StructTypeConstructor_t* x) {
        if( symtab2decls.find(current_scope) == symtab2decls.end() ) {
            Vec<ASR::stmt_t*> result_vec_;
            result_vec_.reserve(al, 0);
            symtab2decls[current_scope] = result_vec_;
        }
        Vec<ASR::stmt_t*>* result_vec = &symtab2decls[current_scope];
        bool remove_original_statement = false;
        PassUtils::ReplacerUtils::replace_StructTypeConstructor(
            x, this, true, remove_original_statement, result_vec);
        *current_expr = nullptr;
    }

};

class InitExprVisitor : public ASR::CallReplacerOnExpressionsVisitor<InitExprVisitor>
{
    private:

        Allocator& al;
        ReplaceInitExpr replacer;
        std::map<SymbolTable*, Vec<ASR::stmt_t*>> symtab2decls;

    public:

        InitExprVisitor(Allocator& al_) :
        al(al_), replacer(al_, symtab2decls) {
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
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
                body.push_back(al, m_body[i]);
            }
            m_body = body.p;
            n_body = body.size();
        }

        void visit_SymbolsContainingSymbolTable() {
            std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(current_scope);
            for (auto &a : var_order) {
                ASR::symbol_t* sym = current_scope->get_symbol(a);
                this->visit_symbol(*sym);
            }
        }

        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            SymbolTable* current_scope_copy = current_scope;
            current_scope = xx.m_symtab;
            visit_SymbolsContainingSymbolTable();
            transform_stmts(xx.m_body, xx.n_body);
            current_scope = current_scope_copy;
        }

        void visit_Function(const ASR::Function_t& x) {
            ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
            SymbolTable* current_scope_copy = current_scope;
            current_scope = xx.m_symtab;
            visit_SymbolsContainingSymbolTable();
            transform_stmts(xx.m_body, xx.n_body);
            current_scope = current_scope_copy;
        }

        void visit_Module(const ASR::Module_t& x) {
            ASR::Module_t& xx = const_cast<ASR::Module_t&>(x);
            SymbolTable* current_scope_copy = current_scope;
            current_scope = xx.m_symtab;
            visit_SymbolsContainingSymbolTable();
            current_scope = current_scope_copy;
        }

        void visit_Variable(const ASR::Variable_t &x) {
            ASR::symbol_t* asr_owner = ASRUtils::get_asr_owner(&(x.base));
            if( !(x.m_symbolic_value &&
                  (ASR::is_a<ASR::ArrayConstant_t>(*x.m_symbolic_value) ||
                   ASR::is_a<ASR::StructTypeConstructor_t>(*x.m_symbolic_value))) ||
                 (ASR::is_a<ASR::Module_t>(*asr_owner) &&
                  ASR::is_a<ASR::ArrayConstant_t>(*x.m_symbolic_value))) {
                return ;
            }

            ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
            replacer.result_var = ASRUtils::EXPR(ASR::make_Var_t(al,
                                    x.base.base.loc, &(xx.base)));

            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_parent_symtab;
            if (x.m_symbolic_value) {
                ASR::expr_t** current_expr_copy = current_expr;
                current_expr = const_cast<ASR::expr_t**>(&(x.m_symbolic_value));
                call_replacer();
                current_expr = current_expr_copy;
                if( x.m_symbolic_value )
                visit_expr(*x.m_symbolic_value);
            }
            visit_ttype(*x.m_type);
            current_scope = current_scope_copy;
        }

};

void pass_replace_init_expr(Allocator &al,
    ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    InitExprVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}


} // namespace LCompilers
