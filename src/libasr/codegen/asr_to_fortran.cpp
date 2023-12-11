#include <libasr/asr.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>
#include <libasr/codegen/asr_to_c_cpp.h>
#include <libasr/codegen/asr_to_fortran.h>

using LCompilers::ASR::is_a;
using LCompilers::ASR::down_cast;

namespace LCompilers {

enum Precedence {
    Eqv = 2,
    NEqv = 2,
    Or = 3,
    And = 4,
    Not = 5,
    CmpOp = 6,
    Add = 8,
    Sub = 8,
    UnaryMinus = 9,
    Mul = 10,
    Div = 10,
    Pow = 11,
    Ext = 13,
};

class ASRToFortranVisitor : public ASR::BaseVisitor<ASRToFortranVisitor>
{
public:
    std::string s;
    bool use_colors;
    int indent_level;
    std::string indent;
    int indent_spaces;
    // The precedence of the last expression, using the table 10.1
    // in the Fortran 2018 standard
    int last_expr_precedence;
    std::string format_string;

    // Used for importing struct type inside interface
    bool is_interface = false;
    std::vector<std::string> import_struct_type;

public:
    ASRToFortranVisitor(bool _use_colors, int _indent)
        : use_colors{_use_colors}, indent_level{0},
            indent_spaces{_indent}
        { }

    /********************************** Utils *********************************/
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
        if (last_expr_precedence == 9 ||
                last_expr_precedence < current_precedence) {
            s = "(" + s + ")";
        }
    }

    std::string binop2str(const ASR::binopType type) {
        switch (type) {
            case (ASR::binopType::Add) : {
                last_expr_precedence = Precedence::Add;
                return " + ";
            } case (ASR::binopType::Sub) : {
                last_expr_precedence = Precedence::Sub;
                return " - ";
            } case (ASR::binopType::Mul) : {
                last_expr_precedence = Precedence::Mul;
                return "*";
            } case (ASR::binopType::Div) : {
                last_expr_precedence = Precedence::Div;
                return "/";
            } case (ASR::binopType::Pow) : {
                last_expr_precedence = Precedence::Pow;
                return "**";
            } default : {
                throw LCompilersException("Binop type not implemented");
            }
        }
    }

    std::string cmpop2str(const ASR::cmpopType type) {
        last_expr_precedence = Precedence::CmpOp;
        switch (type) {
            case (ASR::cmpopType::Eq)    : return " == ";
            case (ASR::cmpopType::NotEq) : return " /= ";
            case (ASR::cmpopType::Lt)    : return " < " ;
            case (ASR::cmpopType::LtE)   : return " <= ";
            case (ASR::cmpopType::Gt)    : return " > " ;
            case (ASR::cmpopType::GtE)   : return " >= ";
            default : throw LCompilersException("Cmpop type not implemented");
        }
    }

    std::string logicalbinop2str(const ASR::logicalbinopType type) {
        switch (type) {
            case (ASR::logicalbinopType::And) : {
                last_expr_precedence = Precedence::And;
                return " .and. ";
            } case (ASR::logicalbinopType::Or) : {
                last_expr_precedence = Precedence::Or;
                return " .or. ";
            } case (ASR::logicalbinopType::Eqv) : {
                last_expr_precedence = Precedence::Eqv;
                return " .eqv. ";
            } case (ASR::logicalbinopType::NEqv) : {
                last_expr_precedence = Precedence::NEqv;
                return " .neqv. ";
            } default : {
                throw LCompilersException("Logicalbinop type not implemented");
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
            case ASR::ttypeType::Integer: {
                r = "integer(";
                r += std::to_string(down_cast<ASR::Integer_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Real: {
                r = "real(";
                r += std::to_string(down_cast<ASR::Real_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Complex: {
                r = "complex(";
                r += std::to_string(down_cast<ASR::Complex_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Character: {
                ASR::Character_t *c = down_cast<ASR::Character_t>(t);
                r = "character(len=";
                if(c->m_len > 0) {
                    r += std::to_string(c->m_len);
                } else {
                    if (c->m_len == -1) {
                        r += "*";
                    } else if (c->m_len == -2) {
                        r += ":";
                    } else if (c->m_len == -3) {
                        visit_expr(*c->m_len_expr);
                        r += s;
                    }
                }
                r += ", kind=";
                r += std::to_string(c->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Logical: {
                r = "logical(";
                r += std::to_string(down_cast<ASR::Logical_t>(t)->m_kind);
                r += ")";
                break;
            } case ASR::ttypeType::Array: {
                ASR::Array_t* arr_type = down_cast<ASR::Array_t>(t);
                std::string bounds = "";
                for (size_t i = 0; i < arr_type->n_dims; i++) {
                    if (i > 0) bounds += ", ";
                    std::string start = "", len = "";
                    if (arr_type->m_dims[i].m_start) {
                        visit_expr(*arr_type->m_dims[i].m_start);
                        start = s;
                    }
                    if (arr_type->m_dims[i].m_length) {
                        visit_expr(*arr_type->m_dims[i].m_length);
                        len = s;
                    }

                    if (len.length() == 0) {
                        bounds += ":";
                    } else {
                        if (start.length() == 0 || start == "1") {
                            bounds += len;
                        } else {
                            bounds += start + ":(" + start + ")+(" + len + ")-1";
                        }
                    }
                }
                r = get_type(arr_type->m_type) + ", dimension(" + bounds + ")";
                break;
            } case ASR::ttypeType::Allocatable: {
                r = get_type(down_cast<ASR::Allocatable_t>(t)->m_type) + ", allocatable";
                break;
            } case ASR::ttypeType::Pointer: {
                r = get_type(down_cast<ASR::Pointer_t>(t)->m_type) + ", pointer";
                break;
            } case ASR::ttypeType::Struct: {
                ASR::Struct_t* struct_type = down_cast<ASR::Struct_t>(t);
                std::string struct_name = ASRUtils::symbol_name(struct_type->m_derived_type);
                r = "type(";
                r += struct_name;
                r += ")";
                if (std::find(import_struct_type.begin(), import_struct_type.end(),
                        struct_name) == import_struct_type.end() && is_interface) {
                    // Push unique struct names;
                    import_struct_type.push_back(struct_name);
                }
                break;
            }
            default:
                throw LCompilersException("The type `"
                    + ASRUtils::type_to_str_python(t) + "` is not handled yet");
        }
        return r;
    }

    template <typename T>
    void handle_compare(const T& x) {
        std::string r = "", m_op = cmpop2str(x.m_op);
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += m_op;
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    /********************************** Unit **********************************/
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

    /********************************* Symbol *********************************/
    void visit_Program(const ASR::Program_t &x) {
        std::string r;
        r = "program";
        r += " ";
        r.append(x.m_name);
        r += "\n";
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::ExternalSymbol_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }
        r += indent + "implicit none";
        r += "\n";
        std::map<std::string, std::vector<std::string>> struct_dep_graph;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::StructType_t>(*item.second) ||
                    ASR::is_a<ASR::EnumType_t>(*item.second) ||
                    ASR::is_a<ASR::UnionType_t>(*item.second)) {
                std::vector<std::string> struct_deps_vec;
                std::pair<char**, size_t> struct_deps_ptr = ASRUtils::symbol_dependencies(item.second);
                for( size_t i = 0; i < struct_deps_ptr.second; i++ ) {
                    struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                }
                struct_dep_graph[item.first] = struct_deps_vec;
            }
        }

        std::vector<std::string> struct_deps = ASRUtils::order_deps(struct_dep_graph);
        for (auto &item : struct_deps) {
            ASR::symbol_t* struct_sym = x.m_symtab->get_symbol(item);
            visit_symbol(*struct_sym);
            r += s;
        }
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
            if (is_a<ASR::Variable_t>(*var_sym)) {
                visit_symbol(*var_sym);
                r += s;
            }
        }

        visit_body(x, r, false);

        bool prepend_contains_keyword = true;
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                if (prepend_contains_keyword) {
                    prepend_contains_keyword = false;
                    r += "\n";
                    r += "contains";
                    r += "\n\n";
                }
                visit_symbol(*item.second);
                r += s;
            }
        }
        r += "end program";
        r += " ";
        r.append(x.m_name);
        r += "\n";
        s = r;
    }

    void visit_Module(const ASR::Module_t &x) {
        std::string r;
        r = "module";
        r += " ";
        r.append(x.m_name);
        r += "\n";
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::ExternalSymbol_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;
            }
        }
        r += indent + "implicit none";
        r += "\n";
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::GenericProcedure_t>(*item.second)) {
                visit_symbol(*item.second);
                r += s;

            }
        }
        std::map<std::string, std::vector<std::string>> struct_dep_graph;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::StructType_t>(*item.second) ||
                    ASR::is_a<ASR::EnumType_t>(*item.second) ||
                    ASR::is_a<ASR::UnionType_t>(*item.second)) {
                std::vector<std::string> struct_deps_vec;
                std::pair<char**, size_t> struct_deps_ptr = ASRUtils::symbol_dependencies(item.second);
                for( size_t i = 0; i < struct_deps_ptr.second; i++ ) {
                    struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                }
                struct_dep_graph[item.first] = struct_deps_vec;
            }
        }

        std::vector<std::string> struct_deps = ASRUtils::order_deps(struct_dep_graph);
        for (auto &item : struct_deps) {
            ASR::symbol_t* struct_sym = x.m_symtab->get_symbol(item);
            visit_symbol(*struct_sym);
            r += s;
        }
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
            if (is_a<ASR::Variable_t>(*var_sym)) {
                visit_symbol(*var_sym);
                r += s;
            }
        }
        std::vector<std::string> func_name;
        std::vector<std::string> interface_func_name;
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *f = down_cast<ASR::Function_t>(item.second);
                if (ASRUtils::get_FunctionType(f)->m_deftype == ASR::deftypeType::Interface) {
                    interface_func_name.push_back(item.first);
                } else {
                    func_name.push_back(item.first);
                }
            }
        }
        for (size_t i = 0; i < interface_func_name.size(); i++) {
            if (i == 0) {
                r += "interface\n";
                is_interface = true;
                inc_indent();
            }
            visit_symbol(*x.m_symtab->get_symbol(interface_func_name[i]));
            r += s;
            if (i < interface_func_name.size() - 1) {
                r += "\n";
            } else {
                dec_indent();
                is_interface = false;
                r += "end interface\n";
            }
        }
        for (size_t i = 0; i < func_name.size(); i++) {
            if (i == 0) {
                r += "\n";
                r += "contains";
                r += "\n\n";
            }
            visit_symbol(*x.m_symtab->get_symbol(func_name[i]));
            r += s;
            if (i < func_name.size()) r += "\n";
        }
        r += "end module";
        r += " ";
        r.append(x.m_name);
        r += "\n";
        s = r;
    }

    void visit_Function(const ASR::Function_t &x) {
        std::string r = indent;
        ASR::FunctionType_t *type = ASR::down_cast<ASR::FunctionType_t>(x.m_function_signature);
        if (type->m_pure) {
            r += "pure ";
        }
        if (type->m_elemental) {
            r += "elemental ";
        }
        bool is_return_var_declared = false;
        if (x.m_return_var) {
            if (!ASRUtils::is_array(ASRUtils::expr_type(x.m_return_var))) {
                is_return_var_declared = true;
                r += get_type(ASRUtils::expr_type(x.m_return_var));
                r += " ";
            }
            r += "function";
        } else {
            r += "subroutine";
        }
        r += " ";
        r.append(x.m_name);
        r += "(";
        for (size_t i = 0; i < x.n_args; i ++) {
            visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1) r += ", ";
        }
        r += ")";
        if (type->m_abi == ASR::abiType::BindC) {
            r += " bind(c";
            if (type->m_bindc_name) {
                r += ", name = \"";
                r += type->m_bindc_name;
                r += "\"";
            }
            r += ")";
        }
        std::string return_var = "";
        if (x.m_return_var) {
            LCOMPILERS_ASSERT(is_a<ASR::Var_t>(*x.m_return_var));
            visit_expr(*x.m_return_var);
            return_var = s;
            if (strcmp(x.m_name, return_var.c_str())) {
                r += " result(" + return_var + ")";
            }
        }
        r += "\n";

        inc_indent();
        {
            std::string variable_declaration;
            std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
            for (auto &item : var_order) {
                if (is_return_var_declared && item == return_var) continue;
                ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
                if (is_a<ASR::Variable_t>(*var_sym)) {
                    visit_symbol(*var_sym);
                    variable_declaration += s;
                }
            }
            for (size_t i = 0; i < import_struct_type.size(); i ++) {
                if (i == 0) {
                    r += indent;
                    r += "import ";
                }
                r += import_struct_type[i];
                if (i < import_struct_type.size() - 1) {
                    r += ", ";
                } else {
                    r += "\n";
                }
            }
            import_struct_type.clear();
            r += variable_declaration;
        }

        // Interface
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *f = down_cast<ASR::Function_t>(item.second);
                if (ASRUtils::get_FunctionType(f)->m_deftype == ASR::deftypeType::Interface) {
                    is_interface = true;
                    r += indent;
                    r += "interface\n";
                    inc_indent();
                    visit_symbol(*item.second);
                    r += s;
                    r += "\n";
                    dec_indent();
                    r += indent;
                    r += "end interface\n";
                    is_interface = false;
                } else {
                    throw CodeGenError("Nested Function is not handled yet");
                }
            }
        }

        visit_body(x, r, false);
        dec_indent();
        r += indent;
        r += "end ";
        if (x.m_return_var) {
            r += "function";
        } else {
            r += "subroutine";
        }
        r += " ";
        r.append(x.m_name);
        r += "\n";
        s = r;
    }

    void visit_GenericProcedure(const ASR::GenericProcedure_t &x) {
        std::string r = indent;
        r += "interface ";
        r.append(x.m_name);
        r += "\n";
        inc_indent();
        r += indent;
        r += "module procedure ";
        for (size_t i = 0; i < x.n_procs; i++) {
            r += ASRUtils::symbol_name(x.m_procs[i]);
            if (i < x.n_procs-1) r += ", ";
        }
        dec_indent();
        r += "\n";
        r += "end interface ";
        r.append(x.m_name);
        r += "\n";
        s = r;
    }

    // void visit_CustomOperator(const ASR::CustomOperator_t &x) {}

    void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
        ASR::symbol_t *sym = down_cast<ASR::symbol_t>(
            ASRUtils::symbol_parent_symtab(x.m_external)->asr_owner);
        if (!is_a<ASR::StructType_t>(*sym)) {
            s = indent;
            s += "use ";
            s.append(x.m_module_name);
            s += ", only: ";
            s.append(x.m_original_name);
            s += "\n";
        }
    }

    void visit_StructType(const ASR::StructType_t &x) {
        std::string r = indent;
        r += "type :: ";
        r.append(x.m_name);
        r += "\n";
        inc_indent();
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
            if (is_a<ASR::Variable_t>(*var_sym)) {
                visit_symbol(*var_sym);
                r += s;
            }
        }
        dec_indent();
        r += "end type ";
        r.append(x.m_name);
        r += "\n";
        s = r;
    }

    // void visit_EnumType(const ASR::EnumType_t &x) {}

    // void visit_UnionType(const ASR::UnionType_t &x) {}

    void visit_Variable(const ASR::Variable_t &x) {
        std::string r = indent;
        std::string dims = "(";
        r += get_type(x.m_type);
        switch (x.m_intent) {
            case ASR::intentType::In : {
                r += ", intent(in)";
                break;
            } case ASR::intentType::InOut : {
                r += ", intent(inout)";
                break;
            } case ASR::intentType::Out : {
                r += ", intent(out)";
                break;
            } case ASR::intentType::Local : {
                // Pass
                break;
            } case ASR::intentType::ReturnVar : {
                // Pass
                break;
            } case ASR::intentType::Unspecified : {
                // Pass
                break;
            }
            default:
                throw LCompilersException("Intent type is not handled");
        }
        if (x.m_presence == ASR::presenceType::Optional) {
            r += ", optional";
        }
        if (x.m_storage == ASR::storage_typeType::Parameter) {
            r += ", parameter";
        } else if (x.m_storage == ASR::storage_typeType::Save) {
            r += ", save";
        }
        if (x.m_value_attr) {
            r += ", value";
        }
        r += " :: ";
        r.append(x.m_name);
        if (x.m_value) {
            r += " = ";
            visit_expr(*x.m_value);
            r += s;
        } else if (x.m_symbolic_value) {
            r += " = ";
            visit_expr(*x.m_symbolic_value);
            r += s;
        }
        r += "\n";
        s = r;
    }

    // void visit_ClassType(const ASR::ClassType_t &x) {}

    // void visit_ClassProcedure(const ASR::ClassProcedure_t &x) {}

    // void visit_AssociateBlock(const ASR::AssociateBlock_t &x) {}

    // void visit_Block(const ASR::Block_t &x) {}

    // void visit_Requirement(const ASR::Requirement_t &x) {}

    // void visit_Template(const ASR::Template_t &x) {}

    /********************************** Stmt **********************************/
    void visit_Allocate(const ASR::Allocate_t &x) {
        std::string r = indent;
        r += "allocate(";
        for (size_t i = 0; i < x.n_args; i ++) {
            visit_expr(*x.m_args[i].m_a);
            r += s;
            if (x.m_args[i].n_dims > 0) {
                r += "(";
                for (size_t j = 0; j < x.m_args[i].n_dims; j ++) {
                    visit_expr(*x.m_args[i].m_dims[j].m_length);
                    r += s;
                    if (j < x.m_args[i].n_dims-1) r += ", ";
                }
                r += ")";
            }
        }
        r += ")\n";
        s = r;
    }

    // void visit_ReAlloc(const ASR::ReAlloc_t &x) {}

    void visit_Assign(const ASR::Assign_t &x) {
        std::string r;
        r += "assign";
        r += " ";
        r += x.m_label;
        r += " ";
        r += "to";
        r += " ";
        r += x.m_variable;
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

    void visit_Associate(const ASR::Associate_t &x) {
        visit_expr(*x.m_target);
        std::string t = std::move(s);
        visit_expr(*x.m_value);
        std::string v = std::move(s);
        s = t + " => " + v + "\n";
    }

    void visit_Cycle(const ASR::Cycle_t &x) {
        s = indent + "cycle";
        if (x.m_stmt_name) {
            s += " " + std::string(x.m_stmt_name);
        }
        s += "\n";
    }

    // void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t &x) {}

    void visit_ImplicitDeallocate(const ASR::ImplicitDeallocate_t &x) {
        std::string r = indent;
        r += "deallocate(";
        for (size_t i = 0; i < x.n_vars; i ++) {
            visit_expr(*x.m_vars[i]);
            r += s;
            if (i < x.n_vars-1) r += ", ";
        }
        r += ") ";
        r += "! Implicit deallocate\n";
        s = r;
    }

    // void visit_DoConcurrentLoop(const ASR::DoConcurrentLoop_t &x) {}

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        std::string r = indent;
        if (x.m_name) {
            r += std::string(x.m_name);
            r += " : ";
        }

        r += "do ";
        visit_expr(*x.m_head.m_v);
        r += s;
        r += " = ";
        visit_expr(*x.m_head.m_start);
        r += s;
        r += ", ";
        visit_expr(*x.m_head.m_end);
        r += s;
        if (x.m_head.m_increment) {
            r += ", ";
            visit_expr(*x.m_head.m_increment);
            r += s;
        }
        r += "\n";
        visit_body(x, r);
        r += indent;
        r += "end do";
        if (x.m_name) {
            r += " " + std::string(x.m_name);
        }
        r += "\n";
        s = r;
    }

    void visit_ErrorStop(const ASR::ErrorStop_t &/*x*/) {
        s = indent;
        s += "error stop";
        s += "\n";
    }

    void visit_Exit(const ASR::Exit_t &x) {
        s = indent + "exit";
        if (x.m_stmt_name) {
            s += " " + std::string(x.m_stmt_name);
        }
        s += "\n";
    }

    // void visit_ForAllSingle(const ASR::ForAllSingle_t &x) {}

    void visit_GoTo(const ASR::GoTo_t &x) {
        std::string r = indent;
        r += "go to";
        r += " ";
        r += std::to_string(x.m_target_id);
        r += "\n";
        s = r;
    }

    void visit_GoToTarget(const ASR::GoToTarget_t &x) {
        std::string r = "";
        r += std::to_string(x.m_id);
        r += " ";
        r += "continue";
        r += "\n";
        s = r;
    }

    void visit_If(const ASR::If_t &x) {
        std::string r = indent;
        r += "if";
        r += " (";
        visit_expr(*x.m_test);
        r += s;
        r += ") ";
        r += "then";
        r += "\n";
        visit_body(x, r);
        for (size_t i = 0; i < x.n_orelse; i++) {
            r += indent;
            r += "else";
            r += "\n";
            inc_indent();
            visit_stmt(*x.m_orelse[i]);
            r += s;
            dec_indent();
        }
        r += indent;
        r += "end if";
        r += "\n";
        s = r;
    }

    // void visit_IfArithmetic(const ASR::IfArithmetic_t &x) {}

    void visit_Print(const ASR::Print_t &x) {
        std::string r = indent;
        r += "print";
        r += " ";
        if (x.n_values > 0 && is_a<ASR::StringFormat_t>(*x.m_values[0])) {
            ASR::StringFormat_t *sf = down_cast<ASR::StringFormat_t>(x.m_values[0]);
            visit_expr(*sf->m_fmt);
            if (is_a<ASR::StringConstant_t>(*sf->m_fmt)
                    && (!startswith(s, "\"(") || !endswith(s, ")\""))) {
                s = "\"(" + s.substr(1, s.size()-2) + ")\"";
            }
            r += s;
        } else {
            r += "*";
        }
        for (size_t i = 0; i < x.n_values; i++) {
            r += ", ";
            visit_expr(*x.m_values[i]);
            r += s;
        }
        r += "\n";
        s = r;
    }

    void visit_FileOpen(const ASR::FileOpen_t &x) {
        std::string r;
        r = indent;
        r += "open";
        r += "(";
        if (x.m_newunit) {
            visit_expr(*x.m_newunit);
            r += s;
        } else {
            throw CodeGenError("open() function must be called with a file unit number");
        }
        if (x.m_filename) {
            r += ", ";
            r += "file=";
            visit_expr(*x.m_filename);
            r += s;
        }
        if (x.m_status) {
            r += ", ";
            r += "status=";
            visit_expr(*x.m_status);
            r += s;
        }
        if (x.m_form) {
            r += ", ";
            r += "form=";
            visit_expr(*x.m_form);
            r += s;
        }
        r += ")";
        r += "\n";
        s = r;
    }

    void visit_FileClose(const ASR::FileClose_t &x) {
        std::string r;
        r = indent;
        r += "close";
        r += "(";
        if (x.m_unit) {
            visit_expr(*x.m_unit);
            r += s;
        } else {
            throw CodeGenError("close() function must be called with a file unit number");
        }
        r += ")";
        r += "\n";
        s = r;
    }

   void visit_FileRead(const ASR::FileRead_t &x) {
        std::string r;
        r = indent;
        r += "read";
        r += "(";
        if (x.m_unit) {
            visit_expr(*x.m_unit);
            r += s;
        } else {
            r += "*";
        }
        if (x.m_fmt) {
            r += ", ";
            r += "fmt=";
            visit_expr(*x.m_fmt);
            r += s;
        } else {
            r += ", *";
        }
        if (x.m_iomsg) {
            r += ", ";
            r += "iomsg=";
            visit_expr(*x.m_iomsg);
            r += s;
        }
        if (x.m_iostat) {
            r += ", ";
            r += "iostat=";
            visit_expr(*x.m_iostat);
            r += s;
        }
        if (x.m_id) {
            r += ", ";
            r += "id=";
            visit_expr(*x.m_id);
            r += s;
        }
        r += ") ";
        for (size_t i = 0; i < x.n_values; i++) {
            visit_expr(*x.m_values[i]);
            r += s;
            if (i < x.n_values - 1) r += ", ";
        }
        r += "\n";
        s = r;
    }

    // void visit_FileBackspace(const ASR::FileBackspace_t &x) {}

    // void visit_FileRewind(const ASR::FileRewind_t &x) {}

    // void visit_FileInquire(const ASR::FileInquire_t &x) {}

    void visit_FileWrite(const ASR::FileWrite_t &x) {
        std::string r = indent;
        r += "write";
        r += "(";
        if (!x.m_unit) {
            r += "*, ";
        }
        if (x.n_values > 0 && is_a<ASR::StringFormat_t>(*x.m_values[0])) {
            ASR::StringFormat_t *sf = down_cast<ASR::StringFormat_t>(x.m_values[0]);
            visit_expr(*sf->m_fmt);
            if (is_a<ASR::StringConstant_t>(*sf->m_fmt)
                    && (!startswith(s, "\"(") || !endswith(s, ")\""))) {
                s = "\"(" + s.substr(1, s.size()-2) + ")\"";
            }
            r += s;
        } else {
            r += "*";
        }
        r += ") ";
        for (size_t i = 0; i < x.n_values; i++) {
            visit_expr(*x.m_values[i]);
            r += s;
            if (i < x.n_values-1) r += ", ";
        }
        r += "\n";
        s = r;
    }

    void visit_Return(const ASR::Return_t &/*x*/) {
        std::string r = indent;
        r += "return";
        r += "\n";
        s = r;
    }

    void visit_Select(const ASR::Select_t &x) {
        std::string r = indent;
        r += "select case";
        r += " (";
        visit_expr(*x.m_test);
        r += s;
        r += ")\n";
        inc_indent();
        if (x.n_body > 0) {
            for(size_t i = 0; i < x.n_body; i ++) {
                visit_case_stmt(*x.m_body[i]);
                r += s;
            }
        }

        if (x.n_default > 0) {
            r += indent;
            r += "case default\n";
            inc_indent();
            for(size_t i = 0; i < x.n_default; i ++) {
                visit_stmt(*x.m_default[i]);
                r += s;
            }
            dec_indent();
        }
        dec_indent();
        r += indent;
        r += "end select\n";
        s = r;
    }

    void visit_Stop(const ASR::Stop_t /*x*/) {
        s = indent;
        s += "stop";
        s += "\n";
    }

    // void visit_Assert(const ASR::Assert_t &x) {}

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        std::string r = indent;
        r += "call ";
        r += ASRUtils::symbol_name(x.m_name);
        r += "(";
        for (size_t i = 0; i < x.n_args; i ++) {
            visit_expr(*x.m_args[i].m_value);
            r += s;
            if (i < x.n_args-1) r += ", ";
        }
        r += ")\n";
        s = r;
    }

    void visit_Where(const ASR::Where_t &x) {
        std::string r;
        r = indent;
        r += "where";
        r += " ";
        r += "(";
        visit_expr(*x.m_test);
        r += s;
        r += ")\n";
        visit_body(x, r);
        for (size_t i = 0; i < x.n_orelse; i++) {
            r += indent;
            r += "else where";
            r += "\n";
            inc_indent();
            visit_stmt(*x.m_orelse[i]);
            r += s;
            dec_indent();
        }
        r += indent;
        r += "end where";
        r += "\n";
        s = r;
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        std::string r = indent;
        if (x.m_name) {
            r += std::string(x.m_name);
            r += " : ";
        }
        r += "do while";
        r += " (";
        visit_expr(*x.m_test);
        r += s;
        r += ")\n";
        visit_body(x, r);
        r += indent;
        r += "end do";
        if (x.m_name) {
            r += " " + std::string(x.m_name);
        }
        r += "\n";
        s = r;
    }

    // void visit_Nullify(const ASR::Nullify_t &x) {}

    // void visit_Flush(const ASR::Flush_t &x) {}

    // void visit_AssociateBlockCall(const ASR::AssociateBlockCall_t &x) {}

    // void visit_SelectType(const ASR::SelectType_t &x) {}

    // void visit_CPtrToPointer(const ASR::CPtrToPointer_t &x) {}

    // void visit_BlockCall(const ASR::BlockCall_t &x) {}

    // void visit_Expr(const ASR::Expr_t &x) {}

    /********************************** Expr **********************************/
    // void visit_IfExp(const ASR::IfExp_t &x) {}

    void visit_ComplexConstructor(const ASR::ComplexConstructor_t &x) {
        visit_expr(*x.m_re);
        std::string re = s;
        visit_expr(*x.m_im);
        std::string im = s;
        s = "(" + re + ", " + im + ")";
    }

    // void visit_NamedExpr(const ASR::NamedExpr_t &x) {}

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        std::string r = "";
        if (x.m_original_name) {
            r += ASRUtils::symbol_name(x.m_original_name);
        } else {
            r += ASRUtils::symbol_name(x.m_name);
        }
        if (r == "bit_size") {
            // TODO: Remove this once bit_size is implemented in IntrinsicScalarFunction
            visit_expr(*x.m_value);
            return;
        }

        r += "(";
        for (size_t i = 0; i < x.n_args; i ++) {
            visit_expr(*x.m_args[i].m_value);
            r += s;
            if (i < x.n_args-1) r += ", ";
        }
        r += ")";
        s = r;
    }

    void visit_IntrinsicScalarFunction(const ASR::IntrinsicScalarFunction_t &x) {
        std::string out;
        switch (x.m_intrinsic_id) {
            SET_INTRINSIC_NAME(Abs, "abs");
            SET_INTRINSIC_NAME(Exp, "exp");
            SET_INTRINSIC_NAME(Max, "max");
            SET_INTRINSIC_NAME(Min, "min");
            SET_INTRINSIC_NAME(Sqrt, "sqrt");
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

    #define SET_ARR_INTRINSIC_NAME(X, func_name)                                \
        case (static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::X)) : {   \
            visit_expr(*x.m_args[0]);                                           \
            out += func_name; break;                                            \
        }

    void visit_IntrinsicArrayFunction(const ASR::IntrinsicArrayFunction_t &x) {
        std::string out;
        switch (x.m_arr_intrinsic_id) {
            SET_ARR_INTRINSIC_NAME(Any, "any");
            SET_ARR_INTRINSIC_NAME(Sum, "sum");
            SET_ARR_INTRINSIC_NAME(Shape, "shape");
            default : {
                throw LCompilersException("IntrinsicFunction: `"
                    + ASRUtils::get_array_intrinsic_name(x.m_arr_intrinsic_id)
                    + "` is not implemented");
            }
        }
        out += "(" + s + ")";
        s = out;
    }

    // void visit_IntrinsicImpureFunction(const ASR::IntrinsicImpureFunction_t &x) {}

    void visit_StructTypeConstructor(const ASR::StructTypeConstructor_t &x) {
        std::string r = indent;
        r += ASRUtils::symbol_name(x.m_dt_sym);
        r += "(";
        for(size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i].m_value);
            r += s;
            if (i < x.n_args - 1) r += ", ";
        }
        r += ")";
        s = r;
    }

    // void visit_EnumTypeConstructor(const ASR::EnumTypeConstructor_t &x) {}

    // void visit_UnionTypeConstructor(const ASR::UnionTypeConstructor_t &x) {}

    // void visit_ImpliedDoLoop(const ASR::ImpliedDoLoop_t &x) {}

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        s = std::to_string(x.m_n);
        last_expr_precedence = Precedence::Ext;
    }

    // void visit_IntegerBOZ(const ASR::IntegerBOZ_t &x) {}

    // void visit_IntegerBitNot(const ASR::IntegerBitNot_t &x) {}

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        visit_expr_with_precedence(*x.m_arg, 9);
        s = "-" + s;
        last_expr_precedence = Precedence::UnaryMinus;
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        handle_compare(x);
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        std::string r = "", m_op = binop2str(x.m_op);
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += m_op;
        visit_expr_with_precedence(*x.m_right, current_precedence);
        if ((x.m_op == ASR::binopType::Sub && last_expr_precedence <= 8) ||
            (x.m_op == ASR::binopType::Div && last_expr_precedence <= 10)) {
            s = "(" + s + ")";
        }
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    // void visit_UnsignedIntegerConstant(const ASR::UnsignedIntegerConstant_t &x) {}

    // void visit_UnsignedIntegerUnaryMinus(const ASR::UnsignedIntegerUnaryMinus_t &x) {}

    // void visit_UnsignedIntegerBitNot(const ASR::UnsignedIntegerBitNot_t &x) {}

    // void visit_UnsignedIntegerCompare(const ASR::UnsignedIntegerCompare_t &x) {}

    // void visit_UnsignedIntegerBinOp(const ASR::UnsignedIntegerBinOp_t &x) {}

    void visit_RealConstant(const ASR::RealConstant_t &x) {
        int kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        if (kind >= 8) {
            s = std::to_string(x.m_r) + "d0";
        } else {
            s = std::to_string(x.m_r);
        }
        last_expr_precedence = Precedence::Ext;
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        visit_expr_with_precedence(*x.m_arg, 9);
        s = "-" + s;
        last_expr_precedence = Precedence::UnaryMinus;
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
        handle_compare(x);
    }

    void visit_RealBinOp(const ASR::RealBinOp_t &x) {
        std::string r = "", m_op = binop2str(x.m_op);
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += m_op;
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    // void visit_RealCopySign(const ASR::RealCopySign_t &x) {}

    void visit_ComplexConstant(const ASR::ComplexConstant_t &x) {
        std::string re = std::to_string(x.m_re);
        std::string im = std::to_string(x.m_im);
        s = "(" + re + ", " + im + ")";
    }

    void visit_ComplexUnaryMinus(const ASR::ComplexUnaryMinus_t &x) {
        visit_expr_with_precedence(*x.m_arg, 9);
        s = "-" + s;
        last_expr_precedence = Precedence::UnaryMinus;
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t &x) {
        handle_compare(x);
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t &x) {
        std::string r = "", m_op = binop2str(x.m_op);
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
        s = ".";
        if (x.m_value) {
            s += "true";
        } else {
            s += "false";
        }
        s += ".";
        last_expr_precedence = Precedence::Ext;
    }

    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        visit_expr_with_precedence(*x.m_arg, 5);
        s = ".not. " + s;
        last_expr_precedence = Precedence::Not;
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
        handle_compare(x);
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t &x) {
        std::string r = "", m_op = logicalbinop2str(x.m_op);
        int current_precedence = last_expr_precedence;
        visit_expr_with_precedence(*x.m_left, current_precedence);
        r += s;
        r += m_op;
        visit_expr_with_precedence(*x.m_right, current_precedence);
        r += s;
        last_expr_precedence = current_precedence;
        s = r;
    }

    void visit_StringConstant(const ASR::StringConstant_t &x) {
        s = "\"";
        s.append(x.m_s);
        s += "\"";
        last_expr_precedence = Precedence::Ext;
    }

    void visit_StringConcat(const ASR::StringConcat_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(s);
        this->visit_expr(*x.m_right);
        std::string right = std::move(s);
        s = left + "//" + right;
    }

    void visit_StringRepeat(const ASR::StringRepeat_t &x) {
        this->visit_expr(*x.m_left);
        std::string str = s;
        this->visit_expr(*x.m_right);
        std::string n = s;
        s = "repeat(" + str + ", " + n + ")";
    }

    void visit_StringLen(const ASR::StringLen_t &x) {
        visit_expr(*x.m_arg);
        s = "len(" + s + ")";
    }

    void visit_StringItem(const ASR::StringItem_t &x) {
        std::string r = "";
        this->visit_expr(*x.m_arg);
        r += s;
        r += "(";
        this->visit_expr(*x.m_idx);
        r += s;
        r += ":";
        r += s;
        r += ")";
        s = r;
    }

    // void visit_StringSection(const ASR::StringSection_t &x) {}

    void visit_StringCompare(const ASR::StringCompare_t &x) {
        handle_compare(x);
    }

    // void visit_StringOrd(const ASR::StringOrd_t &x) {}

    void visit_StringChr(const ASR::StringChr_t &x) {
        visit_expr(*x.m_arg);
        s = "char(" + s + ")";
    }

    void visit_StringFormat(const ASR::StringFormat_t &x) {
        std::string r = "";
        if (format_string.size() > 0) {
            visit_expr(*x.m_fmt);
            format_string = s;
        }
        for (size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1) r += ", ";
        }
        s = r;
    }

    // void visit_CPtrCompare(const ASR::CPtrCompare_t &x) {}

    // void visit_SymbolicCompare(const ASR::SymbolicCompare_t &x) {}

    void visit_Var(const ASR::Var_t &x) {
        s = ASRUtils::symbol_name(x.m_v);
        last_expr_precedence = Precedence::Ext;
    }

    // void visit_FunctionParam(const ASR::FunctionParam_t &x) {}

    void visit_ArrayConstant(const ASR::ArrayConstant_t &x) {
        std::string r = "[";
        for(size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            r += s;
            if (i < x.n_args-1) r += ", ";
        }
        r += "]";
        s = r;
        last_expr_precedence = Precedence::Ext;
    }

    void visit_ArrayItem(const ASR::ArrayItem_t &x) {
        std::string r = "";
        visit_expr(*x.m_v);
        r += s;
        r += "(";
        for(size_t i = 0; i < x.n_args; i++) {
            if (x.m_args[i].m_right) {
                visit_expr(*x.m_args[i].m_right);
                r += s;
            }
            if (i < x.n_args-1) r += ", ";
        }
        r += ")";
        s = r;
        last_expr_precedence = Precedence::Ext;
    }

    void visit_ArraySection(const ASR::ArraySection_t &x) {
        std::string r = "";
        visit_expr(*x.m_v);
        r += s;
        r += "(";
        for (size_t i = 0; i < x.n_args; i++) {
            if (i > 0) {
                r += ", ";
            }
            std::string left, right, step;
            if (x.m_args[i].m_left) {
                visit_expr(*x.m_args[i].m_left);
                left = std::move(s);
                r += left + ":";
            }
            if (x.m_args[i].m_right) {
                visit_expr(*x.m_args[i].m_right);
                right = std::move(s);
                r += right;
            }
            if (x.m_args[i].m_step ) {
                visit_expr(*x.m_args[i].m_step);
                step = std::move(s);
                if (step != "1") {
                    r += ":" + step;
                }
            }
        }
        r += ")";
        s = r;
        last_expr_precedence = Precedence::Ext;
    }

    void visit_ArraySize(const ASR::ArraySize_t &x) {
        visit_expr(*x.m_v);
        std::string r = "size(" + s;
        if (x.m_dim) {
            r += ", ";
            visit_expr(*x.m_dim);
            r += s;
        }
        r += ")";
        s = r;
    }

    void visit_ArrayBound(const ASR::ArrayBound_t &x) {
        std::string r = "";
        if (x.m_bound == ASR::arrayboundType::UBound) {
            r += "ubound(";
        } else if (x.m_bound == ASR::arrayboundType::LBound) {
            r += "lbound(";
        }
        visit_expr(*x.m_v);
        r += s;
        r += ", ";
        visit_expr(*x.m_dim);
        r += s;
        r += ")";
        s = r;
    }

    void visit_ArrayTranspose(const ASR::ArrayTranspose_t &x) {
        visit_expr(*x.m_matrix);
        s = "transpose(" + s + ")";
    }

    void visit_ArrayPack(const ASR::ArrayPack_t &x) {
        std::string r;
        r += "pack";
        r += "(";
        visit_expr(*x.m_array);
        r += s;
        r += ", ";
        visit_expr(*x.m_mask);
        r += s;
        if (x.m_vector) {
            r += ", ";
            visit_expr(*x.m_vector);
            r += s;
        }
        r += ")";
        s = r;
    }

    void visit_ArrayReshape(const ASR::ArrayReshape_t &x) {
        std::string r;
        r += "reshape(";
        visit_expr(*x.m_array);
        r += s;
        r += ", ";
        visit_expr(*x.m_shape);
        r += s;
        r += ")";
        s = r;
    }

    void visit_ArrayAll(const ASR::ArrayAll_t &x) {
        std::string r;
        r += "all";
        r += "(";
        visit_expr(*x.m_mask);
        r += s;
        if (x.m_dim) {
            visit_expr(*x.m_dim);
            r += s;
        }
        r += ")";
        s = r;
    }

    // void visit_BitCast(const ASR::BitCast_t &x) {}

    void visit_StructInstanceMember(const ASR::StructInstanceMember_t &x) {
        std::string r;
        visit_expr(*x.m_v);
        r += s;
        r += "%";
        r += ASRUtils::symbol_name(ASRUtils::symbol_get_past_external(x.m_m));
        s = r;
    }

    // void visit_StructStaticMember(const ASR::StructStaticMember_t &x) {}

    // void visit_EnumStaticMember(const ASR::EnumStaticMember_t &x) {}

    // void visit_UnionInstanceMember(const ASR::UnionInstanceMember_t &x) {}

    // void visit_EnumName(const ASR::EnumName_t &x) {}

    // void visit_EnumValue(const ASR::EnumValue_t &x) {}

    // void visit_OverloadedCompare(const ASR::OverloadedCompare_t &x) {}

    // void visit_OverloadedBinOp(const ASR::OverloadedBinOp_t &x) {}

    // void visit_OverloadedUnaryMinus(const ASR::OverloadedUnaryMinus_t &x) {}

    void visit_Cast(const ASR::Cast_t &x) {
        std::string r;
        visit_expr(*x.m_arg);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 1: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 2: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 4: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast IntegerToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 1: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 2: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 4: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast RealToInteger: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 1: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 2: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 4: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast RealToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::IntegerToInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 1: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 2: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 4: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "int(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast IntegerToInteger: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::ComplexToComplex) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: r = "cmplx(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "cmplx(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast ComplexToComplex: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::IntegerToComplex) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: r = "cmplx(" + s + ", " + "0.0" + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "cmplx(" + s + ", " + "0.0" + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast IntegerToComplex: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::ComplexToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast ComplexToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToComplex) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: r = "cmplx(" + s + ", " + "0.0" + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "cmplx(" + s + ", " + "0.0" + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast IntegerToComplex: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::LogicalToInteger) : {
                s = "int(" + s + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::LogicalToCharacter) : {
                s = "char(" + s + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::IntegerToLogical) : {
                // Implicit conversion between integer -> logical
                break;
            }
            case (ASR::cast_kindType::LogicalToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: r = "real(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast LogicalToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToLogical) : {
                s = "(bool)(" + s + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::CharacterToLogical) : {
                s = "(bool)(len(" + s + ") > 0)";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::ComplexToLogical) : {
                s = "(bool)(" + s + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::IntegerToCharacter) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 1: s = "char(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 2: s = "char(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 4: s = "char(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: s = "char(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast IntegerToCharacter: Unsupported Kind " + \
                                    std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::CharacterToInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 1: s = "ichar(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 2: s = "ichar(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 4: s = "ichar(" + s + ", " + "kind=dest_kind" + ")"; break;
                    case 8: s = "ichar(" + s + ", " + "kind=dest_kind" + ")"; break;
                    default: throw CodeGenError("Cast CharacterToInteger: Unsupported Kind " + \
                                    std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            default : {
                throw CodeGenError("Cast kind " + std::to_string(x.m_kind) + " not implemented",
                    x.base.base.loc);
            }
        }
    }

    void visit_ArrayBroadcast(const ASR::ArrayBroadcast_t &x) {
        // TODO
        visit_expr(*x.m_array);
    }

    void visit_ArrayPhysicalCast(const ASR::ArrayPhysicalCast_t &x) {
        this->visit_expr(*x.m_arg);
    }

    void visit_ComplexRe(const ASR::ComplexRe_t &x) {
        visit_expr(*x.m_arg);
        s = "real(" + s + ")";
    }

    void visit_ComplexIm(const ASR::ComplexIm_t &x) {
        visit_expr(*x.m_arg);
        s = "aimag(" + s + ")";
    }

    // void visit_CLoc(const ASR::CLoc_t &x) {}

    // void visit_PointerToCPtr(const ASR::PointerToCPtr_t &x) {}

    // void visit_GetPointer(const ASR::GetPointer_t &x) {}

    void visit_IntegerBitLen(const ASR::IntegerBitLen_t &x) {
        visit_expr(*x.m_a);
        s = "bit_size(" + s + ")";
    }

    void visit_Ichar(const ASR::Ichar_t &x) {
        visit_expr(*x.m_arg);
        s = "ichar(" + s + ")";
    }

    void visit_Iachar(const ASR::Iachar_t &x) {
        visit_expr(*x.m_arg);
        s = "iachar(" + s + ")";
    }

    // void visit_SizeOfType(const ASR::SizeOfType_t &x) {}

    // void visit_PointerNullConstant(const ASR::PointerNullConstant_t &x) {}

    // void visit_PointerAssociated(const ASR::PointerAssociated_t &x) {}

    void visit_IntrinsicFunctionSqrt(const ASR::IntrinsicFunctionSqrt_t &x) {
        visit_expr(*x.m_arg);
        s = "sqrt(" + s + ")";
    }

    /******************************* Case Stmt ********************************/
    void visit_CaseStmt(const ASR::CaseStmt_t &x) {
        std::string r = indent;
        r += "case (";
        for(size_t i = 0; i < x.n_test; i ++) {
            visit_expr(*x.m_test[i]);
            r += s;
            if (i < x.n_test-1) r += ", ";
        }
        r += ")\n";
        inc_indent();
        for(size_t i = 0; i < x.n_body; i ++) {
            visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }

    void visit_CaseStmt_Range(const ASR::CaseStmt_Range_t &x) {
        std::string r = indent;
        r += "case (";
        if (x.m_start) {
            visit_expr(*x.m_start);
            r += s;
        }
        r += ":";
        if (x.m_end) {
            visit_expr(*x.m_end);
            r += s;
        }
        r += ")\n";
        inc_indent();
        for(size_t i = 0; i < x.n_body; i ++) {
            visit_stmt(*x.m_body[i]);
            r += s;
        }
        dec_indent();
        s = r;
    }

};

Result<std::string> asr_to_fortran(ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics, bool color, int indent) {
    ASRToFortranVisitor v(color, indent);
    try {
        v.visit_TranslationUnit(asr);
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    }
    return v.s;
}

} // namespace LCompilers
