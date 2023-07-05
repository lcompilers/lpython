#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/for_all.h>
#include <libasr/pass/stmt_walk_visitor.h>

namespace LCompilers {

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

class ForAllVisitor : public ASR::StatementWalkVisitor<ForAllVisitor>
{
public:
    ForAllVisitor(Allocator &al) : StatementWalkVisitor(al) {
    }

    void visit_ForAllSingle(const ASR::ForAllSingle_t &x) {
        Location loc = x.base.base.loc;
        ASR::stmt_t *assign_stmt = x.m_assign_stmt;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, 1);
        body.push_back(al, assign_stmt);

        ASR::stmt_t *stmt = ASRUtils::STMT(
            ASR::make_DoConcurrentLoop_t(al, loc, x.m_head, body.p, body.size())
        );
        Vec<ASR::stmt_t*> result;
        result.reserve(al, 1);
        result.push_back(al, stmt);
        pass_result = result;
    }
};

void pass_replace_for_all(Allocator &al, ASR::TranslationUnit_t &unit,
                         const LCompilers::PassOptions& /*pass_options*/) {
    ForAllVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LCompilers
