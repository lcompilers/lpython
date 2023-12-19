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
    CmpOp = 7,
    Add = 8,
    Sub = 12,
    UnaryMinus = 9,
    Mul = 13,
    Div = 13,
    Pow = 15,
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

    void visit_expr_with_last_precedence(const ASR::expr_t &x, int current_precedence) {
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
                r = "int(";
                r += std::to_string(down_cast<ASR::Integer_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Complex : {
                r = "complex(";
                r += std::to_string(down_cast<ASR::Complex_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Character : {
                r = "chr(";
                r += std::to_string(down_cast<ASR::Character_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Logical : {
                r = "bool(";
                r += std::to_string(down_cast<ASR::Logical_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Array : {
                r = get_type(down_cast<ASR::Array_t>(t)->m_type);
                break;
            } case ASR::ttypeType::Allocatable : {
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
            }
        }

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }
        s = r;
    }

    void visit_Module(const ASR::Module_t &x) {
        // Generate code for the lpython function 
        std::string r;
        r = "def";
        r += " ";
        r.append(x.m_name);
        r += "()";
        r += ":";
        r += "\n";
        inc_indent();
        r += "\n";
        dec_indent();
        r += "\n";
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

        visit_body(x, r, true);
        dec_indent();
        r += "\n";
        s = r;
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for main function
        std::string r;
        r = "def";
        r += " ";
        r += "main0():";
        r += "\n";
        inc_indent();
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }
        r.append(x.m_name);
        r += "()";
        r += "\n";

        visit_body(x, r, true);

        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }

        dec_indent();
        r += "\n";
        s = r;
    }

};

Result<std::string> asr_to_lpython(Allocator& al, ASR::TranslationUnit_t &asr,
        diag::Diagnostics& diagnostics, CompilerOptions& co,
        bool color, int indent) {
    ASRToLpythonVisitor v(al, diagnostics, co, color, indent);
    try {
        v.visit_TranslationUnit(asr);
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    }
    return v.s;
}

}  // namespace LCompilers
