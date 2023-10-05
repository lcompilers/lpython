#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/codegen/asr_to_c_cpp.h>
#include <libasr/codegen/asr_to_lpython.h>

using LCompilers::ASR::is_a;
using LCompilers::ASR::down_cast;

namespace LCompilers {

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
    ASRToLpythonVisitor(Allocator& al, diag::Diagnostics& diag)
        : al{ al }
        , diag{ diag }
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
                last_expr_precedence = 12;
                return " + ";
            } case (ASR::binopType::Sub) : {
                last_expr_precedence = 12;
                return " - ";
	    } case (ASR::binopType::Mul) : {
                last_expr_precedence = 13;
                return " * ";
	    } case (ASR::binopType::Div) : {
                last_expr_precedence = 13;
                return " / ";
            // TODO: add an overload in is_op_overloaded
	    //} case (ASR::binopType::FloorDiv) : {
            //    last_expr_precedence = 13;
            //    return " // ";
	    //} case (ASR::binopType::Mod) : {
            //    last_expr_precedence = 13;
            //    return " % ";
	    } case (ASR::binopType::Pow) : {
                last_expr_precedence = 15;
                return " ** ";
	    } default : { 
                throw LCompilersException("Cannot represent the binary operator as a string");
            }
        }
    }

    std::string cmpop2str(const ASR::cmpopType type)
    {
        last_expr_precedence = 7;
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
                last_expr_precedence = 5;
                return " and ";
            } case (ASR::logicalbinopType::Or) : {
                last_expr_precedence = 4;
                return " or ";
	    //} case (ASR::logicalbinopType::Not) : {
            //    last_expr_precedence = 6;
            //    return " not ";
	    } default : {
                throw LCompilersException("Cannot represent the boolean operator as a string");
            }
        }
    }

    // TODO: Bitwise operator
    // TODO: Assignment operator
    // TODO: Membership operator
    // TODO: Identity operator

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
                r += std::to_string(down_cast<ASR::Integer_t>(t)->m_kind);  // TODO: confirm what's m_kind
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
            //} case ASR::ttypeType::Tuple : {  // TODO: make it work
            //    r = "tuple(";
            //    r += std::to_string(down_cast<ASR::Tuple_t>(t)->m_kind);
            //    r = ")";
            //    break;
            //} case ASR::ttypeType::List : {
            //    r = "list(";
            //    r += "[";
            //    r += std::to_string(down_cast<ASR::List_t>(t)->m_kind);
            //    r += "]";
            //    r = ")";
            //    break;
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
        r += ")";
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
        r = "if";
        r += " ";
        r += "__name__";
        r += " == ";
        r += "__main__";
        r += ":";
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

std::string asr_to_lpython(Allocator& al, ASR::TranslationUnit_t &asr,
        diag::Diagnostics& diag, CompilerOptions &co) {
    ASRToLpythonVisitor v(al, diag);
    v.visit_TranslationUnit(asr);
    return v.s;
}

}  // namespace LCompilers
