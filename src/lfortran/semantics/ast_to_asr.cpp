#include <iostream>
#include <map>
#include <memory>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>


namespace LFortran {

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    return (ASR::expr_t*)f;
}

static inline ASR::stmt_t* STMT(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::stmt);
    return (ASR::stmt_t*)f;
}

static inline ASR::ttype_t* TYPE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    return (ASR::ttype_t*)f;
}

class SymbolTableVisitor : public AST::BaseWalkVisitor<SymbolTableVisitor>
{
public:
    ASR::asr_t *asr;
    Allocator &al;
    SymbolTable translation_unit_scope;
    SubroutineScope subroutine_scope;

    SymbolTableVisitor(Allocator &al) : al{al} {}

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        for (size_t i=0; i<x.n_items; i++) {
            visit_ast(*x.m_items[i]);
        }
        asr = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            translation_unit_scope);
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
        for (size_t i=0; i<x.n_decl; i++) {
            std::cout << "decl2" << std::endl;
            visit_unit_decl2(*x.m_decl[i]);
        }
        asr = ASR::make_Subroutine_t(
            al, x.base.base.loc,
            /* a_name */ x.m_name,
            /* a_args */ nullptr,
            /* n_args */ 0,
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_bind */ nullptr,
            /* a_symtab */ subroutine_scope);
        // TODO: change a_symtab from an integer to std::map and put
        // subroutine_scope in it here.
        std::cout << "Subroutine finished:" << std::endl;
        std::cout << pickle((AST::ast_t&)(x)) << std::endl;
        std::cout << "Symbol table:" << std::endl;
        for (auto &a : subroutine_scope.scope) {
            std::cout << "    " << a.first << " " << a.second.type << " " << a.second.intent << std::endl;
        }
        std::cout << "S";
        std::string sym_name = x.m_name;
        if (translation_unit_scope.scope.find(sym_name) != translation_unit_scope.scope.end()) {
            throw SemanticError("Subroutine already defined", asr->loc);
        }
        translation_unit_scope.scope[sym_name] = asr;
    }

    void visit_decl(const AST::decl_t &x) {
        std::string sym = x.m_sym;
        std::string sym_type = x.m_sym_type;
        if (subroutine_scope.scope.find(sym) == subroutine_scope.scope.end()) {
            SubroutineSymbol s;
            s.name = x.m_sym;
            if (sym_type == "integer") {
                s.type = 2;
            } else if (sym_type == "real") {
                s.type = 1;
            } else {
                Location loc;
                // TODO: decl_t does not have location information...
                loc.first_column = 0;
                loc.first_line = 0;
                loc.last_column = 0;
                loc.last_line = 0;
                throw SemanticError("Unsupported type", loc);
            }
            s.intent = 0;
            if (x.n_attrs > 0) {
                AST::Attribute_t *a = (AST::Attribute_t*)(x.m_attrs[0]);
                if (std::string(a->m_name) == "intent") {
                    if (a->n_args > 0) {
                        std::string intent = std::string(a->m_args[0].m_arg);
                        if (intent == "in") {
                            s.intent = 1;
                        } else if (intent == "out") {
                            s.intent = 2;
                        } else if (intent == "inout") {
                            s.intent = 3;
                        } else {
                            Location loc;
                            // TODO: decl_t does not have location information...
                            loc.first_column = 0;
                            loc.first_line = 0;
                            loc.last_column = 0;
                            loc.last_line = 0;
                            throw SemanticError("Incorrect intent specifier", loc);
                        }
                    } else {
                        Location loc;
                        // TODO: decl_t does not have location information...
                        loc.first_column = 0;
                        loc.first_line = 0;
                        loc.last_column = 0;
                        loc.last_line = 0;
                        throw SemanticError("intent() is empty. Must specify intent", loc);
                    }
                }
            }
            subroutine_scope.scope[sym] = s;

        }
        std::cout << "D(";
        std::cout << x.m_sym << " ";
        std::cout << x.m_sym_type;
        std::cout << ")D" << std::endl;
        // self.visit_sequence(node.dims)
        // self.visit_sequence(node.attrs)
        //if node.initializer:
        //    this->visit_expr(*x.m_initializer);
    }

    void visit_Function(const AST::Function_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        ASR::expr_t *return_var = EXPR(ASR::make_VariableOld_t(al, x.base.base.loc,
                x.m_name, nullptr, 1, type));
        asr = ASR::make_Function_t(al, x.base.base.loc,
            /*char* a_name*/ x.m_name,
            /*expr_t** a_args*/ nullptr, /*size_t n_args*/ 0,
            /*stmt_t** a_body*/ nullptr, /*size_t n_body*/ 0,
            /*tbind_t* a_bind*/ nullptr,
            /*expr_t* a_return_var*/ return_var,
            /*char* a_module*/ nullptr,
            /*int *object* a_symtab*/ 0);
    }
};

class BodyVisitor : public AST::BaseVisitor<BodyVisitor>
{
public:
    Allocator &al;
    ASR::asr_t *asr, *tmp;
    BodyVisitor(Allocator &al, ASR::asr_t *unit) : al{al}, asr{unit} {}

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        for (size_t i=0; i<x.n_items; i++) {
            visit_ast(*x.m_items[i]);
        }
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
    }

    void visit_Function(const AST::Function_t &x) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, 8);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            ASR::stmt_t *stmt = STMT(tmp);
            body.push_back(al, stmt);
        }
        // TODO:
        // We must keep track of the current scope, lookup this function in the
        // scope as "_current_function" and attach the body to it. For now we
        // simply assume `asr` is this very function:
        ASR::Function_t *current_fn = (ASR::Function_t*)asr;
        current_fn->m_body = &body.p[0];
        current_fn->n_body = body.size();
    }

    void visit_Assignment(const AST::Assignment_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = EXPR(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value);
    }
    void visit_Name(const AST::Name_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_VariableOld_t(al, x.base.base.loc,
                x.m_id, nullptr, 1, type);
    }
    void visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_Num_t(al, x.base.base.loc, x.m_n, type);
    }
};

ASR::asr_t *ast_to_asr(Allocator &al, AST::TranslationUnit_t &ast)
{
    SymbolTableVisitor v(al);
    v.visit_TranslationUnit(ast);
    ASR::asr_t *unit = v.asr;

    BodyVisitor b(al, unit);
    b.visit_TranslationUnit(ast);
    return unit;
}

} // namespace LFortran
