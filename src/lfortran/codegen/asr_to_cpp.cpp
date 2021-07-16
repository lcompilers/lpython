#include <iostream>
#include <memory>

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/codegen/asr_to_cpp.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/string_utils.h>


namespace LFortran {

using ASR::is_a;
using ASR::down_cast;
using ASR::down_cast2;

// Platform dependent fast unique hash:
uint64_t get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

struct SymbolInfo
{
    bool needs_declaration = true;
    bool intrinsic_function = false;
};

std::string convert_dims(size_t n_dims, ASR::dimension_t *m_dims)
{
    std::string dims;
    for (size_t i=0; i<n_dims; i++) {
        ASR::expr_t *start = m_dims[i].m_start;
        ASR::expr_t *end = m_dims[i].m_end;
        if (!start && !end) {
            dims += "*";
        } else if (start && end) {
            if (is_a<ASR::ConstantInteger_t>(*start) || is_a<ASR::ConstantInteger_t>(*end)) {
                ASR::ConstantInteger_t *s = down_cast<ASR::ConstantInteger_t>(start);
                ASR::ConstantInteger_t *e = down_cast<ASR::ConstantInteger_t>(end);
                if (s->m_n == 1) {
                    dims += "[" + std::to_string(e->m_n) + "]";
                } else {
                    throw CodeGenError("Lower dimension must be 1 for now");
                }
            } else {
                throw CodeGenError("Only numerical dimensions supported for now");
            }
        } else {
            throw CodeGenError("Dimension type not supported");
        }
    }
    return dims;
}

std::string format_type(const std::string &dims, const std::string &type,
        const std::string &name, bool use_ref, bool dummy)
{
    std::string fmt;
    if (dims.size() == 0) {
        std::string ref;
        if (use_ref) ref = "&";
        fmt = type + " " + ref + name;
    } else {
        if (dummy) {
            std::string c;
            if (!use_ref) c = "const ";
            fmt = "const Kokkos::View<" + c + type + dims + "> &" + name;
        } else {
            fmt = "Kokkos::View<" + type + dims + "> " + name
                + "(\"" + name + "\")";
        }
    }
    return fmt;
}

std::string convert_variable_decl(const ASR::Variable_t &v)
{
    std::string sub;
    bool use_ref = (v.m_intent == LFortran::ASRUtils::intent_out || v.m_intent == LFortran::ASRUtils::intent_inout);
    bool dummy = LFortran::ASRUtils::is_arg_dummy(v.m_intent);
    if (is_a<ASR::Integer_t>(*v.m_type)) {
        ASR::Integer_t *t = down_cast<ASR::Integer_t>(v.m_type);
        std::string dims = convert_dims(t->n_dims, t->m_dims);
        sub = format_type(dims, "int", v.m_name, use_ref, dummy);
    } else if (is_a<ASR::Real_t>(*v.m_type)) {
        ASR::Real_t *t = down_cast<ASR::Real_t>(v.m_type);
        std::string dims = convert_dims(t->n_dims, t->m_dims);
        sub = format_type(dims, "float", v.m_name, use_ref, dummy);
    } else if (is_a<ASR::Logical_t>(*v.m_type)) {
        ASR::Logical_t *t = down_cast<ASR::Logical_t>(v.m_type);
        std::string dims = convert_dims(t->n_dims, t->m_dims);
        sub = format_type(dims, "bool", v.m_name, use_ref, dummy);
    } else {
        throw CodeGenError("Type not supported");
    }
    return sub;
}

class ASRToCPPVisitor : public ASR::BaseVisitor<ASRToCPPVisitor>
{
public:
    std::map<uint64_t, SymbolInfo> sym_info;
    std::string src;
    int indentation_level;
    int indentation_spaces;
    bool last_unary_plus;
    bool last_binary_plus;
    bool intrinsic_module = false;

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
        std::string unit_src = "";
        indentation_level = 0;
        indentation_spaces = 4;

        std::string headers =
R"(#include <iostream>
#include <vector>
#include <Kokkos_Core.hpp>

template <typename T>
Kokkos::View<T*> from_std_vector(const std::vector<T> &v)
{
    Kokkos::View<T*> r("r", v.size());
    for (size_t i=0; i < v.size(); i++) {
        r(i) = v[i];
    }
    return r;
}

)";
        unit_src += headers;


        // TODO: We need to pre-declare all functions first, then generate code
        // Otherwise some function might not be found.

        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order
                = LFortran::ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LFORTRAN_ASSERT(x.m_global_scope->scope.find(item)
                    != x.m_global_scope->scope.end());
                if (startswith(item, "lfortran_intrinsic")) {
                    ASR::symbol_t *mod = x.m_global_scope->scope[item];
                    visit_symbol(*mod);
                    unit_src += src;
                }
            }
        }

        // Process procedures first:
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Function_t>(*item.second)
                || is_a<ASR::Subroutine_t>(*item.second)) {
                visit_symbol(*item.second);
                unit_src += src;
            }
        }

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = LFortran::ASRUtils::determine_module_dependencies(x);
        for (auto &item : build_order) {
            LFORTRAN_ASSERT(x.m_global_scope->scope.find(item)
                != x.m_global_scope->scope.end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_global_scope->scope[item];
                visit_symbol(*mod);
                unit_src += src;
            }
        }

        // Then the main program:
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
                unit_src += src;
            }
        }

        src = unit_src;
    }

    void visit_Module(const ASR::Module_t &x) {
        if (startswith(x.m_name, "lfortran_intrinsic_")) {
            intrinsic_module = true;
        } else {
            intrinsic_module = false;
        }
        // Generate code for nested subroutines and functions first:
        std::string contains;
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
                contains += src;
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
                contains += src;
            }
        }
        src = contains;
        intrinsic_module = false;
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
        std::string contains;
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
                contains += src;
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
                contains += src;
            }
        }

        // Generate code for the main program
        indentation_level += 1;
        std::string indent1(indentation_level*indentation_spaces, ' ');
        indentation_level += 1;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);
                decl += indent;
                decl += convert_variable_decl(*v) + ";\n";
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        src = "namespace {\n"
                + contains
                + "\nvoid main2() {\n"
                + decl + body
                + "}\n\n"
                + "}\n"
                + "int main(int argc, char* argv[])\n{\n"
                + indent1 + "Kokkos::initialize(argc, argv);\n"
                + indent1 + "main2();\n"
                + indent1 + "Kokkos::finalize();\n"
                + indent1 + "return 0;\n}\n";
        indentation_level -= 2;
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        indentation_level += 1;
        std::string sub = "void " + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
            sub += convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == LFortran::ASRUtils::intent_local) {
                    SymbolInfo s;
                    s.needs_declaration = true;
                    sym_info[get_hash((ASR::asr_t*)v)] = s;
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == LFortran::ASRUtils::intent_local) {
                    if (sym_info[get_hash((ASR::asr_t*) v)].needs_declaration) {
                        std::string indent(indentation_level*indentation_spaces, ' ');
                        decl += indent;
                        decl += convert_variable_decl(*v) + ";\n";
                    }
                }
            }
        }

        sub += "{\n" + decl + body + "}\n\n";
        src = sub;
        indentation_level -= 1;
    }

    void visit_Function(const ASR::Function_t &x) {
        if (std::string(x.m_name) == "size" && intrinsic_module ) {
            // Intrinsic function `size`
            SymbolInfo s;
            s.intrinsic_function = true;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
            src = "";
            return;
        } else {
            SymbolInfo s;
            s.intrinsic_function = false;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
        }
        std::string sub;
        ASR::Variable_t *return_var = LFortran::ASRUtils::EXPR2VAR(x.m_return_var);
        if (is_a<ASR::Integer_t>(*return_var->m_type)) {
            sub = "int ";
        } else if (is_a<ASR::Real_t>(*return_var->m_type)) {
            sub = "float ";
        } else if (is_a<ASR::Logical_t>(*return_var->m_type)) {
            sub = "bool ";
        } else {
            throw CodeGenError("Return type not supported");
        }
        sub = sub + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
            sub += convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        indentation_level += 1;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == LFortran::ASRUtils::intent_local || v->m_intent == LFortran::ASRUtils::intent_return_var) {
                   decl += indent + convert_variable_decl(*v) + ";\n";
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        body += indent + "return "
            + LFortran::ASRUtils::EXPR2VAR(x.m_return_var)->m_name
            + ";\n";

        if (decl.size() > 0 || body.size() > 0) {
            sub += "{\n" + decl + body + "}\n";
        } else {
            sub[sub.size()-1] = ';';
            sub += "\n";
        }
        sub += "\n";
        src = sub;
        indentation_level -= 1;
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(
            LFortran::ASRUtils::symbol_get_past_external(x.m_name));
        std::string fn_name = fn->m_name;
        if (sym_info[get_hash((ASR::asr_t*)fn)].intrinsic_function) {
            if (fn_name == "size") {
                LFORTRAN_ASSERT(x.n_args > 0);
                visit_expr(*x.m_args[0]);
                std::string var_name = src;
                std::string args;
                if (x.n_args == 1) {
                    args = "0";
                } else {
                    for (size_t i=1; i<x.n_args; i++) {
                        visit_expr(*x.m_args[i]);
                        args += src + "-1";
                        if (i < x.n_args-1) args += ", ";
                    }
                }
                src = var_name + ".extent(" + args + ")";
            } else {
                throw CodeGenError("Intrinsic function '" + fn_name
                        + "' not implemented");
            }
        } else {
            std::string args;
            for (size_t i=0; i<x.n_args; i++) {
                visit_expr(*x.m_args[i]);
                args += src;
                if (i < x.n_args-1) args += ", ";
            }
            src = fn_name + "(" + args + ")";
        }
        last_unary_plus = false;
        last_binary_plus = false;
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        std::string target;
        if (is_a<ASR::Var_t>(*x.m_target)) {
            target = LFortran::ASRUtils::EXPR2VAR(x.m_target)->m_name;
        } else if (is_a<ASR::ArrayRef_t>(*x.m_target)) {
            visit_ArrayRef(*down_cast<ASR::ArrayRef_t>(x.m_target));
            target = src;
        } else {
            LFORTRAN_ASSERT(false)
        }
        this->visit_expr(*x.m_value);
        std::string value = src;
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + target + " = " + value + ";\n";
    }

    void visit_ConstantInteger(const ASR::ConstantInteger_t &x) {
        src = std::to_string(x.m_n);
        last_unary_plus = false;
        last_binary_plus = false;
    }

    void visit_ConstantReal(const ASR::ConstantReal_t &x) {
        src = std::to_string(x.m_r);
        last_unary_plus = false;
        last_binary_plus = false;
    }

    void visit_ConstantString(const ASR::ConstantString_t &x) {
        src = "\"" + std::string(x.m_s) + "\"";
        last_unary_plus = false;
        last_binary_plus = false;
    }

    void visit_ConstantLogical(const ASR::ConstantLogical_t &x) {
        if (x.m_value == true) {
            src = "true";
        } else {
            src = "false";
        }
        last_unary_plus = false;
        last_binary_plus = false;
    }

    void visit_Var(const ASR::Var_t &x) {
        src = ASR::down_cast<ASR::Variable_t>(x.m_v)->m_name;
        last_unary_plus = false;
        last_binary_plus = false;
    }

    void visit_ArrayRef(const ASR::ArrayRef_t &x) {
        std::string out = ASR::down_cast<ASR::Variable_t>(x.m_v)->m_name;
        out += "[";
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i].m_right);
            out += src;
            if (i < x.n_args-1) out += ",";
        }
        out += "-1]";
        src = out;
        last_unary_plus = false;
        last_binary_plus = false;
    }

    void visit_ImplicitCast(const ASR::ImplicitCast_t &x) {
        visit_expr(*x.m_arg);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                // In C++, we do not need to cast int to float explicitly:
                // src = src;
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                src = "(int)(" + src + ")";
                break;
            }
            default : throw CodeGenError("Cast kind not implemented");
        }
    }

    void visit_Compare(const ASR::Compare_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = src;
        this->visit_expr(*x.m_right);
        std::string right = src;
        switch (x.m_op) {
            case (ASR::cmpopType::Eq) : {
                src = left + " == " + right;
                break;
            }
            case (ASR::cmpopType::Gt) : {
                src = left + " > " + right;
                break;
            }
            case (ASR::cmpopType::GtE) : {
                src = left + " >= " + right;
                break;
            }
            case (ASR::cmpopType::Lt) : {
                src = left + " < " + right;
                break;
            }
            case (ASR::cmpopType::LtE) : {
                src = left + " <= " + right;
                break;
            }
            case (ASR::cmpopType::NotEq) : {
                src = left + " != " + right;
                break;
            }
            default : {
                throw CodeGenError("Comparison operator not implemented");
            }
        }
    }

    void visit_UnaryOp(const ASR::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        if (x.m_type->type == ASR::ttypeType::Integer) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // src = src;
                last_unary_plus = false;
                return;
            } else if (x.m_op == ASR::unaryopType::USub) {
                src = "-" + src;
                last_unary_plus = true;
                last_binary_plus = false;
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet");
            }
        } else if (x.m_type->type == ASR::ttypeType::Real) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // src = src;
                last_unary_plus = false;
                return;
            } else if (x.m_op == ASR::unaryopType::USub) {
                src = "-" + src;
                last_unary_plus = true;
                last_binary_plus = false;
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet");
            }
        } else if (x.m_type->type == ASR::ttypeType::Logical) {
            if (x.m_op == ASR::unaryopType::Not) {
                src = "!" + src;
                last_unary_plus = false;
                last_binary_plus = false;
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet in Logical");
            }
        } else {
            throw CodeGenError("UnaryOp: type not supported yet");
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left_val = src;
        if ((last_binary_plus || last_unary_plus)
                    && (x.m_op == ASR::binopType::Mul
                     || x.m_op == ASR::binopType::Div)) {
            left_val = "(" + left_val + ")";
        }
        if (last_unary_plus
                    && (x.m_op == ASR::binopType::Add
                     || x.m_op == ASR::binopType::Sub)) {
            left_val = "(" + left_val + ")";
        }
        this->visit_expr(*x.m_right);
        std::string right_val = src;
        if ((last_binary_plus || last_unary_plus)
                    && (x.m_op == ASR::binopType::Mul
                     || x.m_op == ASR::binopType::Div)) {
            right_val = "(" + right_val + ")";
        }
        if (last_unary_plus
                    && (x.m_op == ASR::binopType::Add
                     || x.m_op == ASR::binopType::Sub)) {
            right_val = "(" + right_val + ")";
        }
        switch (x.m_op) {
            case ASR::binopType::Add: {
                src = left_val + " + " + right_val;
                last_binary_plus = true;
                break;
            }
            case ASR::binopType::Sub: {
                src = left_val + " - " + right_val;
                last_binary_plus = true;
                break;
            }
            case ASR::binopType::Mul: {
                src = left_val + "*" + right_val;
                last_binary_plus = false;
                break;
            }
            case ASR::binopType::Div: {
                src = left_val + "/" + right_val;
                last_binary_plus = false;
                break;
            }
            case ASR::binopType::Pow: {
                src = "std::pow(" + left_val + ", " + right_val + ")";
                last_binary_plus = false;
                break;
            }
            default : throw CodeGenError("Unhandled switch case");
        }
        last_unary_plus = false;
    }

    void visit_BoolOp(const ASR::BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left_val = "(" + src + ")";
        this->visit_expr(*x.m_right);
        std::string right_val = "(" + src + ")";
        switch (x.m_op) {
            case ASR::boolopType::And: {
                src = left_val + " && " + right_val;
                break;
            }
            case ASR::boolopType::Or: {
                src = left_val + " || " + right_val;
                break;
            }
            case ASR::boolopType::NEqv: {
                src = left_val + " != " + right_val;
                break;
            }
            case ASR::boolopType::Eqv: {
                src = left_val + " == " + right_val;
                break;
            }
            default : throw CodeGenError("Unhandled switch case");
        }
        last_binary_plus = false;
        last_unary_plus = false;
    }

    void visit_ConstantArray(const ASR::ConstantArray_t &x) {
        std::string out = "from_std_vector<float>({";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            out += src;
            if (i < x.n_args-1) out += ", ";
        }
        out += "})";
        src = out;
    }

    void visit_Print(const ASR::Print_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
        src = out;
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "while (";
        visit_expr(*x.m_test);
        out += src + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "};\n";
        indentation_level -= 1;
        src = out;
    }

    void visit_Exit(const ASR::Exit_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "break;\n";
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "continue;\n";
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "for (";
        ASR::Variable_t *loop_var = LFortran::ASRUtils::EXPR2VAR(x.m_head.m_v);
        std::string lvname=loop_var->m_name;
        ASR::expr_t *a=x.m_head.m_start;
        ASR::expr_t *b=x.m_head.m_end;
        ASR::expr_t *c=x.m_head.m_increment;
        LFORTRAN_ASSERT(a);
        LFORTRAN_ASSERT(b);
        int increment;
        if (!c) {
            increment = 1;
        } else {
            if (c->type == ASR::exprType::ConstantInteger) {
                increment = down_cast<ASR::ConstantInteger_t>(c)->m_n;
            } else if (c->type == ASR::exprType::UnaryOp) {
                ASR::UnaryOp_t *u = down_cast<ASR::UnaryOp_t>(c);
                LFORTRAN_ASSERT(u->m_op == ASR::unaryopType::USub);
                LFORTRAN_ASSERT(u->m_operand->type == ASR::exprType::ConstantInteger);
                increment = - down_cast<ASR::ConstantInteger_t>(u->m_operand)->m_n;
            } else {
                throw CodeGenError("Do loop increment type not supported");
            }
        }
        std::string cmp_op;
        if (increment > 0) {
            cmp_op = "<=";
        } else {
            cmp_op = ">=";
        }

        out += lvname + "=";
        visit_expr(*a);
        out += src + "; " + lvname + cmp_op;
        visit_expr(*b);
        out += src + "; " + lvname;
        if (increment == 1) {
            out += "++";
        } else if (increment == -1) {
            out += "--";
        } else {
            out += "+=" + std::to_string(increment);
        }
        out += ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "};\n";
        indentation_level -= 1;
        src = out;
    }

    void visit_DoConcurrentLoop(const ASR::DoConcurrentLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "Kokkos::parallel_for(";
        out += "Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(1, ";
        visit_expr(*x.m_head.m_end);
        out += src + "+1)";
        ASR::Variable_t *loop_var = LFortran::ASRUtils::EXPR2VAR(x.m_head.m_v);
        sym_info[get_hash((ASR::asr_t*) loop_var)].needs_declaration = false;
        out += ", KOKKOS_LAMBDA(const long " + std::string(loop_var->m_name)
                + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "});\n";
        indentation_level -= 1;
        src = out;
    }

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "std::cerr << \"ERROR STOP\" << std::endl;\n";
        src += indent + "exit(1);\n";
    }

    void visit_If(const ASR::If_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "if (";
        visit_expr(*x.m_test);
        out += src + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "}";
        if (x.n_orelse == 0) {
            out += ";\n";
        } else {
            out += " else {\n";
            for (size_t i=0; i<x.n_orelse; i++) {
                this->visit_stmt(*x.m_orelse[i]);
                out += src;
            }
            out += indent + "};\n";
        }
        indentation_level -= 1;
        src = out;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(
            LFortran::ASRUtils::symbol_get_past_external(x.m_name));
        std::string out = indent + s->m_name + "(";
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i]->type == ASR::exprType::Var) {
                ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
                std::string arg_name = arg->m_name;
                out += arg_name;
            } else {
                this->visit_expr(*x.m_args[i]);
                out += src;
            }
            if (i < x.n_args-1) out += ", ";
        }
        out += ");\n";
        src = out;
    }

};

std::string asr_to_cpp(ASR::TranslationUnit_t &asr)
{
    ASRToCPPVisitor v;
    v.visit_asr((ASR::asr_t &)asr);
    return v.src;
}

} // namespace LFortran
