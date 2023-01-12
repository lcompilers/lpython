#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/dead_code_removal.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <map>
#include <utility>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;


class DeadCodeRemovalVisitor : public PassUtils::PassVisitor<DeadCodeRemovalVisitor>
{
private:

    std::string rl_path;

public:

    bool dead_code_removed;

    DeadCodeRemovalVisitor(Allocator &al_, const std::string& rl_path_) : PassVisitor(al_, nullptr),
    rl_path(rl_path_), dead_code_removed(false)
    {
        pass_result.reserve(al, 1);
    }

    void visit_If(const ASR::If_t& x) {
        ASR::If_t& xx = const_cast<ASR::If_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
        transform_stmts(xx.m_orelse, xx.n_orelse);
        ASR::expr_t* m_test_value = ASRUtils::expr_value(x.m_test);
        bool m_test_bool;
        if( ASRUtils::is_value_constant(m_test_value, m_test_bool) ) {
            ASR::stmt_t** selected_part = nullptr;
            size_t n_selected_part = 0;
            if( m_test_bool ) {
                selected_part = x.m_body;
                n_selected_part = x.n_body;
            } else {
                selected_part = x.m_orelse;
                n_selected_part = x.n_orelse;
            }
            for( size_t i = 0; i < n_selected_part; i++ ) {
                pass_result.push_back(al, selected_part[i]);
            }
            dead_code_removed = true;
        }
    }

    void visit_Select(const ASR::Select_t& x) {
        ASR::Select_t& xx = const_cast<ASR::Select_t&>(x);
        ASR::expr_t* m_test_value = ASRUtils::expr_value(x.m_test);
        if( !ASRUtils::is_value_constant(m_test_value) ) {
            return ;
        }

        for( size_t i = 0; i < x.n_body; i++ ) {
            ASR::case_stmt_t* case_body = x.m_body[i];
            switch (case_body->type) {
                case ASR::case_stmtType::CaseStmt: {
                    ASR::CaseStmt_t* casestmt = ASR::down_cast<ASR::CaseStmt_t>(case_body);
                    transform_stmts(casestmt->m_body, casestmt->n_body);
                    xx.m_body[i] = (ASR::case_stmt_t*)(&(casestmt->base));
                    for( size_t j = 0; j < casestmt->n_test; j++ ) {
                        if( ASRUtils::is_value_equal(casestmt->m_test[j], x.m_test) ) {
                            for( size_t k = 0; k < casestmt->n_body; k++ ) {
                                pass_result.push_back(al, casestmt->m_body[k]);
                            }
                            return ;
                        }
                    }
                    break;
                }
                case ASR::case_stmtType::CaseStmt_Range: {
                    ASR::CaseStmt_Range_t* casestmt_range = ASR::down_cast<ASR::CaseStmt_Range_t>(case_body);
                    transform_stmts(casestmt_range->m_body, casestmt_range->n_body);
                    xx.m_body[i] = (ASR::case_stmt_t*)(&(casestmt_range->base));
                    if( ASRUtils::is_value_in_range(casestmt_range->m_start, casestmt_range->m_end, x.m_test) ) {
                        for( size_t k = 0; k < casestmt_range->n_body; k++ ) {
                            pass_result.push_back(al, casestmt_range->m_body[k]);
                        }
                        return ;
                    }
                    break;
                }
            }
        }
    }

};

void pass_dead_code_removal(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    DeadCodeRemovalVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
