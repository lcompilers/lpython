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

    bool symbolic_replaces_with_CPtr_Function = false;

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = this->current_scope;
        this->current_scope = xx.m_symtab;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *s = ASR::down_cast<ASR::Variable_t>(item.second);
                this->visit_Variable(*s);
            }
        }
        transform_stmts(xx.m_body, xx.n_body);

        // Add "basic_new_stack" to dependencies if needed
        if (symbolic_replaces_with_CPtr_Function) {
            SetChar function_dependencies;
            function_dependencies.n = 0;
            function_dependencies.reserve(al, 1);
            for( size_t i = 0; i < xx.n_dependencies; i++ ) {
                function_dependencies.push_back(al, xx.m_dependencies[i]);
            }
            function_dependencies.push_back(al, s2c(al, "basic_new_stack"));
            xx.n_dependencies = function_dependencies.size();
            xx.m_dependencies = function_dependencies.p;
        }
        this->current_scope = current_scope_copy;
    }

    void visit_Variable(const ASR::Variable_t& x) {
        ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = xx.m_parent_symtab;
        if (xx.m_type->type == ASR::ttypeType::SymbolicExpression) {
            SymbolTable* module_scope = current_scope->parent;
            symbolic_replaces_with_CPtr_Function = true;
            std::string var_name = xx.m_name;
            std::string placeholder = "_" + std::string(var_name);

            // defining CPtr variable
            ASR::ttype_t *type1 = ASRUtils::TYPE(ASR::make_CPtr_t(al, xx.base.base.loc));
            xx.m_type = type1;

            ASR::ttype_t *type2 = ASRUtils::TYPE(ASR::make_Integer_t(al, xx.base.base.loc, 8));
            ASR::symbol_t* sym2 = ASR::down_cast<ASR::symbol_t>(
                ASR::make_Variable_t(al, xx.base.base.loc, current_scope,
                                    s2c(al, placeholder), nullptr, 0,
                                    xx.m_intent, nullptr,
                                    nullptr, xx.m_storage,
                                    type2, nullptr, xx.m_abi,
                                    xx.m_access, xx.m_presence,
                                    xx.m_value_attr));

            current_scope->add_symbol(s2c(al, placeholder), sym2);

            std::string new_name = "basic_new_stack";
            if (!module_scope->get_symbol(new_name)) {
                std::string header = "symengine/cwrapper.h";
                SymbolTable *fn_symtab = al.make_new<SymbolTable>(module_scope);

                Vec<ASR::expr_t*> args;
                {
                    args.reserve(al, 1);
                    ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, xx.base.base.loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                        nullptr, nullptr, ASR::storage_typeType::Default, type1, nullptr,
                        ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
                    fn_symtab->add_symbol(s2c(al, "x"), arg);
                    args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, arg)));
                }

                Vec<ASR::stmt_t*> body;
                body.reserve(al, 1);

                Vec<char *> dep;
                dep.reserve(al, 1);

                ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, xx.base.base.loc,
                    fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
                    nullptr, ASR::abiType::BindC, ASR::accessType::Public,
                    ASR::deftypeType::Interface, s2c(al, new_name), false, false, false,
                    false, false, nullptr, 0, false, false, false, s2c(al, header));
                ASR::symbol_t *new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
                module_scope->add_symbol(new_name, new_symbol);
            }

            ASR::symbol_t* var_sym = current_scope->get_symbol(var_name);
            ASR::symbol_t* placeholder_sym = current_scope->get_symbol(placeholder);
            ASR::expr_t* target1 = ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, placeholder_sym));
            ASR::expr_t* target2 = ASRUtils::EXPR(ASR::make_Var_t(al, xx.base.base.loc, var_sym));

            // statement 1
            ASR::expr_t* value1 = ASRUtils::EXPR(ASR::make_Cast_t(al, xx.base.base.loc,
                ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, xx.base.base.loc, 0,
                ASRUtils::TYPE(ASR::make_Integer_t(al, xx.base.base.loc, 4)))),
                (ASR::cast_kindType)ASR::cast_kindType::IntegerToInteger, type2,
                ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, xx.base.base.loc, 0, type2))));

            // statement 2
            ASR::expr_t* value2 = ASRUtils::EXPR(ASR::make_PointerNullConstant_t(al, xx.base.base.loc, type1));

            // statement 3
            ASR::expr_t* get_pointer_node = ASRUtils::EXPR(ASR::make_GetPointer_t(al, xx.base.base.loc,
                target1, ASRUtils::TYPE(ASR::make_Pointer_t(al, xx.base.base.loc, type2)), nullptr));
            ASR::expr_t* value3 = ASRUtils::EXPR(ASR::make_PointerToCPtr_t(al, xx.base.base.loc, get_pointer_node,
                type1, nullptr));

            // statement 4
            ASR::symbol_t* basic_new_stack_sym = module_scope->get_symbol(new_name);
            Vec<ASR::call_arg_t> call_args;
            call_args.reserve(al, 1);
            ASR::call_arg_t call_arg;
            call_arg.loc = xx.base.base.loc;
            call_arg.m_value = target2;
            call_args.push_back(al, call_arg);

            // defining the assignment statement
            ASR::stmt_t* stmt1 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target1, value1, nullptr));
            ASR::stmt_t* stmt2 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value2, nullptr));
            ASR::stmt_t* stmt3 = ASRUtils::STMT(ASR::make_Assignment_t(al, xx.base.base.loc, target2, value3, nullptr));
            ASR::stmt_t* stmt4 = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, xx.base.base.loc, basic_new_stack_sym,
                basic_new_stack_sym, call_args.p, call_args.n, nullptr));

            pass_result.push_back(al, stmt1);
            pass_result.push_back(al, stmt2);
            pass_result.push_back(al, stmt3);
            pass_result.push_back(al, stmt4);
        }
        current_scope = current_scope_copy;
    }
};

void pass_replace_symbolic(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceSymbolicVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LCompilers