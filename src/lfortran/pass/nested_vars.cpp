#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/nested_vars.h>
#include <llvm/IR/Verifier.h>

namespace LFortran {

using ASR::down_cast;

/*

This ASR pass is solely an analysis pass to determine if nested procedures
need access to variables from an outer scope. Such variables get hashed
and later compared to variable declarations while emitting IR. If we get a 
hash match, that variable is placed in a global struct where it can be accessed
from the inner scope

*/

uint64_t static get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

class NestedVarVisitor : public ASR::BaseWalkVisitor<NestedVarVisitor>
{
private:
    size_t nesting_depth = 0;
    SymbolTable* current_scope;
    llvm::LLVMContext &context;
    std::vector<uint64_t> &needed_globals;

public:

    NestedVarVisitor(llvm::LLVMContext &context, std::vector<uint64_t> 
        &needed_globals) : context{context}, 
        needed_globals{needed_globals} { };
    // Basically want to store a hash for each procedure and a hash for the
    // needed global types
    //std::map<uint64_t, std::vector<llvm::Type*>> nested_func_types;
    uint32_t cur_func_hash;
    uint32_t par_func_hash;
    std::vector<llvm::Type*> proc_types;
    std::map<uint64_t, std::vector<llvm::Type*>> nested_func_types;

    inline llvm::Type* getIntType(int a_kind, bool get_pointer=false) {
        llvm::Type* type_ptr = nullptr;
        if( get_pointer ) {
            switch(a_kind)
            {
                case 4:
                    type_ptr = llvm::Type::getInt32PtrTy(context);
                    break;
                case 8:
                    type_ptr = llvm::Type::getInt64PtrTy(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits integer kinds are supported.");
            }
        } else {
            switch(a_kind)
            {
                case 4:
                    type_ptr = llvm::Type::getInt32Ty(context);
                    break;
                case 8:
                    type_ptr = llvm::Type::getInt64Ty(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits integer kinds are supported.");
            }
        }
        return type_ptr;
    }

    inline llvm::Type* getFPType(int a_kind, bool get_pointer=false) {
        llvm::Type* type_ptr = nullptr;
        if( get_pointer ) {
            switch(a_kind)
            {
                case 4:
                    type_ptr = llvm::Type::getFloatPtrTy(context);
                    break;
                case 8:
                    type_ptr =  llvm::Type::getDoublePtrTy(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits real kinds are supported.");
            }
        } else {
            switch(a_kind)
            {
                case 4:
                    type_ptr = llvm::Type::getFloatTy(context);
                    break;
                case 8:
                    type_ptr = llvm::Type::getDoubleTy(context);
                    break;
                default:
                    throw CodeGenError("Only 32 and 64 bits real kinds are supported.");
            }
        }
        return type_ptr;
    }

    template<typename T>
    void visit_procedure(const T &x) {
        nesting_depth++;
        if (nesting_depth == 1) {
            for (auto &item : x.m_symtab->scope) {
                if (ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                    par_func_hash = cur_func_hash;
                    ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(
                            item.second);
                    visit_Subroutine(*s);
                }
                if (ASR::is_a<ASR::Function_t>(*item.second)) {
                    par_func_hash = cur_func_hash;
                    ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
                            item.second);
                    visit_Function(*s);
                }
            }
        } else {
            std::vector<llvm::Type*> proc_types_i;
            nested_func_types.insert({cur_func_hash, proc_types_i});
            current_scope = x.m_symtab;
            for (size_t i = 0; i < x.n_body; i++) {
                visit_stmt(*x.m_body[i]);
            }
        }
        nesting_depth--;
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        cur_func_hash = get_hash((ASR::asr_t*)&x);
        visit_procedure(x);
    }

    void visit_Function(const ASR::Function_t &x) {
        cur_func_hash = get_hash((ASR::asr_t*)&x);
        visit_procedure(x);
    }

    void visit_Var(const ASR::Var_t &x) {
        // Only attempt if we are actually in a nested function
        if (nesting_depth > 1) {
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(
                    symbol_get_past_external(x.m_v));
            // If the variable is not defined in the current scope, it is a
            // "needed global" since we need to be able to access it from the
            // nested procedure.
            if (current_scope->scope.find(v->m_name) ==
                        current_scope->scope.end()) {
                uint32_t h = get_hash((ASR::asr_t*)v);
                llvm::Type* rel_type;
                auto finder = std::find(needed_globals.begin(), 
                        needed_globals.end(), h);

                if (finder == needed_globals.end()) {
                    needed_globals.push_back(h);
                    switch(v->m_type->type){
                        case ASR::ttypeType::Integer: {
                            int a_kind = down_cast<ASR::Integer_t>(v->m_type)->
                                m_kind;
                            rel_type = getIntType(a_kind)->getPointerTo();
                            break;
                        }
                        case ASR::ttypeType::Real: {
                            int a_kind = down_cast<ASR::Real_t>(v->m_type)->
                                m_kind;
                            rel_type = getFPType(a_kind)->getPointerTo();
                            break;
                        }
                        default: {
                                throw CodeGenError("Variable type not \
                                        supported in nested functions");
                            break;
                        }
                    }
                    proc_types.push_back(rel_type);
                    nested_func_types[par_func_hash].push_back(rel_type);
                }
            }
        }
    }


};

std::map<uint64_t, std::vector<llvm::Type*>> pass_find_nested_vars(
        ASR::TranslationUnit_t &unit, llvm::LLVMContext &context,
        std::vector<uint64_t> &needed_globals) {
    NestedVarVisitor v(context, needed_globals);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
    return v.nested_func_types;
}


} // namespace LFortran
