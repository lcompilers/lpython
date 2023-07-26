#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_symbolic.h>
#include <libasr/pass/pass_utils.h>

#include <vector>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;


class ReplaceSymbolicVisitor : public PassUtils::PassVisitor<ReplaceSymbolicVisitor>
{
public:
    ReplaceSymbolicVisitor(Allocator &al_) :
    PassVisitor(al_, nullptr) {
        pass_result.reserve(al, 1);
    }

    bool symbolic_replaces_with_CPtr_Module = false;
    bool symbolic_replaces_with_CPtr_Function = false;

    void visit_Module(const ASR::Module_t &x) {
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
            if(symbolic_replaces_with_CPtr_Module){
                std::string new_name = current_scope->get_unique_name("basic_new_stack");
                if(current_scope->get_symbol(new_name)) return;
                std::string header = "symengine/cwrapper.h";
                SymbolTable *fn_symtab = al.make_new<SymbolTable>(current_scope);

                Vec<ASR::expr_t*> args;
                {
                    args.reserve(al, 1);
                    ASR::ttype_t *arg_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc));
                    ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, arg_type, nullptr,
                        ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, arg)));
                }
                Vec<ASR::stmt_t*> body;
                body.reserve(al, 1);

                Vec<char *> dep;
                dep.reserve(al, 1);

                ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x.base.base.loc,
                    fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                    nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                    ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                    false, false, nullptr, 0, false, false, false);
                ASR::symbol_t *new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                current_scope->add_symbol(new_name, new_symbol);
                symbolic_replaces_with_CPtr_Module = false;
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Function(const ASR::Function_t& x) {
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = xx.m_symtab;
        for (auto &item : current_scope->get_scope()) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                this->visit_symbol(*item.second);
                if(symbolic_replaces_with_CPtr_Function){
                    std::string var = std::string(item.first);
                    std::string placeholder = "_" + var;
                    ASR::symbol_t* var_sym = current_scope->get_symbol(var);
                    ASR::symbol_t* placeholder_sym = current_scope->get_symbol(placeholder);
                    ASR::expr_t* target1 = ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, placeholder_sym));
                    ASR::expr_t* target2 = ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, var_sym));

                    // statement 1
                    int cast_kind = ASR::cast_kindType::IntegerToInteger;
                    ASR::ttype_t *type1 = ASRUtils::TYPE(ASR::make_Integer_t(al, xx.base.base.loc, 4));
                    ASR::ttype_t *type2 = ASRUtils::TYPE(ASR::make_Integer_t(al, xx.base.base.loc, 8));
                    ASR::expr_t* cast_tar = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, xx.base.base.loc, 0, type1));
                    ASR::expr_t* cast_val = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, xx.base.base.loc, 0, type2));
                    ASR::expr_t* value1 = ASRUtils::EXPR(ASR::make_Cast_t(al, xx.base.base.loc, cast_tar, (ASR::cast_kindType)cast_kind, type2, cast_val));

                    // statement 2
                    ASR::ttype_t *type3 = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
                    ASR::expr_t* value2 = ASRUtils::EXPR(ASR::make_PointerNullConstant_t(al, xx.base.base.loc, type3));

                    // statement 3
                    ASR::ttype_t *type4 = ASRUtils::TYPE(ASR::make_Pointer_t(al, xx.base.base.loc, type2));
                    ASR::expr_t* get_pointer_node = ASRUtils::EXPR(ASR::make_GetPointer_t(al, xx.base.base.loc, target1, type4, nullptr));
                    ASR::expr_t* value3 = ASRUtils::EXPR(ASR::make_PointerToCPtr_t(al, xx.base.base.loc, get_pointer_node, type3, nullptr));

                    // statement 4
                    // ASR::symbol_t* basic_new_stack_sym = current_scope->parent->get_symbol("basic_new_stack");
                    // Vec<ASR::call_arg_t> call_args;
                    // call_args.reserve(al, 1);
                    // ASR::call_arg_t call_arg;
                    // call_arg.loc = xx.base.base.loc;
                    // call_arg.m_value = target2;
                    // call_args.push_back(al, call_arg);

                    // defining the assignment statement
                    ASR::stmt_t* stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target1, value1, nullptr));
                    ASR::stmt_t* stmt2 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value2, nullptr));
                    ASR::stmt_t* stmt3 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value3, nullptr));
                    //ASR::stmt_t* stmt4 = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, xx.base.base.loc, basic_new_stack_sym, nullptr, call_args.p, call_args.n, nullptr));

                    // push stmt1 into the updated body vector
                    pass_result.push_back(al, stmt1);
                    pass_result.push_back(al, stmt2);
                    pass_result.push_back(al, stmt3);
                    //pass_result.push_back(al, stmt4);

                    // updated x.m_body and x.n_bdoy with that of the updated vector
                    // x.m_body = updated_body.p;
                    // x.n_body = updated_body.size();
                    transform_stmts(xx.m_body, xx.n_body);
                    symbolic_replaces_with_CPtr_Function = false;
                }
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_Variable(const ASR::Variable_t& x) {
        if (x.m_type->type == ASR::ttypeType::SymbolicExpression) {
            symbolic_replaces_with_CPtr_Module = true;
            symbolic_replaces_with_CPtr_Function = true;
            std::string var_name = x.m_name;
            std::string placeholder = "_" + std::string(x.m_name);

            // defining CPtr variable
            ASR::ttype_t *type1 = ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc));
            ASR::symbol_t* sym1 = ASR::down_cast<ASR::symbol_t>(
                ASR::make_Variable_t(al, x.base.base.loc, current_scope,
                                    s2c(al, var_name), nullptr, 0,
                                    x.m_intent, nullptr,
                                    nullptr, x.m_storage,
                                    type1, nullptr, x.m_abi,
                                    x.m_access, x.m_presence,
                                    x.m_value_attr));

            ASR::ttype_t *type2 = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
            ASR::symbol_t* sym2 = ASR::down_cast<ASR::symbol_t>(
                ASR::make_Variable_t(al, x.base.base.loc, current_scope,
                                    s2c(al, placeholder), nullptr, 0,
                                    x.m_intent, nullptr,
                                    nullptr, x.m_storage,
                                    type2, nullptr, x.m_abi,
                                    x.m_access, x.m_presence,
                                    x.m_value_attr));

            current_scope->erase_symbol(s2c(al, var_name));
            current_scope->add_symbol(s2c(al, var_name), sym1);
            current_scope->add_symbol(s2c(al, placeholder), sym2);
        }
    }
};

void pass_replace_symbolic(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceSymbolicVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LCompilers
