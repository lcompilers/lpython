#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/codegen/asr_to_c_cpp.h>
#include <libasr/codegen/asr_to_python.h>

using LCompilers::ASR::is_a;
using LCompilers::ASR::down_cast;

namespace LCompilers {

enum Precedence {
    Or = 4,
    And = 5,
    Not = 6,
    CmpOp = 7,
    Add = 12,
    Sub = 12,
    Mul = 13,
    Div = 13,
    BitNot = 14,
    UnaryMinus = 14,
    Exp = 15,
    Pow = 15,
    Constant = 18,
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
                r += "i";
                r += std::to_string(ASRUtils::extract_kind_from_ttype_t(t)*8);
                break;
            } case ASR::ttypeType::Real : {
                r += "f";
                r += std::to_string(ASRUtils::extract_kind_from_ttype_t(t)*8);
                break;
            } case ASR::ttypeType::Complex : {
                r += "c";
                r += std::to_string(ASRUtils::extract_kind_from_ttype_t(t)*8);
                break;
            } case ASR::ttypeType::String : {
                r = "str";
                break;
            } case ASR::ttypeType::Logical : {
                r = "bool";
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
            }
        }

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
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
            // TODO: Specify the datatype of the argument here
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
         if (ASR::is_a<ASR::StringFormat_t>(*x.m_text)) {
            ASR::StringFormat_t str_fmt = *ASR::down_cast<ASR::StringFormat_t>(x.m_text);
            for (size_t i = 0; i < str_fmt.n_args; i++) {
                visit_expr(*str_fmt.m_args[i]);
                r += s;
                if (i < str_fmt.n_args-1)
                    r += ", ";
            }
        } else if (ASR::is_a<ASR::String_t>(*ASRUtils::expr_type(x.m_text))){
            visit_expr(*x.m_text);
            r += s;
        } else {
            throw CodeGenError("print statment supported for stringformat and single character argument",
                x.base.base.loc);
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

    void visit_Return(const ASR::Return_t /*&x*/) {
        // TODO: Handle cases for returning an expression/value
        s = indent + "return" + "\n";
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        std::string r = indent;
        r += ASRUtils::symbol_name(x.m_name);
        r += "(";
        for (size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i].m_value);
            r += s;
            if (i < x.n_args - 1)
                r += ", ";
        }
        r += ")\n";
        s = r;
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        std::string r = "";
        if (x.m_original_name) {
            r += ASRUtils::symbol_name(x.m_original_name);
        } else {
            r += ASRUtils::symbol_name(x.m_name);
        }

        r += "(";
        for (size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i].m_value);
            r += s;
            if (i < x.n_args - 1)
                r += ", ";
        }
        r += ")";
        s = r;
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
        r += "if ";
        visit_expr(*x.m_test);
        r += s;
        r += ":\n";
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
                r += indent + "else:\n";
                inc_indent();
                visit_stmt(*x.m_orelse[i]);
                r += s;
                dec_indent();
            }
        }
        s = r;
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        std::string r = indent;
        r += "while ";
        visit_expr(*x.m_test);
        r += s;
        r += ":\n";
        visit_body(x, r);
        s = r;
    }

    void visit_NamedExpr(const ASR::NamedExpr_t &x) {
        this->visit_expr(*x.m_target);
        std::string t = std::move(s);
        this->visit_expr(*x.m_value);
        std::string v = std::move(s);
        s = "(" + t + " := " + v + ")";
    }

    void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t &x) {
        std::string r = indent;
        r += "del ";
        for (size_t i = 0; i < x.n_vars; i++) {
            if (i > 0) {
                r += ", ";
            }
            visit_expr(*x.m_vars[i]);
            r += s;
        }
        s = r;
    }

    void visit_IntrinsicElementalFunction(const ASR::IntrinsicElementalFunction_t &x) {
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
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += cmpop2str(x.m_op);
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    void visit_StringConstant(const ASR::StringConstant_t &x) {
        s = "\"";
        s.append(x.m_s);
        s += "\"";
        last_expr_precedence = Precedence::Constant;
    }

    void visit_StringChr(const ASR::StringChr_t &x) {
        visit_expr(*x.m_arg);
        s = "chr(" + s + ")";
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        std::string r;
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += binop2str(x.m_op);
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        std::string r;
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += cmpop2str(x.m_op);
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        s = std::to_string(x.m_n);
        last_expr_precedence = Precedence::Constant;
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        visit_expr_with_precedence(*x.m_arg, 14);
        s =  "-" + s;
        last_expr_precedence = Precedence::UnaryMinus;
    }

    void visit_IntegerBitNot(const ASR::IntegerBitNot_t &x) {
        visit_expr_with_precedence(*x.m_arg, 14);
        s = "~" + s;
        last_expr_precedence = Precedence::BitNot;
    }

    void visit_RealConstant(const ASR::RealConstant_t &x) {
        s = std::to_string(x.m_r);
        last_expr_precedence = Precedence::Constant;
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
       std::string r;
       int current_precedence = last_expr_precedence;
       visit_expr_with_precedence(*x.m_left, current_precedence);
       r += s;
       r += cmpop2str(x.m_op);
       visit_expr_with_precedence(*x.m_right, current_precedence);
       r += s;
       last_expr_precedence = current_precedence;
       s = r;
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        visit_expr_with_precedence(*x.m_arg, 14);
        s =  "-" + s;
        last_expr_precedence = Precedence::UnaryMinus;
    }

    void visit_RealBinOp(const ASR::RealBinOp_t &x) {
        std::string r;
        std::string m_op = binop2str(x.m_op);
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += m_op;
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
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
        last_expr_precedence = Precedence::Constant;
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
       int current_precedence = last_expr_precedence;
       visit_expr_with_precedence(*x.m_left, current_precedence);
       r += s;
       r += cmpop2str(x.m_op);
       visit_expr_with_precedence(*x.m_right, current_precedence);
       r += s;
       last_expr_precedence = current_precedence;
       s = r;
    }

    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        visit_expr_with_precedence(*x.m_arg, 6);
        s = "not " + s;
        last_expr_precedence = Precedence::Not;
    }

    void visit_StringConcat(const ASR::StringConcat_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        s = left + " + " + right;
    }

    void visit_StringRepeat(const ASR::StringRepeat_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        s = left + " * " + right;
    }

    void visit_StringOrd(const ASR::StringOrd_t &x) {
        std::string r;
        r = "ord(";
        visit_expr(*x.m_arg);
        r += s;
        r += ")";
        s = r;
    }

    void visit_StringLen(const ASR::StringLen_t &x) {
        visit_expr(*x.m_arg);
        s += "len(" + s + ")";
    }

    void visit_IfExp(const ASR::IfExp_t &x) {
        std::string r;
        visit_expr(*x.m_body);
        r += s;
        r += " if ";
        visit_expr(*x.m_test);
        r += s;
        r += " else ";
        visit_expr(*x.m_orelse);
        r += s;
        s = r;
    }

    void visit_ComplexConstant(const ASR::ComplexConstant_t &x) {
        std::string re = std::to_string(x.m_re);
        std::string im = std::to_string(x.m_im);
        s = "complex(" + re + ", " + im + ")";
    }

    void visit_ComplexUnaryMinus(const ASR::ComplexUnaryMinus_t &x) {
        visit_expr_with_precedence(*x.m_arg, 14);
        s = "-" + s;
        last_expr_precedence = Precedence::UnaryMinus;
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t &x) {
        std::string r;
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += cmpop2str(x.m_op);
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    void visit_Assert(const ASR::Assert_t &x) {
        std::string r = indent;
        r += "assert ";
        visit_expr(*x.m_test);
        r += s;
        if (x.m_msg) {
            r += ", ";
            visit_expr(*x.m_msg);
            r += s;
        }
        r += "\n";
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
