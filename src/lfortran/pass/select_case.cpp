#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/select_case.h>


namespace LFortran {

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
        test_expr = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::Eq, Case_Stmt->m_test[0], LFortran::ASRUtils::expr_type(a_test), nullptr));
    } else if( Case_Stmt->n_test == 2 ) {
        ASR::expr_t* left = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::Eq, Case_Stmt->m_test[0], LFortran::ASRUtils::expr_type(a_test), nullptr));
        ASR::expr_t* right = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::Eq, Case_Stmt->m_test[1], LFortran::ASRUtils::expr_type(a_test), nullptr));
        test_expr = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, left, ASR::binopType::Add, right, LFortran::ASRUtils::expr_type(left), nullptr, nullptr));
    } else {
        ASR::expr_t* left = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::Eq, Case_Stmt->m_test[0], LFortran::ASRUtils::expr_type(a_test), nullptr));
        ASR::expr_t* right = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::Eq, Case_Stmt->m_test[1], LFortran::ASRUtils::expr_type(a_test), nullptr));
        test_expr = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, left, ASR::binopType::Add, right, LFortran::ASRUtils::expr_type(left), nullptr, nullptr));
        for( std::uint32_t j = 2; j < Case_Stmt->n_test; j++ ) {
            ASR::expr_t* newExpr = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::Eq, Case_Stmt->m_test[j], LFortran::ASRUtils::expr_type(a_test), nullptr));
            test_expr = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, test_expr, ASR::binopType::Add, newExpr, LFortran::ASRUtils::expr_type(newExpr), nullptr, nullptr));
        }
    }
    return test_expr;
}

inline ASR::expr_t* gen_test_expr_CaseStmt_Range(Allocator& al, const Location& loc, ASR::CaseStmt_Range_t* Case_Stmt, ASR::expr_t* a_test) {
    ASR::expr_t* test_expr = nullptr;
    if( Case_Stmt->m_start != nullptr && Case_Stmt->m_end == nullptr ) {
        test_expr = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, Case_Stmt->m_start, ASR::cmpopType::LtE, a_test, LFortran::ASRUtils::expr_type(a_test), nullptr));
    } else if( Case_Stmt->m_start == nullptr && Case_Stmt->m_end != nullptr ) {
        test_expr = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::LtE,  Case_Stmt->m_end, LFortran::ASRUtils::expr_type(a_test), nullptr));
    } else if( Case_Stmt->m_start != nullptr && Case_Stmt->m_end != nullptr ) {
        ASR::expr_t* left = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, Case_Stmt->m_start, ASR::cmpopType::LtE, a_test, LFortran::ASRUtils::expr_type(a_test), nullptr));
        ASR::expr_t* right = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc, a_test, ASR::cmpopType::LtE,  Case_Stmt->m_end, LFortran::ASRUtils::expr_type(a_test), nullptr));
        test_expr = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, left, ASR::binopType::Mul, right, LFortran::ASRUtils::expr_type(left), nullptr, nullptr));
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
            last_if_else = LFortran::ASRUtils::STMT(ASR::make_If_t(al, x.base.base.loc, test_expr, Case_Stmt->m_body, Case_Stmt->n_body, x.m_default, x.n_default));
            break;
        }
        case ASR::case_stmtType::CaseStmt_Range: {
            ASR::CaseStmt_Range_t* Case_Stmt = (ASR::CaseStmt_Range_t*)(&(case_body->base));
            ASR::expr_t* test_expr = gen_test_expr_CaseStmt_Range(al, x.base.base.loc, Case_Stmt, a_test);
            last_if_else = LFortran::ASRUtils::STMT(ASR::make_If_t(al, x.base.base.loc, test_expr, Case_Stmt->m_body, Case_Stmt->n_body, x.m_default, x.n_default));
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
        last_if_else = LFortran::ASRUtils::STMT(ASR::make_If_t(al, x.base.base.loc, test_expr, m_body, n_body, if_body_vec.p, if_body_vec.size()));
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

class SelectCaseVisitor : public ASR::BaseWalkVisitor<SelectCaseVisitor>
{
private:
    Allocator &al;
    Vec<ASR::stmt_t*> select_case_result;
public:
    SelectCaseVisitor(Allocator &al) : al{al} {
        select_case_result.n = 0;

    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            select_case_result.n = 0;
            visit_stmt(*m_body[i]);
            if (select_case_result.size() > 0) {
                for (size_t j=0; j<select_case_result.size(); j++) {
                    body.push_back(al, select_case_result[j]);
                }
                select_case_result.n = 0;
            } else {
                body.push_back(al, m_body[i]);
            }
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
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Select(const ASR::Select_t &x) {
        select_case_result = replace_selectcase(al, x);
    }
};

void pass_replace_select_case(Allocator &al, ASR::TranslationUnit_t &unit) {
    SelectCaseVisitor v(al);
    // Each call transforms only one layer of nested loops, so we call it twice
    // to transform doubly nested loops:
    v.visit_TranslationUnit(unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
