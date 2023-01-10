#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/select_case.h>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces select-case construct with if-then-else-if statements. The function
`pass_replace_select_case` transforms the ASR tree in-place.

Converts:

    select case (a)

       case (b:c)
          ...

       case (d:e)
          ...
      case (f)
          ...

       case default
          ...

    end select

to:

    if ( b <= a && a <= c ) then
        ...
    else if ( d <= a && a <= e ) then
        ...
    else if (f) then
        ...
    else
        ...
    end if

*/

inline ASR::expr_t* gen_test_expr_CaseStmt(Allocator& al, const Location& loc, ASR::CaseStmt_t* Case_Stmt, ASR::expr_t* a_test) {
    ASR::expr_t* test_expr = nullptr;
    if( Case_Stmt->n_test == 1 ) {
        test_expr = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[0], ASR::cmpopType::Eq);
    } else if( Case_Stmt->n_test == 2 ) {
        ASR::expr_t* left = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[0], ASR::cmpopType::Eq);
        ASR::expr_t* right = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[1], ASR::cmpopType::Eq);
        test_expr = PassUtils::create_binop_helper(al, loc, left, right, ASR::binopType::Add);
    } else {
        ASR::expr_t* left = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[0], ASR::cmpopType::Eq);
        ASR::expr_t* right = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[1], ASR::cmpopType::Eq);
        test_expr = PassUtils::create_binop_helper(al, loc, left, right, ASR::binopType::Add);
        for( std::uint32_t j = 2; j < Case_Stmt->n_test; j++ ) {
            ASR::expr_t* newExpr = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[j], ASR::cmpopType::Eq);
            test_expr = PassUtils::create_binop_helper(al, loc, test_expr, newExpr, ASR::binopType::Add);
        }
    }
    return test_expr;
}

inline ASR::expr_t* gen_test_expr_CaseStmt_Range(Allocator& al, const Location& loc, ASR::CaseStmt_Range_t* Case_Stmt, ASR::expr_t* a_test) {
    ASR::expr_t* test_expr = nullptr;
    if( Case_Stmt->m_start != nullptr && Case_Stmt->m_end == nullptr ) {
        test_expr = PassUtils::create_compare_helper(al, loc, Case_Stmt->m_start, a_test, ASR::cmpopType::LtE);
    } else if( Case_Stmt->m_start == nullptr && Case_Stmt->m_end != nullptr ) {
        test_expr = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_end, ASR::cmpopType::LtE);
    } else if( Case_Stmt->m_start != nullptr && Case_Stmt->m_end != nullptr ) {
        ASR::expr_t* left = PassUtils::create_compare_helper(al, loc, Case_Stmt->m_start, a_test, ASR::cmpopType::LtE);
        ASR::expr_t* right = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_end, ASR::cmpopType::LtE);
        test_expr = PassUtils::create_binop_helper(al, loc, left, right, ASR::binopType::Mul);
    }
    return test_expr;
}

void case_to_if(Allocator& al, const ASR::Select_t& x, ASR::expr_t* a_test, Vec<ASR::stmt_t*>& body) {
    int idx = (int) x.n_body - 1;
    ASR::case_stmt_t* case_body = x.m_body[idx];
    ASR::stmt_t* last_if_else = nullptr;
    switch(case_body->type) {
        case ASR::case_stmtType::CaseStmt: {
            ASR::CaseStmt_t* Case_Stmt = (ASR::CaseStmt_t*)(&(case_body->base));
            ASR::expr_t* test_expr = gen_test_expr_CaseStmt(al, x.base.base.loc, Case_Stmt, a_test);
            last_if_else = ASRUtils::STMT(ASR::make_If_t(al, x.base.base.loc, test_expr, Case_Stmt->m_body, Case_Stmt->n_body, x.m_default, x.n_default));
            break;
        }
        case ASR::case_stmtType::CaseStmt_Range: {
            ASR::CaseStmt_Range_t* Case_Stmt = (ASR::CaseStmt_Range_t*)(&(case_body->base));
            ASR::expr_t* test_expr = gen_test_expr_CaseStmt_Range(al, x.base.base.loc, Case_Stmt, a_test);
            last_if_else = ASRUtils::STMT(ASR::make_If_t(al, x.base.base.loc, test_expr, Case_Stmt->m_body, Case_Stmt->n_body, x.m_default, x.n_default));
            break;
        }
    }

    for( idx = (int) x.n_body - 2; idx >= 0; idx-- ) {
        ASR::case_stmt_t* case_body = x.m_body[idx];
        ASR::expr_t* test_expr = nullptr;
        ASR::stmt_t** m_body = nullptr;
        size_t n_body = 0;
        switch(case_body->type) {
            case ASR::case_stmtType::CaseStmt: {
                ASR::CaseStmt_t* Case_Stmt = (ASR::CaseStmt_t*)(&(case_body->base));
                test_expr = gen_test_expr_CaseStmt(al, x.base.base.loc, Case_Stmt, a_test);
                m_body = Case_Stmt->m_body;
                n_body = Case_Stmt->n_body;
                break;
            }
            case ASR::case_stmtType::CaseStmt_Range: {
                ASR::CaseStmt_Range_t* Case_Stmt = (ASR::CaseStmt_Range_t*)(&(case_body->base));
                test_expr = gen_test_expr_CaseStmt_Range(al, x.base.base.loc, Case_Stmt, a_test);
                m_body = Case_Stmt->m_body;
                n_body = Case_Stmt->n_body;
                break;
            }
        }
        Vec<ASR::stmt_t*> if_body_vec;
        if_body_vec.reserve(al, 1);
        if_body_vec.push_back(al, last_if_else);
        last_if_else = ASRUtils::STMT(ASR::make_If_t(al, x.base.base.loc, test_expr, m_body, n_body, if_body_vec.p, if_body_vec.size()));
    }
    body.reserve(al, 1);
    body.push_back(al, last_if_else);
}

Vec<ASR::stmt_t*> replace_selectcase(Allocator &al, const ASR::Select_t &select_case) {
    ASR::expr_t *a = select_case.m_test;
    Vec<ASR::stmt_t*> body;
    case_to_if(al, select_case, a, body);
    /*
    std::cout << "Input:" << std::endl;
    std::cout << pickle((ASR::asr_t&)loop);
    std::cout << "Output:" << std::endl;
    std::cout << pickle((ASR::asr_t&)*stmt1);
    std::cout << pickle((ASR::asr_t&)*stmt2);
    std::cout << "--------------" << std::endl;
    */
    return body;
}

class SelectCaseVisitor : public PassUtils::PassVisitor<SelectCaseVisitor>
{

public:
    SelectCaseVisitor(Allocator &al) : PassVisitor(al, nullptr) {
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Select(const ASR::Select_t &x) {
        pass_result = replace_selectcase(al, x);
    }
};

void pass_replace_select_case(Allocator &al, ASR::TranslationUnit_t &unit,
                              const LCompilers::PassOptions& /*pass_options*/) {
    SelectCaseVisitor v(al);
    // Each call transforms only one layer of nested loops, so we call it twice
    // to transform doubly nested loops:
    v.visit_TranslationUnit(unit);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
