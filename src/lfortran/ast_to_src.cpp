#include <cctype>
#include <lfortran/ast_to_src.h>
#include <lfortran/string_utils.h>
#include <lfortran/bigint.h>

using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::operatorType;
using LFortran::AST::BaseVisitor;
using LFortran::AST::StrOp_t;


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
        throw LFortranException("Unknown type");
    }

    std::string boolop2str(const AST::boolopType type)
    {
        switch (type) {
            case (AST::boolopType::And) : return " .and. ";
            case (AST::boolopType::Or) : return " .or. ";
            case (AST::boolopType::Eqv) : return " .eqv. ";
            case (AST::boolopType::NEqv) : return " .neqv. ";
        }
        throw LFortranException("Unknown type");
    }

    std::string cmpop2str(const AST::cmpopType type)
    {
        switch (type) {
            case (AST::cmpopType::Eq) : return " == ";
            case (AST::cmpopType::Gt) : return " > ";
            case (AST::cmpopType::GtE) : return " >= ";
            case (AST::cmpopType::Lt) : return " < ";
            case (AST::cmpopType::LtE) : return " <= ";
            case (AST::cmpopType::NotEq) : return " /= ";
        }
        throw LFortranException("Unknown type");
    }

    std::string strop2str(const AST::stroperatorType type)
    {
        switch (type) {
            case (AST::stroperatorType::Concat) : return " // ";
        }
        throw LFortranException("Unknown type");
    }

    std::string intrinsicop2str(const AST::intrinsicopType type)
    {
        switch (type) {
            case (AST::intrinsicopType::AND) : return ".and.";
            case (AST::intrinsicopType::OR) : return ".or.";
            case (AST::intrinsicopType::EQV) : return ".eqv.";
            case (AST::intrinsicopType::NEQV) : return ".neqv.";
            case (AST::intrinsicopType::PLUS) : return "+";
            case (AST::intrinsicopType::MINUS) : return "-";
            case (AST::intrinsicopType::STAR) : return "*";
            case (AST::intrinsicopType::DIV) : return "/";
            case (AST::intrinsicopType::POW) : return "**";
            case (AST::intrinsicopType::NOT) : return ".not.";
            case (AST::intrinsicopType::EQ) : return "==";
            case (AST::intrinsicopType::GT) : return ">";
            case (AST::intrinsicopType::GTE) : return ">=";
            case (AST::intrinsicopType::LT) : return "<";
            case (AST::intrinsicopType::LTE) : return "<=";
            case (AST::intrinsicopType::NOTEQ) : return "/=";
            case (AST::intrinsicopType::CONCAT) : return "//";
        }
        throw LFortranException("Unknown type");
    }

    std::string symbol2str(const AST::symbolType type)
    {
        switch (type) {
            case (AST::symbolType::None) : return "";
            case (AST::symbolType::Arrow) : return " => ";
            case (AST::symbolType::Equal) : return " = ";
            case (AST::symbolType::Asterisk) : return "*";
            case (AST::symbolType::DoubleAsterisk) : return "*(*)";
            case (AST::symbolType::Slash) : return "/";
        }
        throw LFortranException("Unknown type");
    }
}

namespace AST {

class ASTToSRCVisitor : public BaseVisitor<ASTToSRCVisitor>
{
public:
    std::string s;
    bool use_colors;
    int indent_level;
    std::string indent;
    int indent_spaces;
    bool indent_unit;
    // The precedence of the last expression, using the table
    // 10.1 in the Fortran 2018 standard:
    int last_expr_precedence;

    // Syntax highlighting groups
    enum gr {
        // Type
        UnitHeader,  // program, end program
        Type,        // integer, real, character, complex

        // Constant
        String,      // "some string"
        Integer,     // 234
        Real,        // 5.5_dp
        Logical,     // .true., .false.
        Complex,     // (7,8)

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
                case (gr::Logical) :
                case (gr::Complex) : {
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

    void inc_indent() {
        indent_level++;
        indent = std::string(indent_level*indent_spaces, ' ');
    }

    void dec_indent() {
        indent_level--;
        indent = std::string(indent_level*indent_spaces, ' ');
    }

    std::string print_trivia_inside(const trivia_t &x) {
        std::string r = " ";
        auto y = (TriviaNode_t &)x;
        if(y.n_inside > 0) {
            for (size_t i=0; i<y.n_inside; i++) {
                switch (y.m_inside[i]->type) {
                    case trivia_nodeType::Comment: {
                        r += std::string(
                            down_cast<Comment_t>(y.m_inside[i])->m_comment
                        );
                        r += "\n";
                        break;
                    }
                    case trivia_nodeType::EOLComment: {
                        r += std::string(
                            down_cast<EOLComment_t>(y.m_inside[i])->m_comment
                        );
                        r += "\n";
                        break;
                    }
                    case trivia_nodeType::EndOfLine: {
                        r += "\n";
                        break;
                    }
                    case trivia_nodeType::Semicolon: {
                        r += "; ";
                        break;
                    }
                }
            }
        } else {
            return "\n";
        }
        return r;
    }

    std::string print_trivia_after(const trivia_t &x) {
        std::string r = " ";
        auto y = (TriviaNode_t &)x;
        if(y.n_after > 0) {
            for (size_t i=0; i<y.n_after; i++) {
                switch (y.m_after[i]->type) {
                    case trivia_nodeType::Comment: {
                        r += std::string(
                            down_cast<Comment_t>(y.m_after[i])->m_comment
                        );
                        r += "\n";
                        break;
                    }
                    case trivia_nodeType::EOLComment: {
                        r += std::string(
                            down_cast<EOLComment_t>(y.m_after[i])->m_comment
                        );
                        r += "\n";
                        break;
                    }
                    case trivia_nodeType::EndOfLine: {
                        r += "\n";
                        break;
                    }
                    case trivia_nodeType::Semicolon: {
                        r += "; ";
                        break;
                    }
                }
            }
        } else {
            return "\n";
        }
        return r;
    }

    void visit_TranslationUnit(const TranslationUnit_t &x) {
        std::string r;
        for (size_t i=0; i<x.n_items; i++) {
            this->visit_ast(*x.m_items[i]);
            r += s;
            if (i < x.n_items-1) {
                if (is_a<expr_t>(*x.m_items[i]) || (
                        (  is_a<mod_t>(*x.m_items[i])
                        || is_a<program_unit_t>(*x.m_items[i])
                        ) &&
                        (  is_a<mod_t>(*x.m_items[i+1])
                        || is_a<program_unit_t>(*x.m_items[i+1])
                        )
                            )
                        ) {
                    r.append("\n");
                }
            }
        }
        s = r;
    }

    void visit_Module(const Module_t &x) {
        std::string r = "";
        r += syn(gr::UnitHeader);
        r.append("module");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }

        r += format_unit_body(x);

        r += syn(gr::UnitHeader);
        r.append("end module");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }
    void visit_Submodule(const Submodule_t &x) {
        std::string r = "";
        r += syn(gr::UnitHeader);
        r.append("submodule");
        r += syn();
        r += " (";
        r.append(x.m_id);
        if(x.m_parent_name) {
            r += ":";
            r.append(x.m_parent_name);
        }
        r += ") ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }
        if (indent_unit) inc_indent();
        if(x.n_use > 0) {
            for (size_t i=0; i<x.n_use; i++) {
                this->visit_unit_decl1(*x.m_use[i]);
                r.append(s);
            }
            r.append("\n");
        }
        if(x.n_decl > 0) {
            for (size_t i=0; i<x.n_decl; i++) {
                this->visit_unit_decl2(*x.m_decl[i]);
                r.append(s);
            }
            r.append("\n");
        }
        if (x.n_contains > 0) {
            r += "\n";
            r += syn(gr::UnitHeader);
            r.append("contains");
            r += syn();
            r += "\n\n";
            inc_indent();
            for (size_t i=0; i<x.n_contains; i++) {
                this->visit_program_unit(*x.m_contains[i]);
                r.append(s);
                r.append("\n");
            }
            dec_indent();
        }
        if (indent_unit) dec_indent();
        r += syn(gr::UnitHeader);
        r.append("end submodule");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_BlockData(const BlockData_t &x) {
        std::string r = indent;
        r += syn(gr::UnitHeader);
        r += "block data";
        r += syn();
        if(x.m_name) {
            r += " ";
            r.append(x.m_name);
        }
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_use; i++) {
            this->visit_unit_decl1(*x.m_use[i]);
            r.append(s);
        }
        r += format_implicit(x);
        for (size_t i=0; i<x.n_decl; i++) {
            this->visit_unit_decl2(*x.m_decl[i]);
            r.append(s);
        }
        dec_indent();
        r += indent;
        r += syn(gr::UnitHeader);
        r.append("end block data");
        r += syn();
        if(x.m_name) {
            r += " ";
            r.append(x.m_name);
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Program(const Program_t &x) {
        std::string r;
        r += syn(gr::UnitHeader);
        r.append("program");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }

        r += format_unit_body(x);
        r += syn(gr::UnitHeader);
        r.append("end program");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Subroutine(const Subroutine_t &x) {
        std::string r = indent;
        for (size_t i=0; i<x.n_attributes; i++) {
            visit_decl_attribute(*x.m_attributes[i]);
            r += s;
            r.append(" ");
        }
        r += syn(gr::UnitHeader);
        r.append("subroutine");
        r += syn();
        r += " ";
        r.append(x.m_name);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_arg(x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
        }
        r.append(")");
        if (x.m_bind) {
            r.append(" ");
            this->visit_bind(*x.m_bind);
            r.append(s);
        }
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }

        r += format_unit_body(x, !indent_unit);
        r += indent;
        r += syn(gr::UnitHeader);
        r.append("end subroutine");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Procedure(const Procedure_t &x) {
        std::string r = indent;
        for (size_t i=0; i<x.n_attributes; i++) {
            visit_decl_attribute(*x.m_attributes[i]);
            r += s;
            r.append(" ");
        }
        r += syn(gr::UnitHeader);
        r.append("procedure");
        r += syn();
        r += " ";
        r.append(x.m_name);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_arg(x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
        }
        r.append(")");
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }

        r += format_unit_body(x, !indent_unit);
        r += indent;
        r += syn(gr::UnitHeader);
        r.append("end procedure");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_DerivedType(const DerivedType_t &x) {
        std::string r = indent;
        r += syn(gr::UnitHeader);
        r.append("type");
        r += syn();
        for (size_t i=0; i<x.n_attrtype; i++) {
            if (i == 0) r.append(", ");
            this->visit_decl_attribute(*x.m_attrtype[i]);
            r.append(s);
            if (i < x.n_attrtype-1) r.append(", ");
        }
        r.append(" :: ");
        r.append(x.m_name);
        for (size_t i=0; i<x.n_namelist; i++) {
            if (i == 0) r += "(";
            r.append(x.m_namelist[i]);
            if (i < x.n_namelist-1) r.append(", ");
            else r += ")";
        }
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_items; i++) {
            visit_unit_decl2(*x.m_items[i]);
            r.append(s);
        }
        dec_indent();
        if (x.n_contains) {
            r += "\n";
            r += syn(gr::UnitHeader);
            r.append("contains");
            r += syn();
            r += "\n\n";
            for (size_t i=0; i<x.n_contains; i++) {
                this->visit_procedure_decl(*x.m_contains[i]);
                r.append(s);
                // r.append("\n");
            }
        }
        r += syn(gr::UnitHeader);
        r.append(indent + "end type ");
        r += syn();
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }
    void visit_DerivedTypeProc(const DerivedTypeProc_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("procedure");
        r += syn();
        if (x.m_name) {
            r += "(";
            r.append(x.m_name);
            r += ")";
        }
        for (size_t i=0; i<x.n_attr; i++) {
            if (i == 0) r.append(", ");
            this->visit_decl_attribute(*x.m_attr[i]);
            r.append(s);
            if (i < x.n_attr-1) r.append(", ");
        }
        r.append(" :: ");
        for (size_t i=0; i<x.n_symbols; i++) {
            this->visit_use_symbol(*x.m_symbols[i]);
            r.append(s);
            if (i < x.n_symbols-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }
    void visit_GenericOperator(const GenericOperator_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("generic");
        r += syn();
        if(x.n_attr > 0 && x.m_attr[0] != nullptr){
            r += ", ";
            this->visit_decl_attribute(*x.m_attr[0]);
            r.append(s);
        }
        r += " :: operator(" + intrinsicop2str(x.m_op) + ")";
        r += " => ";
        for (size_t i=0; i<x.n_names; i++) {
            r.append(x.m_names[i]);
            if (i < x.n_names-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }
    void visit_GenericDefinedOperator(const GenericDefinedOperator_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("generic");
        r += syn();
        if(x.n_attr > 0 && x.m_attr[0] != nullptr){
            r += ", ";
            this->visit_decl_attribute(*x.m_attr[0]);
            r.append(s);
        }
        r += " :: operator(";
        r += "." + std::string(x.m_optype) + ".";
        r += ")";
        r += " => ";
        for (size_t i=0; i<x.n_names; i++) {
            r.append(x.m_names[i]);
            if (i < x.n_names-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }
    void visit_GenericAssignment(const GenericAssignment_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("generic");
        r += syn();
        if(x.n_attr > 0 && x.m_attr[0] != nullptr){
            r += ", ";
            this->visit_decl_attribute(*x.m_attr[0]);
            r.append(s);
        }
        r += " :: assignment(=) => ";
        for (size_t i=0; i<x.n_names; i++) {
            r.append(x.m_names[i]);
            if (i < x.n_names-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }
    void visit_GenericName(const GenericName_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("generic");
        r += syn();
        if(x.n_attr > 0 && x.m_attr[0] != nullptr){
            r += ", ";
            this->visit_decl_attribute(*x.m_attr[0]);
            r.append(s);
        }
        r += " :: ";
        r.append(x.m_name);
        r += " => ";
        for (size_t i=0; i<x.n_names; i++) {
            r.append(x.m_names[i]);
            if (i < x.n_names-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_GenericWrite(const GenericWrite_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("generic");
        r += syn();
        if(x.n_attr > 0 && x.m_attr[0] != nullptr){
            r += ", ";
            this->visit_decl_attribute(*x.m_attr[0]);
            r.append(s);
        }
        r += " :: ";
        r += "write(";
        r.append(x.m_id);
        r += ")";
        r += " => ";
        for (size_t i=0; i<x.n_names; i++) {
            r.append(x.m_names[i]);
            if (i < x.n_names-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_GenericRead(const GenericRead_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("generic");
        r += syn();
        if(x.n_attr > 0 && x.m_attr[0] != nullptr){
            r += ", ";
            this->visit_decl_attribute(*x.m_attr[0]);
            r.append(s);
        }
        r += " :: ";
        r += "read(";
        r.append(x.m_id);
        r += ")";
        r += " => ";
        for (size_t i=0; i<x.n_names; i++) {
            r.append(x.m_names[i]);
            if (i < x.n_names-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_FinalName(const FinalName_t &x) {
        std::string r;
        r += syn(gr::String);
        r.append("final :: ");
        r += syn();
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Private(const Private_t &x) {
        std::string r;
        r += syn(gr::Type);
        r.append("private");
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Enum(const Enum_t & x) {
        std::string r = indent;
        r += syn(gr::UnitHeader);
        r.append("enum");
        r += syn();
        if(x.n_attr) {
            r.append(", ");
            for (size_t i=0; i<x.n_attr; i++) {
                this->visit_decl_attribute(*x.m_attr[i]);
                r.append(s);
            }
        }

        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_items; i++) {
            this->visit_unit_decl2(*x.m_items[i]);
            r.append(s);
        }
        dec_indent();
        r += syn(gr::UnitHeader);
        r.append("end enum");
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Interface(const Interface_t &x) {
        std::string r;
        if(x.m_header->type == AbstractInterfaceHeader) {
            r += "abstract ";
        }
        r += syn(gr::UnitHeader);
        r.append("interface");
        r += syn();
        this->visit_interface_header(*x.m_header);
        r.append(s);
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_items; i++) {
            this->visit_interface_item(*x.m_items[i]);
            r.append(s);
        }
        dec_indent();
        r += syn(gr::UnitHeader);
        r.append("end interface");
        r += syn();
        this->visit_interface_header(*x.m_header);
        r.append(s);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_InterfaceHeader(const InterfaceHeader_t &/* x */) {
        s = "";
    }

    void visit_InterfaceHeaderName(const InterfaceHeaderName_t &x) {
        s = " ";
        s += x.m_name;
    }

    void visit_InterfaceHeaderAssignment
            (const InterfaceHeaderAssignment_t &/* x */) {
        s = " assignment (=)";
    }

    void visit_InterfaceHeaderOperator
            (const InterfaceHeaderOperator_t &x) {
        s = " operator (" + intrinsicop2str(x.m_op) + ")";
    }

    void visit_InterfaceHeaderDefinedOperator
            (const InterfaceHeaderDefinedOperator_t &x) {
        s = " operator (." + std::string(x.m_operator_name) + ".)";
    }

    void visit_AbstractInterfaceHeader
            (const AbstractInterfaceHeader_t &/* x */) {
        s = "";
    }

    void visit_InterfaceHeaderWrite(const InterfaceHeaderWrite_t &x) {
        s = " write(";
        s.append(x.m_id);
        s += ")";
    }
    void visit_InterfaceHeaderRead(const InterfaceHeaderRead_t &x) {
        s = " read(";
        s.append(x.m_id);
        s += ")";
    }

    void visit_InterfaceModuleProcedure(const InterfaceModuleProcedure_t &x) {
        std::string r = indent;
        for (size_t i=0; i<x.n_attributes; i++) {
            visit_decl_attribute(*x.m_attributes[i]);
            r += s;
            r.append(" ");
        }
        r += syn(gr::UnitHeader);
        r.append("procedure ");
        r += syn();
        for (size_t i=0; i<x.n_names; i++) {
            r.append(x.m_names[i]);
            if (i < x.n_names-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_InterfaceProc(const InterfaceProc_t &x) {
        std::string r;
        this->visit_program_unit(*x.m_proc);
        r.append(s);
        s = r;
    }

    template <typename T>
    std::string format_import(const T &x) {
        std::string r;
        for (size_t i=0; i<x.n_import; i++) {
            this->visit_import_statement(*x.m_import[i]);
            r.append(s);
        }
        return r;
    }

    std::string format_import(const Program_t &/*x*/) {
        return "";
    }

    std::string format_import(const Module_t &/*x*/) {
        return "";
    }

    template <typename T>
    std::string format_implicit(const T &x) {
        std::string r;
        for (size_t i=0; i<x.n_implicit; i++) {
            this->visit_implicit_statement(*x.m_implicit[i]);
            r.append(s);
        }
        return r;
    }

    template <typename T>
    std::string format_body(const T &x) {
        std::string r;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        return r;
    }

    std::string format_body(const Module_t &/*x*/) {
        return "";
    }


    template <typename T>
    std::string format_unit_body(const T &x, bool indent_contains=false) {
        std::string r;
        if (indent_unit) inc_indent();
        for (size_t i=0; i<x.n_use; i++) {
            this->visit_unit_decl1(*x.m_use[i]);
            r.append(s);
        }
        r += format_import(x);
        r += format_implicit(x);
        for (size_t i=0; i<x.n_decl; i++) {
            this->visit_unit_decl2(*x.m_decl[i]);
            r.append(s);
        }
        r += format_body(x);
        if (x.n_contains > 0) {
            r += "\n";
            r += syn(gr::UnitHeader);
            if (indent_unit) dec_indent();
            r.append(indent + "contains");
            if (indent_unit) inc_indent();
            r += syn();
            r += "\n\n";
            if (indent_contains) inc_indent();
            for (size_t i=0; i<x.n_contains; i++) {
                this->visit_program_unit(*x.m_contains[i]);
                r.append(s);
                r.append("\n");
            }
            if (indent_contains) dec_indent();
        }
        if (indent_unit) dec_indent();
        return r;
    }

    void visit_Function(const Function_t &x) {
        std::string r = indent;
        for (size_t i=0; i<x.n_attributes; i++) {
            visit_decl_attribute(*x.m_attributes[i]);
            r += s;
            r.append(" ");
        }
        r += syn(gr::UnitHeader);
        r.append("function");
        r += syn();
        r += " ";
        r.append(x.m_name);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_arg(x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
        }
        r.append(")");
        if (x.m_return_var) {
            r += " ";
            r += syn(gr::UnitHeader);
            r.append("result");
            r += syn();
            r += "(";
            this->visit_expr(*x.m_return_var);
            r.append(s);
            r.append(")");
        }
        if (x.m_bind) {
            r.append(" ");
            this->visit_bind(*x.m_bind);
            r.append(s);
        }
        if(x.m_trivia){
            r += print_trivia_inside(*x.m_trivia);
        } else {
            r.append("\n");
        }

        r += format_unit_body(x, !indent_unit);
        r += indent;
        r += syn(gr::UnitHeader);
        r.append("end function");
        r += syn();
        r += " ";
        r.append(x.m_name);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Use(const Use_t &x) {
        std::string r = indent;
        r += syn(gr::UnitHeader);
        r += "use";
        r += syn();
        for (size_t i=0; i<x.n_nature; i++) {
            r += ", ";
            this->visit_decl_attribute(*x.m_nature[i]);
            r.append(s);
            r += " ::";
        }
        r += " ";
        r.append(x.m_module);
        if (x.m_only_present || x.n_symbols > 0) {
            r.append(", ");
        }
        if (x.m_only_present) {
            r += syn(gr::UnitHeader);
            r += "only";
            r += syn();
            r += ":";
            if (x.n_symbols > 0) r.append(" ");
        }
        for (size_t i=0; i<x.n_symbols; i++) {
            this->visit_use_symbol(*x.m_symbols[i]);
            r.append(s);
            if (i < x.n_symbols-1) r.append(", ");
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Import(const Import_t &x) {
        std::string r = indent;
        r += syn(gr::UnitHeader);
        r += "import";
        r += syn();
        if (x.m_mod == import_modifierType::ImportNone) {
            r.append(", ");
            r += syn(gr::UnitHeader);
            r += "none";
            r += syn();
        } else if (x.m_mod == import_modifierType::ImportOnly) {
            r.append(", ");
            r += syn(gr::UnitHeader);
            r += "only";
            r += syn();
            r += ":";
        } else if (x.m_mod == import_modifierType::ImportAll) {
            r.append(", ");
            r += syn(gr::UnitHeader);
            r += "all";
            r += syn();
        } else if (x.m_mod == import_modifierType::ImportDefault) {
            if (x.n_symbols) r += " ::";
        }
        if (x.n_symbols > 0) {
            r += " ";
            for (size_t i=0; i<x.n_symbols; i++) {
                r.append(x.m_symbols[i]);
                if (i < x.n_symbols-1) r.append(", ");
            }
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ImplicitNone(const ImplicitNone_t &x) {
        std::string r = indent;
        r += syn(gr::UnitHeader);
        r += "implicit none";
        r += syn();
        if (x.n_specs > 0) {
            r += " (";
            for (size_t i=0; i<x.n_specs; i++) {
                visit_implicit_none_spec(*x.m_specs[i]);
                r += s;
                if (i < x.n_specs-1) r.append(", ");
            }
            r += ")";
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ImplicitNoneExternal(const ImplicitNoneExternal_t &/*x*/) {
        s = "external";
    }
    void visit_ImplicitNoneType(const ImplicitNoneType_t &/*x*/) {
        s = "type";
    }

    void visit_Implicit(const Implicit_t &x) {
        std::string r = indent;
        r += syn(gr::UnitHeader);
        r += "implicit";
        r += syn();
        r += " ";
        visit_decl_attribute(*x.m_type);
        r += s;
        if (x.n_kind > 0) {
            r += " (";
            for (size_t i=0; i<x.n_kind; i++) {
                visit_letter_spec(*x.m_kind[i]);
                r += s;
            }
            r += ")";
        }
        if (x.n_specs > 0) {
            r += " (";
            for (size_t i=0; i<x.n_specs; i++) {
                visit_letter_spec(*x.m_specs[i]);
                r += s;
                if (i < x.n_specs-1) r.append(",");
            }
            r += ")";
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_LetterSpec(const LetterSpec_t &x) {
        std::string r;
        if (x.m_start) {
            r += x.m_start;
            r += "-";
        }
        r += x.m_end;
        s = r;
    }

    void visit_Declaration(const Declaration_t &x) {
        std::string r = indent;
        if (x.m_vartype == nullptr &&
                x.n_attributes == 1 &&
                is_a<SimpleAttribute_t>(*x.m_attributes[0]) &&
                down_cast<SimpleAttribute_t>(x.m_attributes[0])->m_attr ==
                    simple_attributeType::AttrParameter) {
            // The parameter statement is printed differently than other
            // attributes
            r += syn(gr::Type);
            r += "parameter";
            r += syn();
            r += "(";
            for (size_t i=0; i<x.n_syms; i++) {
                visit_var_sym(x.m_syms[i]);
                r += s;
                if (i < x.n_syms-1) r.append(", ");
            }
            r += ")";
        } else if (x.m_vartype == nullptr &&
                x.n_attributes == 1 &&
                is_a<AttrNamelist_t>(*x.m_attributes[0])) {
            // The namelist statement is printed differently than other
            // atttributes
            r += syn(gr::Type);
            r.append("namelist");
            r += syn();
            r.append(" /");
            r += down_cast<AttrNamelist_t>(x.m_attributes[0])->m_name;
            r.append("/ ");
            for (size_t i=0; i<x.n_syms; i++) {
                visit_var_sym(x.m_syms[i]);
                r += s;
                if (i < x.n_syms-1) r.append(", ");
            }
        } else if (x.m_vartype == nullptr && x.n_attributes == 1 &&
                is_a<SimpleAttribute_t>(*x.m_attributes[0]) &&
                down_cast<SimpleAttribute_t>(x.m_attributes[0])->m_attr
                == simple_attributeType::AttrCommon) {
            visit_Common(x);
            r.append(s);
        } else {
            if (x.m_vartype) {
                visit_decl_attribute(*x.m_vartype);
                r += s;
                if (x.n_attributes > 0) r.append(", ");
            }
            for (size_t i=0; i<x.n_attributes; i++) {
                if(x.m_attributes[i]->type == decl_attributeType::AttrData
                        && i == 0 ){
                    r += syn(gr::Type);
                    r += "data ";
                    r += syn();
                }
                visit_decl_attribute(*x.m_attributes[i]);
                r += s;
                if (i < x.n_attributes-1) r.append(", ");
            }
            if (x.n_syms > 0) {
                r.append(" :: ");
                for (size_t i=0; i<x.n_syms; i++) {
                    visit_var_sym(x.m_syms[i]);
                    r += s;
                    if (i < x.n_syms-1) r.append(", ");
                }
            }
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_var_sym(const var_sym_t &x) {
        std::string r = "";
        if(x.m_name && x.m_sym == Slash){
            r += "/";
            r.append(x.m_name);
            r += "/";
        } else if(x.m_name) {
            r.append(x.m_name);
            if(x.m_sym == DoubleAsterisk) {
                r += symbol2str(x.m_sym);
            }
        }
        if (x.n_dim > 0) {
            r.append("(");
            for (size_t i=0; i<x.n_dim; i++) {
                visit_dimension(x.m_dim[i]);
                r += s;
                if (i < x.n_dim-1) r.append(",");
            }
            r.append(")");
        }
        if (x.n_codim > 0) {
            r.append("[");
            for (size_t i=0; i<x.n_codim; i++) {
                visit_codimension(x.m_codim[i]);
                r += s;
                if (i < x.n_codim-1) r.append(",");
            }
            r.append("]");
        }
        if (x.m_initializer) {
            visit_expr(*x.m_initializer);
            r += symbol2str(x.m_sym) + s;
        }
        if (x.m_spec) {
            this->visit_decl_attribute(*x.m_spec);
            r.append(s);
        }
        s = r;
    }

    void visit_Common(const Declaration_t &x) {
        std::string r;
        r += syn(gr::Type);
        r += "common ";
        r += syn();
        for (size_t i=0; i<x.n_syms; i++) {
            if(x.m_syms[i].m_name){
                r += "/";
                r.append(x.m_syms[i].m_name);
                r += "/ ";
            }
            if (x.m_syms[i].m_initializer) {
                visit_expr(*x.m_syms[i].m_initializer);
                r += s;
            }
            if (i < x.n_syms-1) r.append(", ");
        }
        s = r;
    }

    void visit_AttrData(const AttrData_t &x) {
        std::string r;
        for (size_t i=0; i<x.n_object; i++) {
            this->visit_expr(*x.m_object[i]);
            r.append(s);
            if (i < x.n_object-1) r.append(", ");
        }
        r += "/";
        for (size_t i=0; i<x.n_value; i++) {
            this->visit_expr(*x.m_value[i]);
            r.append(s);
            if (i < x.n_value-1) r.append(", ");
        }
        r += "/";
        s = r;
    }

    void visit_DataImpliedDo(const DataImpliedDo_t &x) {
        std::string r;
        r += "(";
        for (size_t i=0; i<x.n_object_list; i++) {
            this->visit_expr(*x.m_object_list[i]);
            r.append(s);
            if (i < x.n_object_list-1) r.append(", ");
        }
        r += ", ";
        if (x.m_type) {
            this->visit_decl_attribute(*x.m_type);
            r.append(s);
            r += " :: ";
        }
        r.append(x.m_var);
        r += " = ";
        this->visit_expr(*x.m_start);
        r.append(s);
        r += ", ";
        this->visit_expr(*x.m_end);
        r.append(s);
        if (x.m_increment) {
            r += ", ";
            this->visit_expr(*x.m_increment);
            r.append(s);
        }
        r += ")";
        s = r;
        last_expr_precedence = 13;
    }

    void visit_AttrEquivalence(const AttrEquivalence_t &x) {
        std::string r;
        r += syn(gr::Type);
        r += "equivalence ";
        r += syn();
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_equi(x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
        }
        s = r;
    }

    void visit_equi(const equi_t &x) {
        std::string r;
        r += "(";
        for (size_t i=0; i<x.n_set_list; i++) {
            this->visit_expr(*x.m_set_list[i]);
            r.append(s);
            if (i < x.n_set_list-1) r.append(", ");
        }
        r += ")";
        s = r;
    }
#define ATTRTYPE(x) \
            case (simple_attributeType::Attr##x) : \
                r.append(to_lower(#x)); \
                break;

    void visit_SimpleAttribute(const SimpleAttribute_t &x) {
        std::string r;
        r += syn(gr::Type);
        switch (x.m_attr) {
            ATTRTYPE(Abstract)
            ATTRTYPE(Allocatable)
            ATTRTYPE(Asynchronous)
            ATTRTYPE(Contiguous)
            ATTRTYPE(Deferred)
            ATTRTYPE(Elemental)
            ATTRTYPE(Enumerator)
            ATTRTYPE(External)
            ATTRTYPE(Impure)
            ATTRTYPE(Intrinsic)
            ATTRTYPE(Kind)
            ATTRTYPE(Len)
            ATTRTYPE(Module)
            ATTRTYPE(NoPass)
            ATTRTYPE(NonDeferred)
            ATTRTYPE(Non_Intrinsic)
            ATTRTYPE(Optional)
            ATTRTYPE(Parameter)
            ATTRTYPE(Pointer)
            ATTRTYPE(Private)
            ATTRTYPE(Protected)
            ATTRTYPE(Public)
            ATTRTYPE(Pure)
            ATTRTYPE(Recursive)
            ATTRTYPE(Save)
            ATTRTYPE(Sequence)
            ATTRTYPE(Target)
            ATTRTYPE(Value)
            ATTRTYPE(Volatile)
            default :
                throw LFortranException("Attribute type not implemented");
        }
        r += syn();
        s = r;
    }

#define ATTRTYPE2(x, y) \
            case (decl_typeType::Type##x) : \
                r.append(y); \
                break;

    void visit_AttrType(const AttrType_t &x) {
        std::string r;
        r += syn(gr::Type);
        switch (x.m_type) {
            ATTRTYPE2(Class, "class")
            ATTRTYPE2(Character, "character")
            ATTRTYPE2(Complex, "complex")
            ATTRTYPE2(DoublePrecision, "double precision")
            ATTRTYPE2(Integer, "integer")
            ATTRTYPE2(Logical, "logical")
            ATTRTYPE2(Procedure, "procedure")
            ATTRTYPE2(Real, "real")
            ATTRTYPE2(Type, "type")
            default :
                throw LFortranException("Attribute type not implemented");
        }
        r += syn();
        if (x.n_kind > 0) {
            r.append("(");

            // Determine proper canonical printing of kinds
            // TODO: Move this part into a separate AST pass
            kind_item_t k[2];
            LFORTRAN_ASSERT(x.n_kind <= 2);
            for (size_t i=0; i<x.n_kind; i++) {
                k[i] = x.m_kind[i];
            }
            if (x.n_kind == 1 && (
                    x.m_type == decl_typeType::TypeReal ||
                    x.m_type == decl_typeType::TypeInteger ||
                    x.m_type == decl_typeType::TypeLogical ||
                    x.m_type == decl_typeType::TypeComplex
                ) && (
                    k[0].m_id == nullptr || std::string(k[0].m_id) == "kind"
                )) {
                k[0].m_id = nullptr;
            } else if (x.n_kind == 1 &&
                    x.m_type == decl_typeType::TypeCharacter
                && (
                    k[0].m_id == nullptr || std::string(k[0].m_id) == "len"
                )) {
                k[0].m_id = (char*)"len";
            } else if (x.n_kind == 2 &&
                    x.m_type == decl_typeType::TypeCharacter
                && (
                    k[0].m_id != nullptr && k[1].m_id != nullptr
                ) && std::string(k[1].m_id) == "len") {
                    std::swap(k[0], k[1]);
            }
            if (x.m_type == decl_typeType::TypeCharacter &&
                k[0].m_id == nullptr) {
                    k[0].m_id = (char*)"len";
            }

            for (size_t i=0; i<x.n_kind; i++) {
                visit_kind_item(k[i]);
                r += s;
                if (i < x.n_kind-1) r.append(", ");
            }
            r.append(")");
        }
        if (x.m_name) {
            r.append("(");
            r.append(x.m_name);
            r.append(")");
        }
        if (x.m_sym == symbolType::Asterisk) {
            r.append("(*)");
        } else if(x.m_sym == symbolType::DoubleAsterisk){
            r.append("*(*)");
        }
        s = r;
    }

    void visit_kind_item(const kind_item_t &x) {
        std::string r;
        if (x.m_id) {
            r.append(x.m_id);
            r.append("=");
        }
        r += kind_value(x.m_type, x.m_value);
        s = r;
    }

    void visit_AttrIntent(const AttrIntent_t &x) {
        std::string r;
        r += syn(gr::Type);
        r += "intent";
        r += syn();
        r += "(";
        r += syn(gr::Type);
        switch (x.m_intent) {
            case (attr_intentType::In) : {
                r.append("in");
                break;
            }
            case (attr_intentType::Out) : {
                r.append("out");
                break;
            }
            case (attr_intentType::InOut) : {
                r.append("inout");
                break;
            }
        }
        r += syn();
        r += ")";
        s = r;
    }

    void visit_AttrBind(const AttrBind_t &x) {
        visit_bind(*x.m_bind);
    }

    void visit_AttrExtends(const AttrExtends_t &x) {
        std::string r;
        r += syn(gr::Type);
        r += "extends";
        r += syn();
        r += "(";
        r.append(x.m_name);
        r += ")";
        s = r;
    }

    void visit_AttrDimension(const AttrDimension_t &x) {
        std::string r;
        r += syn(gr::Type);
        r += "dimension";
        r += syn();
        if (x.n_dim > 0) {
            r += "(";
            for (size_t i=0; i<x.n_dim; i++) {
                visit_dimension(x.m_dim[i]);
                r += s;
                if (i < x.n_dim-1) r.append(",");
            }
            r += ")";
        }
        s = r;
    }

    void visit_AttrCodimension(const AttrCodimension_t &x) {
        std::string r;
        r += syn(gr::Type);
        r += "codimension";
        r += syn();
        if (x.n_codim > 0) {
            r += "[";
            for (size_t i=0; i<x.n_codim; i++) {
                visit_codimension(x.m_codim[i]);
                r += s;
                if (i < x.n_codim-1) r.append(",");
            }
            r += "]";
        }
        s = r;
    }

    void visit_AttrPass(const AttrPass_t &x) {
        std::string r;
        r += syn(gr::Type);
        r += "pass";
        r += syn();
        if (x.m_name) {
            r += "(";
            r.append(x.m_name);
            r += ")";
        }
        s = r;
    }
    void visit_AttrAssignment(const AttrAssignment_t &/*x*/) {
        s = "assignment (=)";
    }

    void visit_AttrIntrinsicOperator(const AttrIntrinsicOperator_t &x) {
        s = "operator (" + intrinsicop2str(x.m_op) + ")";
    }

    void visit_AttrDefinedOperator(const AttrDefinedOperator_t &x) {
        s = "operator (." + std::string(x.m_op_name) + ".)";
    }

    void visit_AttrStat(const AttrStat_t &x) {
        std::string r;
        r = "stat = ";
        r.append(x.m_variable);
        s = r;
    }

    void visit_AttrErrmsg(const AttrErrmsg_t &x) {
        std::string r;
        r = "errmsg = ";
        r.append(x.m_variable);
        s = r;
    }

    void visit_AttrNewIndex(const AttrNewIndex_t &x) {
        std::string r;
        r = "new_index = ";
        this->visit_expr(*x.m_value);
        r.append(s);
        s = r;
    }

    void visit_AttrEventWaitKwArg(const AttrEventWaitKwArg_t &x) {
        std::string r = "";
        r.append(x.m_id);
        r += " = ";
        this->visit_expr(*x.m_value);
        r.append(s);
        s = r;
    }

    void visit_Assign(const Assign_t &x) {
        std::string r = indent;
        r += print_label(x);
        r.append("assign");
        r += " " + std::to_string(x.m_assign_label);
        r += " to ";
        r += x.m_variable;
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Assignment(const Assignment_t &x) {
        std::string r = indent;
        r += print_label(x);
        this->visit_expr(*x.m_target);
        r.append(s);
        r.append(" = ");
        this->visit_expr(*x.m_value);
        r.append(s);
        if (x.m_trivia) {
            r += print_trivia_after(*x.m_trivia);
        } else {
            r += "\n";
        }
        s = r;
    }

    void visit_GoTo(const GoTo_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Call);
        r += "go to";
        r += syn();
        r.append(" ");
        if(x.n_labels > 0) {
            r += "(";
            for (size_t i=0; i<x.n_labels; i++) {
                this->visit_expr(*x.m_labels[i]);
                r.append(s);
                if (i < x.n_labels-1) r.append(", ");
            }
            r += "), ";
        }
        this->visit_expr(*x.m_goto_label);
        r.append(s);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Associate(const Associate_t &x) {
        std::string r = indent;
        r += print_label(x);
        this->visit_expr(*x.m_target);
        r.append(s);
        r.append(" => ");
        this->visit_expr(*x.m_value);
        r.append(s);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_SubroutineCall(const SubroutineCall_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Call);
        r += "call";
        r += syn();
        r += " ";
        for (size_t i=0; i<x.n_member; i++) {
            r.append(x.m_member[i].m_name);
            for (size_t j=0; j<x.m_member[i].n_args; j++) {
                r += "(";
                expr_t *start = x.m_member[i].m_args[j].m_start;
                expr_t *end = x.m_member[i].m_args[j].m_end;
                expr_t *step = x.m_member[i].m_args[j].m_step;
                if (step != nullptr) {
                    if (start) {
                        this->visit_expr(*start);
                        r.append(s);
                    }
                    r += ":";
                    if (end) {
                        this->visit_expr(*end);
                        r.append(s);
                    }
                    if (is_a<Num_t>(*step) &&
                            down_cast<Num_t>(step)->m_n != 1) {
                        r += ":";
                        this->visit_expr(*step);
                        r.append(s);
                    }
                } else if (end != nullptr && start == nullptr) {
                    this->visit_expr(*end);
                    r.append(s);
                } else {
                    throw LFortranException("Incorrect array elements");
                }
                r += ")";
            }
            r.append("%");
        }
        r.append(x.m_name);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i].m_end) {
                this->visit_expr(*x.m_args[i].m_end);
                r.append(s);
            } else {
                r += ":";
            }
            if (i < x.n_args-1) r.append(", ");
            else if (x.n_keywords > 0) r.append(", ");
        }
        for (size_t i=0; i<x.n_keywords; i++) {
            this->visit_keyword(x.m_keywords[i]);
            r.append(s);
            if (i < x.n_keywords-1) r.append(", ");
        }
        r.append(")");
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Allocate(const Allocate_t &x) {
        std::string r = indent;
        r.append("allocate");
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i].m_start) {
                this->visit_expr(*x.m_args[i].m_start);
                r.append(s);
                r += ":";
            }
            if (x.m_args[i].m_end) {
                this->visit_expr(*x.m_args[i].m_end);
                r.append(s);
            } else {
                r += ":";
            }
            if (x.m_args[i].m_step) {
                this->visit_expr(*x.m_args[i].m_step);
                r.append(s);
            }
            if (i < x.n_args-1) r.append(", ");
        }
        if (x.n_keywords > 0) r.append(", ");
        for (size_t i=0; i<x.n_keywords; i++) {
            r.append(std::string(x.m_keywords[i].m_arg));
            r += "=";
            this->visit_expr(*x.m_keywords[i].m_value);
            r.append(s);
            if (i < x.n_keywords-1) r.append(", ");
        }
        r.append(")");
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Deallocate(const Deallocate_t &x) {
        std::string r = indent;
        r += print_label(x);
        r.append("deallocate");
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i].m_start) {
                this->visit_expr(*x.m_args[i].m_start);
                r.append(s);
            }
            if (x.m_args[i].m_end) {
                this->visit_expr(*x.m_args[i].m_end);
                r.append(s);
            } else {
                r += ":";
            }
            if (x.m_args[i].m_step) {
                this->visit_expr(*x.m_args[i].m_step);
                r.append(s);
            }
            if (i < x.n_args-1) r.append(", ");
        }
        if (x.n_keywords > 0) r.append(", ");
        for (size_t i=0; i<x.n_keywords; i++) {
            r.append(std::string(x.m_keywords[i].m_arg));
            r += "=";
            this->visit_expr(*x.m_keywords[i].m_value);
            r.append(s);
            if (i < x.n_keywords-1) r.append(", ");
        }
        r.append(")");
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_If(const If_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Conditional);
        r += "if";
        r += syn();
        r += " (";
        this->visit_expr(*x.m_test);
        r += s;
        r += ") ";
        r += syn(gr::Conditional);
        r += "then";
        r += syn();
        if(x.m_if_trivia){
            r += print_trivia_after(*x.m_if_trivia);
        } else if(x.m_else_trivia) {
            r += print_trivia_inside(*x.m_else_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        if (x.n_orelse > 0) {
            r += indent;
            r += syn(gr::Conditional);
            r += "else";
            r += syn();
            if(x.m_else_trivia){
                r += print_trivia_after(*x.m_else_trivia);
            } else {
                r.append("\n");
            }
            inc_indent();
            for (size_t i=0; i<x.n_orelse; i++) {
                this->visit_stmt(*x.m_orelse[i]);
                r += s;
            }
            dec_indent();
        }
        r += indent;
        r += syn(gr::Conditional);
        r += "end if";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_IfArithmetic(const IfArithmetic_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Conditional);
        r += "if";
        r += syn();
        r += " (";
        this->visit_expr(*x.m_test);
        r += s;
        r += ") ";
        r += std::to_string(x.m_lt_label);
        r += ", " + std::to_string(x.m_eq_label);
        r += ", " + std::to_string(x.m_gt_label);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Where(const Where_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Repeat);
        r += "where";
        r += syn();
        r += " (";
        this->visit_expr(*x.m_test);
        r += s;
        r += ")";
        if(x.m_t_inside){
            r += print_trivia_inside(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        if (x.n_orelse > 0) {
            r += indent;
            r += syn(gr::Conditional);
            r += "else where";
            r += syn();
            if(x.m_t_inside){
                r += print_trivia_after(*x.m_t_inside);
            } else {
                r.append("\n");
            }
            inc_indent();
            for (size_t i=0; i<x.n_orelse; i++) {
                this->visit_stmt(*x.m_orelse[i]);
                r += s;
            }
            dec_indent();
        }
        r += indent;
        r += syn(gr::Repeat);
        r += "end where";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Entry(const Entry_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("entry");
        r += syn();
        r += " ";
        r.append(x.m_name);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_arg(x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
        }
        r.append(")");
        if (x.m_return_var) {
            r += " ";
            r += syn(gr::UnitHeader);
            r.append("result");
            r += syn();
            r += "(";
            this->visit_expr(*x.m_return_var);
            r.append(s);
            r.append(")");
        }
        if (x.m_bind) {
            r.append(" ");
            this->visit_bind(*x.m_bind);
            r.append(s);
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Stop(const Stop_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("stop");
        r += syn();
        if (x.m_code) {
            this->visit_expr(*x.m_code);
            r += " " + s;
        }
        if (x.m_quiet) {
            this->visit_expr(*x.m_quiet);
            r += ", quiet = " + s;
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ErrorStop(const ErrorStop_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("error stop");
        r += syn();
        if (x.m_code) {
            this->visit_expr(*x.m_code);
            r += " " + s;
        }
        if (x.m_quiet) {
            this->visit_expr(*x.m_quiet);
            r += ", quiet = " + s;
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_EventPost(const EventPost_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("event post");
        r += syn();
        r += " (";
        this->visit_expr(*x.m_variable);
        r.append(s);
        if (x.m_stat) {
            r += ", ";
            for (size_t i=0; i<x.n_stat; i++) {
                this->visit_event_attribute(*x.m_stat[i]);
                r.append(s);
            }
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_EventWait(const EventWait_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("event wait");
        r += syn();
        r += " (";
        this->visit_expr(*x.m_variable);
        r.append(s);
        if (x.m_spec) {
            r += ", ";
            for (size_t i=0; i<x.n_spec; i++) {
                this->visit_event_attribute(*x.m_spec[i]);
                r.append(s);
                if (i < x.n_spec-1) r.append(", ");
            }
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_SyncAll(const SyncAll_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("sync all");
        r += syn();
        if (x.m_stat) {
            r += " (";
            for (size_t i=0; i<x.n_stat; i++) {
                this->visit_event_attribute(*x.m_stat[i]);
                r.append(s);
                if (i < x.n_stat-1) r.append(", ");
            }
            r += ")";
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_SyncImages(const SyncImages_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("sync images");
        r += syn();
        r += "(";
        if (x.m_image_set) {
            this->visit_expr(*x.m_image_set);
            r.append(s);
        }
        if(x.m_sym == Asterisk) {
            r += symbol2str(x.m_sym);
        }
        for (size_t i=0; i<x.n_stat; i++) {
            if(i == 0) r += ", ";
            this->visit_event_attribute(*x.m_stat[i]);
            r.append(s);
            if (i < x.n_stat-1) r.append(", ");
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_SyncMemory(const SyncMemory_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("sync memory");
        r += syn();
        if (x.m_stat) {
            r += "(";
            for (size_t i=0; i<x.n_stat; i++) {
                this->visit_event_attribute(*x.m_stat[i]);
                r.append(s);
                if (i < x.n_stat-1) r.append(", ");
            }
            r += ")";
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_SyncTeam(const SyncTeam_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("sync team");
        r += syn();
        r += "(";
        this->visit_expr(*x.m_value);
        r.append(s);
        for (size_t i=0; i<x.n_stat; i++) {
            if(i == 0) r += ", ";
            this->visit_event_attribute(*x.m_stat[i]);
            r.append(s);
            if (i < x.n_stat-1) r.append(", ");
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_DoLoop(const DoLoop_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Repeat);
        r += "do";
        r += syn();
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
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        dec_indent();
        r += indent;
        r += syn(gr::Repeat);
        r.append("end do");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ImpliedDoLoop(const ImpliedDoLoop_t &x) {
        std::string r = "";
        r += "(";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            r.append(s);
            r.append(", ");
        }
        r.append(x.m_var);
        r.append(" = ");
        this->visit_expr(*x.m_start);
        r.append(s);
        r.append(", ");
        this->visit_expr(*x.m_end);
        r.append(s);
        if (x.m_increment) {
            r.append(", ");
            this->visit_expr(*x.m_increment);
            r.append(s);
        }
        r.append(")");
        s = r;
        last_expr_precedence = 13;
    }

    void visit_assoc(const var_sym_t &x) {
        std::string r = "";
        r.append(x.m_name);
        if (x.n_dim > 0) {
            r.append("(");
            for (size_t i=0; i<x.n_dim; i++) {
                visit_dimension(x.m_dim[i]);
                r += s;
                if (i < x.n_dim-1) r.append(",");
            }
            r.append(")");
        }
        if (x.m_initializer) {
            visit_expr(*x.m_initializer);
            r += " => " + s;
        }
        s = r;
    }

    void visit_AssociateBlock(const AssociateBlock_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::UnitHeader);
        r += "associate";
        r += syn();
        r.append(" (");
        for (size_t i=0; i<x.n_syms; i++) {
            this->visit_assoc(x.m_syms[i]);
            r.append(s);
            if (i < x.n_syms-1) r.append(", ");
        }
        r.append(")");
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        dec_indent();
        r += syn(gr::UnitHeader);
        r.append("end associate");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Block(const Block_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::UnitHeader);
        r += "block";
        r += syn();
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_use; i++) {
            this->visit_unit_decl1(*x.m_use[i]);
            r.append(s);
        }
        r += format_import(x);
        for (size_t i=0; i<x.n_decl; i++) {
            this->visit_unit_decl2(*x.m_decl[i]);
            r.append(s);
        }
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        dec_indent();
        r += indent;
        r += syn(gr::UnitHeader);
        r.append("end block");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ChangeTeam(const ChangeTeam_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::UnitHeader);
        r += "change team";
        r += syn();
        r += "(";
        this->visit_expr(*x.m_team_value);
        r.append(s);
        for (size_t i=0; i<x.n_coarray_assoc; i++) {
            if(i == 0) r += ", ";
            this->visit_team_attribute(*x.m_coarray_assoc[i]);
            r.append(s);
            if (i < x.n_coarray_assoc-1) r.append(", ");
        }
        for (size_t i=0; i<x.n_sync; i++) {
            if(i == 0) r += ", ";
            this->visit_event_attribute(*x.m_sync[i]);
            r.append(s);
            if (i < x.n_sync-1) r.append(", ");
        }
        r += ")";
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        dec_indent();
        r += indent;
        r += syn(gr::UnitHeader);
        r.append("end team");
        r += syn();
        if (x.m_sync_stat) {
            r += " (";
            for (size_t i=0; i<x.n_sync_stat; i++) {
                this->visit_event_attribute(*x.m_sync_stat[i]);
                r.append(s);
                if (i < x.n_sync_stat-1) r.append(", ");
            }
            r += ")";
        }
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_CoarrayAssociation(const CoarrayAssociation_t &x) {
        std::string r = "";
        this->visit_expr(*x.m_coarray);
        r.append(s);
        r += " => ";
        this->visit_expr(*x.m_selector);
        r.append(s);
        s = r;
    }

    void visit_Critical(const Critical_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::UnitHeader);
        r += "critical";
        r += syn();
        if (x.m_sync_stat) {
            r += " (";
            for (size_t i=0; i<x.n_sync_stat; i++) {
                this->visit_event_attribute(*x.m_sync_stat[i]);
                r.append(s);
            }
            r += ")";
        }
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        dec_indent();
        r += indent;
        r += syn(gr::UnitHeader);
        r.append("end critical");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_DoConcurrentLoop(const DoConcurrentLoop_t &x) {
        if (x.n_control != 1) {
            throw SemanticError("Do concurrent: exactly one control statement is required for now",
            x.base.base.loc);
        }
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Repeat);
        r += "do concurrent";
        r += syn();

        AST::ConcurrentControl_t &h = *(AST::ConcurrentControl_t*) x.m_control[0];
        r.append(" (");
        if (h.m_var) {
            r.append(h.m_var);
            r.append(" = ");
        }
        if (h.m_start) {
            this->visit_expr(*h.m_start);
            r.append(s);
            r.append(":");
        }
        if (h.m_end) {
            this->visit_expr(*h.m_end);
            r.append(s);
        }
        if (h.m_increment) {
            r.append(":");
            this->visit_expr(*h.m_increment);
            r.append(s);
        }
        r.append(")");
        if (x.m_mask) {
            this->visit_expr(*x.m_mask);
            r += s;
        }
        for (size_t i=0; i<x.n_locality; i++) {
            this->visit_concurrent_locality(*x.m_locality[i]);
            r.append(s);
        }
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        dec_indent();
        r += indent;
        r += syn(gr::Repeat);
        r.append("end do");
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ForAll(const ForAll_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Repeat);
        r += "forall";
        r += syn();
        r.append(" (");
        for (size_t i=0; i<x.n_control; i++) {
            this->visit_concurrent_control(*x.m_control[i]);
            r.append(s);
            if (i < x.n_control-1) r.append(", ");
        }
        if (x.m_mask) {
            r += ", ";
            this->visit_expr(*x.m_mask);
            r += s;
        }
        r.append(")");
        for (size_t i=0; i<x.n_locality; i++) {
            this->visit_concurrent_locality(*x.m_locality[i]);
            r.append(s);
        }
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r.append(s);
        }
        dec_indent();
        r += indent;
        r += syn(gr::Repeat);
        r.append("end forall");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ForAllSingle(const ForAllSingle_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Repeat);
        r += "forall";
        r += syn();
        r.append(" (");
        for (size_t i=0; i<x.n_control; i++) {
            this->visit_concurrent_control(*x.m_control[i]);
            r.append(s);
            if (i < x.n_control-1) r.append(", ");
        }
        if (x.m_mask) {
            r += ", ";
            this->visit_expr(*x.m_mask);
            r += s;
        }
        r.append(")");
        r.append("\n");
        inc_indent();
        this->visit_stmt(*x.m_assign);
        r.append(s);
        dec_indent();
        r += indent;
        r += syn(gr::Repeat);
        r.append("end forall");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_ConcurrentControl(const ConcurrentControl_t &x) {
        std::string r;
        if (x.m_var) {
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
        s = r;
    }

    void visit_ConcurrentLocal(const ConcurrentLocal_t &x) {
        std::string r;
        r += " local(";
        for (size_t i=0; i<x.n_vars; i++) {
            r.append(x.m_vars[i]);
            if (i < x.n_vars-1) r.append(", ");
        }
        r += ")";
        s = r;
    }

    void visit_ConcurrentLocalInit(const ConcurrentLocalInit_t &x) {
        std::string r;
        r += " localinit(";
        for (size_t i=0; i<x.n_vars; i++) {
            r.append(x.m_vars[i]);
            if (i < x.n_vars-1) r.append(", ");
        }
        r += ")";
        s = r;
    }

    void visit_ConcurrentShared(const ConcurrentShared_t &x) {
        std::string r;
        r += " shared(";
        for (size_t i=0; i<x.n_vars; i++) {
            r.append(x.m_vars[i]);
            if (i < x.n_vars-1) r.append(", ");
        }
        r += ")";
        s = r;
    }

    void visit_ConcurrentDefault(const ConcurrentDefault_t &/* x */ ) {
        std::string r;
        r += " default(none)";
        s = r;
    }

    void visit_ConcurrentReduce(const ConcurrentReduce_t &x) {
        std::string r;
        r += " reduce(" + visit_reduce_opType(x.m_op) + ": ";
        for (size_t i=0; i<x.n_vars; i++) {
            r.append(x.m_vars[i]);
            if (i < x.n_vars-1) r.append(", ");
        }
        r += ")";
        s = r;
    }

    inline std::string visit_reduce_opType(const reduce_opType &x) {
        switch (x) {
            case (reduce_opType::ReduceAdd) :
                return "+";
            case (reduce_opType::ReduceMul) :
                return "*";
            case (reduce_opType::ReduceMIN) :
                return "min";
            case (reduce_opType::ReduceMAX) :
                return "max";
            default:
                return "";
        }
    }

    void visit_Cycle(const Cycle_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("cycle");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Continue(const Continue_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("continue");
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Exit(const Exit_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("exit");
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Return(const Return_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r.append("return");
        r += syn();
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            r += " " + s;
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_WhileLoop(const WhileLoop_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Repeat);
        r += "do while";
        r += syn();
        r += " (";
        this->visit_expr(*x.m_test);
        r += s;
        r += ")";
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        r += indent;
        r += syn(gr::Repeat);
        r += "end do";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Print(const Print_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "print";
        r += syn();
        r += " ";
        if (x.m_fmt) {
            this->visit_expr(*x.m_fmt);
            r.append(s);
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
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Write(const Write_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "write";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i].m_value == nullptr) {
                r += "*";
            } else {
                this->visit_expr(*x.m_args[i].m_value);
                r += s;
            }
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            if (x.m_kwargs[i].m_value == nullptr) {
                r += "*";
            } else {
                this->visit_expr(*x.m_kwargs[i].m_value);
                r += s;
            }
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if (x.n_values > 0) {
            r += " ";
            for (size_t i=0; i<x.n_values; i++) {
                this->visit_expr(*x.m_values[i]);
                r += s;
                if (i < x.n_values-1) r += ", ";
            }
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Read(const Read_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "read";
        r += syn();
        if (x.m_format) {
            r += " ";
            this->visit_expr(*x.m_format);
            r.append(s);
        }
        if(x.n_args || x.n_kwargs) {
            r += "(";
            for (size_t i=0; i<x.n_args; i++) {
                if (x.m_args[i].m_value == nullptr) {
                    r += "*";
                } else {
                    this->visit_expr(*x.m_args[i].m_value);
                    r += s;
                }
                if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
            }
            for (size_t i=0; i<x.n_kwargs; i++) {
                r += x.m_kwargs[i].m_arg;
                r += "=";
                if (x.m_kwargs[i].m_value == nullptr) {
                    r += "*";
                } else {
                    this->visit_expr(*x.m_kwargs[i].m_value);
                    r += s;
                }
                if (i < x.n_kwargs-1) r += ", ";
            }
            r += ")";
        } else if(!x.m_format) {
            r += " *,";
        }
        if (x.n_values > 0) {
            if (x.m_format) {
                r += ",";
            }
            r += " ";
            for (size_t i=0; i<x.n_values; i++) {
                this->visit_expr(*x.m_values[i]);
                r += s;
                if (i < x.n_values-1) r += ", ";
            }
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Close(const Close_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "close";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Open(const Open_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "open";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Inquire(const Inquire_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "inquire";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if (x.n_values > 0) {
            r += " ";
            for (size_t i=0; i<x.n_values; i++) {
                this->visit_expr(*x.m_values[i]);
                r += s;
                if (i < x.n_values-1) r += ", ";
            }
        }
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Rewind(const Rewind_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "rewind";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Nullify(const Nullify_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "nullify";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Backspace(const Backspace_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "backspace";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Flush(const Flush_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "flush";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_Endfile(const Endfile_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "endfile";
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r += ", ";
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            r += x.m_kwargs[i].m_arg;
            r += "=";
            this->visit_expr(*x.m_kwargs[i].m_value);
            r += s;
            if (i < x.n_kwargs-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    template <typename Node>
    std::string print_label(const Node &x) {
        if (x.m_label == 0) {
            return "";
        } else {
            return std::to_string(x.m_label) + " ";
        }
    }

    template <typename Node>
    std::string print_stmt_name(const Node &x) {
        if (x.m_stmt_name == nullptr) {
            return "";
        } else {
            return std::string(x.m_stmt_name) + ": ";
        }
    }

    template <typename Node>
    std::string end_stmt_name(const Node &x) {
        if (x.m_stmt_name == nullptr) {
            return "";
        } else {
            return " " + std::string(x.m_stmt_name);
        }
    }

    void visit_Format(const Format_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "format";
        r += syn();
        r += "(" + std::string(x.m_fmt) + ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_FormTeam(const FormTeam_t &x) {
        std::string r=indent;
        r += print_label(x);
        r += syn(gr::Keyword);
        r += "form team";
        r += syn();
        r += "(";
        this->visit_expr(*x.m_team_number);
        r.append(s);
        r += ", ";
        r.append(x.m_team_var);
        for (size_t i=0; i<x.n_sync_stat; i++) {
            if(i == 0) r += ", ";
            this->visit_event_attribute(*x.m_sync_stat[i]);
            r.append(s);
            if (i < x.n_sync_stat-1) r.append(", ");
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_BoolOp(const BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (AST::boolopType::And) : {
                last_expr_precedence = 4;
                break;
            }
            case (AST::boolopType::Or) : {
                last_expr_precedence = 3;
                break;
            }
            case (AST::boolopType::Eqv) : {
                last_expr_precedence = 2;
                break;
            }
            case (AST::boolopType::NEqv) : {
                last_expr_precedence = 2;
                break;
            }
        }
        if (left_precedence >= last_expr_precedence) {
            s += left;
        } else {
            s += "(" + left + ")";
        }
        s +=  boolop2str(x.m_op);
        if (right_precedence >= last_expr_precedence) {
            s += right;
        } else {
            s += "(" + right + ")";
        }
    }

    void visit_BinOp(const BinOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (operatorType::Add) : {
                last_expr_precedence = 8;
                break;
            }
            case (operatorType::Sub) : {
                last_expr_precedence = 8;
                break;
            }
            case (operatorType::Mul) : {
                last_expr_precedence = 10;
                break;
            }
            case (operatorType::Div) : {
                last_expr_precedence = 10;
                break;
            }
            case (operatorType::Pow) : {
                last_expr_precedence = 11;
                break;
            }
        }
        s = "";
        if (left_precedence == 9) {
            s += "(" + left + ")";
        } else {
            if (left_precedence >= last_expr_precedence) {
                s += left;
            } else {
                s += "(" + left + ")";
            }
        }
        s +=  op2str(x.m_op);
        if (right_precedence == 9) {
            s += "(" + right + ")";
        } else if (x.m_op == operatorType::Sub) {
            if (right_precedence > last_expr_precedence) {
                s += right;
            } else {
                s += "(" + right + ")";
            }
        } else {
            if (right_precedence >= last_expr_precedence) {
                s += right;
            } else {
                s += "(" + right + ")";
            }
        }
    }

    void visit_DefBinOp(const DefBinOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        int right_precedence = last_expr_precedence;
        last_expr_precedence = 1;

        if (left_precedence >= last_expr_precedence) {
            s += left;
        } else {
            s += "(" + left + ")";
        }
        s += "." + std::string(x.m_op) + ".";
        if (right_precedence >= last_expr_precedence) {
            s += right;
        } else {
            s += "(" + right + ")";
        }
    }

    void visit_DefUnaryOp(const DefUnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        std::string right = std::move(s);
        int right_precedence = last_expr_precedence;
        last_expr_precedence = 12;

        s += "." + std::string(x.m_op) + ".";
        if (right_precedence >= last_expr_precedence) {
            s += right;
        } else {
            s += "(" + right + ")";
        }
    }

    void visit_StrOp(const StrOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        int right_precedence = last_expr_precedence;
        last_expr_precedence = 7;
        if (left_precedence >= last_expr_precedence) {
            s += left;
        } else {
            s += "(" + left + ")";
        }
        s += strop2str(x.m_op);
        if (right_precedence >= last_expr_precedence) {
            s += right;
        } else {
            s += "(" + right + ")";
        }
    }

    void visit_UnaryOp(const UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        int expr_precedence = last_expr_precedence;
        if (x.m_op == AST::unaryopType::USub) {
            last_expr_precedence = 9;
            if (expr_precedence >= last_expr_precedence) {
                s = "-" + s;
            } else {
                s = "-(" + s + ")";
            }
        } else if (x.m_op == AST::unaryopType::UAdd) {
            // Skip unary plus, keep the previous precedence
        } else if (x.m_op == AST::unaryopType::Not) {
            last_expr_precedence = 5;
            if (expr_precedence >= last_expr_precedence) {
                s = ".not. " + s;
            } else {
                s = ".not.(" + s + ")";
            }
        } else {
            throw LFortranException("Unary op type not implemented");
        }
    }

    void visit_Compare(const Compare_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        int right_precedence = last_expr_precedence;
        last_expr_precedence = 6;
        if (left_precedence >= last_expr_precedence) {
            s += left;
        } else {
            s += "(" + left + ")";
        }
        s +=  cmpop2str(x.m_op);
        if (right_precedence >= last_expr_precedence) {
            s += right;
        } else {
            s += "(" + right + ")";
        }
    }

    void visit_FuncCallOrArray(const FuncCallOrArray_t &x) {
        std::string r;
        if (x.n_member > 0) {
            for (size_t i=0; i<x.n_member; i++) {
                r.append(x.m_member[i].m_name);
                if (x.m_member[i].n_args > 0) {
                    r.append("(");
                    for (size_t j=0; j<x.m_member[i].n_args; j++) {
                        expr_t *start = x.m_member[i].m_args[j].m_start;
                        expr_t *end = x.m_member[i].m_args[j].m_end;
                        expr_t *step = x.m_member[i].m_args[j].m_step;
                        // TODO: Also show start, and step correctly
                        LFORTRAN_ASSERT(start == nullptr);
                        LFORTRAN_ASSERT(end != nullptr);
                        LFORTRAN_ASSERT(step == nullptr);
                        if (end) {
                            this->visit_expr(*end);
                            r.append(s);
                        }
                        if (start) {}
                        if (step) {}
                        if (i < x.m_member[i].n_args-1) r.append(",");
                    }
                    r.append(")");
                }
                r.append("%");
            }
        }
        r.append(x.m_func);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_fnarg(x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
            else if (x.n_keywords > 0) r.append(", ");
        }
        for (size_t i=0; i<x.n_keywords; i++) {
            this->visit_keyword(x.m_keywords[i]);
            r.append(s);
            if (i < x.n_keywords-1) r.append(", ");
        }
        r.append(")");
        for (size_t i=0; i<x.n_subargs; i++) {
            r.append("(");
            this->visit_fnarg(x.m_subargs[i]);
            r.append(s);
            if (i < x.n_subargs-1) r.append(", ");
            r.append(")");
        }
        s = r;
        last_expr_precedence = 13;
    }

    void visit_CoarrayRef(const CoarrayRef_t &x) {
        std::string r;
        if (x.n_member > 0) {
            for (size_t i=0; i<x.n_member; i++) {
                r.append(x.m_member[i].m_name);
                if (x.m_member[i].n_args > 0) {
                    r.append("(");
                    for (size_t j=0; j<x.m_member[i].n_args; j++) {
                        expr_t *start = x.m_member[i].m_args[j].m_start;
                        expr_t *end = x.m_member[i].m_args[j].m_end;
                        expr_t *step = x.m_member[i].m_args[j].m_step;
                        if (step != nullptr) {
                            if (start) {
                                this->visit_expr(*start);
                                r.append(s);
                            }
                            r += ":";
                            if (end) {
                                this->visit_expr(*end);
                                r.append(s);
                            }
                            if (is_a<Num_t>(*step) &&
                                    down_cast<Num_t>(step)->m_n != 1) {
                                r += ":";
                                this->visit_expr(*step);
                                r.append(s);
                            }
                        } else if (end != nullptr && start == nullptr) {
                            this->visit_expr(*end);
                            r.append(s);
                        } else {
                            throw LFortranException("Incorrect coarray elements");
                        }
                        if (i < x.m_member[i].n_args-1) r.append(",");
                    }
                    r.append(")");
                }
                r.append("%");
            }
        }
        r.append(x.m_name);
        if(x.n_args > 0) {
            r.append("(");
            for (size_t i=0; i<x.n_args; i++) {
                this->visit_fnarg(x.m_args[i]);
                r.append(s);
                if (i < x.n_args-1) r.append(", ");
            }
            if (x.n_fnkw > 0) r.append(", ");
            for (size_t i=0; i<x.n_fnkw; i++) {
                this->visit_keyword(x.m_fnkw[i]);
                r.append(s);
                if (i < x.n_fnkw-1) r.append(", ");
            }
            r.append(")");
        }
        r.append("[");
        for (size_t i=0; i<x.n_coargs; i++) {
            this->visit_coarrayarg(x.m_coargs[i]);
            r.append(s);
            if (i < x.n_coargs-1) r.append(", ");
        }
        if (x.n_cokw > 0) r.append(", ");
        for (size_t i=0; i<x.n_cokw; i++) {
            this->visit_keyword(x.m_cokw[i]);
            r.append(s);
            if (i < x.n_cokw-1) r.append(", ");
        }
        r.append("]");
        s = r;
        last_expr_precedence = 13;
    }

    void visit_ArrayInitializer(const ArrayInitializer_t &x) {
        std::string r = "[";
        if (x.m_vartype) {
            this->visit_decl_attribute(*x.m_vartype);
            r.append(s);
            r += " :: ";
        } else if (x.m_classtype) {
            r.append(x.m_classtype);
            r += " :: ";
        }
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            r.append(s);
            if (i < x.n_args-1) r.append(", ");
        }
        r.append("]");
        s = r;
        last_expr_precedence = 13;
    }

    void visit_Num(const Num_t &x) {
        s = syn(gr::Integer);
        s += BigInt::int_to_str(x.m_n);
        if (x.m_kind) {
            s += "_";
            s += x.m_kind;
        }
        s += syn();
        last_expr_precedence = 13;
    }

    void visit_Parenthesis(const Parenthesis_t &x) {
        std::string r = "(";
        this->visit_expr(*x.m_operand);
        r.append(s);
        r += ")";
        s = r;
        last_expr_precedence = 13;
    }

    void visit_Real(const Real_t &x) {
        s = syn(gr::Real);
        s += x.m_n;
        s += syn();
        last_expr_precedence = 13;
    }

    void visit_String(const String_t &x) {
        s = syn(gr::String);
        std::string r = x.m_s;
        int dq = 0, sq = 0;
        for (auto x: r) {
            if (x == '"') dq++;
            if (x == '\'') sq++;
        }
        if (dq > sq) {
            s += "'";
            s += replace(r, "'", "''");
            s += "'";
        } else {
            s += "\"";
            s += replace(r, "\"", "\"\"");
            s += "\"";
        }
        s += syn();
        last_expr_precedence = 13;
    }

    void visit_BOZ(const BOZ_t &x) {
        s = syn(gr::Integer);
        s += "\"" + std::string(x.m_s) + "\"";
        s += syn();
        last_expr_precedence = 13;
    }


    void visit_Complex(const Complex_t &x){
        std::string r;
        s = syn(gr::Complex);
        r += "(";
        this->visit_expr(*x.m_re);
        r.append(s);
        r += ", ";
        this->visit_expr(*x.m_im);
        r.append(s);
        r += ")";
        r += syn();
        s = r;
        last_expr_precedence = 13;
    }

    void visit_Name(const Name_t &x) {
        std::string r = "";
        if (x.n_member > 0) {
            for (size_t i=0; i<x.n_member; i++) {
                r.append(x.m_member[i].m_name);
                if (x.m_member[i].n_args > 0) {
                    r.append("(");
                    for (size_t j=0; j<x.m_member[i].n_args; j++) {
                        expr_t *start = x.m_member[i].m_args[j].m_start;
                        expr_t *end = x.m_member[i].m_args[j].m_end;
                        expr_t *step = x.m_member[i].m_args[j].m_step;
                        if (step != nullptr) {
                            if (start) {
                                this->visit_expr(*start);
                                r.append(s);
                            }
                            r += ":";
                            if (end) {
                                this->visit_expr(*end);
                                r.append(s);
                            }
                            if (is_a<Num_t>(*step) &&
                                    down_cast<Num_t>(step)->m_n != 1) {
                                r += ":";
                                this->visit_expr(*step);
                                r.append(s);
                            }
                        } else if (end != nullptr && start == nullptr) {
                            this->visit_expr(*end);
                            r.append(s);
                        } else {
                            throw LFortranException("Incorrect array elements");
                        }
                        if (i < x.m_member[i].n_args-1) r.append(",");
                    }
                    r.append(")");
                }
                r.append("%");
            }
        }
        r.append(std::string(x.m_id));
        s = r;
        last_expr_precedence = 13;
    }

    void visit_Logical(const Logical_t &x) {
        s = syn(gr::Logical);
        if (x.m_value) {
            s += ".true.";
        } else {
            s += ".false.";
        }
        s += syn();
        last_expr_precedence = 13;
    }

    std::string kind_value(const AST::kind_item_typeType &type,
            const AST::expr_t *value)
    {
        switch (type) {
            case (AST::kind_item_typeType::Value) :
                LFORTRAN_ASSERT(value != nullptr);
                this->visit_expr(*value);
                return s;
            case (AST::kind_item_typeType::Colon) :
                return ":";
            case (AST::kind_item_typeType::Star) :
                return "*";
            default :
                throw LFortranException("Unknown type");
        }
    }

    void visit_dimension(const dimension_t &x) {
        if (x.m_end_star == dimension_typeType::DimensionExpr) {
            std::string left;
            bool left_is_one=false;
            if (x.m_start) {
                this->visit_expr(*x.m_start);
                left = s;
                if (x.m_start->type == AST::exprType::Num) {
                    left_is_one = (AST::down_cast<AST::Num_t>(x.m_start)->m_n == 1);
                };
            }
            std::string right;
            if (x.m_end) {
                this->visit_expr(*x.m_end);
                right = s;
            }
            if (left_is_one && right != "") {
                s = right;
            } else {
                s = left + ":" + right;
            }
        } else if (x.m_end_star == dimension_typeType::DimensionStar) {
            if (x.m_start) {
                this->visit_expr(*x.m_start);
                s += ":*";
            } else {
                s = "*";
            }
        } else {
            LFORTRAN_ASSERT(x.m_end_star == dimension_typeType::AssumedRank);
            s = "..";
        }
    }

    void visit_codimension(const codimension_t &x) {
        if (x.m_end_star == codimension_typeType::CodimensionExpr) {
            std::string left;
            bool left_is_one=false;
            if (x.m_start) {
                this->visit_expr(*x.m_start);
                left = s;
                if (x.m_start->type == AST::exprType::Num) {
                    left_is_one = (AST::down_cast<AST::Num_t>(x.m_start)->m_n == 1);
                };
            }
            std::string right;
            if (x.m_end) {
                this->visit_expr(*x.m_end);
                right = s;
            }
            if (left_is_one && right != "") {
                s = right;
            } else {
                s = left + ":" + right;
            }
        } else {
            LFORTRAN_ASSERT(x.m_end_star == codimension_typeType::CodimensionStar);
            if (x.m_start) {
                this->visit_expr(*x.m_start);
                s += ":*";
            } else {
                s = "*";
            }
        }
    }

    void visit_arg(const arg_t &x) {
        s = std::string(x.m_arg);
    }

    void visit_keyword(const keyword_t &x) {
        std::string r;
        r += x.m_arg;
        r += "=";
        this->visit_expr(*x.m_value);
        r += s;
        s = r;
    }

    void visit_fnarg(const fnarg_t &x) {
        std::string r;
        if (x.m_step) {
            // Array section
            if (x.m_start) {
                this->visit_expr(*x.m_start);
                r += s;
            }
            r += ":";
            if (x.m_end) {
                this->visit_expr(*x.m_end);
                r += s;
            }
            if (is_a<Num_t>(*x.m_step) && down_cast<Num_t>(x.m_step)->m_n == 1) {
                // Nothing, a:b:1 is printed as a:b
            } else {
                r += ":";
                this->visit_expr(*x.m_step);
                r += s;
            }
        } else {
            // Array element
            LFORTRAN_ASSERT(x.m_end);
            LFORTRAN_ASSERT(!x.m_start);
            this->visit_expr(*x.m_end);
            r = s;
        }
        s = r;
    }

    void visit_coarrayarg(const coarrayarg_t &x) {
        std::string r;
        if (x.m_step) {
            // Array section
            if (x.m_start) {
                this->visit_expr(*x.m_start);
                r += s;
            }
            if(x.m_star == codimension_typeType::CodimensionStar) {
                r += "*";
            } else {
                r += ":";
            }
            if (x.m_end) {
                this->visit_expr(*x.m_end);
                r += s;
            }
            if (is_a<Num_t>(*x.m_step) && down_cast<Num_t>(x.m_step)->m_n == 1) {
                // Nothing, a:b:1 is printed as a:b
            } else {
                r += ":";
                this->visit_expr(*x.m_step);
                r += s;
            }
        } else {
            // Array element
            LFORTRAN_ASSERT(x.m_end);
            LFORTRAN_ASSERT(!x.m_start);
            this->visit_expr(*x.m_end);
            r = s;
        }
        s = r;
    }

    void visit_Bind(const Bind_t &x) {
        std::string r;
        r += syn(gr::UnitHeader);
        r.append("bind");
        r += syn();
        r += "(";
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1 || x.n_kwargs > 0) r.append(", ");
        }
        for (size_t i=0; i<x.n_kwargs; i++) {
            visit_keyword(x.m_kwargs[i]);
            r += s;
            if (i < x.n_kwargs-1) r.append(", ");
        }
        r += ")";
        s = r;
    }

    void visit_ArrayIndex(const ArrayIndex_t &x) {
        std::string r;
        if (x.m_left) {
            this->visit_expr(*x.m_left);
            r += s;
        }
        r.append(":");
        if (x.m_right) {
            this->visit_expr(*x.m_right);
            r += s;
        }
        if (x.m_step) {
            r.append(":");
            this->visit_expr(*x.m_step);
            r += s;
        };
        s = r;
    }

    void visit_UseSymbol(const UseSymbol_t &x) {
        s = "";
        if (x.m_rename) {
            s.append(x.m_rename);
            s.append(" => ");
        }
        s.append(x.m_sym);
    }

    void visit_UseAssignment(const UseAssignment_t & /*x*/) {
        s = "assignment (=)";
    }

    void visit_IntrinsicOperator(const IntrinsicOperator_t &x) {
        s = "operator (" + intrinsicop2str(x.m_op) + ")";
    }

    void visit_RenameOperator(const RenameOperator_t &x) {
        s = "operator(." + std::string(x.m_local_defop) + ".)";
        s += " => ";
        s += "operator(." + std::string(x.m_use_defop) + ".)";
    }

    void visit_DefinedOperator(const DefinedOperator_t &x) {
        s = "operator (." + std::string(x.m_opName) + ".)";
    }

    void visit_UseWrite(const UseWrite_t &x) {
        s = "write(";
        s.append(x.m_id);
        s += ")";
    }
    void visit_UseRead(const UseRead_t &x) {
        s = "read(";
        s.append(x.m_id);
        s += ")";
    }


    void visit_Select(const Select_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Conditional);
        r += "select case";
        r += syn();
        r += " (";
        this->visit_expr(*x.m_test);
        r += s;
        r += ")";
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_case_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        r += indent;
        r += syn(gr::Conditional);
        r += "end select";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_CaseStmt(const CaseStmt_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "case";
        r += syn();
        r += " (";
        for (size_t i=0; i<x.n_test; i++) {
            this->visit_case_cond(*x.m_test[i]);
            r += s;
            if (i < x.n_test-1) r += ", ";
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }

    void visit_CaseCondExpr(const CaseCondExpr_t &x) {
        this->visit_expr(*x.m_cond);
    }

    void visit_CaseCondRange(const CaseCondRange_t &x) {
        std::string r;
        if (x.m_start) {
            this->visit_expr(*x.m_start);
            r += s;
        }
        r.append(":");
        if (x.m_end) {
            this->visit_expr(*x.m_end);
            r += s;
        }
        s = r;
    }

    void visit_CaseStmt_Default(const CaseStmt_Default_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "case default";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }

    void visit_SelectRank(const SelectRank_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Conditional);
        r += "select rank";
        r += syn();
        r += " (";
        if (x.m_assoc_name) {
            r.append(x.m_assoc_name);
            r += "=>";
        }
        this->visit_expr(*x.m_selector);
        r += s;
        r += ")";
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_rank_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        r += syn(gr::Conditional);
        r += "end select";
        r += syn();
        r += end_stmt_name(x);
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_RankExpr(const RankExpr_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "rank";
        r += syn();
        r += " (";
        this->visit_expr(*x.m_value);
        r.append(s);
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }

    void visit_RankStar(const RankStar_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "rank";
        r += syn();
        r += " (*)";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }

    void visit_RankDefault(const RankDefault_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "rank default";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }

    void visit_SelectType(const SelectType_t &x) {
        std::string r = indent;
        r += print_label(x);
        r += print_stmt_name(x);
        r += syn(gr::Conditional);
        r += "select type";
        r += syn();
        r += " (";
        if (x.m_assoc_name) {
            r.append(x.m_assoc_name);
            r += "=>";
        }
        this->visit_expr(*x.m_selector);
        r += s;
        r += ")";
        if(x.m_t_inside){
            r += print_trivia_after(*x.m_t_inside);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_type_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        r += syn(gr::Conditional);
        r += "end select";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        s = r;
    }

    void visit_TypeStmtName(const TypeStmtName_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "type is";
        r += syn();
        r += " (";
        if (x.m_name) {
            r.append(x.m_name);
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }
    void visit_TypeStmtType(const TypeStmtType_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "type is";
        r += syn();
        r += " (";
        if (x.m_vartype) {
            this->visit_decl_attribute(*x.m_vartype);
            r += s;
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }
    void visit_ClassStmt(const ClassStmt_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "class is";
        r += syn();
        r += " (";
        if (x.m_id) {
            r.append(x.m_id);
        }
        r += ")";
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }
    void visit_ClassDefault(const ClassDefault_t &x) {
        std::string r = indent;
        r += syn(gr::Conditional);
        r += "class default";
        r += syn();
        if(x.m_trivia){
            r += print_trivia_after(*x.m_trivia);
        } else {
            r.append("\n");
        }
        inc_indent();
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }
};

}

std::string ast_to_src(AST::TranslationUnit_t &ast, bool color, int indent,
        bool indent_unit) {
    AST::ASTToSRCVisitor v(color, indent, indent_unit);
    v.visit_ast((AST::ast_t &)ast);
    return v.s;
}

}
