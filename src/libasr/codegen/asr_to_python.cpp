#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/codegen/asr_to_c_cpp.h>
#include <libasr/codegen/asr_to_python.h>

using LCompilers::ASR::is_a;
using LCompilers::ASR::down_cast;

namespace LCompilers {

enum Precedence {
    Exp = 15,
    Pow = 15,
    UnaryMinus = 14,
    Mul = 13,
    Div = 13,
    Add = 12,
    Sub = 12,
    CmpOp = 7,
    And = 5,
    Or = 4,
};

class ASRToLpythonVisitor : public ASR::BaseVisitor<ASRToLpythonVisitor>
{
public:
    Allocator& al;
    diag::Diagnostics& diag;
    std::string s;
    bool use_colors;
    int indent_level;
    std::string indent;
    int indent_spaces;
    // Following same order as Python 3.x
    // https://docs.python.org/3/reference/expressions.html#expression-lists
    int last_expr_precedence;

public:
    ASRToLpythonVisitor(Allocator& al, diag::Diagnostics& diag, CompilerOptions& /*co*/, bool _use_colors, int _indent)
        : al{ al }, diag{ diag }, use_colors{_use_colors}, indent_level{0},
            indent_spaces{_indent}
        { }

    void inc_indent() {
        indent_level++;
        indent = std::string(indent_level*indent_spaces, ' ');
    }

    void dec_indent() {
        indent_level--;
        indent = std::string(indent_level*indent_spaces, ' ');
    }

    void visit_expr_with_precedence(const ASR::expr_t &x, int current_precedence) {
        visit_expr(x);
        if (last_expr_precedence == 18 ||
                last_expr_precedence < current_precedence) {
            s = "(" + s + ")";
        }
    }

    std::string binop2str(const ASR::binopType type)
    {
        switch (type) {
            case (ASR::binopType::Add) : {
                last_expr_precedence = Precedence::Add;
                return " + ";
            } case (ASR::binopType::Sub) : {
                last_expr_precedence = Precedence::Sub;
                return " - ";
	    } case (ASR::binopType::Mul) : {
                last_expr_precedence = Precedence::Mul;
                return " * ";
	    } case (ASR::binopType::Div) : {
                last_expr_precedence = Precedence::Div;
                return " / ";
	    } case (ASR::binopType::Pow) : {
                last_expr_precedence = Precedence::Pow;
                return " ** ";
	    } default : { 
                throw LCompilersException("Cannot represent the binary operator as a string");
            }
        }
    }

    std::string cmpop2str(const ASR::cmpopType type)
    {
        last_expr_precedence = Precedence::CmpOp;
        switch (type) {
            case (ASR::cmpopType::Eq) : return " == ";
            case (ASR::cmpopType::NotEq) : return " != ";
            case (ASR::cmpopType::Lt) : return " < ";
            case (ASR::cmpopType::LtE) : return " <= ";
            case (ASR::cmpopType::Gt) : return " > ";
            case (ASR::cmpopType::GtE) : return " >= ";
            default : throw LCompilersException("Cannot represent the boolean operator as a string");
        }
    }

    std::string logicalbinop2str(const ASR::logicalbinopType type)
    {
        switch (type) {
            case (ASR::logicalbinopType::And) : {
                last_expr_precedence = Precedence::And;
                return " and ";
            } case (ASR::logicalbinopType::Or) : {
                last_expr_precedence = Precedence::Or;
                return " or ";
	    } default : {
                throw LCompilersException("Cannot represent the boolean operator as a string");
            }
        }
    }

    template <typename T>
    void visit_body(const T &x, std::string &r, bool apply_indent=true) {
        if (apply_indent) {
            inc_indent();
        }
        for (size_t i = 0; i < x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            r += s;
        }
        if (apply_indent) {
            dec_indent();
        }
    }

    std::string get_type(const ASR::ttype_t *t) {
        std::string r = "";
        switch (t->type) {
            case ASR::ttypeType::Integer : {
                std::string out;
                out = std::to_string(down_cast<ASR::Integer_t>(t)->m_kind);
                if (out == "1") {
                    r += "i8";
                } else if (out == "2") {
                    r += "i16";
                } else if (out == "4") {
                    r += "i32";
                } else if (out == "8") {
                    r += "i64";
                }
                break;
            } case ASR::ttypeType::Real : {
                std::string out;
                out = std::to_string(down_cast<ASR::Real_t>(t)->m_kind);
                if (out == "4") {
                    r += "f32";
                } else if (out == "8") {
                    r += "f64";
                }
                break;
            } case ASR::ttypeType::Complex : {
                std::string out;
                out = std::to_string(down_cast<ASR::Complex_t>(t)->m_kind);
                if (out == "4") {
                    r += "c32";
                } else if (out == "8") {
                    r += "c64";
                }
                break;
            } case ASR::ttypeType::Character : {
                r = "utf";
                r += std::to_string(down_cast<ASR::Character_t>(t)->m_kind);
                break;
            } case ASR::ttypeType::Logical : {
                r = "bool";
                break;
            } case ASR::ttypeType::Array: {
                r = get_type(down_cast<ASR::Array_t>(t)->m_type);
                break;
            } case ASR::ttypeType::Allocatable: {
                r = get_type(down_cast<ASR::Allocatable_t>(t)->m_type);
                break;
	    } default : {
                throw LCompilersException("The type `"
                    + ASRUtils::type_to_str_python(t) + "` is not handled yet");
            }
        }
        return r;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        std::string r = "";

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Module_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
                r += "\n";
            }
        }

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
                r += "\n";
            }
        }

        // Main program
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }
        s = r;
    }

    void visit_Module(const ASR::Module_t &x) {
        std::string r;

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }
        s = r;
    }

    void visit_Function(const ASR::Function_t &x) {
        // Generate code for the lpython function
        std::string r;
        r = "def";
        r += " ";
        r.append(x.m_name);
        r += "(";
        for (size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args - 1) {
                r += ", ";
            }
        }
        r += "):";
        r += "\n";

        inc_indent();
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }
        dec_indent();

        visit_body(x, r, true);

        s = r;
    }

    void visit_Program(const ASR::Program_t &x) {
        std::string r;

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }

        r += "\n";
        s = r;
    }

    void visit_Variable(const ASR::Variable_t &x) {
        std::string r = indent;
        r += x.m_name;
        r += ": ";
        r += get_type(x.m_type);
        r += "\n";
        s = r;
    }

    void visit_Print(const ASR::Print_t &x) {
        std::string r = indent;
        r += "print(";
        for (size_t i = 0; i < x.n_values; i++) {
            visit_expr(*x.m_values[i]);
            r += s;
            if (i < x.n_values-1)
                r += ", ";
        }
        r += ")";
        r += "\n";
        s = r;
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        std::string r = indent;
        visit_expr(*x.m_target);
        r += s;
        r += " = ";
        visit_expr(*x.m_value);
        r += s;
        r += "\n";
        s = r;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t /*&x*/) {
        std::string r = indent;
    }

    void visit_Cast(const ASR::Cast_t &x) {
        // TODO
        visit_expr(*x.m_arg);
    }

    void visit_Var(const ASR::Var_t &x) {
        s = ASRUtils::symbol_name(x.m_v);
    }

    void visit_If(const ASR::If_t &x) {
        std::string r = indent;
        r += "if";
        visit_expr(*x.m_test);
        r += s;
        r += ":";
        r += "\n";
        inc_indent();
        for (size_t i = 0; i < x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        if (x.n_orelse == 0) {
            r += "\n";
        } else {
            for (size_t i = 0; i < x.n_orelse; i++) {
                r += "else:";
                r += "\n";
                inc_indent();
                visit_stmt(*x.m_orelse[i]);
                r += s;
                dec_indent();
            }
            r += "\n";
        }
        s = r;
    }

    void visit_IntrinsicScalarFunction(const ASR::IntrinsicScalarFunction_t &x) {
        std::string out;
        switch (x.m_intrinsic_id) {
            SET_INTRINSIC_NAME(Abs, "abs");
            default : {
                throw LCompilersException("IntrinsicScalarFunction: `"
                    + ASRUtils::get_intrinsic_name(x.m_intrinsic_id)
                    + "` is not implemented");
            }
        }
        LCOMPILERS_ASSERT(x.n_args == 1);
        visit_expr(*x.m_args[0]);
        out += "(" + s + ")";
        s = out;
    }

    void visit_StringCompare(const ASR::StringCompare_t &x) {
        std::string r;
        r = " ";
        visit_expr(*x.m_left);
        r += s;
        r += cmpop2str(x.m_op);
        visit_expr(*x.m_right);
        r += s;
        s = r;
    }

    void visit_StringConstant(const ASR::StringConstant_t &x) {
        s = "\"";
        s.append(x.m_s);
        s += "\"";
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        std::string r;
        // TODO: Handle precedence based on the last_operator_precedence
        r = "(";
        visit_expr(*x.m_left);
        r += s;
        r += binop2str(x.m_op);
        visit_expr(*x.m_right);
        r += s;
        r += ")";
        s = r;
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        std::string r;
        // TODO: Handle precedence based on the last_operator_precedence
        r = "(";
        visit_expr(*x.m_left);
        r += s;
        r += cmpop2str(x.m_op);
        visit_expr(*x.m_right);
        r += s;
        r += ")";
        s = r;
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        s = std::to_string(x.m_n);
    }

    void visit_RealConstant(const ASR::RealConstant_t &x) {
        s = std::to_string(x.m_r);
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
       std::string r;
       r = "(";
       visit_expr(*x.m_left);
       r += s;
       r += cmpop2str(x.m_op);
       visit_expr(*x.m_right);
       r += s;
       r += ")";
       s = r;
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        std::string r;
        if (x.m_value) {
            r += "True";
        } else {
            r += "False";
        }
        s = r;
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t &x) {
        std::string r;
        std::string m_op = logicalbinop2str(x.m_op);
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += m_op;
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
       std::string r;
       r = "(";
       visit_expr(*x.m_left);
       r += s;
       r += cmpop2str(x.m_op);
       visit_expr(*x.m_right);
       r += s;
       r += ")";
       s = r;
    }
};

Result<std::string> asr_to_python(Allocator& al, ASR::TranslationUnit_t &asr,
        diag::Diagnostics& diagnostics, CompilerOptions& co,
        bool color, int indent) {
    ASRToLpythonVisitor v(al, diagnostics, co, color, indent=4);
    try {
        v.visit_TranslationUnit(asr);
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    }
    return v.s;
}

}  // namespace LCompilers
