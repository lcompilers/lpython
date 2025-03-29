#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/insert_deallocate.h>

#include <vector>
#include <utility>


namespace LCompilers {

class InsertDeallocate: public ASR::CallReplacerOnExpressionsVisitor<InsertDeallocate>
{
    private:

        Allocator& al;

    public:

        InsertDeallocate(Allocator& al_) : al(al_) {}

        template <typename T>
        void visit_Symbol(const T& x) {
            Vec<ASR::expr_t*> to_be_deallocated;
            to_be_deallocated.reserve(al, 1);
            for( auto& itr: x.m_symtab->get_scope() ) {
                if( ASR::is_a<ASR::Variable_t>(*itr.second) &&
                    ASR::is_a<ASR::Allocatable_t>(*ASRUtils::symbol_type(itr.second)) &&
                    ASRUtils::is_array(ASRUtils::symbol_type(itr.second)) &&
                    ASRUtils::symbol_intent(itr.second) == ASRUtils::intent_local ) {
                    to_be_deallocated.push_back(al, ASRUtils::EXPR(
                        ASR::make_Var_t(al, x.base.base.loc, itr.second)));
                }
            }
            if( to_be_deallocated.size() > 0 ) {
                T& xx = const_cast<T&>(x);
                Vec<ASR::stmt_t*> body;
                body.from_pointer_n_copy(al, xx.m_body, xx.n_body);
                body.push_back(al, ASRUtils::STMT(ASR::make_ImplicitDeallocate_t(
                    al, x.base.base.loc, to_be_deallocated.p, to_be_deallocated.size())));
                xx.m_body = body.p;
                xx.n_body = body.size();
            }
        }

        void visit_Function(const ASR::Function_t& x) {
            visit_Symbol(x);
            ASR::CallReplacerOnExpressionsVisitor<InsertDeallocate>::visit_Function(x);
        }

        void visit_Program(const ASR::Program_t& x) {
            visit_Symbol(x);
            ASR::CallReplacerOnExpressionsVisitor<InsertDeallocate>::visit_Program(x);
        }

};

void pass_insert_deallocate(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &/*pass_options*/) {
    InsertDeallocate v(al);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
