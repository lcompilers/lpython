#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/asr_builder.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_function_call_in_declaration.h>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces function calls in declarations with a new function call.
The function `pass_replace_function_call_in_declaration` transforms the ASR tree inplace.

Converts:

pure function diag_rsp_mat(A) result(res)
real, intent(in) :: A(:,:)
real :: res(minval(shape(A)))

res = 123.71_4
end function diag_rsp_mat

To:

pure integer function __lcompilers_created_helper_function_(A) result(r)
real, intent(in) :: A(:,:)
r = minval(shape(A))
end function __lcompilers_created_helper_function_

pure function diag_rsp_mat(A) result(res)
real, intent(in) :: A(:,:)
real :: res(__lcompilers_created_helper_function_(A))

res = 123.71_4
end function diag_rsp_mat

*/

class ReplaceFunctionCall : public ASR::BaseExprReplacer<ReplaceFunctionCall>
{
public:
    Allocator& al;
    SymbolTable* current_scope = nullptr;
    ASR::expr_t* assignment_value = nullptr;
    ASR::expr_t* call_for_return_var = nullptr;
    ASR::Function_t* current_function = nullptr;
    Vec<ASR::expr_t*>* newargsp = nullptr;

    struct ArgInfo {
        int arg_number;
        ASR::ttype_t* arg_type;
        ASR::expr_t* arg_expr;
        ASR::expr_t* arg_param;
    };

    ReplaceFunctionCall(Allocator &al_) : al(al_) {}

    void replace_FunctionParam(ASR::FunctionParam_t* x) {
        if( newargsp == nullptr ) {
            return ;
        }
        *current_expr = newargsp->p[x->m_param_number];
    }

    void replace_FunctionParam_with_FunctionArgs(ASR::expr_t*& value, Vec<ASR::expr_t*>& new_args) {
        if( !value ) {
            return ;
        }
        newargsp = &new_args;
        ASR::expr_t** current_expr_copy = current_expr;
        current_expr = &value;
        replace_expr(value);
        current_expr = current_expr_copy;
        newargsp = nullptr;
    }

    bool exists_in_arginfo(int arg_number, std::vector<ArgInfo>& indicies) {
        for (auto info: indicies) {
            if (info.arg_number == arg_number) return true;
        }
        return false;
    }

    void helper_get_arg_indices_used(ASR::expr_t* arg, std::vector<ArgInfo>& indicies) {
        if (is_a<ASR::ArrayPhysicalCast_t>(*arg)) {
            ASR::ArrayPhysicalCast_t* cast = ASR::down_cast<ASR::ArrayPhysicalCast_t>(arg);
            arg = cast->m_arg;
        }
        if (is_a<ASR::FunctionCall_t>(*arg)) {
            get_arg_indices_used_functioncall(ASR::down_cast<ASR::FunctionCall_t>(arg), indicies);
        } else if (is_a<ASR::IntrinsicArrayFunction_t>(*arg)) {
            get_arg_indices_used(ASR::down_cast<ASR::IntrinsicArrayFunction_t>(arg), indicies);
        } else if (is_a<ASR::FunctionParam_t>(*arg)) {
            ASR::FunctionParam_t* param = ASR::down_cast<ASR::FunctionParam_t>(arg);
            ArgInfo info = {static_cast<int>(param->m_param_number), param->m_type, current_function->m_args[param->m_param_number], arg};
            if (!exists_in_arginfo(param->m_param_number, indicies)) {
                indicies.push_back(info);
            }
        } else if (is_a<ASR::ArraySize_t>(*arg)) {
            ASR::ArraySize_t* size = ASR::down_cast<ASR::ArraySize_t>(arg);
            helper_get_arg_indices_used(size->m_v, indicies);
        } else if (is_a<ASR::IntegerCompare_t>(*arg)) {
            ASR::IntegerCompare_t* comp = ASR::down_cast<ASR::IntegerCompare_t>(arg);
            helper_get_arg_indices_used(comp->m_left, indicies);
            helper_get_arg_indices_used(comp->m_right, indicies);
        }
    }

    void get_arg_indices_used_functioncall(ASR::FunctionCall_t* x, std::vector<ArgInfo>& indicies) {
        for (size_t i = 0; i < x->n_args; i++) {
            ASR::expr_t* arg = x->m_args[i].m_value;
            helper_get_arg_indices_used(arg, indicies);
        }
        return;
    }

    template<typename T>
    void get_arg_indices_used(T* x, std::vector<ArgInfo>& indicies) {
        for (size_t i = 0; i < x->n_args; i++) {
            ASR::expr_t* arg = x->m_args[i];
            helper_get_arg_indices_used(arg, indicies);
        }
        return;
    }

    void replace_IntrinsicArrayFunction(ASR::IntrinsicArrayFunction_t *x) {
        if (!current_scope || !current_function || !assignment_value) return;

        if( newargsp != nullptr ) {
            BaseExprReplacer<ReplaceFunctionCall>::replace_IntrinsicArrayFunction(x);
            return ;
        }

        std::vector<ArgInfo> indicies;
        get_arg_indices_used(x, indicies);

        SymbolTable* global_scope = current_scope;
        while (global_scope->parent) {
            global_scope = global_scope->parent;
        }
        SetChar current_function_dependencies; current_function_dependencies.clear(al);
        SymbolTable* new_scope = al.make_new<SymbolTable>(global_scope);

        ASRUtils::SymbolDuplicator sd(al);
        ASRUtils::ASRBuilder b(al, x->base.base.loc);
        Vec<ASR::expr_t*> new_args; new_args.reserve(al, indicies.size());
        Vec<ASR::call_arg_t> new_call_args; new_call_args.reserve(al, indicies.size());
        Vec<ASR::call_arg_t> args_for_return_var; args_for_return_var.reserve(al, indicies.size());

        Vec<ASR::stmt_t*> new_body; new_body.reserve(al, 1);
        std::string new_function_name = global_scope->get_unique_name("__lcompilers_created_helper_function_", false);
        ASR::ttype_t* integer_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, 4));
        ASR::expr_t* return_var = b.Variable(new_scope, new_scope->get_unique_name("__lcompilers_return_var_", false), integer_type, ASR::intentType::ReturnVar);

        for (auto arg: indicies) {
            ASR::expr_t* arg_expr = arg.arg_expr;
            if (is_a<ASR::Var_t>(*arg_expr)) {
                ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(arg_expr);
                sd.duplicate_symbol(var->m_v, new_scope);
                ASR::expr_t* new_var_expr = ASRUtils::EXPR(ASR::make_Var_t(al, var->base.base.loc, new_scope->get_symbol(ASRUtils::symbol_name(var->m_v))));
                new_args.push_back(al, new_var_expr);
            }
            ASR::call_arg_t new_call_arg; new_call_arg.loc = arg_expr->base.loc; new_call_arg.m_value = arg.arg_param;
            new_call_args.push_back(al, new_call_arg);

            ASR::call_arg_t arg_for_return_var; arg_for_return_var.loc = arg_expr->base.loc; arg_for_return_var.m_value = arg.arg_expr;
            args_for_return_var.push_back(al, arg_for_return_var);
        }
        replace_FunctionParam_with_FunctionArgs(assignment_value, new_args);
        new_body.push_back(al, b.Assignment(return_var, assignment_value));
        ASR::asr_t* new_function = ASRUtils::make_Function_t_util(al, current_function->base.base.loc,
                    new_scope, s2c(al, new_function_name), current_function_dependencies.p, current_function_dependencies.n,
                    new_args.p, new_args.n,
                    new_body.p, new_body.n,
                    return_var,
                    ASR::abiType::Source, ASR::accessType::Public, ASR::deftypeType::Implementation,
                    nullptr, false, false, false, false, false, nullptr, 0,
                    false, false, false);

        ASR::symbol_t* new_function_sym = ASR::down_cast<ASR::symbol_t>(new_function);
        global_scope->add_or_overwrite_symbol(new_function_name, new_function_sym);

        ASR::expr_t* new_function_call = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x->base.base.loc,
                        new_function_sym,
                        new_function_sym,
                        new_call_args.p, new_call_args.n,
                        integer_type,
                        nullptr,
                        nullptr));
        *current_expr = new_function_call;

        ASR::expr_t* function_call_for_return_var = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, x->base.base.loc,
                        new_function_sym,
                        new_function_sym,
                        args_for_return_var.p, args_for_return_var.n,
                        integer_type,
                        nullptr,
                        nullptr));
        call_for_return_var = function_call_for_return_var;
    }

};

class FunctionTypeVisitor : public ASR::CallReplacerOnExpressionsVisitor<FunctionTypeVisitor>
{
public:

    Allocator &al;
    ReplaceFunctionCall replacer;
    SymbolTable* current_scope;
    Vec<ASR::stmt_t*> pass_result;


    FunctionTypeVisitor(Allocator &al_) : al(al_), replacer(al_) {
        current_scope = nullptr;
        pass_result.reserve(al, 1);
    }

    void call_replacer_(ASR::expr_t* value) {
        replacer.current_expr = current_expr;
        replacer.current_scope = current_scope;
        replacer.assignment_value = value;
        ASR::asr_t* asr_owner = current_scope->asr_owner;
        if (asr_owner) {
            ASR::Function_t* func = ASR::down_cast2<ASR::Function_t>(asr_owner);
            replacer.current_function = func;
        }
        replacer.replace_expr(*current_expr);
        replacer.current_scope = nullptr;
        replacer.current_function = nullptr;
        replacer.assignment_value = nullptr;
    }

    bool is_function_call_or_intrinsic_array_function(ASR::expr_t* expr) {
        if (!expr) return false;
        if (is_a<ASR::FunctionCall_t>(*expr)) {
            return true;
        } else if (is_a<ASR::IntrinsicArrayFunction_t>(*expr)) {
            return true;
        }
        return false;
    }

    void set_type_of_result_var(const ASR::FunctionType_t &x, ASR::Function_t* func) {
        if( !ASR::is_a<ASR::Array_t>(*x.m_return_var_type) ) {
            return ;
        }
        ASR::ttype_t* return_type_copy = ASRUtils::duplicate_type(al, x.m_return_var_type);
        ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(return_type_copy);
        Vec<ASR::expr_t*> new_args; new_args.reserve(al, func->n_args);
        for (size_t j = 0; j < func->n_args; j++) {
            new_args.push_back(al, func->m_args[j]);
        }
        for( size_t i = 0; i < array_t->n_dims; i++ ) {
            replacer.replace_FunctionParam_with_FunctionArgs(array_t->m_dims[i].m_start, new_args);
            replacer.replace_FunctionParam_with_FunctionArgs(array_t->m_dims[i].m_length, new_args);
        }
        ASRUtils::EXPR2VAR(func->m_return_var)->m_type = return_type_copy;
    }

    void visit_FunctionType(const ASR::FunctionType_t &x) {
        if (!current_scope) return;

        ASR::ttype_t* return_var_type = x.m_return_var_type;

        if (return_var_type && ASRUtils::is_array(return_var_type)) {
            ASR::Function_t* func = nullptr;
            ASR::asr_t* asr_owner = current_scope->asr_owner;
            if (ASR::is_a<ASR::symbol_t>(*asr_owner)) {
                ASR::symbol_t* sym = ASR::down_cast<ASR::symbol_t>(asr_owner);
                if (ASR::is_a<ASR::Function_t>(*sym)) {
                    func = ASR::down_cast2<ASR::Function_t>(current_scope->asr_owner);
                }
            }
            if (!func) return;
            ASR::Array_t* arr = ASR::down_cast<ASR::Array_t>(ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(return_var_type)));
            for (size_t i = 0; i < arr->n_dims; i++) {
                ASR::dimension_t dim = arr->m_dims[i];
                ASR::expr_t* start = dim.m_start;
                ASR::expr_t* end = dim.m_length;
                if (start && is_a<ASR::IntegerBinOp_t>(*start)) {
                    ASR::IntegerBinOp_t* binop = ASR::down_cast<ASR::IntegerBinOp_t>(start);
                    if (is_function_call_or_intrinsic_array_function(binop->m_left)) {
                        ASR::expr_t** current_expr_copy = current_expr;
                        current_expr = const_cast<ASR::expr_t**>(&(binop->m_left));
                        this->call_replacer_(binop->m_left);
                        current_expr = current_expr_copy;
                    }
                    if (is_function_call_or_intrinsic_array_function(binop->m_right)) {
                        ASR::expr_t** current_expr_copy = current_expr;
                        current_expr = const_cast<ASR::expr_t**>(&(binop->m_right));
                        this->call_replacer_(binop->m_right);
                        current_expr = current_expr_copy;
                    }

                }
                if (end && is_a<ASR::IntegerBinOp_t>(*end)) {
                    ASR::IntegerBinOp_t* binop = ASR::down_cast<ASR::IntegerBinOp_t>(end);
                    if (is_function_call_or_intrinsic_array_function(binop->m_left)) {
                        ASR::expr_t** current_expr_copy = current_expr;
                        current_expr = const_cast<ASR::expr_t**>(&(binop->m_left));
                        this->call_replacer_(binop->m_left);
                        current_expr = current_expr_copy;
                    }
                    if (is_function_call_or_intrinsic_array_function(binop->m_right)) {
                        ASR::expr_t** current_expr_copy = current_expr;
                        current_expr = const_cast<ASR::expr_t**>(&(binop->m_right));
                        this->call_replacer_(binop->m_right);
                        current_expr = current_expr_copy;
                    }

                }
                if (is_function_call_or_intrinsic_array_function(start)) {
                    ASR::expr_t** current_expr_copy = current_expr;
                    current_expr = const_cast<ASR::expr_t**>(&(ASR::down_cast<ASR::Array_t>(x.m_return_var_type)->m_dims[i].m_start));
                    this->call_replacer_(start);
                    current_expr = current_expr_copy;
                }
                if (is_function_call_or_intrinsic_array_function(end)) {
                    ASR::expr_t** current_expr_copy = current_expr;
                    current_expr = const_cast<ASR::expr_t**>(&(ASR::down_cast<ASR::Array_t>(x.m_return_var_type)->m_dims[i].m_length));
                    this->call_replacer_(end);
                    current_expr = current_expr_copy;
                }
            }

            set_type_of_result_var(x, func);
        }
    }

    void visit_Function(const ASR::Function_t &x) {
        current_scope = x.m_symtab;
        this->visit_ttype(*x.m_function_signature);
        current_scope = nullptr;
    }

};

void pass_replace_function_call_in_declaration(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/) {
    FunctionTypeVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LCompilers
