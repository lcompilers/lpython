#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <cmath>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/semantics/asr_implicit_cast_rules.h>
#include <lfortran/semantics/asr_common_visitor.h>
#include <lfortran/semantics/asr_symboltable_visitor.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>

namespace LFortran{
    ASR::symbol_t* SymbolTableVisitor::resolve_symbol(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string sub_name = id;
        ASR::symbol_t *sub = scope->resolve_symbol(sub_name);
        if (!sub) {
            throw SemanticError("Symbol '" + sub_name + "' not declared", loc);
        }
        return sub;
    }

    void SymbolTableVisitor::visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        if (!current_scope) {
            current_scope = al.make_new<SymbolTable>(nullptr);
        }
        LFORTRAN_ASSERT(current_scope != nullptr);
        global_scope = current_scope;
        for (size_t i=0; i<x.n_items; i++) {
            AST::astType t = x.m_items[i]->type;
            if (t != AST::astType::expr && t != AST::astType::stmt) {
                visit_ast(*x.m_items[i]);
            }
        }
        global_scope = nullptr;
        asr = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            current_scope, nullptr, 0);
    }

    void SymbolTableVisitor::visit_Module(const AST::Module_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        current_module_dependencies.reserve(al, 4);
        generic_procedures.clear();
        in_module = true;
        for (size_t i=0; i<x.n_use; i++) {
            visit_unit_decl1(*x.m_use[i]);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        add_generic_procedures();
        add_class_procedures();
        asr = ASR::make_Module_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            current_module_dependencies.p,
            current_module_dependencies.n,
            false);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Module already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
        in_module = false;
    }

    void SymbolTableVisitor::visit_Program(const AST::Program_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        current_module_dependencies.reserve(al, 4);
        for (size_t i=0; i<x.n_use; i++) {
            visit_unit_decl1(*x.m_use[i]);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        asr = ASR::make_Program_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            current_module_dependencies.p,
            current_module_dependencies.n,
            /* a_body */ nullptr,
            /* n_body */ 0);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Program already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
    }

    void SymbolTableVisitor::visit_Subroutine(const AST::Subroutine_t &x) {
        ASR::accessType s_access = dflt_access;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            current_procedure_args.push_back(arg);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            if (current_scope->scope.find(arg_s) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + arg_s + "' not defined", x.base.base.loc);
            }
            ASR::symbol_t *var = current_scope->scope[arg_s];
            args.push_back(al, LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }
        std::string sym_name = x.m_name;
        if (assgnd_access.count(sym_name)) {
            s_access = assgnd_access[sym_name];
        }
        if (is_interface){
            deftype = ASR::deftypeType::Interface;
        }
        asr = ASR::make_Subroutine_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ nullptr,
            /* n_body */ 0,
            ASR::abiType::Source,
            s_access, deftype);
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            ASR::symbol_t *f1 = parent_scope->scope[sym_name];
            ASR::Subroutine_t *f2 = ASR::down_cast<ASR::Subroutine_t>(f1);
            if (f2->m_abi == ASR::abiType::Interactive) {
                // Previous declaration will be shadowed
            } else {
                throw SemanticError("Subroutine already defined", asr->loc);
            }
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
        /* FIXME: This can become incorrect/get cleared prematurely, perhaps
           in nested functions, and also in callback.f90 test, but it may not
           matter since we would have already checked the intent */
        current_procedure_args.clear();
    }

    AST::AttrType_t* SymbolTableVisitor::find_return_type(AST::decl_attribute_t** attributes,
            size_t n, const Location &loc) {
        AST::AttrType_t* r = nullptr;
        bool found = false;
        for (size_t i=0; i<n; i++) {
            if (AST::is_a<AST::AttrType_t>(*attributes[i])) {
                if (found) {
                    throw SemanticError("Return type declared twice", loc);
                } else {
                    r = AST::down_cast<AST::AttrType_t>(attributes[i]);
                    found = true;
                }
            }
        }
        return r;
    }

    void SymbolTableVisitor::visit_Function(const AST::Function_t &x) {
        // Extract local (including dummy) variables first
        ASR::accessType s_access = dflt_access;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            current_procedure_args.push_back(arg);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        // Convert and check arguments
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            if (current_scope->scope.find(arg_s) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + arg_s + "' not defined", x.base.base.loc);
            }
            ASR::symbol_t *var = current_scope->scope[arg_s];
            args.push_back(al, LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }

        // Handle the return variable and type
        // First determine the name of the variable: either the function name
        // or result(...)
        char *return_var_name;
        if (x.m_return_var) {
            if (x.m_return_var->type == AST::exprType::Name) {
                return_var_name = ((AST::Name_t*)(x.m_return_var))->m_id;
            } else {
                throw SemanticError("Return variable must be an identifier",
                    x.m_return_var->base.loc);
            }
        } else {
            return_var_name = x.m_name;
        }

        // Determine the type of the variable, the type is either specified as
        //     integer function f()
        // or in local variables as
        //     integer :: f
        ASR::asr_t *return_var;
        AST::AttrType_t *return_type = find_return_type(x.m_attributes,
            x.n_attributes, x.base.base.loc);
        if (current_scope->scope.find(std::string(return_var_name)) == current_scope->scope.end()) {
            // The variable is not defined among local variables, extract the
            // type from "integer function f()" and add the variable.
            if (!return_type) {
                throw SemanticError("Return type not specified",
                        x.base.base.loc);
            }
            ASR::ttype_t *type;
            int a_kind = 4;
            if (return_type->m_kind != nullptr) {
                visit_expr(*return_type->m_kind->m_value);
                ASR::expr_t* kind_expr = LFortran::ASRUtils::EXPR(asr);
                a_kind = ASRUtils::extract_kind(kind_expr, x.base.base.loc);
            }
            switch (return_type->m_type) {
                case (AST::decl_typeType::TypeInteger) : {
                    type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, a_kind, nullptr, 0));
                    break;
                }
                case (AST::decl_typeType::TypeReal) : {
                    type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc, a_kind, nullptr, 0));
                    break;
                }
                case (AST::decl_typeType::TypeComplex) : {
                    type = LFortran::ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc, a_kind, nullptr, 0));
                    break;
                }
                case (AST::decl_typeType::TypeLogical) : {
                    type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
                    break;
                }
                default :
                    throw SemanticError("Return type not supported",
                            x.base.base.loc);
            }
            // Add it as a local variable:
            return_var = ASR::make_Variable_t(al, x.base.base.loc,
                current_scope, return_var_name, LFortran::ASRUtils::intent_return_var, nullptr, nullptr,
                ASR::storage_typeType::Default, type,
                ASR::abiType::Source, ASR::Public, ASR::presenceType::Required);
            current_scope->scope[std::string(return_var_name)]
                = ASR::down_cast<ASR::symbol_t>(return_var);
        } else {
            if (return_type) {
                throw SemanticError("Cannot specify the return type twice",
                    x.base.base.loc);
            }
            // Extract the variable from the local scope
            return_var = (ASR::asr_t*) current_scope->scope[std::string(return_var_name)];
            ASR::down_cast2<ASR::Variable_t>(return_var)->m_intent = LFortran::ASRUtils::intent_return_var;
        }

        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
            ASR::down_cast<ASR::symbol_t>(return_var));

        // Create and register the function
        std::string sym_name = x.m_name;
        if (assgnd_access.count(sym_name)) {
            s_access = assgnd_access[sym_name];
        }

        if (is_interface) {
            deftype = ASR::deftypeType::Interface;
        }
        asr = ASR::make_Function_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_return_var */ LFortran::ASRUtils::EXPR(return_var_ref),
            ASR::abiType::Source, s_access, deftype);
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            ASR::symbol_t *f1 = parent_scope->scope[sym_name];
            ASR::Function_t *f2 = ASR::down_cast<ASR::Function_t>(f1);
            if (f2->m_abi == ASR::abiType::Interactive) {
                // Previous declaration will be shadowed
            } else {
                throw SemanticError("Function already defined", asr->loc);
            }
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
        current_procedure_args.clear();
    }

    void SymbolTableVisitor::visit_StrOp(const AST::StrOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(asr);
        CommonVisitorMethods::visit_StrOp(al, x, left, right, asr);
    }

    void SymbolTableVisitor::visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = LFortran::ASRUtils::EXPR(asr);
        CommonVisitorMethods::visit_UnaryOp(al, x, operand, asr);
    }

    void SymbolTableVisitor::visit_BoolOp(const AST::BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(asr);
        CommonVisitorMethods::visit_BoolOp(al, x, left, right, asr);
    }

    void SymbolTableVisitor::visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(asr);
        CommonVisitorMethods::visit_Compare(al, x, left, right, asr);
    }

    void SymbolTableVisitor::visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(asr);
        CommonVisitorMethods::visit_BinOp(al, x, left, right, asr);
    }

    void SymbolTableVisitor::visit_String(const AST::String_t &x) {
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                8, nullptr, 0));
        asr = ASR::make_ConstantString_t(al, x.base.base.loc, x.m_s, type);
    }

    void SymbolTableVisitor::visit_Logical(const AST::Logical_t &x) {
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        asr = ASR::make_ConstantLogical_t(al, x.base.base.loc, x.m_value, type);
    }

    void SymbolTableVisitor::visit_Complex(const AST::Complex_t &x) {
        this->visit_expr(*x.m_re);
        ASR::expr_t *re = LFortran::ASRUtils::EXPR(asr);
        this->visit_expr(*x.m_im);
        ASR::expr_t *im = LFortran::ASRUtils::EXPR(asr);
        int re_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(LFortran::ASRUtils::expr_type(re));
        int im_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(LFortran::ASRUtils::expr_type(im));
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                std::max(re_kind, im_kind), nullptr, 0));
        asr = ASR::make_ConstantComplex_t(al, x.base.base.loc,
                re, im, type);
    }

    void SymbolTableVisitor::process_dims(Allocator &al, Vec<ASR::dimension_t> &dims,
        AST::dimension_t *m_dim, size_t n_dim) {
        LFORTRAN_ASSERT(dims.size() == 0);
        dims.reserve(al, n_dim);
        for (size_t i=0; i<n_dim; i++) {
            ASR::dimension_t dim;
            dim.loc = m_dim[i].loc;
            if (m_dim[i].m_start) {
                this->visit_expr(*m_dim[i].m_start);
                dim.m_start = LFortran::ASRUtils::EXPR(asr);
            } else {
                dim.m_start = nullptr;
            }
            if (m_dim[i].m_end) {
                this->visit_expr(*m_dim[i].m_end);
                dim.m_end = LFortran::ASRUtils::EXPR(asr);
            } else {
                dim.m_end = nullptr;
            }
            dims.push_back(al, dim);
        }
    }

    void SymbolTableVisitor::visit_Declaration(const AST::Declaration_t &x) {
        if (x.m_vartype == nullptr &&
                x.n_attributes == 1 &&
                AST::is_a<AST::AttrNamelist_t>(*x.m_attributes[0])) {
            //char *name = down_cast<AttrNamelist_t>(x.m_attributes[0])->m_name;
            throw SemanticError("Namelists not implemented yet", x.base.base.loc);
        }
        for (size_t i=0; i<x.n_attributes; i++) {
            if (AST::is_a<AST::AttrType_t>(*x.m_attributes[i])) {
                throw SemanticError("Type must be declared first",
                    x.base.base.loc);
            };
        }
        if (x.m_vartype == nullptr) {
            // Examples:
            // private
            // public
            // private :: x, y, z
            if (x.n_attributes == 0) {
                throw SemanticError("No attribute specified",
                    x.base.base.loc);
            }
            if (x.n_attributes > 1) {
                throw SemanticError("Only one attribute can be specified if type is missing",
                    x.base.base.loc);
            }
            LFORTRAN_ASSERT(x.n_attributes == 1);
            if (AST::is_a<AST::SimpleAttribute_t>(*x.m_attributes[0])) {
                AST::SimpleAttribute_t *sa =
                    AST::down_cast<AST::SimpleAttribute_t>(x.m_attributes[0]);
                if (x.n_syms == 0) {
                    // Example:
                    // private
                    if (sa->m_attr == AST::simple_attributeType
                            ::AttrPrivate) {
                        dflt_access = ASR::accessType::Private;
                    } else if (sa->m_attr == AST::simple_attributeType
                            ::AttrPublic) {
                        // Do nothing (public access is the default)
                        LFORTRAN_ASSERT(dflt_access == ASR::accessType::Public);
                    } else if (sa->m_attr == AST::simple_attributeType
                            ::AttrSave) {
                        if (in_module) {
                            // Do nothing (all variables implicitly have the
                            // save attribute in a module/main program)
                        } else {
                            throw SemanticError("Save Attribute not "
                                    "supported yet", x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("Attribute declaration not "
                                "supported yet", x.base.base.loc);
                    }
                } else {
                    // Example:
                    // private :: x, y, z
                    for (size_t i=0; i<x.n_syms; i++) {
                        AST::var_sym_t &s = x.m_syms[i];
                        std::string sym = s.m_name;
                        if (sa->m_attr == AST::simple_attributeType
                                ::AttrPrivate) {
                            assgnd_access[sym] = ASR::accessType::Private;
                        } else if (sa->m_attr == AST::simple_attributeType
                                ::AttrPublic) {
                            assgnd_access[sym] = ASR::accessType::Public;
                        } else if (sa->m_attr == AST::simple_attributeType
                                ::AttrOptional) {
                            assgnd_presence[sym] = ASR::presenceType::Optional;
                        } else {
                            throw SemanticError("Attribute declaration not "
                                    "supported", x.base.base.loc);
                        }
                    }
                }
            } else {
                throw SemanticError("Attribute declaration not supported",
                    x.base.base.loc);
            }
        } else {
            // Example
            // real(dp), private :: x, y(3), z
            for (size_t i=0; i<x.n_syms; i++) {
                AST::var_sym_t &s = x.m_syms[i];
                std::string sym = s.m_name;
                ASR::accessType s_access = dflt_access;
                ASR::presenceType s_presence = dflt_presence;
                AST::AttrType_t *sym_type =
                    AST::down_cast<AST::AttrType_t>(x.m_vartype);
                if (assgnd_access.count(sym)) {
                    s_access = assgnd_access[sym];
                }
                if (assgnd_presence.count(sym)) {
                    s_presence = assgnd_presence[sym];
                }
                ASR::storage_typeType storage_type =
                        ASR::storage_typeType::Default;
                bool is_pointer = false;
                if (current_scope->scope.find(sym) !=
                        current_scope->scope.end()) {
                    if (current_scope->parent != nullptr) {
                        // re-declaring a global scope variable is allowed
                        // Otherwise raise an error
                        throw SemanticError("Symbol already declared",
                                x.base.base.loc);
                    }
                }
                ASR::intentType s_intent;
                if (std::find(current_procedure_args.begin(),
                        current_procedure_args.end(), s.m_name) !=
                        current_procedure_args.end()) {
                    s_intent = LFortran::ASRUtils::intent_unspecified;
                } else {
                    s_intent = LFortran::ASRUtils::intent_local;
                }
                Vec<ASR::dimension_t> dims;
                dims.reserve(al, 0);
                if (x.n_attributes > 0) {
                    for (size_t i=0; i < x.n_attributes; i++) {
                        AST::decl_attribute_t *a = x.m_attributes[i];
                        if (AST::is_a<AST::SimpleAttribute_t>(*a)) {
                            AST::SimpleAttribute_t *sa =
                                AST::down_cast<AST::SimpleAttribute_t>(a);
                            if (sa->m_attr == AST::simple_attributeType
                                    ::AttrPrivate) {
                                s_access = ASR::accessType::Private;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrPublic) {
                                s_access = ASR::accessType::Public;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrParameter) {
                                storage_type = ASR::storage_typeType::Parameter;
                            } else if( sa->m_attr == AST::simple_attributeType
                                    ::AttrAllocatable ) {
                                storage_type = ASR::storage_typeType::Allocatable;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrPointer) {
                                is_pointer = true;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrOptional) {
                                s_presence = ASR::presenceType::Optional;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrTarget) {
                                // Do nothing for now
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrAllocatable) {
                                // TODO
                            } else {
                                throw SemanticError("Attribute type not implemented yet",
                                        x.base.base.loc);
                            }
                        } else if (AST::is_a<AST::AttrIntent_t>(*a)) {
                            AST::AttrIntent_t *ai =
                                AST::down_cast<AST::AttrIntent_t>(a);
                            switch (ai->m_intent) {
                                case (AST::attr_intentType::In) : {
                                    s_intent = LFortran::ASRUtils::intent_in;
                                    break;
                                }
                                case (AST::attr_intentType::Out) : {
                                    s_intent = LFortran::ASRUtils::intent_out;
                                    break;
                                }
                                case (AST::attr_intentType::InOut) : {
                                    s_intent = LFortran::ASRUtils::intent_inout;
                                    break;
                                }
                                default : {
                                    s_intent = LFortran::ASRUtils::intent_unspecified;
                                    break;
                                }
                            }
                        } else if (AST::is_a<AST::AttrDimension_t>(*a)) {
                            AST::AttrDimension_t *ad =
                                AST::down_cast<AST::AttrDimension_t>(a);
                            if (dims.size() > 0) {
                                throw SemanticError("Dimensions specified twice",
                                        x.base.base.loc);
                            }
                            process_dims(al, dims, ad->m_dim, ad->n_dim);
                        } else {
                            throw SemanticError("Attribute type not implemented yet",
                                    x.base.base.loc);
                        }
                    }
                }
                if (s.n_dim > 0) {
                    if (dims.size() > 0) {
                        throw SemanticError("Cannot specify dimensions both ways",
                                x.base.base.loc);
                    }
                    process_dims(al, dims, s.m_dim, s.n_dim);
                }
                ASR::ttype_t *type;
                int a_kind = 4;
                if (sym_type->m_kind != nullptr) {
                    visit_expr(*sym_type->m_kind->m_value);
                    ASR::expr_t* kind_expr = LFortran::ASRUtils::EXPR(asr);
                    a_kind = ASRUtils::extract_kind(kind_expr, x.base.base.loc);
                }
                if (sym_type->m_type == AST::decl_typeType::TypeReal) {
                    if (is_pointer) {
                        type = LFortran::ASRUtils::TYPE(ASR::make_RealPointer_t(al, x.base.base.loc,
                            a_kind, dims.p, dims.size()));
                    } else {
                        type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc,
                            a_kind, dims.p, dims.size()));
                    }
                } else if (sym_type->m_type == AST::decl_typeType::TypeInteger) {
                    if (is_pointer) {
                        type = LFortran::ASRUtils::TYPE(ASR::make_IntegerPointer_t(al, x.base.base.loc, a_kind, dims.p, dims.size()));
                    } else {
                        type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, a_kind, dims.p, dims.size()));
                    }
                } else if (sym_type->m_type == AST::decl_typeType::TypeLogical) {
                    type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4,
                        dims.p, dims.size()));
                } else if (sym_type->m_type == AST::decl_typeType::TypeComplex) {
                    if( is_pointer ) {
                        type = LFortran::ASRUtils::TYPE(ASR::make_ComplexPointer_t(al, x.base.base.loc, a_kind,
                                    dims.p, dims.size()));
                    } else {
                        type = LFortran::ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc, a_kind,
                                    dims.p, dims.size()));
                    }
                } else if (sym_type->m_type == AST::decl_typeType::TypeCharacter) {
                    type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 4,
                        dims.p, dims.size()));
                } else if (sym_type->m_type == AST::decl_typeType::TypeType) {
                    LFORTRAN_ASSERT(sym_type->m_name);
                    std::string derived_type_name = sym_type->m_name;
                    ASR::symbol_t *v = current_scope->resolve_symbol(derived_type_name);
                    if (!v) {
                        throw SemanticError("Derived type '"
                            + derived_type_name + "' not declared", x.base.base.loc);
                    }
                    type = LFortran::ASRUtils::TYPE(ASR::make_Derived_t(al, x.base.base.loc, v,
                        dims.p, dims.size()));
                } else if (sym_type->m_type == AST::decl_typeType::TypeClass) {
                    LFORTRAN_ASSERT(sym_type->m_name);
                    std::string derived_type_name = sym_type->m_name;
                    ASR::symbol_t *v = current_scope->resolve_symbol(derived_type_name);
                    if (!v) {
                        throw SemanticError("Derived type '"
                            + derived_type_name + "' not declared", x.base.base.loc);
                    }
                    type = LFortran::ASRUtils::TYPE(ASR::make_Class_t(al,
                        x.base.base.loc, v, dims.p, dims.size()));
                } else {
                    throw SemanticError("Type not implemented yet.",
                         x.base.base.loc);
                }
                ASR::expr_t* init_expr = nullptr;
                if (s.m_initializer != nullptr) {
                    this->visit_expr(*s.m_initializer);
                    init_expr = LFortran::ASRUtils::EXPR(asr);
                    ASR::ttype_t *init_type = LFortran::ASRUtils::expr_type(init_expr);
                    ImplicitCastRules::set_converted_value(al, x.base.base.loc, &init_expr, init_type, type);
                }
                ASR::asr_t *v = ASR::make_Variable_t(al, x.base.base.loc, current_scope,
                        s.m_name, s_intent, init_expr, nullptr, storage_type, type,
                        ASR::abiType::Source, s_access, s_presence);
                current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(v);
            } // for m_syms
        }
    }

    Vec<ASR::expr_t*> SymbolTableVisitor::visit_expr_list(AST::fnarg_t *ast_list, size_t n) {
        Vec<ASR::expr_t*> asr_list;
        asr_list.reserve(al, n);
        for (size_t i=0; i<n; i++) {
            visit_expr(*ast_list[i].m_end);
            ASR::expr_t *expr = LFortran::ASRUtils::EXPR(asr);
            asr_list.push_back(al, expr);
        }
        return asr_list;
    }

    void SymbolTableVisitor::visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x) {
        SymbolTable *scope = current_scope;
        std::string var_name = x.m_func;
        ASR::symbol_t *v = current_scope->resolve_symbol(var_name);
        if (!v) {
            std::string remote_sym = to_lower(var_name);
            if (intrinsic_procedures.find(remote_sym)
                        != intrinsic_procedures.end()) {
                std::string module_name = intrinsic_procedures[remote_sym];

                bool shift_scope = false;
                if (current_scope->parent->parent) {
                    current_scope = current_scope->parent;
                    shift_scope = true;
                }
                ASR::Module_t *m = LFortran::ASRUtils::load_module(al, current_scope->parent,
                    module_name, x.base.base.loc, true);
                if (shift_scope) current_scope = scope;

                ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
                if (!t) {
                    throw SemanticError("The symbol '" + remote_sym
                        + "' not found in the module '" + module_name + "'",
                        x.base.base.loc);
                }

                ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
                ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                    al, mfn->base.base.loc,
                    /* a_symtab */ current_scope,
                    /* a_name */ mfn->m_name,
                    (ASR::symbol_t*)mfn,
                    m->m_name, mfn->m_name,
                    ASR::accessType::Private
                    );
                std::string sym = mfn->m_name;
                current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                v = ASR::down_cast<ASR::symbol_t>(fn);
                // Add the module `m` to current module dependencies
                if (!present(current_module_dependencies, m->m_name)) {
                    current_module_dependencies.push_back(al, m->m_name);
                }
            } else {
                throw SemanticError("Function '" + var_name + "' not found"
                    " or not implemented yet (if it is intrinsic)",
                    x.base.base.loc);
            }
        }
        Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
        ASR::ttype_t *type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(
                LFortran::ASRUtils::symbol_get_past_external(v))
                ->m_return_var)->m_type;
        asr = ASR::make_FunctionCall_t(al, x.base.base.loc, v, nullptr,
            args.p, args.size(), nullptr, 0, type, nullptr, nullptr);
    }

    void SymbolTableVisitor::visit_DerivedType(const AST::DerivedType_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        dt_name = x.m_name;
        for (size_t i=0; i<x.n_items; i++) {
            this->visit_unit_decl2(*x.m_items[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_procedure_decl(*x.m_contains[i]);
        }
        std::string sym_name = x.m_name;
        if (current_scope->scope.find(sym_name) != current_scope->scope.end()) {
            throw SemanticError("DerivedType already defined", x.base.base.loc);
        }
        asr = ASR::make_DerivedType_t(al, x.base.base.loc, current_scope,
            x.m_name, ASR::abiType::Source, dflt_access);
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);

        current_scope = parent_scope;
    }

    void SymbolTableVisitor::visit_InterfaceProc(const AST::InterfaceProc_t &x) {
        is_interface = true;
        visit_program_unit(*x.m_proc);
        is_interface = false;
        return;
    }

    void SymbolTableVisitor::visit_DerivedTypeProc(const AST::DerivedTypeProc_t &x) {
        for (size_t i = 0; i < x.n_symbols; i++) {
            AST::UseSymbol_t *use_sym = AST::down_cast<AST::UseSymbol_t>(
                x.m_symbols[i]);
            if (use_sym->m_rename) {
                class_procedures[dt_name][use_sym->m_rename] = use_sym->m_sym;
            } else {
                class_procedures[dt_name][use_sym->m_sym] = use_sym->m_sym;
            }
        }
    }

    void SymbolTableVisitor::visit_Interface(const AST::Interface_t &x) {
        if (AST::is_a<AST::InterfaceHeaderName_t>(*x.m_header)) {
            char *generic_name = AST::down_cast<AST::InterfaceHeaderName_t>(x.m_header)->m_name;
            std::vector<std::string> proc_names;
            for (size_t i = 0; i < x.n_items; i++) {
                AST::interface_item_t *item = x.m_items[i];
                if (AST::is_a<AST::InterfaceModuleProcedure_t>(*item)) {
                    AST::InterfaceModuleProcedure_t *proc
                        = AST::down_cast<AST::InterfaceModuleProcedure_t>(item);
                    for (size_t i = 0; i < proc->n_names; i++) {
                        char *proc_name = proc->m_names[i];
                        proc_names.push_back(std::string(proc_name));
                    }
                } else {
                    throw SemanticError("Interface procedure type not imlemented yet", item->base.loc);
                }
            }
            generic_procedures[std::string(generic_name)] = proc_names;
        } else if (AST::is_a<AST::InterfaceHeader_t>(*x.m_header)) {
            std::vector<std::string> proc_names;
            for (size_t i = 0; i < x.n_items; i++) {
                visit_interface_item(*x.m_items[i]);
            }
        } else {
            throw SemanticError("Interface type not imlemented yet", x.base.base.loc);
        }
    }

    void SymbolTableVisitor::add_generic_procedures() {
        for (auto &proc : generic_procedures) {
            Location loc;
            loc.first_line = 1;
            loc.last_line = 1;
            loc.first_column = 1;
            loc.last_column = 1;
            Str s;
            s.from_str_view(proc.first);
            char *generic_name = s.c_str(al);
            Vec<ASR::symbol_t*> symbols;
            symbols.reserve(al, proc.second.size());
            for (auto &pname : proc.second) {
                ASR::symbol_t *x;
                Str s;
                s.from_str_view(pname);
                char *name = s.c_str(al);
                x = resolve_symbol(loc, name);
                symbols.push_back(al, x);
            }
            ASR::asr_t *v = ASR::make_GenericProcedure_t(al, loc,
                current_scope,
                generic_name, symbols.p, symbols.size(), ASR::Public);
            current_scope->scope[proc.first] = ASR::down_cast<ASR::symbol_t>(v);
        }
    }

    void SymbolTableVisitor::add_class_procedures() {
        for (auto &proc : class_procedures) {
            Location loc;
            loc.first_line = 1;
            loc.last_line = 1;
            loc.first_column = 1;
            loc.last_column = 1;
            ASR::DerivedType_t *clss = ASR::down_cast<ASR::DerivedType_t>(
                current_scope->scope[proc.first]);
            for (auto &pname : proc.second) {
                ASR::symbol_t *proc_sym = current_scope->scope[pname.second];
                Str s;
                s.from_str_view(pname.first);
                char *name = s.c_str(al);
                s.from_str_view(pname.second);
                char *proc_name = s.c_str(al);
                ASR::asr_t *v = ASR::make_ClassProcedure_t(al, loc,
                    current_scope, name, proc_name, proc_sym,
                    ASR::abiType::Source);
                ASR::symbol_t *cls_proc_sym = ASR::down_cast<ASR::symbol_t>(v);
                clss->m_symtab->scope[pname.first] = cls_proc_sym;
            }
        }
    }

    void SymbolTableVisitor::visit_Use(const AST::Use_t &x) {
        std::string msym = x.m_module;
        if (!present(current_module_dependencies, x.m_module)) {
            current_module_dependencies.push_back(al, x.m_module);
        }
        ASR::symbol_t *t = current_scope->parent->resolve_symbol(msym);
        if (!t) {
            t = (ASR::symbol_t*)LFortran::ASRUtils::load_module(al, current_scope->parent,
                msym, x.base.base.loc, false);
        }
        if (!ASR::is_a<ASR::Module_t>(*t)) {
            throw SemanticError("The symbol '" + msym + "' must be a module",
                x.base.base.loc);
        }
        ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(t);
        if (x.n_symbols == 0) {
            // Import all symbols from the module, e.g.:
            //     use a
            for (auto &item : m->m_symtab->scope) {
                // TODO: only import "public" symbols from the module
                if (ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                    ASR::Subroutine_t *msub = ASR::down_cast<ASR::Subroutine_t>(item.second);
                    ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                        al, msub->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ msub->m_name,
                        (ASR::symbol_t*)msub,
                        m->m_name, msub->m_name,
                        dflt_access
                        );
                    std::string sym = msub->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(sub);
                } else if (ASR::is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(item.second);
                    ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                        al, mfn->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ mfn->m_name,
                        (ASR::symbol_t*)mfn,
                        m->m_name, mfn->m_name,
                        dflt_access
                        );
                    std::string sym = mfn->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                } else if (ASR::is_a<ASR::GenericProcedure_t>(*item.second)) {
                    ASR::GenericProcedure_t *gp = ASR::down_cast<
                        ASR::GenericProcedure_t>(item.second);
                    ASR::asr_t *ep = ASR::make_ExternalSymbol_t(
                        al, gp->base.base.loc,
                        current_scope,
                        /* a_name */ gp->m_name,
                        (ASR::symbol_t*)gp,
                        m->m_name, gp->m_name,
                        dflt_access
                        );
                    std::string sym = gp->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(ep);
                } else if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                    ASR::Variable_t *mvar = ASR::down_cast<ASR::Variable_t>(item.second);
                    ASR::asr_t *var = ASR::make_ExternalSymbol_t(
                        al, mvar->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ mvar->m_name,
                        (ASR::symbol_t*)mvar,
                        m->m_name, mvar->m_name,
                        dflt_access
                        );
                    std::string sym = mvar->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(var);
                } else {
                    throw LFortranException("'" + item.first + "' is not supported yet for declaring with use.");
                }
            }
        } else {
            // Only import individual symbols from the module, e.g.:
            //     use a, only: x, y, z
            for (size_t i = 0; i < x.n_symbols; i++) {
                std::string remote_sym = AST::down_cast<AST::UseSymbol_t>(x.m_symbols[i])->m_sym;
                std::string local_sym;
                if (AST::down_cast<AST::UseSymbol_t>(x.m_symbols[i])->m_rename) {
                    local_sym = AST::down_cast<AST::UseSymbol_t>(x.m_symbols[i])->m_rename;
                } else {
                    local_sym = remote_sym;
                }
                ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
                if (!t) {
                    throw SemanticError("The symbol '" + remote_sym + "' not found in the module '" + msym + "'",
                        x.base.base.loc);
                }
                if (ASR::is_a<ASR::Subroutine_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Subroutine already defined",
                            x.base.base.loc);
                    }
                    ASR::Subroutine_t *msub = ASR::down_cast<ASR::Subroutine_t>(t);
                    // `msub` is the Subroutine in a module. Now we construct
                    // an ExternalSymbol that points to
                    // `msub` via the `external` field.
                    Str name;
                    name.from_str(al, local_sym);
                    ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                        al, msub->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ name.c_str(al),
                        (ASR::symbol_t*)msub,
                        m->m_name, msub->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(sub);
                } else if (ASR::is_a<ASR::GenericProcedure_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Symbol already defined",
                            x.base.base.loc);
                    }
                    ASR::GenericProcedure_t *gp = ASR::down_cast<ASR::GenericProcedure_t>(t);
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *ep = ASR::make_ExternalSymbol_t(
                        al, t->base.loc,
                        current_scope,
                        /* a_name */ cname,
                        t,
                        m->m_name, gp->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(ep);
                } else if (ASR::is_a<ASR::ExternalSymbol_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Symbol already defined",
                            x.base.base.loc);
                    }
                    // Repack ExternalSymbol to point directly to the original symbol
                    ASR::ExternalSymbol_t *es = ASR::down_cast<ASR::ExternalSymbol_t>(t);
                    ASR::asr_t *ep = ASR::make_ExternalSymbol_t(
                        al, es->base.base.loc,
                        current_scope,
                        /* a_name */ es->m_name,
                        es->m_external,
                        es->m_module_name, es->m_original_name,
                        es->m_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(ep);
                } else if (ASR::is_a<ASR::Function_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Function already defined",
                            x.base.base.loc);
                    }
                    ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
                    // `mfn` is the Function in a module. Now we construct
                    // an ExternalSymbol that points to it.
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                        al, mfn->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ cname,
                        (ASR::symbol_t*)mfn,
                        m->m_name, mfn->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(fn);
                } else if (ASR::is_a<ASR::Variable_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Variable already defined",
                            x.base.base.loc);
                    }
                    ASR::Variable_t *mv = ASR::down_cast<ASR::Variable_t>(t);
                    // `mv` is the Variable in a module. Now we construct
                    // an ExternalSymbol that points to it.
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *v = ASR::make_ExternalSymbol_t(
                        al, mv->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ cname,
                        (ASR::symbol_t*)mv,
                        m->m_name, mv->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(v);
                } else if( ASR::is_a<ASR::DerivedType_t>(*t) ) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Derived type already defined",
                            x.base.base.loc);
                    }
                    ASR::DerivedType_t *mv = ASR::down_cast<ASR::DerivedType_t>(t);
                    // `mv` is the Variable in a module. Now we construct
                    // an ExternalSymbol that points to it.
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *v = ASR::make_ExternalSymbol_t(
                        al, mv->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ cname,
                        (ASR::symbol_t*)mv,
                        m->m_name, mv->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(v);
                } else {
                    throw LFortranException("Only Subroutines, Functions, Variables and Derived supported in 'use'");
                }
            }
        }
    }

    void SymbolTableVisitor::visit_Real(const AST::Real_t &x) {
        int a_kind = ASRUtils::extract_kind(x.m_n);
        double r = ASRUtils::extract_real(x.m_n);
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc,
                a_kind, nullptr, 0));
        asr = ASR::make_ConstantReal_t(al, x.base.base.loc, r, type);
    }

    ASR::asr_t* SymbolTableVisitor::resolve_variable(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string var_name = id;
        ASR::symbol_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            throw SemanticError("Variable '" + var_name + "' not declared", loc);
        }
        return ASR::make_Var_t(al, loc, v);
    }

    void SymbolTableVisitor::visit_Name(const AST::Name_t &x) {
        asr = resolve_variable(x.base.base.loc, x.m_id);
    }

    void SymbolTableVisitor::visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        if (BigInt::is_int_ptr(x.m_n)) {
            throw SemanticError("Integer constants larger than 2^62-1 are not implemented yet", x.base.base.loc);
        } else {
            LFORTRAN_ASSERT(!BigInt::is_int_ptr(x.m_n));
            asr = ASR::make_ConstantInteger_t(al, x.base.base.loc, x.m_n, type);
        }
    }

    void SymbolTableVisitor::visit_Parenthesis(const AST::Parenthesis_t &x) {
        visit_expr(*x.m_operand);
    }

} // namespace LFortran
