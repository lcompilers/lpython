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

class SymbolRenameVisitor: public ASR::BaseWalkVisitor<SymbolRenameVisitor> {
    public:
    std::map<ASR::symbol_t*, std::string> sym_to_renamed;
    bool module_name_mangling;
    bool global_symbols_mangling;
    bool intrinsic_symbols_mangling;
    bool all_symbols_mangling;
    bool should_mangle = false;

    SymbolRenameVisitor(
    bool mm, bool gm, bool im, bool am) : module_name_mangling(mm),
    global_symbols_mangling(gm), intrinsic_symbols_mangling(im),
    all_symbols_mangling(am){}


    std::string update_name(std::string curr_name) {
        if (startswith(curr_name, "_lpython") || startswith(curr_name, "_lfortran") ) {
            return curr_name;
        }
        return curr_name + lcompilers_unique_ID;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        ASR::TranslationUnit_t& xx = const_cast<ASR::TranslationUnit_t&>(x);
        std::map<ASR::symbol_t*, std::string> tmp_scope;
        for (auto &a : xx.m_global_scope->get_scope()) {
            visit_symbol(*a.second);
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
    }

    void visit_Module(const ASR::Module_t &x) {
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        bool should_mangle_copy = should_mangle;
        if (all_symbols_mangling || module_name_mangling || should_mangle) {
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        if ((x.m_intrinsic && intrinsic_symbols_mangling) ||
                (global_symbols_mangling && startswith(x.m_name, "_global_symbols"))) {
            should_mangle = true;
        }
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        should_mangle = should_mangle_copy;
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type->m_abi != ASR::abiType::BindC) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            if (all_symbols_mangling || should_mangle) {
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
    }

    void visit_GenericProcedure(const ASR::GenericProcedure_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
    }

    void visit_CustomOperator(const ASR::CustomOperator_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
    }

    void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
    }

    void visit_StructType(const ASR::StructType_t &x) {
        if (x.m_abi != ASR::abiType::BindC) {
            if (all_symbols_mangling || should_mangle) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_EnumType(const ASR::EnumType_t &x) {
        if (x.m_abi != ASR::abiType::BindC) {
            if (all_symbols_mangling || should_mangle) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_UnionType(const ASR::UnionType_t &x) {
        if (x.m_abi != ASR::abiType::BindC) {
            if (all_symbols_mangling || should_mangle) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Variable(const ASR::Variable_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
    }

    void visit_ClassType(const ASR::ClassType_t &x) {
        if (x.m_abi != ASR::abiType::BindC) {
            if (all_symbols_mangling || should_mangle) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_ClassProcedure(const ASR::ClassProcedure_t &x) {
        if (x.m_abi != ASR::abiType::BindC) {
            if (all_symbols_mangling || should_mangle) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
    }

    void visit_AssociateBlock(const ASR::AssociateBlock_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Block(const ASR::Block_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Requirement(const ASR::Requirement_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Template(const ASR::Template_t &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

};


class UniqueSymbolVisitor: public ASR::BaseWalkVisitor<UniqueSymbolVisitor> {
    private:

    Allocator& al;

    public:
    std::map<ASR::symbol_t*, std::string> sym_to_new_name;
    std::map<std::string, ASR::symbol_t*> current_scope;

    UniqueSymbolVisitor(Allocator& al_,
    std::map<ASR::symbol_t*, std::string> sn) : al(al_), sym_to_new_name(sn){}


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        ASR::TranslationUnit_t& xx = const_cast<ASR::TranslationUnit_t&>(x);
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        current_scope = x.m_global_scope->get_scope();
        for (auto &a : xx.m_global_scope->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_global_scope->erase_symbol(a.first);
                xx.m_global_scope->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Program(const ASR::Program_t &x) {
        ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        for (size_t i=0; i<xx.n_dependencies; i++) {
            if (current_scope.find(xx.m_dependencies[i]) != current_scope.end()) {
                sym = current_scope[xx.m_dependencies[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_dependencies[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
                visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Module(const ASR::Module_t &x) {
        ASR::Module_t& xx = const_cast<ASR::Module_t&>(x);
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        for (size_t i=0; i<xx.n_dependencies; i++) {
            if (current_scope.find(xx.m_dependencies[i]) != current_scope.end()) {
                sym = current_scope[xx.m_dependencies[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_dependencies[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
                visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        for (size_t i=0; i<xx.n_dependencies; i++) {
            if (current_scope.find(xx.m_dependencies[i]) != current_scope.end()) {
                sym = current_scope[xx.m_dependencies[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_dependencies[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
                visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_GenericProcedure(const ASR::GenericProcedure_t &x) {
        ASR::GenericProcedure_t& xx = const_cast<ASR::GenericProcedure_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
    }

    void visit_CustomOperator(const ASR::CustomOperator_t &x) {
        ASR::CustomOperator_t& xx = const_cast<ASR::CustomOperator_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
    }

    void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
        ASR::ExternalSymbol_t& xx = const_cast<ASR::ExternalSymbol_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        SymbolTable* s = ASRUtils::symbol_parent_symtab(x.m_external);
        ASR::symbol_t *asr_owner = ASR::down_cast<ASR::symbol_t>(s->asr_owner);
        if (sym_to_new_name.find(x.m_external) != sym_to_new_name.end()) {
            xx.m_original_name = s2c(al, sym_to_new_name[x.m_external]);
        }
        if (sym_to_new_name.find(asr_owner) != sym_to_new_name.end()) {
            xx.m_module_name = s2c(al, sym_to_new_name[asr_owner]);
        }
    }

    void visit_StructType(const ASR::StructType_t &x) {
        ASR::StructType_t& xx = const_cast<ASR::StructType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            if (current_scope.find(xx.m_dependencies[i]) != current_scope.end()) {
                sym = current_scope[xx.m_dependencies[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_dependencies[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        current_scope = x.m_symtab->get_scope();
        for (size_t i=0; i<xx.n_members; i++) {
            if (current_scope.find(xx.m_members[i]) != current_scope.end()) {
                sym = current_scope[xx.m_members[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_members[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_EnumType(const ASR::EnumType_t &x) {
        ASR::EnumType_t& xx = const_cast<ASR::EnumType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            if (current_scope.find(xx.m_dependencies[i]) != current_scope.end()) {
                sym = current_scope[xx.m_dependencies[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_dependencies[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        current_scope = x.m_symtab->get_scope();
        for (size_t i=0; i<xx.n_members; i++) {
            if (current_scope.find(xx.m_members[i]) != current_scope.end()) {
                sym = current_scope[xx.m_members[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_members[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_UnionType(const ASR::UnionType_t &x) {
        ASR::UnionType_t& xx = const_cast<ASR::UnionType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        for (size_t i=0; i<xx.n_dependencies; i++) {
            if (current_scope.find(xx.m_dependencies[i]) != current_scope.end()) {
                sym = current_scope[xx.m_dependencies[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_dependencies[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        current_scope = x.m_symtab->get_scope();
        for (size_t i=0; i<xx.n_members; i++) {
            if (current_scope.find(xx.m_members[i]) != current_scope.end()) {
                sym = current_scope[xx.m_members[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_members[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Variable(const ASR::Variable_t &x) {
        ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        for (size_t i=0; i<xx.n_dependencies; i++) {
            if (current_scope.find(xx.m_dependencies[i]) != current_scope.end()) {
                sym = current_scope[xx.m_dependencies[i]];
                if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
                    xx.m_dependencies[i] = s2c(al, sym_to_new_name[sym]);
                }
            }
        }
    }

    void visit_ClassType(const ASR::ClassType_t &x) {
        ASR::ClassType_t& xx = const_cast<ASR::ClassType_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_ClassProcedure(const ASR::ClassProcedure_t &x) {
        ASR::ClassProcedure_t& xx = const_cast<ASR::ClassProcedure_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
    }

    void visit_AssociateBlock(const ASR::AssociateBlock_t &x) {
        ASR::AssociateBlock_t& xx = const_cast<ASR::AssociateBlock_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Block(const ASR::Block_t &x) {
        ASR::Block_t& xx = const_cast<ASR::Block_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Requirement(const ASR::Requirement_t &x) {
        ASR::Requirement_t& xx = const_cast<ASR::Requirement_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Template(const ASR::Template_t &x) {
        ASR::Template_t& xx = const_cast<ASR::Template_t&>(x);
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (sym_to_new_name.find(sym) != sym_to_new_name.end()) {
            xx.m_name = s2c(al, sym_to_new_name[sym]);
        }
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        current_scope = x.m_symtab->get_scope();
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        for (auto &a: current_scope) {
            if (sym_to_new_name.find(a.second) != sym_to_new_name.end()) {
                xx.m_symtab->erase_symbol(a.first);
                xx.m_symtab->add_symbol(sym_to_new_name[a.second], a.second);
            }
        }
        current_scope = current_scope_copy;
    }

};


void pass_unique_symbols(Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    SymbolRenameVisitor v(true, true, true, true);
    v.visit_TranslationUnit(unit);
    UniqueSymbolVisitor u(al, v.sym_to_renamed);
    u.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LCompilers
