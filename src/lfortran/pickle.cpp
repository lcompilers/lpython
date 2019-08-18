#include <string>
#include <lfortran/pickle.h>

using LFortran::AST::ast_t;
using LFortran::AST::expr_t;
using LFortran::AST::stmt_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::Assignment_t;
using LFortran::AST::Subroutine_t;
using LFortran::AST::astType;
using LFortran::AST::exprType;
using LFortran::AST::stmtType;
using LFortran::AST::operatorType;
using LFortran::AST::BaseWalkVisitor;


namespace LFortran {

std::string stmt2str(const stmtType type)
{
    switch (type) {
        case (stmtType::Assignment) : return "Assignment";
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
        case (operatorType::Add) : return "Add";
        case (operatorType::Sub) : return "Sub";
        case (operatorType::Mul) : return "Mul";
        case (operatorType::Div) : return "Div";
        case (operatorType::Pow) : return "Pow";
    }
    throw std::runtime_error("Unknown type");
}

class PickleVisitor : public BaseWalkVisitor<PickleVisitor>
{
    std::string s;
public:
    PickleVisitor() {
        s.reserve(100000);
    }
    void visit_Subroutine(const Subroutine_t &x) {
        s.append("(");
        s.append("Subroutine");
        s.append(" ");
        s.append(std::to_string(x.n_body));
        s.append(" ");
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_body[i]);
        }
        s.append(")");
    }
    void visit_BinOp(const BinOp_t &x) {
        s.append("(");
        s.append(expr2str(x.base.type));
        s.append(" ");
        s.append(op2str(x.m_op));
        s.append(" ");
        this->visit_expr(*x.m_left);
        s.append(" ");
        this->visit_expr(*x.m_right);
        s.append(")");
    }
    void visit_Assignment(const Assignment_t &x) {
        s.append("(");
        s.append(stmt2str(x.base.type));
        s.append(" ");
        this->visit_expr(*x.m_target);
        s.append(" ");
        this->visit_expr(*x.m_value);
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
