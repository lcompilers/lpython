#include <string>
#include <lfortran/pickle.h>

using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::exprType;
using LFortran::AST::operatorType;
using LFortran::AST::BaseWalkVisitor;


namespace LFortran {

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

std::string pickle(LFortran::AST::expr_t &ast) {
    PickleVisitor v;
    v.visit_expr(ast);
    return v.get_str();
}

}
