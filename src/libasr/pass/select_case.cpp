#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_select_case.h>


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
        test_expr = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc, left,
            ASR::logicalbinopType::Or, right, ASRUtils::expr_type(left), nullptr));
    } else {
        ASR::expr_t* left = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[0], ASR::cmpopType::Eq);
        ASR::expr_t* right = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[1], ASR::cmpopType::Eq);
        test_expr = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc, left,
            ASR::logicalbinopType::Or, right, ASRUtils::expr_type(left), nullptr));
        for( std::uint32_t j = 2; j < Case_Stmt->n_test; j++ ) {
            ASR::expr_t* newExpr = PassUtils::create_compare_helper(al, loc, a_test, Case_Stmt->m_test[j], ASR::cmpopType::Eq);
            test_expr = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc, test_expr,
                ASR::logicalbinopType::Or, newExpr, ASRUtils::expr_type(test_expr), nullptr));
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
        test_expr = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc, left,
            ASR::logicalbinopType::And, right, ASRUtils::expr_type(left), nullptr));
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

void case_to_if_with_fall_through(Allocator& al, const ASR::Select_t& x,
    ASR::expr_t* a_test, Vec<ASR::stmt_t*>& body, SymbolTable* scope) {
    body.reserve(al, x.n_body + 1);
    const Location& loc = x.base.base.loc;
    ASR::symbol_t* case_found_sym = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
        al, loc, scope, s2c(al, scope->get_unique_name("case_found")), nullptr, 0,
        ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr, ASR::abiType::Source,
        ASR::accessType::Public, ASR::presenceType::Required, false));
    scope->add_symbol(scope->get_unique_name("case_found"), case_found_sym);
    ASR::expr_t* true_asr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, true,
        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4))));
    ASR::expr_t* false_asr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, false,
        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4))));
    ASR::expr_t* case_found = ASRUtils::EXPR(ASR::make_Var_t(al, loc, case_found_sym));
    body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, loc, case_found, false_asr, nullptr)));
    int label_id = ASRUtils::LabelGenerator::get_instance()->get_unique_label();
    for( size_t idx = 0; idx < x.n_body; idx++ ) {
        ASR::case_stmt_t* case_body = x.m_body[idx];
        switch(case_body->type) {
            case ASR::case_stmtType::CaseStmt: {
                ASR::CaseStmt_t* Case_Stmt = ASR::down_cast<ASR::CaseStmt_t>(case_body);
                ASR::expr_t* test_expr = gen_test_expr_CaseStmt(al, loc, Case_Stmt, a_test);
                test_expr = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc, test_expr,
                    ASR::logicalbinopType::Or, case_found, ASRUtils::expr_type(case_found), nullptr));
                Vec<ASR::stmt_t*> case_body; case_body.reserve(al, Case_Stmt->n_body);
                case_body.from_pointer_n(Case_Stmt->m_body, Case_Stmt->n_body);
                case_body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(
                        al, loc, case_found, true_asr, nullptr)));
                if( !Case_Stmt->m_fall_through ) {
                    case_body.push_back(al, ASRUtils::STMT(ASR::make_GoTo_t(al, loc,
                        label_id, s2c(al, scope->get_unique_name("switch_case_label")))));
                }
                body.push_back(al, ASRUtils::STMT(ASR::make_If_t(al, loc,
                    test_expr, case_body.p, case_body.size(), nullptr, 0)));
                break;
            }
            case ASR::case_stmtType::CaseStmt_Range: {
                LCOMPILERS_ASSERT(false);
                break;
            }
        }
    }
    for( size_t id = 0; id < x.n_default; id++ ) {
        body.push_back(al, x.m_default[id]);
    }
    SymbolTable* block_symbol_table = al.make_new<SymbolTable>(scope);
    ASR::symbol_t* empty_block = ASR::down_cast<ASR::symbol_t>(ASR::make_Block_t(
        al, loc, block_symbol_table, s2c(al, scope->get_unique_name("switch_case_label")),
        nullptr, 0));
    scope->add_symbol(scope->get_unique_name("switch_case_label"), empty_block);
    body.push_back(al, ASRUtils::STMT(ASR::make_BlockCall_t(al, loc, label_id, empty_block)));
}

Vec<ASR::stmt_t*> replace_selectcase_with_fall_through(
    Allocator &al, const ASR::Select_t &select_case,
    SymbolTable* scope) {
    ASR::expr_t *a = select_case.m_test;
    Vec<ASR::stmt_t*> body;
    case_to_if_with_fall_through(al, select_case, a, body, scope);
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
        if( x.m_enable_fall_through ) {
            pass_result = replace_selectcase_with_fall_through(al, x, current_scope);
        } else {
            pass_result = replace_selectcase(al, x);
        }
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
