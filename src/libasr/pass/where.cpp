#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_where.h>


namespace LCompilers {

/*
This ASR pass replaces where with do loops and array expression assignments.
The function `pass_replace_where` transforms the ASR tree in-place.

Converts:

    where(a > b)
        a = 2.0
    else where(a == 2.0)
        b = 3.0
    else where
        a = b * 2.0 / x * 3.0
    endwhere

to:

    do i = lbound(1, a), ubound(1, a)
        if (a(i) > b(i))
            a(i) = 2.0
        else if (a(i) == 2.0)
            b(i) = 3.0
        else
            a(i) = b(i) * 2.0 / x(i) * 3.0
        end if
    end do
*/

class TransformWhereVisitor: public ASR::CallReplacerOnExpressionsVisitor<TransformWhereVisitor> {
    private:

    Allocator& al;
    Vec<ASR::stmt_t*> pass_result;
    Vec<ASR::stmt_t*>* parent_body;

    public:

    TransformWhereVisitor(Allocator& al_):
        al(al_), parent_body(nullptr) {
        pass_result.n = 0;
        pass_result.reserve(al, 0);
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, 1);
        for (size_t i = 0; i < n_body; i++) {
            pass_result.n = 0;
            pass_result.reserve(al, 1);
            Vec<ASR::stmt_t*>* parent_body_copy = parent_body;
            parent_body = &body;
            visit_stmt(*m_body[i]);
            parent_body = parent_body_copy;
            if( pass_result.size() > 0 ) {
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
            } else {
                body.push_back(al, m_body[i]);
            }
        }
        m_body = body.p;
        n_body = body.size();
        pass_result.n = 0;
    }

    ASR::stmt_t* transform_Where_to_If(const ASR::Where_t& x) {
        Vec<ASR::stmt_t*> or_else_vec; or_else_vec.reserve(al, x.n_orelse);
        Vec<ASR::stmt_t*> body_vec; body_vec.reserve(al, x.n_body);
        for( size_t i = 0; i < x.n_body; i++ ) {
            if( ASR::is_a<ASR::Associate_t>(*x.m_body[i]) ) {
                LCOMPILERS_ASSERT(parent_body != nullptr);
                parent_body->push_back(al, x.m_body[i]);
            } else if( ASR::is_a<ASR::Where_t>(*x.m_body[i]) ) {
                ASR::stmt_t* body_stmt = transform_Where_to_If(
                    *ASR::down_cast<ASR::Where_t>(x.m_body[i]));
                body_vec.push_back(al, body_stmt);
            } else {
                body_vec.push_back(al, x.m_body[i]);
            }
        }
        for( size_t i = 0; i < x.n_orelse; i++ ) {
            if( ASR::is_a<ASR::Associate_t>(*x.m_orelse[i]) ) {
                LCOMPILERS_ASSERT(parent_body != nullptr);
                parent_body->push_back(al, x.m_orelse[i]);
            } else if( ASR::is_a<ASR::Where_t>(*x.m_orelse[i]) ) {
                ASR::stmt_t* or_else_stmt = transform_Where_to_If(
                    *ASR::down_cast<ASR::Where_t>(x.m_orelse[i]));
                or_else_vec.push_back(al, or_else_stmt);
            } else {
                or_else_vec.push_back(al, x.m_orelse[i]);
            }
        }

        return ASRUtils::STMT(ASR::make_If_t(al, x.base.base.loc,
            x.m_test, body_vec.p, body_vec.size(), or_else_vec.p, or_else_vec.size()));
    }

    void visit_Where(const ASR::Where_t& x) {
        ASR::stmt_t* if_stmt = transform_Where_to_If(x);
        pass_result.push_back(al, if_stmt);
    }

};

void pass_replace_where(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/) {
    TransformWhereVisitor v(al);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
