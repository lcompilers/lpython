#include <string>
#include <lfortran/pickle.h>

using LFortran::AST::ast_t;
using LFortran::AST::Declaration_t;
using LFortran::AST::expr_t;
using LFortran::AST::stmt_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::Compare_t;
using LFortran::AST::If_t;
using LFortran::AST::Assignment_t;
using LFortran::AST::WhileLoop_t;
using LFortran::AST::Exit_t;
using LFortran::AST::Return_t;
using LFortran::AST::Cycle_t;
using LFortran::AST::DoLoop_t;
using LFortran::AST::Subroutine_t;
using LFortran::AST::Function_t;
using LFortran::AST::Program_t;
using LFortran::AST::astType;
using LFortran::AST::exprType;
using LFortran::AST::stmtType;
using LFortran::AST::operatorType;
using LFortran::AST::cmpopType;
using LFortran::AST::PickleBaseVisitor;


namespace LFortran {

std::string stmt2str(const stmtType type)
{
    switch (type) {
        case (stmtType::Assignment) : return "=";
        default : throw std::runtime_error("Unknown type");
    }
    throw std::runtime_error("Unknown type");
}

std::string expr2str(const exprType type)
{
    switch (type) {
        case (exprType::BoolOp) : return "BoolOp";
        case (exprType::BinOp) : return "BinOp";
        case (exprType::UnaryOp) : return "UnaryOp";
        default : throw std::runtime_error("Unknown type");
    }
    throw std::runtime_error("Unknown type");
}

std::string op2str(const operatorType type)
{
    switch (type) {
        case (operatorType::Add) : return "+";
        case (operatorType::Sub) : return "-";
        case (operatorType::Mul) : return "*";
        case (operatorType::Div) : return "/";
        case (operatorType::Pow) : return "**";
    }
    throw std::runtime_error("Unknown type");
}

std::string compare2str(const cmpopType type)
{
    switch (type) {
        case (cmpopType::Eq) : return "==";
        case (cmpopType::NotEq) : return "/=";
        case (cmpopType::Lt) : return "<";
        case (cmpopType::LtE) : return "<=";
        case (cmpopType::Gt) : return ">";
        case (cmpopType::GtE) : return ">=";
    }
    throw std::runtime_error("Unknown type");
}

class PickleVisitor : public PickleBaseVisitor<PickleVisitor>
{
public:
    void visit_BinOp(const BinOp_t &x) {
        s.append("(");
        // We do not print BinOp +, but rather just +. It is still uniquely
        // determined that + means BinOp's +.
        /*
        s.append(expr2str(x.base.type));
        s.append(" ");
        */
        s.append(op2str(x.m_op));
        s.append(" ");
        this->visit_expr(*x.m_left);
        s.append(" ");
        this->visit_expr(*x.m_right);
        s.append(")");
    }
    void visit_Compare(const Compare_t &x) {
        s.append("(");
        s.append(compare2str(x.m_op));
        s.append(" ");
        this->visit_expr(*x.m_left);
        s.append(" ");
        this->visit_expr(*x.m_right);
        s.append(")");
    }
    void visit_Declaration(const Declaration_t &x) {
        s.append("(decl ");
        LFORTRAN_ASSERT(x.n_vars == 1);
        s.append(x.m_vars[0].m_sym);
        s.append(" ");
        s.append(x.m_vars[0].m_sym_type);
        s.append(")");
    }
    void visit_Name(const Name_t &x) {
        s.append(x.m_id);
    }
    void visit_Num(const Num_t &x) {
        s.append(std::to_string(x.m_n));
    }
    std::string get_str() {
        return s;
    }
};

std::string pickle(LFortran::AST::ast_t &ast) {
    PickleVisitor v;
    v.visit_ast(ast);
    return v.get_str();
}

}
