#include <string>
#include <lfortran/pickle.h>

using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::operatorType;
using LFortran::AST::BaseWalkVisitor;


namespace LFortran {

class PickleVisitor : public BaseWalkVisitor<PickleVisitor>
{
    std::string s;
public:
    PickleVisitor() {
        s.reserve(100000);
    }
    void visit_BinOp(const BinOp_t &x) {
        s.append(std::to_string(x.base.type));
        s.append(std::to_string(x.m_op));
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
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
