#include <string>
#include <lfortran/pickle.h>

using LFortran::AST::ast_t;
using LFortran::AST::Declaration_t;
using LFortran::AST::expr_t;
using LFortran::AST::stmt_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
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
using LFortran::AST::BaseWalkVisitor;


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

class PickleVisitor : public BaseWalkVisitor<PickleVisitor>
{
    std::string s;
public:
    PickleVisitor() {
        s.reserve(100000);
    }
    void visit_Subroutine(const Subroutine_t &x) {
        s.append("(");
        s.append("sub");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_Function(const Function_t &x) {
        s.append("(");
        s.append("fn");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_decl; i++) {
            LFORTRAN_ASSERT(x.m_decl[i]->base.type == astType::unit_decl2)
            this->visit_unit_decl2(*x.m_decl[i]);
            if (i < x.n_decl-1) s.append(" ");
        }
        s.append("]");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_If(const If_t &x) {
        s.append("(");
        s.append("if");
        s.append(" ");
        this->visit_expr(*x.m_test);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_orelse; i++) {
            LFORTRAN_ASSERT(x.m_orelse[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_orelse[i]);
            if (i < x.n_orelse-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_WhileLoop(const WhileLoop_t &x) {
        s.append("(");
        s.append("while");
        s.append(" ");
        this->visit_expr(*x.m_test);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_DoLoop(const DoLoop_t &x) {
        s.append("(");
        s.append("do");
        s.append(" ");
        if (x.m_head.m_var) {
            s.append(x.m_head.m_var);
        } else {
            s.append("()");
        }
        s.append(" ");
        if (x.m_head.m_start) {
            this->visit_expr(*x.m_head.m_start);
        } else {
            s.append("()");
        }
        s.append(" ");
        if (x.m_head.m_end) {
            this->visit_expr(*x.m_head.m_end);
        } else {
            s.append("()");
        }
        s.append(" ");
        if (x.m_head.m_increment) {
            this->visit_expr(*x.m_head.m_increment);
        } else {
            s.append("()");
        }
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_Program(const Program_t &x) {
        s.append("(");
        s.append("prog");
        s.append(" ");
        s.append(x.m_name);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == astType::stmt)
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
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
    void visit_Assignment(const Assignment_t &x) {
        s.append("(");
        s.append(stmt2str(x.base.type));
        s.append(" ");
        this->visit_expr(*x.m_target);
        s.append(" ");
        this->visit_expr(*x.m_value);
        s.append(")");
    }
    void visit_Exit(const Exit_t &x) {
        s.append("(exit)");
    }
    void visit_Return(const Return_t &x) {
        s.append("(return)");
    }
    void visit_Cycle(const Cycle_t &x) {
        s.append("(cycle)");
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
