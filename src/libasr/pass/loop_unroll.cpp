#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/loop_unroll.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <map>
#include <utility>
#include <cmath>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

class LoopUnrollVisitor : public PassUtils::PassVisitor<LoopUnrollVisitor>
{
private:

    std::string rl_path;

    int64_t unroll_factor;

    ASR::ExprStmtDuplicator node_duplicator;

public:

    LoopUnrollVisitor(Allocator &al_, const std::string& rl_path_,
                      size_t unroll_factor_) :
    PassVisitor(al_, nullptr), rl_path(rl_path_),
    unroll_factor(unroll_factor_), node_duplicator(al_)
    {
        pass_result.reserve(al, 1);
    }

    void visit_DoLoop(const ASR::DoLoop_t& x) {
        ASR::DoLoop_t& xx = const_cast<ASR::DoLoop_t&>(x);
        ASR::do_loop_head_t x_head = x.m_head;
        ASR::expr_t* x_start = ASRUtils::expr_value(x_head.m_start);
        ASR::expr_t* x_end = ASRUtils::expr_value(x_head.m_end);
        ASR::expr_t* x_inc = nullptr;
        if( x_head.m_increment ) {
            x_inc = ASRUtils::expr_value(x_head.m_increment);
        } else {
            ASR::ttype_t* int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
            x_inc = ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, x_head.m_end->base.loc, 1, int32_type));
        }

        int64_t _start, _end, _inc;
        if( !ASRUtils::is_value_constant(x_start, _start) ||
            !ASRUtils::is_value_constant(x_end, _end) ||
            !ASRUtils::is_value_constant(x_inc, _inc) ) {
            return ;
        }
        int64_t loop_size = std::ceil( ((float) (_end - _start + 1)) / ((float) _inc) );
        int64_t unroll_factor_ = std::min(unroll_factor, loop_size);
        bool create_unrolled_loop = unroll_factor_ < loop_size;
        int64_t new_end = unroll_factor_ * (loop_size / unroll_factor_);
        int64_t remaining_part = loop_size % unroll_factor_;
        ASR::ttype_t *int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                                                            4, nullptr, 0));
        xx.m_head.m_end = ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, x_end->base.loc, new_end, int32_type));

        Vec<ASR::stmt_t*> init_and_whileloop = PassUtils::replace_doloop(al, x);
        ASR::stmt_t* whileloop_stmt = init_and_whileloop[1];
        ASR::WhileLoop_t* whileloop = ASR::down_cast<ASR::WhileLoop_t>(whileloop_stmt);
        ASR::stmt_t* init_stmt = init_and_whileloop[0];

        Vec<ASR::stmt_t*> unrolled_loop;
        unrolled_loop.reserve(al, whileloop->n_body * unroll_factor_);
        for( size_t i = 0; i < whileloop->n_body; i++ ) {
            unrolled_loop.push_back(al, whileloop->m_body[i]);
        }

        for( int64_t j = 1; j < unroll_factor_; j++ ) {
            for( size_t i = 0; i < whileloop->n_body; i++ ) {
                node_duplicator.success = true;
                ASR::stmt_t* m_body_copy = node_duplicator.duplicate_stmt(whileloop->m_body[i]);
                if( !node_duplicator.success ) {
                    return ;
                }
                unrolled_loop.push_back(al, m_body_copy);
            }
        }

        pass_result.push_back(al, init_stmt);
        if( create_unrolled_loop ) {
            ASR::stmt_t* unrolled_whileloop = ASRUtils::STMT(ASR::make_WhileLoop_t(al, x.base.base.loc,
                                                             whileloop->m_test, unrolled_loop.p, unrolled_loop.size()));
            pass_result.push_back(al, unrolled_whileloop);
            for( int64_t i = 0; i < remaining_part; i++ ) {
                for( size_t i = 0; i < whileloop->n_body; i++ ) {
                    ASR::stmt_t* m_body_copy = node_duplicator.duplicate_stmt(whileloop->m_body[i]);
                    pass_result.push_back(al, m_body_copy);
                }
            }
        } else {
            for( size_t i = 0; i < unrolled_loop.size(); i++ ) {
                pass_result.push_back(al, unrolled_loop[i]);
            }
        }
    }

};

void pass_loop_unroll(Allocator &al, ASR::TranslationUnit_t &unit,
                      const std::string& rl_path,
                      int64_t unroll_factor) {
    LoopUnrollVisitor v(al, rl_path, unroll_factor);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
