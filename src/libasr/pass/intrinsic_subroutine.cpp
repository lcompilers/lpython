#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_intrinsic_subroutine.h>
#include <libasr/pass/intrinsic_subroutine_registry.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LCompilers {

/*

This ASR pass replaces the IntrinsicSubroutine node with a call to an
implementation in ASR that we construct (and cache) on the fly for the actual
arguments.

Call this pass if you do not want to implement intrinsic subroutines directly
in the backend.

*/

class ReplaceIntrinsicSubroutines : public ASR::CallReplacerOnExpressionsVisitor<ReplaceIntrinsicSubroutines>
{
    private:

        Allocator& al;
        SymbolTable* global_scope;
        bool remove_original_statement;
        Vec<ASR::stmt_t*> pass_result;
        Vec<ASR::stmt_t*>* parent_body;

    public:

        ReplaceIntrinsicSubroutines(Allocator& al_) :
        al(al_), remove_original_statement(false) {
            parent_body = nullptr;
            pass_result.n = 0;
        }

        void visit_IntrinsicImpureSubroutine(const ASR::IntrinsicImpureSubroutine_t &x) {
            Vec<ASR::call_arg_t> new_args; new_args.reserve(al, x.n_args);
            // Replace any IntrinsicImpureSubroutines in the argument first:
            for( size_t i = 0; i < x.n_args; i++ ) {
                ASR::call_arg_t arg0;
                arg0.loc = x.m_args[i]->base.loc;
                arg0.m_value = x.m_args[i]; // Use the converted arg
                new_args.push_back(al, arg0);
            }
            ASRUtils::impl_subroutine instantiate_subroutine =
                ASRUtils::IntrinsicImpureSubroutineRegistry::get_instantiate_subroutine(x.m_sub_intrinsic_id);
            if( instantiate_subroutine == nullptr ) {
                return ;
            }
            Vec<ASR::ttype_t*> arg_types;
            arg_types.reserve(al, x.n_args);
            for( size_t i = 0; i < x.n_args; i++ ) {
                arg_types.push_back(al, ASRUtils::expr_type(x.m_args[i]));
            }
            ASR::stmt_t* subroutine_call = instantiate_subroutine(al, x.base.base.loc,
                global_scope, arg_types, new_args, x.m_overload_id);
            remove_original_statement = true;
            pass_result.push_back(al, subroutine_call);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            bool remove_original_statement_copy = remove_original_statement;
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            if( parent_body ) {
                for (size_t j=0; j < pass_result.size(); j++) {
                    parent_body->push_back(al, pass_result[j]);
                }
            }
            for (size_t i=0; i<n_body; i++) {
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                remove_original_statement = false;
                Vec<ASR::stmt_t*>* parent_body_copy = parent_body;
                parent_body = &body;
                visit_stmt(*m_body[i]);
                parent_body = parent_body_copy;
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                if( !remove_original_statement ) {
                    body.push_back(al, m_body[i]);
                }
            }
            m_body = body.p;
            n_body = body.size();
            pass_result.n = 0;
            remove_original_statement = remove_original_statement_copy;
        }

        // TODO: Only Program and While is processed, we need to process all calls
        // to visit_stmt().
        // TODO: Only TranslationUnit's and Program's symbol table is processed
        // for transforming function->subroutine if they return arrays
        void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_symtab;

            global_scope = x.m_symtab;
            while( global_scope->parent ) {
                global_scope = global_scope->parent;
            }

            std::vector<std::string> build_order
                = ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_symtab->get_symbol(item));
                ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                visit_symbol(*mod);
            }

            // Now visit everything else
            for (auto &item : x.m_symtab->get_scope()) {
                if (!ASR::is_a<ASR::Module_t>(*item.second)) {
                    this->visit_symbol(*item.second);
                }
            }
            current_scope = current_scope_copy;
        }

        void visit_Module(const ASR::Module_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_symtab;

            global_scope = x.m_symtab;
            while( global_scope->parent ) {
                global_scope = global_scope->parent;
            }

            // Now visit everything else
            for (auto &item : x.m_symtab->get_scope()) {
                this->visit_symbol(*item.second);
            }
            current_scope = current_scope_copy;
        }

        void visit_Program(const ASR::Program_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            SymbolTable* current_scope_copy = current_scope;
            current_scope = xx.m_symtab;
            global_scope = xx.m_symtab;

            while( global_scope->parent ) {
                global_scope = global_scope->parent;
            }

            for (auto &item : x.m_symtab->get_scope()) {
                if (ASR::is_a<ASR::AssociateBlock_t>(*item.second)) {
                    ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                    visit_AssociateBlock(*s);
                }
                if (ASR::is_a<ASR::Function_t>(*item.second)) {
                    visit_Function(*ASR::down_cast<ASR::Function_t>(
                        item.second));
                }
            }

            transform_stmts(xx.m_body, xx.n_body);
            current_scope = current_scope_copy;
        }

};

void pass_replace_intrinsic_subroutine(Allocator &al, ASR::TranslationUnit_t &unit,
                             const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceIntrinsicSubroutines v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
