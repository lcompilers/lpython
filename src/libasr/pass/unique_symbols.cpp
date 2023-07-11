#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/unique_symbols.h>
#include <libasr/pass/pass_utils.h>
#include <unordered_map>


namespace LCompilers {

using ASR::down_cast;

class UniqueSymbolVisitor: public ASR::CallReplacerOnExpressionsVisitor<UniqueSymbolVisitor> {
    private:

    Allocator& al;

    public:
    std::map<ASR::symbol_t*, std::string> orig_to_new_name;
    std::string mod_name = "";

    UniqueSymbolVisitor(Allocator& al_) : al(al_){}


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        ASR::TranslationUnit_t& xx = const_cast<ASR::TranslationUnit_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = xx.m_global_scope;
        xx.m_global_scope->create_unique_symbols();
        for (auto &a : xx.m_global_scope->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
    }

    void visit_Program(const ASR::Program_t &x) {
        ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : xx.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        transform_stmts(xx.m_body, xx.n_body);
        current_scope = current_scope_copy;
    }

    void visit_Module(const ASR::Module_t &x) {
        ASR::Module_t& xx = const_cast<ASR::Module_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        mod_name = orig_to_new_name[sym] + "_";
        xx.m_symtab->create_unique_symbols(mod_name);
        for (auto &a : xx.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
        mod_name = "";
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : x.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        visit_ttype(*x.m_function_signature);
        for (size_t i=0; i<x.n_args; i++) {
            ASR::expr_t** current_expr_copy_0 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_args[i]));
            call_replacer();
            current_expr = current_expr_copy_0;
            if( x.m_args[i] )
            visit_expr(*x.m_args[i]);
        }
        transform_stmts(xx.m_body, xx.n_body);
        if (xx.m_return_var) {
            ASR::expr_t** current_expr_copy_1 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_return_var));
            call_replacer();
            current_expr = current_expr_copy_1;
            if( x.m_return_var )
            visit_expr(*x.m_return_var);
        }
        current_scope = current_scope_copy;
    }
    void visit_GenericProcedure(const ASR::GenericProcedure_t &x) {
        ASR::GenericProcedure_t& xx = const_cast<ASR::GenericProcedure_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
    }
    void visit_CustomOperator(const ASR::CustomOperator_t &x) {
        ASR::CustomOperator_t& xx = const_cast<ASR::CustomOperator_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
    }

    void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
        ASR::ExternalSymbol_t& xx = const_cast<ASR::ExternalSymbol_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        LCOMPILERS_ASSERT(orig_to_new_name.find(xx.m_external) != orig_to_new_name.end());
        xx.m_original_name = s2c(al, orig_to_new_name[xx.m_external]);
        ASR::Module_t *m = ASRUtils::get_sym_module(x.m_external);
        ASR::symbol_t *m_sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)m);
        LCOMPILERS_ASSERT(orig_to_new_name.find(m_sym) != orig_to_new_name.end());
        xx.m_module_name = s2c(al, orig_to_new_name[m_sym]);
    }

    void visit_StructType(const ASR::StructType_t &x) {
        ASR::StructType_t& xx = const_cast<ASR::StructType_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : x.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_initializers; i++) {
            visit_call_arg(x.m_initializers[i]);
        }
        if (x.m_alignment) {
            ASR::expr_t** current_expr_copy_2 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_alignment));
            call_replacer();
            current_expr = current_expr_copy_2;
            if( x.m_alignment )
            visit_expr(*x.m_alignment);
        }
        current_scope = current_scope_copy;
    }
    void visit_EnumType(const ASR::EnumType_t &x) {
        ASR::EnumType_t& xx = const_cast<ASR::EnumType_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : x.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        visit_ttype(*x.m_type);
        current_scope = current_scope_copy;
    }
    void visit_UnionType(const ASR::UnionType_t &x) {
        ASR::UnionType_t& xx = const_cast<ASR::UnionType_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : x.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_initializers; i++) {
            visit_call_arg(x.m_initializers[i]);
        }
        current_scope = current_scope_copy;
    }
    void visit_Variable(const ASR::Variable_t &x) {
        ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_parent_symtab;
        if (x.m_symbolic_value) {
            ASR::expr_t** current_expr_copy_3 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_symbolic_value));
            call_replacer();
            current_expr = current_expr_copy_3;
            if( x.m_symbolic_value )
            visit_expr(*x.m_symbolic_value);
        }
        if (x.m_value) {
            ASR::expr_t** current_expr_copy_4 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            call_replacer();
            current_expr = current_expr_copy_4;
            if( x.m_value )
            visit_expr(*x.m_value);
        }
        visit_ttype(*x.m_type);
        current_scope = current_scope_copy;
    }
    void visit_ClassType(const ASR::ClassType_t &x) {
        ASR::ClassType_t& xx = const_cast<ASR::ClassType_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : xx.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
    }
    void visit_ClassProcedure(const ASR::ClassProcedure_t &x) {
        ASR::ClassProcedure_t& xx = const_cast<ASR::ClassProcedure_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
    }
    void visit_AssociateBlock(const ASR::AssociateBlock_t &x) {
        ASR::AssociateBlock_t& xx = const_cast<ASR::AssociateBlock_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : xx.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        transform_stmts(xx.m_body, xx.n_body);
        current_scope = current_scope_copy;
    }
    void visit_Block(const ASR::Block_t &x) {
        ASR::Block_t& xx = const_cast<ASR::Block_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : xx.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        transform_stmts(xx.m_body, xx.n_body);
        current_scope = current_scope_copy;
    }
    void visit_Requirement(const ASR::Requirement_t &x) {
        ASR::Requirement_t& xx = const_cast<ASR::Requirement_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : xx.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
    }
    void visit_Template(const ASR::Template_t &x) {
        ASR::Template_t& xx = const_cast<ASR::Template_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        LCOMPILERS_ASSERT(orig_to_new_name.find(sym) != orig_to_new_name.end());
        xx.m_name = s2c(al, orig_to_new_name[sym]);
        xx.m_symtab->create_unique_symbols();
        for (auto &a : xx.m_symtab->get_scope()) {
            orig_to_new_name[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
    }

};


void pass_unique_symbols(Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    UniqueSymbolVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LCompilers
