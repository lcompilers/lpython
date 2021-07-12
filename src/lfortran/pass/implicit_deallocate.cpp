#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/array_op.h>
#include <lfortran/pass/pass_utils.h>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

class InsertImplicitDeallocateVisitor : public ASR::BaseWalkVisitor<InsertImplicitDeallocateVisitor>
{
private:
    Allocator &al;
    ASR::TranslationUnit_t &unit;

    /*
        It stores the address of symbol table of current scope,
        which can be program, function, subroutine or even
        global scope.
    */
    SymbolTable* current_scope;

public:
    InsertImplicitDeallocateVisitor(Allocator &al, ASR::TranslationUnit_t &unit) : 
    al{al}, unit{unit}, current_scope{nullptr} {
    }

    ASR::stmt_t* create_implicit_deallocate(const Location& loc) {
        Vec<ASR::symbol_t*> del_syms;
        del_syms.reserve(al, 0);
        for( auto& item: current_scope->scope ) {
            if( item.second->type == ASR::symbolType::Variable ) {
                const ASR::symbol_t* sym = LFortran::ASRUtils::symbol_get_past_external(item.second);
                ASR::Variable_t* var = down_cast<ASR::Variable_t>(sym);
                if( var->m_storage == ASR::storage_typeType::Allocatable && 
                    var->m_intent == ASR::intentType::Local ) {
                    del_syms.push_back(al, item.second);
                }
            }
        }
        if( del_syms.size() == 0 ) {
            return nullptr;
        }
        return LFortran::ASRUtils::STMT(ASR::make_ImplicitDeallocate_t(al, loc, 
                    del_syms.p, del_syms.size()));
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            visit_stmt(*m_body[i]);
            if( m_body[i]->type == ASR::stmtType::Return ) {
                ASR::stmt_t* impl_del = create_implicit_deallocate(m_body[i]->base.loc);
                if( impl_del != nullptr ) {
                    body.push_back(al, impl_del);
                }
            }
            body.push_back(al, m_body[i]);
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
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);

        // Transform nested functions and subroutines
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Subroutine_t &xx = const_cast<ASR::Subroutine_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);
    }

};

void pass_insert_implicit_deallocate(Allocator &al, ASR::TranslationUnit_t &unit) {
    InsertImplicitDeallocateVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
