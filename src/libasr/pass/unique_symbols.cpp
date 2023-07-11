#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/unique_symbols.h>
#include <libasr/pass/pass_utils.h>
#include <unordered_map>
#include <set>


std::string lcompilers_unique_ID;

namespace LCompilers {

using ASR::down_cast;

class UniqueSymbolVisitor: public ASR::BaseWalkVisitor<UniqueSymbolVisitor> {
    private:

    Allocator& al;

    public:
    std::map<ASR::symbol_t*, std::string> sym_to_new_name;
    std::string mod_name = "";
    std::set<std::string> skip_list;

    UniqueSymbolVisitor(Allocator& al_) : al(al_){}


    std::string update_name(std::string curr_name, std::string prefix="") {
        if (startswith(curr_name, "_lpython") || startswith(curr_name, "_lfortran") ) {
            return curr_name;
        }
        if (skip_list.find(curr_name) != skip_list.end()) {
            return curr_name;
        }
        return prefix + curr_name + lcompilers_unique_ID;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        ASR::TranslationUnit_t& xx = const_cast<ASR::TranslationUnit_t&>(x);
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        for (auto &a : xx.m_global_scope->get_scope()) {
            tmp_scope[a.second] = a.first;
            if (sym_to_new_name.find(a.second) == sym_to_new_name.end()) {
                visit_symbol(*a.second);
            }
        }
        for (auto &a: tmp_scope) {
            xx.m_global_scope->erase_symbol(a.second);
            xx.m_global_scope->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            xx.m_dependencies[i] = s2c(al, update_name(xx.m_dependencies[i]));
        }
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            if (sym_to_new_name.find(a.second) == sym_to_new_name.end()) {
                visit_symbol(*a.second);
            }
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_Module(const ASR::Module_t &x) {
        ASR::Module_t& xx = const_cast<ASR::Module_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        mod_name = xx.m_name;
        sym_to_new_name[sym] = xx.m_name;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            xx.m_dependencies[i] = s2c(al, update_name(xx.m_dependencies[i]));
        }
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            if (sym_to_new_name.find(a.second) == sym_to_new_name.end()) {
                visit_symbol(*a.second);
            }
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
        mod_name = "";
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type->m_abi == ASR::abiType::BindC) {
            skip_list.insert(x.m_name);
        }
        ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            xx.m_dependencies[i] = s2c(al, update_name(xx.m_dependencies[i]));
        }
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            if (sym_to_new_name.find(a.second) == sym_to_new_name.end()) {
                visit_symbol(*a.second);
            }
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_GenericProcedure(const ASR::GenericProcedure_t &x) {
        ASR::GenericProcedure_t& xx = const_cast<ASR::GenericProcedure_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
    }

    void visit_CustomOperator(const ASR::CustomOperator_t &x) {
        ASR::CustomOperator_t& xx = const_cast<ASR::CustomOperator_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
    }

    void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
        ASR::ExternalSymbol_t& xx = const_cast<ASR::ExternalSymbol_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        ASR::Module_t *m = ASRUtils::get_sym_module(x.m_external);
        if (m) {
            ASR::symbol_t *m_sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)m);
            if (sym_to_new_name.find(m_sym) == sym_to_new_name.end()) {
                visit_symbol(*m_sym);
            }
        } else {
            this->visit_symbol(*x.m_external);
        }
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        xx.m_original_name = s2c(al, update_name(xx.m_original_name));
        xx.m_module_name = s2c(al, update_name(xx.m_module_name));
    }

    void visit_StructType(const ASR::StructType_t &x) {
        ASR::StructType_t& xx = const_cast<ASR::StructType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            xx.m_dependencies[i] = s2c(al, update_name(xx.m_dependencies[i]));
        }
        for (size_t i=0; i<xx.n_members; i++) {
            xx.m_members[i] = s2c(al, update_name(xx.m_members[i]));
        }
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_EnumType(const ASR::EnumType_t &x) {
        ASR::EnumType_t& xx = const_cast<ASR::EnumType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            xx.m_dependencies[i] = s2c(al, update_name(xx.m_dependencies[i]));
        }
        for (size_t i=0; i<xx.n_members; i++) {
            xx.m_members[i] = s2c(al, update_name(xx.m_members[i]));
        }
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_UnionType(const ASR::UnionType_t &x) {
        ASR::UnionType_t& xx = const_cast<ASR::UnionType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            xx.m_dependencies[i] = s2c(al, update_name(xx.m_dependencies[i]));
        }
        for (size_t i=0; i<xx.n_members; i++) {
            xx.m_members[i] = s2c(al, update_name(xx.m_members[i]));
        }
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_Variable(const ASR::Variable_t &x) {
        ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            xx.m_dependencies[i] = s2c(al, update_name(xx.m_dependencies[i]));
        }
    }

    void visit_ClassType(const ASR::ClassType_t &x) {
        ASR::ClassType_t& xx = const_cast<ASR::ClassType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_ClassProcedure(const ASR::ClassProcedure_t &x) {
        ASR::ClassProcedure_t& xx = const_cast<ASR::ClassProcedure_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
    }

    void visit_AssociateBlock(const ASR::AssociateBlock_t &x) {
        ASR::AssociateBlock_t& xx = const_cast<ASR::AssociateBlock_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_Block(const ASR::Block_t &x) {
        ASR::Block_t& xx = const_cast<ASR::Block_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_Requirement(const ASR::Requirement_t &x) {
        ASR::Requirement_t& xx = const_cast<ASR::Requirement_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
    }

    void visit_Template(const ASR::Template_t &x) {
        ASR::Template_t& xx = const_cast<ASR::Template_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            return;
        }
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        xx.m_name = s2c(al, update_name(xx.m_name));
        sym_to_new_name[sym] = xx.m_name;
        for (auto &a : xx.m_symtab->get_scope()) {
            tmp_scope[a.second] = a.first;
            this->visit_symbol(*a.second);
        }
        for (auto &a: tmp_scope) {
            xx.m_symtab->erase_symbol(a.second);
            xx.m_symtab->add_symbol(sym_to_new_name[a.first], a.first);
        }
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
