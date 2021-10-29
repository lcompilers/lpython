#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/for_all.h>


namespace LFortran {

/*
 * This ASR pass replaces forall statement with Do Concurrent.
 *
 * Converts:
 *      forall(i=start:end:inc) array(i) = i
 *
 * to:
 *      do concurrent (i=start:end:inc)
 *          array(i) = i
 *      end do
 */

class ForAllVisitor : public ASR::BaseWalkVisitor<ForAllVisitor>
{
private:
    Allocator &al;
    Vec<ASR::stmt_t*> for_all_result;

public:
    ForAllVisitor(Allocator &al) : al{al} {
        for_all_result.n = 0;
    }

    void visit_ForAllSingle(const ASR::ForAllSingle_t &x) {
        Location loc = x.base.base.loc;
        ASR::stmt_t *assign_stmt = x.m_assign_stmt;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, 1);
        body.push_back(al, assign_stmt);

        ASR::stmt_t *stmt = LFortran::ASRUtils::STMT(
            ASR::make_DoConcurrentLoop_t(al, loc, x.m_head, body.p, body.size())
        );
        Vec<ASR::stmt_t*> result;
        result.reserve(al, 1);
        result.push_back(al, stmt);
        for_all_result = result;
    }
};

void pass_replace_forall(Allocator &al, ASR::TranslationUnit_t &unit) {
    ForAllVisitor v(al);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran
