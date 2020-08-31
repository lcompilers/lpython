#include <lfortran/ast_to_src.h>

using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::operatorType;
using LFortran::AST::BaseVisitor;


namespace LFortran {

namespace {

    std::string op2str(const operatorType type)
    {
        switch (type) {
            case (operatorType::Add) : return " + ";
            case (operatorType::Sub) : return " - ";
            case (operatorType::Mul) : return "*";
            case (operatorType::Div) : return "/";
            case (operatorType::Pow) : return "**";
        }
        throw std::runtime_error("Unknown type");
    }

}

namespace AST {

class ASTToSRCVisitor : public BaseVisitor<ASTToSRCVisitor>
{
public:
    std::string s;
    bool use_colors;
    int indent_level;
    int indent_spaces;
    bool indent_unit;

    // Syntax highlighting groups
    enum gr {
        // Type
        UnitHeader,  // program, end program
        Type,        // integer, real, character

        // Constant
        String,      // "some string"
        Integer,     // 234
        Real,        // 5.5_dp
        Logical,     // .true., .false.

        // Functions
        Call,        // call
        Intrinsic,   // exp, sin, cos

        // Statements
        Conditional, // if, then, else, end if, where, select case
        Repeat,      // do, while, forall
        Keyword,     // any other keyword: print

        // Reset
        Reset,       // resets any previously set formatting
    };
    std::string syn_reset;

public:
    ASTToSRCVisitor(bool color, int indent, bool indent_unit)
            : use_colors{color}, indent_level{0},
            indent_spaces{indent}, indent_unit{indent_unit}
        { }

    std::string syn(const gr &g=gr::Reset) {
        std::string syn_color;
        if (use_colors) {
            switch (g) {
                case (gr::UnitHeader) : {
                    syn_color = color(fgB::cyan);
                    syn_reset = color(fg::reset);
                    break;
                }

                case (gr::Type) : {
                    syn_color = color(fgB::green);
                    syn_reset = color(fg::reset);
                    break;
                }

                case (gr::String) :
                case (gr::Integer) :
                case (gr::Real) :
                case (gr::Logical) : {
                    syn_color = color(fg::magenta);
                    syn_reset = color(fg::reset);
                    break;
                }

                case (gr::Call) :
                case (gr::Intrinsic) : {
                    syn_color = color(fgB::cyan) + color(style::bold);
                    syn_reset = color(fg::reset) + color(style::reset);
                    break;
                }

                case (gr::Conditional) :
                case (gr::Repeat) :
                case (gr::Keyword) : {
                    syn_color = color(fg::yellow);
                    syn_reset = color(fg::reset);
                    break;
                }

                case (gr::Reset) : {
                    syn_color = syn_reset;
                    syn_reset = "";
                    break;
                } ;

                default : {
                    throw LFortranException("Syntax Group not implemented");
                }
            }
        }
        return syn_color;
    }

    void visit_TranslationUnit(const TranslationUnit_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("translationunit");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("Unimplementedobject");
        s.append(")");
    }
    void visit_Module(const Module_t &x) {
        std:: string r = "";
        r.append("module");
        r.append(" ");
        r.append(x.m_name);
        r.append("\n");
        if (indent_unit) indent_level++;
        for (size_t i=0; i<x.n_use; i++) {
            this->visit_unit_decl1(*x.m_use[i]);
            r.append(s);
            if (i < x.n_use-1) r.append("\n");
        }
        r.append("\n");
        for (size_t i=0; i<x.n_decl; i++) {
            this->visit_unit_decl2(*x.m_decl[i]);
            r.append(s);
            if (i < x.n_decl-1) r.append("\n");
        }
        r.append("\n");
        for (size_t i=0; i<x.n_contains; i++) {
            this->visit_program_unit(*x.m_contains[i]);
            r.append(s);
            if (i < x.n_contains-1) r.append("\n");
        }
        if (indent_unit) indent_level--;
        r.append("\nend module ");
        r.append(x.m_name);
        r.append("\n");
        s = r;
    }
    void visit_Program(const Program_t &x) {
        std::string r;
        r += syn(gr::UnitHeader);
        r.append("program");
        r += syn();
        r += " ";
        r.append(x.m_name);
        r.append("\n");
        if (indent_unit) indent_level++;
        for (size_t i=0; i<x.n_use; i++) {
            this->visit_unit_decl1(*x.m_use[i]);
            r.append(s);
            r.append("\n");
        }
        for (size_t i=0; i<x.n_decl; i++) {
            this->visit_unit_decl2(*x.m_decl[i]);
            r.append(s);
            r.append("\n");
        }
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
            r.append("\n");
        }
        if (x.n_contains > 0) {
            for (size_t i=0; i<x.n_contains; i++) {
                this->visit_program_unit(*x.m_contains[i]);
                r.append(s);
                r.append("\n");
            }
        }
        if (indent_unit) indent_level--;
        r += syn(gr::UnitHeader);
        r.append("end program ");
        r += syn();
        r.append(x.m_name);
        r.append("\n");
        s = r;
    }
    void visit_Subroutine(const Subroutine_t &x) {
        std::string r = "subroutine ";
        r.append(x.m_name);
        if (x.n_args > 0) {
            r.append("(");
            for (size_t i=0; i<x.n_args; i++) {
                this->visit_arg(x.m_args[i]);
                r.append(s);
                if (i < x.n_args-1) r.append(", ");
            }
            r.append(")");
        }
        r.append("\n");
        for (size_t i=0; i<x.n_use; i++) {
            this->visit_unit_decl1(*x.m_use[i]);
            r.append(s);
            r.append("\n");
        }
        for (size_t i=0; i<x.n_decl; i++) {
            this->visit_unit_decl2(*x.m_decl[i]);
            r.append(s);
            r.append("\n");
        }
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
            r.append("\n");
        }
        if (x.n_contains > 0) {
            r.append("contains\n");
            for (size_t i=0; i<x.n_contains; i++) {
                this->visit_program_unit(*x.m_contains[i]);
                r.append(s);
                r.append("\n");
            }
        }
        r.append("end subroutine ");
        r.append(x.m_name);
        r.append("\n");
        s = r;
    }
    void visit_Function(const Function_t &x) {
        std::string r = "";
        if (x.m_return_type) {
            r.append(x.m_return_type);
            r.append(" ");
        }
        r.append("function ");
        r.append(x.m_name);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_arg(x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) s.append(", ");
        }
        r.append(")");
        if (x.m_return_var) {
            r.append(" result(");
            this->visit_expr(*x.m_return_var);
            r.append(s);
            r.append(")");
        }
        r.append(" ");
        if (x.m_bind) {
            this->visit_tbind(*x.m_bind);
            r.append(s);
        }
        r.append("\n");

        for (size_t i=0; i<x.n_use; i++) {
            this->visit_unit_decl1(*x.m_use[i]);
            r.append(s);
            r.append("\n");
        }
        for (size_t i=0; i<x.n_decl; i++) {
            this->visit_unit_decl2(*x.m_decl[i]);
            r.append(s);
            r.append("\n");
        }
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
            r.append("\n");
        }
        for (size_t i=0; i<x.n_contains; i++) {
            this->visit_program_unit(*x.m_contains[i]);
            r.append(s);
            r.append("\n");
        }
        r.append("end function ");
        r.append(x.m_name);
        r.append("\n");

        s = r;
    }
    void visit_Use(const Use_t &x) {
        std::string r = "use ";
        r.append(x.m_module);
        if (x.n_symbols > 0) {
            r.append(", only: ");
            for (size_t i=0; i<x.n_symbols; i++) {
                this->visit_use_symbol(*x.m_symbols[i]);
                r.append(s);
                if (i < x.n_symbols-1) r.append(", ");
            }
        }
        s = r;
    }
    void visit_Declaration(const Declaration_t &x) {
        std::string r = "";
        for (size_t i=0; i<x.n_vars; i++) {
            this->visit_decl(x.m_vars[i]);
            r.append(s);
            if (i < x.n_vars-1) r.append("\n");
        }
        s = r;
    }
    void visit_Private(const Private_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("private");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("Unimplementedidentifier");
        s.append(")");
    }
    void visit_Public(const Public_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("public");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("Unimplementedidentifier");
        s.append(")");
    }
    void visit_Interface(const Interface_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("interface");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append(x.m_name);
        s.append(" ");
        s.append("Unimplementedidentifier");
        s.append(")");
    }
    void visit_Interface2(const Interface2_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("interface2");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        if (x.m_name) {
            s.append(x.m_name);
        } else {
            s.append("()");
        }
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_procs; i++) {
            this->visit_program_unit(*x.m_procs[i]);
            if (i < x.n_procs-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_Assignment(const Assignment_t &x) {
        std::string indent(indent_level*indent_spaces, ' ');
        std::string r = indent;
        this->visit_expr(*x.m_target);
        r.append(s);
        r.append(" = ");
        this->visit_expr(*x.m_value);
        r.append(s);
        s = r;
    }
    void visit_Associate(const Associate_t &x) {
        std::string r = "";
        this->visit_expr(*x.m_target);
        r.append(s);
        r.append(" => ");
        this->visit_expr(*x.m_value);
        r.append(s);
        s = r;
    }
    void visit_SubroutineCall(const SubroutineCall_t &x) {
        std::string r = "call ";
        r.append(x.m_name);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(" ");
        }
        r.append(")");
        s = r;
    }
    void visit_BuiltinCall(const BuiltinCall_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("builtincall");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append(x.m_name);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            if (i < x.n_args-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_If(const If_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("if");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        this->visit_expr(*x.m_test);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
            if (i < x.n_orelse-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_Where(const Where_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("where");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        this->visit_expr(*x.m_test);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
            if (i < x.n_orelse-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_Stop(const Stop_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("stop");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        if (x.m_code) {
            this->visit_expr(*x.m_code);
        } else {
            s.append("()");
        }
        s.append(")");
    }
    void visit_ErrorStop(const ErrorStop_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("errorstop");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(")");
    }
    void visit_DoLoop(const DoLoop_t &x) {
        std::string r = "do";
        if (x.m_var) {
            r.append(" ");
            r.append(x.m_var);
            r.append(" = ");
        }
        if (x.m_start) {
            this->visit_expr(*x.m_start);
            r.append(s);
            r.append(", ");
        }
        if (x.m_end) {
            this->visit_expr(*x.m_end);
            r.append(s);
        }
        if (x.m_increment) {
            r.append(", ");
            this->visit_expr(*x.m_increment);
            r.append(s);
        }
        r.append("\n");
        indent_level += 4;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            for (int i=0; i < indent_level; i++) r.append(" ");
            r.append(s);
            r.append("\n");
        }
        indent_level -= 4;
        r.append("end do");
        s = r;
    }
    void visit_DoConcurrentLoop(const DoConcurrentLoop_t &x) {
        std::string r = "do concurrent";
        if (x.m_var) {
            r.append(" (");
            r.append(x.m_var);
            r.append(" = ");
        }
        if (x.m_start) {
            this->visit_expr(*x.m_start);
            r.append(s);
            r.append(":");
        }
        if (x.m_end) {
            this->visit_expr(*x.m_end);
            r.append(s);
        }
        if (x.m_increment) {
            r.append(":");
            this->visit_expr(*x.m_increment);
            r.append(s);
        }
        r.append(")\n");
        indent_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
            r.append("\n");
        }
        indent_level -= 1;
        r.append("end do");
        s = r;
    }
    void visit_Select(const Select_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("select");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        this->visit_expr(*x.m_test);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_case_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_default; i++) {
            this->visit_stmt(*x.m_default[i]);
            if (i < x.n_default-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_Cycle(const Cycle_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("cycle");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(")");
    }
    void visit_Exit(const Exit_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("exit");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(")");
    }
    void visit_Return(const Return_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("return");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(")");
    }
    void visit_WhileLoop(const WhileLoop_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("while");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        this->visit_expr(*x.m_test);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_Print(const Print_t &x) {
        std::string r;
        r += syn(gr::Keyword);
        r += "print";
        r += syn();
        r += " ";
        if (x.m_fmt) {
            r += x.m_fmt;
        } else {
            r += "*";
        }
        if (x.n_values > 0) {
            r += ", ";
            for (size_t i=0; i<x.n_values; i++) {
                this->visit_expr(*x.m_values[i]);
                r += s;
                if (i < x.n_values-1) r += ", ";
            }
        }
        s = r;
    }
    void visit_BoolOp(const BoolOp_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("boolop");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        this->visit_expr(*x.m_left);
        s.append(" ");
        s.append("boolopType" + std::to_string(x.m_op));
        s.append(" ");
        this->visit_expr(*x.m_right);
        s.append(")");
    }
    void visit_BinOp(const BinOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        s = "(" + left + ")" + op2str(x.m_op) + "(" + right + ")";
    }
    void visit_UnaryOp(const UnaryOp_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("unaryop");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("unaryopType" + std::to_string(x.m_op));
        s.append(" ");
        this->visit_expr(*x.m_operand);
        s.append(")");
    }
    void visit_Compare(const Compare_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("compare");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        this->visit_expr(*x.m_left);
        s.append(" ");
        s.append("cmpopType" + std::to_string(x.m_op));
        s.append(" ");
        this->visit_expr(*x.m_right);
        s.append(")");
    }
    void visit_FuncCall(const FuncCall_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("funccall");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append(x.m_func);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            if (i < x.n_args-1) s.append(" ");
        }
        s.append("]");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_keywords; i++) {
            this->visit_keyword(x.m_keywords[i]);
            if (i < x.n_keywords-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_FuncCallOrArray(const FuncCallOrArray_t &x) {
        std::string r = "";
        r.append(x.m_func);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) s.append(", ");
        }
        for (size_t i=0; i<x.n_keywords; i++) {
            this->visit_keyword(x.m_keywords[i]);
            r.append(s);
            if (i < x.n_keywords-1) r.append(", ");
        }
        r.append(")");
        s = r;
    }
    void visit_Array(const Array_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("array");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append(x.m_name);
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_array_index(*x.m_args[i]);
            if (i < x.n_args-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_ArrayInitializer(const ArrayInitializer_t &x) {
        std::string r = "[";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
        }
        r.append("]");
        s = r;
    }
    void visit_Num(const Num_t &x) {
        s = std::to_string(x.m_n);
    }
    void visit_Real(const Real_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("real");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("\"" + std::string(x.m_n) + "\"");
        s.append(")");
    }
    void visit_Str(const Str_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("str");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("\"" + std::string(x.m_s) + "\"");
        s.append(")");
    }
    void visit_Name(const Name_t &x) {
        s = std::string(x.m_id);
    }
    void visit_Constant(const Constant_t &/*x*/) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("constant");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("Unimplementedconstant");
        s.append(")");
    }
    void visit_decl(const decl_t &x) {
        std::string r = "";
        r += syn(gr::Type);
        r += std::string(x.m_sym_type);
        r += syn();
        if (x.n_attrs > 0) {
            for (size_t i=0; i<x.n_attrs; i++) {
                r.append(", ");
                this->visit_attribute(*x.m_attrs[i]);
                r.append(s);
            }
        }
        r.append(" :: ");
        r.append(x.m_sym);
        if (x.n_dims > 0) {
            r.append("(");
            for (size_t i=0; i<x.n_dims; i++) {
                this->visit_dimension(x.m_dims[i]);
                r.append(s);
                if (i < x.n_dims-1) r.append(",");
            }
            r.append(")");
        }
        r.append(" ");
        if (x.m_initializer) {
            r.append("=");
            this->visit_expr(*x.m_initializer);
            r.append(s);
        }
        s = r;
    }
    void visit_dimension(const dimension_t &x) {
        std::string r = "";
        if (x.m_start) {
            this->visit_expr(*x.m_start);
            r.append(s);
        } else {
            r.append("");
        }
        r.append(":");
        if (x.m_end) {
            this->visit_expr(*x.m_end);
            r.append(s);
        } else {
            r.append("");
        }
        s = r;
    }
    void visit_Attribute(const Attribute_t &x) {
        std::string r;
        r.append(x.m_name);
        if (x.n_args > 0) {
            r.append("(");
            for (size_t i=0; i<x.n_args; i++) {
                this->visit_attribute_arg(x.m_args[i]);
                r.append(s);
                if (i < x.n_args-1) r.append(" ");
            }
            r.append(")");
        }
        s = r;
    }
    void visit_attribute_arg(const attribute_arg_t &x) {
        s = std::string(x.m_arg);
    }
    void visit_arg(const arg_t &x) {
        s = std::string(x.m_arg);
    }
    void visit_keyword(const keyword_t &x) {
        s.append("(");
        if (x.m_arg) {
            s.append(x.m_arg);
        } else {
            s.append("()");
        }
        s.append(" ");
        this->visit_expr(*x.m_value);
        s.append(")");
    }
    void visit_Bind(const Bind_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("bind");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_keyword(x.m_args[i]);
            if (i < x.n_args-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_ArrayIndex(const ArrayIndex_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("arrayindex");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        if (x.m_left) {
            this->visit_expr(*x.m_left);
        } else {
            s.append("()");
        }
        s.append(" ");
        if (x.m_right) {
            this->visit_expr(*x.m_right);
        } else {
            s.append("()");
        }
        s.append(" ");
        if (x.m_step) {
            this->visit_expr(*x.m_step);
        } else {
            s.append("()");
        }
        s.append(")");
    }
    void visit_CaseStmt(const CaseStmt_t &x) {
        s.append("(");
        s.append("CaseStmt");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_test; i++) {
            this->visit_expr(*x.m_test[i]);
            if (i < x.n_test-1) s.append(" ");
        }
        s.append("]");
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (i < x.n_body-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
    }
    void visit_UseSymbol(const UseSymbol_t &x) {
        s = "";
        if (x.m_rename) {
            s.append(x.m_rename);
            s.append(" => ");
        }
        s.append(x.m_sym);
    }
};

}

std::string ast_to_src(LFortran::AST::ast_t &ast, bool color, int indent,
        bool indent_unit) {
    AST::ASTToSRCVisitor v(color, indent, indent_unit);
    v.visit_ast(ast);
    return v.s;
}

}
