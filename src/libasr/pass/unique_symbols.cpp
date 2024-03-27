#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/unique_symbols.h>
#include <libasr/pass/pass_utils.h>
#include <unordered_map>
#include <set>
#include<unordered_set>


extern std::string lcompilers_unique_ID;

/*
ASR pass for replacing symbol names with some new name, mostly because
we want to generate unique symbols for each generated ASR output
so that it doesn't give linking errors for the generated backend code like C.

This is done using two classes:
1. SymbolRenameVisitor - This captures symbols to be renamed based on the options
   provided:
   1.1: module_name_mangling - Mangles the module name.
   1.2: global_symbols_mangling - Mangles all the global symbols.
   1.3: intrinsic_symbols_mangling - Mangles all the intrinsic symbols.
   1.4: all_symbols_mangling - Mangles all possible symbols.

   Note: this skips BindC functions and symbols starting with `_lpython` or `_lfortran`

2. UniqueSymbolVisitor: Renames all the captured symbols from SymbolRenameVisitor.
*/
namespace LCompilers {

using ASR::down_cast;

uint64_t static inline get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

class SymbolRenameVisitor: public ASR::BaseWalkVisitor<SymbolRenameVisitor> {
    public:
    std::unordered_map<ASR::symbol_t*, std::string> sym_to_renamed;
    bool module_name_mangling;
    bool global_symbols_mangling;
    bool intrinsic_symbols_mangling;
    bool all_symbols_mangling;
    bool bindc_mangling = false;
    bool fortran_mangling;
    bool c_mangling;
    bool should_mangle = false;
    std::vector<std::string> parent_function_name;
    std::string module_name = "";
    SymbolTable* current_scope = nullptr;

    SymbolRenameVisitor(bool mm, bool gm, bool im, bool am, bool bcm, bool fm, bool cm) :
    module_name_mangling(mm), global_symbols_mangling(gm), intrinsic_symbols_mangling(im),
    all_symbols_mangling(am), bindc_mangling(bcm), fortran_mangling(fm), c_mangling(cm) {}


    const std::unordered_set<std::string> reserved_keywords_c = {
         "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic",
         "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local", "auto",
         "break", "case", "char", "_Bool", "const", "continue", "default", "do",
         "double", "else", "enum", "extern", "float", "for", "goto", "if", "int",
         "long", "register", "return", "short", "signed", "sizeof", "static",
         "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
    };

    //TODO: Implement other backends mangling when refactoring the pass infrastructure
    void mangle_c(ASR::symbol_t* sym, const std::string& name){
        if (reserved_keywords_c.find(name) != reserved_keywords_c.end()) {
            sym_to_renamed[sym] = "_xx_"+std::string(name)+"_xx_";
        }
        return;
    }

    std::string update_name(std::string curr_name) {
        if (startswith(curr_name, "_lpython") || startswith(curr_name, "_lfortran") ) {
            return curr_name;
        } else if (startswith(curr_name, "_lcompilers_") && current_scope) {
            // mangle intrinsic functions
            uint64_t hash = get_hash(current_scope->asr_owner);
            return module_name + curr_name + "_" + std::to_string(hash) + "_" + lcompilers_unique_ID;
        } else if (parent_function_name.size() > 0) {
            // add parent function name to suffix
            std::string name = module_name + curr_name + "_";
            for (auto &a: parent_function_name) {
                name += a + "_";
            }
            return name + lcompilers_unique_ID;
        }
        return module_name + curr_name + "_" + lcompilers_unique_ID;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        ASR::TranslationUnit_t& xx = const_cast<ASR::TranslationUnit_t&>(x);
        current_scope = xx.m_symtab;
        std::unordered_map<ASR::symbol_t*, std::string> tmp_scope;
        for (auto &a : xx.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        SymbolTable *current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
    }

    void visit_Module(const ASR::Module_t &x) {
        SymbolTable *current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        bool should_mangle_copy = should_mangle;
        std::string mod_name_copy = module_name;
        module_name = std::string(x.m_name) + "_";
        if (all_symbols_mangling || module_name_mangling || should_mangle) {
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        if ((global_symbols_mangling && startswith(x.m_name, "_global_symbols"))) {
            should_mangle = true;
        }
        for (auto &a : x.m_symtab->get_scope()) {
            visit_symbol(*a.second);
        }
        should_mangle = should_mangle_copy;
        module_name = mod_name_copy;
        current_scope = current_scope_copy;
    }

    bool is_nested_function(ASR::symbol_t *sym) {
        if (ASR::is_a<ASR::Function_t>(*sym)) {
            ASR::Function_t* f = ASR::down_cast<ASR::Function_t>(sym);
            ASR::ttype_t* f_signature= f->m_function_signature;
            ASR::FunctionType_t *f_type = ASR::down_cast<ASR::FunctionType_t>(f_signature);
            if (f_type->m_abi == ASR::abiType::BindC && f_type->m_deftype == ASR::deftypeType::Interface) {
                // this is an interface function
                return false;
            }
            return true;
        } else {
            return false;
        }
    }

    void visit_Function(const ASR::Function_t &x) {
        SymbolTable *current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (bindc_mangling || f_type->m_abi != ASR::abiType::BindC) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            if (all_symbols_mangling || should_mangle) {
                sym_to_renamed[sym] = update_name(x.m_name);
            }
            if ( fortran_mangling ) {
                if ( sym_to_renamed.find(sym) != sym_to_renamed.end()
                        && startswith(sym_to_renamed[sym], "_") ) {
                    sym_to_renamed[sym] = current_scope->parent->get_unique_name(
                        "f" + sym_to_renamed[sym]);
                } else if ( startswith(x.m_name, "_") ) {
                    sym_to_renamed[sym] = current_scope->parent->get_unique_name(
                        "f" + std::string(x.m_name));
                }
            }
            if ( c_mangling ) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                mangle_c(sym , std::string(x.m_name));
            }
        }
        if (intrinsic_symbols_mangling && startswith(x.m_name, "_lcompilers_")) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        for (auto &a : x.m_symtab->get_scope()) {
            bool nested_function = is_nested_function(a.second);
            if (nested_function) {
                parent_function_name.push_back(x.m_name);
            }
            visit_symbol(*a.second);
            if (nested_function) {
                parent_function_name.pop_back();
            }
        }
        current_scope = current_scope_copy;
    }

    template <typename T>
    void visit_symbols_1(T &x) {
        ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
        if (all_symbols_mangling || should_mangle) {
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        if ( fortran_mangling ) {
            if ( sym_to_renamed.find(sym) != sym_to_renamed.end()
                    && startswith(sym_to_renamed[sym], "_") ) {
                sym_to_renamed[sym] = current_scope->get_unique_name("v" +
                    sym_to_renamed[sym]);
            } else if ( startswith(x.m_name, "_") ) {
                sym_to_renamed[sym] = current_scope->get_unique_name("v" +
                    std::string(x.m_name));
            }
        }
        if ( c_mangling ) {
            mangle_c(sym , std::string(x.m_name));
        }
    }

    void visit_GenericProcedure(const ASR::GenericProcedure_t &x) {
        visit_symbols_1(x);
    }

    void visit_CustomOperator(const ASR::CustomOperator_t &x) {
        visit_symbols_1(x);
    }

    void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
        visit_symbols_1(x);
    }

    void visit_Variable(const ASR::Variable_t &x) {
        visit_symbols_1(x);
    }

    template <typename T>
    void visit_symbols_2(T &x) {
        if (bindc_mangling || x.m_abi != ASR::abiType::BindC) {
            if (all_symbols_mangling || should_mangle) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
        if ( c_mangling ) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            mangle_c(sym , std::string(x.m_name));
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_StructType(const ASR::StructType_t &x) {
        visit_symbols_2(x);
    }

    void visit_EnumType(const ASR::EnumType_t &x) {
        visit_symbols_2(x);
    }

    void visit_UnionType(const ASR::UnionType_t &x) {
        visit_symbols_2(x);
    }

    void visit_ClassType(const ASR::ClassType_t &x) {
        visit_symbols_2(x);
    }

    void visit_ClassProcedure(const ASR::ClassProcedure_t &x) {
        if (bindc_mangling || x.m_abi != ASR::abiType::BindC) {
            if (all_symbols_mangling || should_mangle) {
                ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
                sym_to_renamed[sym] = update_name(x.m_name);
            }
        }
        if (c_mangling ) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            mangle_c(sym , std::string(x.m_name));
        }
    }

    template <typename T>
    void visit_symbols_3(T &x) {
        if (all_symbols_mangling || should_mangle) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            sym_to_renamed[sym] = update_name(x.m_name);
        }
        if ( c_mangling ) {
            ASR::symbol_t *sym = ASR::down_cast<ASR::symbol_t>((ASR::asr_t*)&x);
            mangle_c(sym , std::string(x.m_name));
        }
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_AssociateBlock(const ASR::AssociateBlock_t &x) {
        visit_symbols_3(x);
    }

    void visit_Block(const ASR::Block_t &x) {
        visit_symbols_3(x);
    }

    void visit_Requirement(const ASR::Requirement_t &x) {
        visit_symbols_3(x);
    }

    void visit_Template(const ASR::Template_t &x) {
        visit_symbols_3(x);
    }

};


class UniqueSymbolVisitor: public ASR::BaseWalkVisitor<UniqueSymbolVisitor> {
    private:

    Allocator& al;

    public:
    std::unordered_map<ASR::symbol_t*, std::string>& sym_to_new_name;
    std::map<std::string, ASR::symbol_t*> current_scope;

    UniqueSymbolVisitor(Allocator& al_,
    std::unordered_map<ASR::symbol_t*, std::string> &sn) : al(al_), sym_to_new_name(sn){}


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        ASR::TranslationUnit_t& xx = const_cast<ASR::TranslationUnit_t&>(x);
        std::map<std::string, ASR::symbol_t*> current_scope_copy = current_scope;
        current_scope = x.m_symtab->get_scope();
        for (auto &a : xx.m_symtab->get_scope()) {
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

    template <typename T>
    void update_symbols_1(const T &x) {
        T& xx = const_cast<T&>(x);
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

    void visit_Program(const ASR::Program_t &x) {
        update_symbols_1(x);
    }

    void visit_Module(const ASR::Module_t &x) {
        update_symbols_1(x);
    }

    void visit_Function(const ASR::Function_t &x) {
        update_symbols_1(x);
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

    template <typename T>
    void update_symbols_2(const T &x) {
        T& xx = const_cast<T&>(x);
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

    void visit_StructType(const ASR::StructType_t &x) {
        update_symbols_2(x);
    }

    void visit_EnumType(const ASR::EnumType_t &x) {
        update_symbols_2(x);
    }

    void visit_UnionType(const ASR::UnionType_t &x) {
        update_symbols_2(x);
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

    template <typename T>
    void update_symbols_3(const T &x) {
        T& xx = const_cast<T&>(x);
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

    void visit_AssociateBlock(const ASR::AssociateBlock_t &x) {
        update_symbols_3(x);
    }

    void visit_Block(const ASR::Block_t &x) {
        update_symbols_3(x);
    }

    void visit_Requirement(const ASR::Requirement_t &x) {
        update_symbols_3(x);
    }

    void visit_Template(const ASR::Template_t &x) {
       update_symbols_3(x);
    }

};


void pass_unique_symbols(Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& pass_options) {
    /*
     * This pass is applied iff the following options are passed; otherwise, return
     * MANGLING_OPTION="--all-mangling"
     * MANGLING_OPTION="--module-mangling"
     * MANGLING_OPTION="--global-mangling"
     * MANGLING_OPTION="--intrinsic-mangling"
     * COMPILER_SPECIFIC_OPTION="--generate-object-code" // LFortran
     * COMPILER_SPECIFIC_OPTION="--separate-compilation" // LPython
     * Usage:
     *    `$MANGLING_OPTION $COMPILER_SPECIFIC_OPTION`
     * The following are used by LFortran, Usage:
     *    `$MANGLING_OPTIONS --mangle-underscore [$COMPILER_SPECIFIC_OPTION]`
     *    * `--apply-fortran-mangling [$MANGLING_OPTION] [$COMPILER_SPECIFIC_OPTION]`
     */
    bool any_present = (pass_options.module_name_mangling || pass_options.global_symbols_mangling ||
                    pass_options.intrinsic_symbols_mangling || pass_options.all_symbols_mangling ||
                    pass_options.bindc_mangling || pass_options.fortran_mangling);
    if (pass_options.mangle_underscore) {
        lcompilers_unique_ID = "";
    }
    if ((!any_present || (!(pass_options.mangle_underscore ||
            pass_options.fortran_mangling) && lcompilers_unique_ID.empty())) &&
                !pass_options.c_mangling) {
        // `--mangle-underscore` doesn't require `lcompilers_unique_ID`
        // `lcompilers_unique_ID` is not mandatory for `--apply-fortran-mangling`
        return;
    }
    SymbolRenameVisitor v(pass_options.module_name_mangling,
                pass_options.global_symbols_mangling,
                pass_options.intrinsic_symbols_mangling,
                pass_options.all_symbols_mangling,
                pass_options.bindc_mangling,
                pass_options.fortran_mangling,
                pass_options.c_mangling);
    v.visit_TranslationUnit(unit);
    UniqueSymbolVisitor u(al, v.sym_to_renamed);
    u.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LCompilers
