#include <cctype>

#include <lfortran/ast_to_openmp.h>

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

    inline std::string convert_to_lowercase(const std::string &s) {
       std::string res;
       for(auto x: s) res.push_back(std::tolower(x));
       return res;
    }

}

namespace AST {

class ASTToOPENMPVisitor : public BaseVisitor<ASTToOPENMPVisitor>
{
public:
    std::string s;
    bool use_colors;
    int indent_level;
public:
    ASTToOPENMPVisitor() : use_colors{false}, indent_level{0} { }
    void visit_TranslationUnit(const TranslationUnit_t &/* x */) {
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
        r.append("\nend module ");
        r.append(x.m_name);
        r.append("\n");
        s = r;
    }
    void visit_Program(const Program_t &x) {
        std::string r = "program ";
        r.append(x.m_name);
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
            for (size_t i=0; i<x.n_contains; i++) {
                this->visit_program_unit(*x.m_contains[i]);
                r.append(s);
                r.append("\n");
            }
        }
        r.append("end program ");
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
        if (x.m_vartype == nullptr &&
                x.n_attributes == 1 &&
                is_a<SimpleAttribute_t>(*x.m_attributes[0]) &&
                down_cast<SimpleAttribute_t>(x.m_attributes[0])->m_attr ==
                    simple_attributeType::AttrParameter) {
            // The parameter statement is printed differently than other
            // attributes
            r += "parameter";
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
            r.append("namelist");
            r.append(" /");
            r += down_cast<AttrNamelist_t>(x.m_attributes[0])->m_name;
            r.append("/ ");
            for (size_t i=0; i<x.n_syms; i++) {
                visit_var_sym(x.m_syms[i]);
                r += s;
                if (i < x.n_syms-1) r.append(", ");
            }
        } else {
            if (x.m_vartype) {
                visit_decl_attribute(*x.m_vartype);
                r += s;
                if (x.n_attributes > 0) r.append(", ");
            }
            for (size_t i=0; i<x.n_attributes; i++) {
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
        s = r;
    }

    void visit_var_sym(const var_sym_t &x) {
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
            r += "=" + s;
        }
        s = r;
    }

#define ATTRTYPE(x) \
            case (simple_attributeType::Attr##x) : \
                r.append(convert_to_lowercase(#x)); \
                break;

    void visit_SimpleAttribute(const SimpleAttribute_t &x) {
        std::string r;
        switch (x.m_attr) {
            ATTRTYPE(Abstract)
            ATTRTYPE(Allocatable)
            ATTRTYPE(Contiguous)
            ATTRTYPE(Elemental)
            ATTRTYPE(Enumerator)
            ATTRTYPE(Impure)
            ATTRTYPE(Module)
            ATTRTYPE(NoPass)
            ATTRTYPE(Optional)
            ATTRTYPE(Parameter)
            ATTRTYPE(Pointer)
            ATTRTYPE(Private)
            ATTRTYPE(Protected)
            ATTRTYPE(Public)
            ATTRTYPE(Pure)
            ATTRTYPE(Recursive)
            ATTRTYPE(Save)
            ATTRTYPE(Target)
            ATTRTYPE(Value)
            default :
                throw LFortranException("Attribute type not implemented");
        }
        s = r;
    }

#define ATTRTYPE2(x, y) \
            case (decl_typeType::Type##x) : \
                r.append(y); \
                break;

    void visit_AttrType(const AttrType_t &x) {
        std::string r;
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
        r += "intent";
        r += "(";
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
        r += ")";
        s = r;
    }

    void visit_AttrDimension(const AttrDimension_t &x) {
        std::string r;
        r += "dimension";
        r += "(";
        for (size_t i=0; i<x.n_dim; i++) {
            visit_dimension(x.m_dim[i]);
            r += s;
            if (i < x.n_dim-1) r.append(", ");
        }
        r += ")";
        s = r;
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
        s.append(AST::down_cast<AST::InterfaceHeaderName_t>(x.m_header)->m_name);
        s.append(" ");
        s.append("Unimplementedidentifier");
        s.append(")");
    }
    void visit_Assignment(const Assignment_t &x) {
        std::string r = "";
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
            this->visit_expr(*x.m_args[i].m_end);
            r.append(s);
            if (i < x.n_args-1) r.append(" ");
        }
        r.append(")");
        s = r;
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
    void visit_ErrorStop(const ErrorStop_t &/* x */) {
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
    //Converts do concurrent to a regular do loop. Adds OpenMP pragmas.
    void visit_DoConcurrentLoop(const DoConcurrentLoop_t &x) {
        if (x.n_control != 1) {
            throw SemanticError("Do concurrent: exactly one control statement is required for now",
            x.base.base.loc);
        }
        AST::ConcurrentControl_t &h = *(AST::ConcurrentControl_t*) x.m_control[0];
        AST::ConcurrentReduce_t *red=nullptr;
        for (size_t i=0; i < x.n_locality; i++) {
            if (x.m_locality[i]->type == AST::concurrent_localityType::ConcurrentReduce) {
                red = (AST::ConcurrentReduce_t*) x.m_locality[i];
                break;
            }
        }

        std::string r = "";
        if (red)
        {
            LFORTRAN_ASSERT(red->n_vars == 1)
            r.append("!$OMP DO REDUCTION(");
            //This will need expanded
            if (red->m_op == AST::reduce_opType::ReduceAdd) {
                r.append("+");
            } else if (red->m_op == AST::reduce_opType::ReduceMIN) {
                r.append("MIN"); //Is this right? Check later.
            }
            r.append(":");
            r.append(red->m_vars[0]);
            r.append(")\n");
        } else {
            r.append("!$OMP PARALLEL DO\n");
        }
        r.append("do ");
        if (h.m_var) {
//            r.append(" (");
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
//        r.append(")\n");
        r.append("\n");
        indent_level += 4;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            for (int i=0; i < indent_level; i++) r.append(" ");
            r.append(s);
            r.append("\n");
        }
        indent_level -= 4;
        r.append("end do\n");
        if (red)
            r.append("!$OMP END DO");
        else
            r.append("!$OMP END PARALLEL DO");
        s = r;
    }
    void visit_Cycle(const Cycle_t &/* x */) {
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
    void visit_Exit(const Exit_t &/* x */) {
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
    void visit_Return(const Return_t &/* x */) {
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
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("print");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        if (x.m_fmt) {
            s.append(x.m_fmt);
        } else {
            s.append("()");
        }
        s.append(" ");
        s.append("[");
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            if (i < x.n_values-1) s.append(" ");
        }
        s.append("]");
        s.append(")");
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
    void visit_FuncCallOrArray(const FuncCallOrArray_t &x) {
        std::string r = "";
        r.append(x.m_func);
        r.append("(");
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i].m_end);
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
    void visit_String(const String_t &x) {
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
    void visit_Logical(const Logical_t &/* x */) {
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

std::string ast_to_openmp(LFortran::AST::ast_t &ast) {
    AST::ASTToOPENMPVisitor v;
    v.visit_ast(ast);
    return v.s;
}

}
