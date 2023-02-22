#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/array_op.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class ReplaceArrayOp: public ASR::BaseExprReplacer<ReplaceArrayOp> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;
    size_t result_counter;
    bool& use_custom_loop_params;
    Vec<ASR::expr_t*>& result_lbound;
    Vec<ASR::expr_t*>& result_ubound;
    Vec<ASR::expr_t*>& result_inc;

    public:

    SymbolTable* current_scope;
    ASR::expr_t* result_var;

    ReplaceArrayOp(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
                   bool& use_custom_loop_params_,
                   Vec<ASR::expr_t*>& result_lbound_,
                   Vec<ASR::expr_t*>& result_ubound_,
                   Vec<ASR::expr_t*>& result_inc_) :
    al(al_), pass_result(pass_result_),
    result_counter(0), use_custom_loop_params(use_custom_loop_params_),
    result_lbound(result_lbound_), result_ubound(result_ubound_),
    result_inc(result_inc_), current_scope(nullptr), result_var(nullptr) {}

    void replace_Var(ASR::Var_t* x) {
        if( !(result_var != nullptr && PassUtils::is_array(result_var)) ) {
            return ;
        }

        const Location& loc = x->base.base.loc;
        int rank_var = PassUtils::get_rank(*current_expr);
        int n_dims = rank_var;
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope);
        ASR::stmt_t* doloop = nullptr;
        for( int i = n_dims - 1; i >= 0; i-- ) {
            // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
            ASR::do_loop_head_t head;
            head.m_v = idx_vars[i];
            head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
            head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
            head.m_increment = nullptr;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                ASR::expr_t* ref = nullptr;
                if( rank_var > 0 ) {
                    ref = PassUtils::create_array_ref(*current_expr, idx_vars, al);
                } else {
                    ref = *current_expr;
                }
                ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, ref, nullptr));
                doloop_body.push_back(al, assign);
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, head, doloop_body.p, doloop_body.size()));
        }
        pass_result.push_back(al, doloop);
    }

    template <typename T>
    ASR::expr_t* generate_element_wise_operation(const Location& loc, ASR::expr_t* left, ASR::expr_t* right, T* x) {
        switch( x->class_type ) {
            case ASR::exprType::IntegerBinOp:
                return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                            al, loc, left, (ASR::binopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::RealBinOp:
                return ASRUtils::EXPR(ASR::make_RealBinOp_t(
                            al, loc, left, (ASR::binopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::ComplexBinOp:
                return ASRUtils::EXPR(ASR::make_ComplexBinOp_t(
                            al, loc, left, (ASR::binopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::LogicalBinOp:
                return ASRUtils::EXPR(ASR::make_LogicalBinOp_t(
                            al, loc, left, (ASR::logicalbinopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::IntegerCompare:
                return ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                            al, loc, left, (ASR::cmpopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::RealCompare:
                return ASRUtils::EXPR(ASR::make_RealCompare_t(
                            al, loc, left, (ASR::cmpopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::ComplexCompare:
                return ASRUtils::EXPR(ASR::make_ComplexCompare_t(
                            al, loc, left, (ASR::cmpopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::LogicalCompare:
                return ASRUtils::EXPR(ASR::make_LogicalCompare_t(
                            al, loc, left, (ASR::cmpopType)x->m_op,
                            right, x->m_type, nullptr));

            default:
                throw LCompilersException("The desired operation is not supported yet for arrays.");
        }
    }

    template <typename T>
    void replace_ArrayOpCommon(T* x, std::string res_prefix) {
        const Location& loc = x->base.base.loc;
        bool current_status = use_custom_loop_params;
        use_custom_loop_params = false;
        ASR::expr_t* result_var_copy = result_var;

        ASR::expr_t** current_expr_copy_35 = current_expr;
        current_expr = &(x->m_left);
        result_var = nullptr;
        this->replace_expr(x->m_left);
        ASR::expr_t* left = *current_expr;
        current_expr = current_expr_copy_35;

        ASR::expr_t** current_expr_copy_36 = current_expr;
        current_expr = &(x->m_right);
        result_var = nullptr;
        this->replace_expr(x->m_right);
        ASR::expr_t* right = *current_expr;
        current_expr = current_expr_copy_36;

        use_custom_loop_params = current_status;
        result_var = result_var_copy;

        // TODO: Replace with ASRUtils::extract_dimensions_from_ttype
        int rank_left = PassUtils::get_rank(left);
        int rank_right = PassUtils::get_rank(right);
        if( rank_left == 0 && rank_right == 0 ) {
            return ;
        }

        if( rank_left > 0 && rank_right > 0 ) {

            if( rank_left != rank_right ) {
                // TODO: This should be checked by verify() and thus should not happen
                throw LCompilersException("Cannot generate loop for operands "
                                          "of different shapes");
            }

            if( result_var == nullptr ) {
                result_var = PassUtils::create_var(result_counter, res_prefix, loc,
                                                   left, al, current_scope);
                result_counter += 1;
            }
            *current_expr = result_var;

            int n_dims = rank_left;
            Vec<ASR::expr_t*> idx_vars, idx_vars_value;
            PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope, "_t");
            // TODO: This only works when slicing is done continuously and then no
            // slicing happens. For example, v(:, :, k, i) is continuously sliced
            // but v(:, k, :, i) is not. So we need to use the already overloaded version
            // of PassUtils::create_idx_vars and fill idx_vars correctly.
            if( use_custom_loop_params ) {
                for( size_t k = idx_vars.size(); k < result_lbound.size(); k++ ) {
                    idx_vars.push_back(al, result_lbound.p[k]);
                }
            }
            PassUtils::create_idx_vars(idx_vars_value, n_dims, loc, al, current_scope, "_v");
            ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, int32_type));
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                if( use_custom_loop_params ) {
                    head.m_start = result_lbound[i];
                    head.m_end = result_ubound[i];
                    head.m_increment = result_inc[i];
                } else {
                    head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
                    head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
                    head.m_increment = nullptr;
                }
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref_1 = PassUtils::create_array_ref(left, idx_vars_value, al);
                    ASR::expr_t* ref_2 = PassUtils::create_array_ref(right, idx_vars_value, al);
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::expr_t* op_el_wise = generate_element_wise_operation(loc, ref_1, ref_2, x);
                    ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    ASR::expr_t* idx_lb = PassUtils::get_bound(left, i + 1, "lbound", al);
                    ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, idx_vars_value[i+1], idx_lb, nullptr));
                    doloop_body.push_back(al, set_to_one);
                    doloop_body.push_back(al, doloop);
                }
                ASR::expr_t* inc_expr = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, idx_vars_value[i],
                                                                ASR::binopType::Add, const_1, int32_type, nullptr));
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, idx_vars_value[i], inc_expr, nullptr));
                doloop_body.push_back(al, assign_stmt);
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, head, doloop_body.p, doloop_body.size()));
            }
            use_custom_loop_params = false;
            ASR::expr_t* idx_lb = PassUtils::get_bound(right, 1, "lbound", al);
            ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, idx_vars_value[0], idx_lb, nullptr));
            pass_result.push_back(al, set_to_one);
            pass_result.push_back(al, doloop);
        } else if( (rank_left == 0 && rank_right > 0) ||
                   (rank_right == 0 && rank_left > 0) ) {
            ASR::expr_t *arr_expr = nullptr, *other_expr = nullptr;
            int n_dims = 0;
            if( rank_left > 0 ) {
                arr_expr = left;
                other_expr = right;
                n_dims = rank_left;
            } else {
                arr_expr = right;
                other_expr = left;
                n_dims = rank_right;
            }
            if( result_var == nullptr ) {
                result_var = PassUtils::create_var(result_counter, res_prefix, loc,
                                                    arr_expr, al, current_scope);
                result_counter += 1;
            }
            *current_expr = result_var;

            Vec<ASR::expr_t*> idx_vars, idx_vars_value;
            PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope, "_t");
            // TODO: This only works when slicing is done continuously and then no
            // slicing happens. For example, v(:, :, k, i) is continuously sliced
            // but v(:, k, :, i) is not. So we need to use the already overloaded version
            // of PassUtils::create_idx_vars and fill idx_vars correctly.
            if( use_custom_loop_params ) {
                for( size_t k = idx_vars.size(); k < result_lbound.size(); k++ ) {
                    idx_vars.push_back(al, result_lbound.p[k]);
                }
            }
            PassUtils::create_idx_vars(idx_vars_value, n_dims, loc, al, current_scope, "_v");
            ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, int32_type));
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                if( use_custom_loop_params ) {
                    head.m_start = result_lbound[i];
                    head.m_end = result_ubound[i];
                    head.m_increment = result_inc[i];
                } else {
                    head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
                    head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
                    head.m_increment = nullptr;
                }
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(arr_expr, idx_vars_value, al);
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::expr_t *lexpr = nullptr, *rexpr = nullptr;
                    if( rank_left > 0 ) {
                        lexpr = ref;
                        rexpr = other_expr;
                    } else {
                        rexpr = ref;
                        lexpr = other_expr;
                    }
                    ASR::expr_t* op_el_wise = generate_element_wise_operation(loc, lexpr, rexpr, x);
                    ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    ASR::expr_t* op_expr = nullptr;
                    if( rank_left > 0 ) {
                        op_expr = left;
                    } else {
                        op_expr = right;
                    }
                    ASR::expr_t* idx_lb = PassUtils::get_bound(op_expr, i + 2, "lbound", al);
                    ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                                                idx_vars_value[i + 1], idx_lb, nullptr));
                    doloop_body.push_back(al, set_to_one);
                    doloop_body.push_back(al, doloop);
                }
                ASR::expr_t* inc_expr = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, idx_vars_value[i],
                                                                ASR::binopType::Add, const_1, int32_type, nullptr));
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, idx_vars_value[i], inc_expr, nullptr));
                doloop_body.push_back(al, assign_stmt);
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, head, doloop_body.p, doloop_body.size()));
            }
            use_custom_loop_params = false;
            ASR::expr_t* op_expr = nullptr;
            if( rank_left > 0 ) {
                op_expr = left;
            } else {
                op_expr = right;
            }
            ASR::expr_t* idx_lb = PassUtils::get_bound(op_expr, 1, "lbound", al);
            ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                                        idx_vars_value[0], idx_lb, nullptr));
            pass_result.push_back(al, set_to_one);
            pass_result.push_back(al, doloop);
        }
        result_var = nullptr;
    }

    void replace_Cast(ASR::Cast_t* x) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        BaseExprReplacer::replace_Cast(x);
        result_var = result_var_copy;
        ASR::expr_t* tmp_val = x->m_arg;

        if( !PassUtils::is_array(tmp_val) ) {
            return ;
        }

        const Location& loc = x->base.base.loc;
        if( result_var == nullptr ) {
            PassUtils::fix_dimension(x, tmp_val);
            result_var = PassUtils::create_var(result_counter, std::string("_implicit_cast_res"),
                                               loc, *current_expr, al, current_scope);
            result_counter += 1;
        }

        int n_dims = PassUtils::get_rank(result_var);
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope);
        ASR::stmt_t* doloop = nullptr;
        for( int i = n_dims - 1; i >= 0; i-- ) {
            ASR::do_loop_head_t head;
            head.m_v = idx_vars[i];
            head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
            head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
            head.m_increment = nullptr;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                ASR::expr_t* ref = PassUtils::create_array_ref(tmp_val, idx_vars, al);
                ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                ASR::expr_t* impl_cast_el_wise = ASRUtils::EXPR(ASR::make_Cast_t(al, loc, ref, x->m_kind, x->m_type, nullptr));
                ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, impl_cast_el_wise, nullptr));
                doloop_body.push_back(al, assign);
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, head, doloop_body.p, doloop_body.size()));
        }
        pass_result.push_back(al, doloop);
        *current_expr = result_var;
    }

    template <typename T>
    void replace_UnaryOp(T* x, int unary_type, std::string res_prefix) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;

        ASR::expr_t** current_expr_copy_22 = current_expr;
        current_expr = &(x->m_arg);
        self().replace_expr(x->m_arg);
        current_expr = current_expr_copy_22;

        result_var = nullptr;
        ASR::expr_t** current_expr_copy_23 = current_expr;
        current_expr = &(x->m_value);
        self().replace_expr(x->m_value);
        current_expr = current_expr_copy_23;

        result_var = result_var_copy;

        ASR::expr_t* operand = x->m_arg;
        int rank_operand = PassUtils::get_rank(operand);
        if( rank_operand == 0 ) {
            return ;
        }

        const Location& loc = x->base.base.loc;
        if( rank_operand > 0 ) {
            if( result_var == nullptr ) {
                result_var = PassUtils::create_var(result_counter, res_prefix,
                                loc, operand, al, current_scope);
                result_counter += 1;
            }
            *current_expr = result_var;

            int n_dims = rank_operand;
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
                head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(operand, idx_vars, al);
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::expr_t* op_el_wise = nullptr;
                    if (unary_type == 0) {
                        op_el_wise = ASRUtils::EXPR(ASR::make_IntegerUnaryMinus_t(al, loc,
                            ref, x->m_type, nullptr));
                    } else if (unary_type == 1) {
                        op_el_wise = ASRUtils::EXPR(ASR::make_RealUnaryMinus_t(al, loc,
                            ref, x->m_type, nullptr));
                    } else if (unary_type == 2) {
                        op_el_wise = ASRUtils::EXPR(ASR::make_ComplexUnaryMinus_t(al, loc,
                            ref, x->m_type, nullptr));
                    } else if (unary_type == 3) {
                        op_el_wise = ASRUtils::EXPR(ASR::make_IntegerBitNot_t(al, loc,
                            ref, x->m_type, nullptr));
                    } else if (unary_type == 4) {
                        op_el_wise = ASRUtils::EXPR(ASR::make_LogicalNot_t(al, loc,
                            ref, x->m_type, nullptr));
                    }
                    ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res,
                                            op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
        }
    }

    void replace_IntegerUnaryMinus(ASR::IntegerUnaryMinus_t* x) {
        replace_UnaryOp(x, 0, "_integer_unary_op_res");
    }

    void replace_RealUnaryMinus(ASR::RealUnaryMinus_t* x) {
        replace_UnaryOp(x, 1, "_real_unary_op_res");
    }

    void replace_ComplexUnaryMinus(ASR::ComplexUnaryMinus_t* x) {
        replace_UnaryOp(x, 2, "_complex_unary_op_res");
    }

    void replace_IntegerBitNot(ASR::IntegerBitNot_t* x) {
        replace_UnaryOp(x, 3, "_integerbitnot_unary_op_res");
    }

    void replace_LogicalNot(ASR::LogicalNot_t* x) {
        replace_UnaryOp(x, 4, "_logicalnot_unary_op_res");
    }

    void replace_RealBinOp(ASR::RealBinOp_t* x) {
        replace_ArrayOpCommon(x, "_real_bin_op_res");
    }

    void replace_IntegerBinOp(ASR::IntegerBinOp_t* x) {
        replace_ArrayOpCommon(x, "_integer_bin_op_res");
    }

    void replace_ComplexBinOp(ASR::ComplexBinOp_t* x) {
        replace_ArrayOpCommon<ASR::ComplexBinOp_t>(x, "_complex_bin_op_res");
    }

    void replace_LogicalBinOp(ASR::LogicalBinOp_t* x) {
        replace_ArrayOpCommon<ASR::LogicalBinOp_t>(x, "_logical_bin_op_res");
    }

    void replace_IntegerCompare(ASR::IntegerCompare_t* x) {
        replace_ArrayOpCommon<ASR::IntegerCompare_t>(x, "_integer_comp_op_res");
    }

    void replace_RealCompare(ASR::RealCompare_t* x) {
        replace_ArrayOpCommon<ASR::RealCompare_t>(x, "_real_comp_op_res");
    }

    void replace_ComplexCompare(ASR::ComplexCompare_t* x) {
        replace_ArrayOpCommon<ASR::ComplexCompare_t>(x, "_complex_comp_op_res");
    }

    void replace_LogicalCompare(ASR::LogicalCompare_t* x) {
        replace_ArrayOpCommon<ASR::LogicalCompare_t>(x, "_logical_comp_op_res");
    }

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        std::string x_name;
        if( x->m_name->type == ASR::symbolType::ExternalSymbol ) {
            x_name = ASR::down_cast<ASR::ExternalSymbol_t>(x->m_name)->m_name;
        } else if( x->m_name->type == ASR::symbolType::Function ) {
            x_name = ASR::down_cast<ASR::Function_t>(x->m_name)->m_name;
        }

        // The following checks if the name of a function actually
        // points to a subroutine. If true this would mean that the
        // original function returned an array and is now a subroutine.
        // So the current function call will be converted to a subroutine
        // call. In short, this check acts as a signal whether to convert
        // a function call to a subroutine call.
        if (current_scope == nullptr) {
            return ;
        }

        const Location& loc = x->base.base.loc;
        ASR::symbol_t *sub = current_scope->resolve_symbol(x_name);
        if (sub && ASR::is_a<ASR::Function_t>(*sub)
            && ASR::down_cast<ASR::Function_t>(sub)->m_return_var == nullptr) {
            if( result_var == nullptr ) {
                result_var = PassUtils::create_var(result_counter, "_func_call_res",
                                loc, x->m_type, al, current_scope);
                result_counter += 1;
            }
            Vec<ASR::call_arg_t> s_args;
            s_args.reserve(al, x->n_args + 1);
            for( size_t i = 0; i < x->n_args; i++ ) {
                s_args.push_back(al, x->m_args[i]);
            }
            ASR::call_arg_t result_arg;
            result_arg.loc = result_var->base.loc;
            result_arg.m_value = result_var;
            s_args.push_back(al, result_arg);
            *current_expr = result_var;
            ASR::stmt_t* subrout_call = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, loc,
                                            sub, nullptr, s_args.p, s_args.size(), nullptr));
            pass_result.push_back(al, subrout_call);
        } else if( PassUtils::is_elemental(x->m_name) ) {
            std::vector<bool> array_mask(x->n_args, false);
            bool at_least_one_array = false;
            for( size_t iarg = 0; iarg < x->n_args; iarg++ ) {
                array_mask[iarg] = ASRUtils::is_array(
                    ASRUtils::expr_type(x->m_args[iarg].m_value));
                at_least_one_array = at_least_one_array || array_mask[iarg];
            }
            if (!at_least_one_array) {
                return ;
            }
            std::string res_prefix = "_elemental_func_call_res";
            ASR::expr_t* result_var_copy = result_var;
            bool is_all_rank_0 = true;
            std::vector<ASR::expr_t*> operands;
            ASR::expr_t* operand = nullptr;
            int common_rank = 0;
            bool are_all_rank_same = true;
            for( size_t iarg = 0; iarg < x->n_args; iarg++ ) {
                result_var = nullptr;
                ASR::expr_t** current_expr_copy_9 = current_expr;
                current_expr = &(x->m_args[iarg].m_value);
                self().replace_expr(x->m_args[iarg].m_value);
                operand = *current_expr;
                current_expr = current_expr_copy_9;
                operands.push_back(operand);
                int rank_operand = PassUtils::get_rank(operand);
                if( common_rank == 0 ) {
                    common_rank = rank_operand;
                }
                if( common_rank != rank_operand &&
                    rank_operand > 0 ) {
                    are_all_rank_same = false;
                }
                array_mask[iarg] = (rank_operand > 0);
                is_all_rank_0 = is_all_rank_0 && (rank_operand <= 0);
            }
            if( is_all_rank_0 ) {
                return ;
            }
            if( !are_all_rank_same ) {
                throw LCompilersException("Broadcasting support not yet available "
                                          "for different shape arrays.");
            }
            result_var = result_var_copy;
            if( result_var == nullptr ) {
                result_var = PassUtils::create_var(result_counter, res_prefix,
                                loc, operand, al, current_scope);
                result_counter += 1;
            }
            *current_expr = result_var;

            int n_dims = common_rank;
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
                head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    Vec<ASR::call_arg_t> ref_args;
                    ref_args.reserve(al, x->n_args);
                    for( size_t iarg = 0; iarg < x->n_args; iarg++ ) {
                        ASR::expr_t* ref = operands[iarg];
                        if( array_mask[iarg] ) {
                            ref = PassUtils::create_array_ref(operands[iarg], idx_vars, al);
                        }
                        ASR::call_arg_t ref_arg;
                        ref_arg.loc = ref->base.loc;
                        ref_arg.m_value = ref;
                        ref_args.push_back(al, ref_arg);
                    }
                    Vec<ASR::dimension_t> empty_dim;
                    empty_dim.reserve(al, 1);
                    ASR::ttype_t* dim_less_type = ASRUtils::duplicate_type(al, x->m_type, &empty_dim);
                    ASR::expr_t* op_el_wise = nullptr;
                    op_el_wise = ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
                                    x->m_name, x->m_original_name, ref_args.p, ref_args.size(), dim_less_type,
                                    nullptr, x->m_dt));
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
        }
        result_var = nullptr;
    }


};

class ArrayOpVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArrayOpVisitor>
{
    private:

        Allocator& al;
        bool use_custom_loop_params;
        ReplaceArrayOp replacer;
        Vec<ASR::stmt_t*> pass_result;
        Vec<ASR::expr_t*> result_lbound, result_ubound, result_inc;
        bool remove_original_statement;

    public:

        ArrayOpVisitor(Allocator& al_) :
        al(al_), use_custom_loop_params(false),
        replacer(al_, pass_result, use_custom_loop_params,
                 result_lbound, result_ubound, result_inc),
        remove_original_statement(false) {
            pass_result.n = 0;
            result_lbound.n = 0;
            result_ubound.n = 0;
            result_inc.n = 0;
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
            replacer.current_scope = current_scope;
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            for (size_t i=0; i<n_body; i++) {
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                remove_original_statement = false;
                visit_stmt(*m_body[i]);
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                if( !remove_original_statement ) {
                    body.push_back(al, m_body[i]);
                }
            }
            m_body = body.p;
            n_body = body.size();
            replacer.result_var = nullptr;
        }

        ASR::symbol_t* create_subroutine_from_function(ASR::Function_t* s) {
            for( auto& s_item: s->m_symtab->get_scope() ) {
                ASR::symbol_t* curr_sym = s_item.second;
                if( curr_sym->type == ASR::symbolType::Variable ) {
                    ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(curr_sym);
                    if( var->m_intent == ASR::intentType::Unspecified ) {
                        var->m_intent = ASR::intentType::In;
                    } else if( var->m_intent == ASR::intentType::ReturnVar ) {
                        var->m_intent = ASR::intentType::Out;
                    }
                }
            }
            Vec<ASR::expr_t*> a_args;
            a_args.reserve(al, s->n_args + 1);
            for( size_t i = 0; i < s->n_args; i++ ) {
                a_args.push_back(al, s->m_args[i]);
            }
            LCOMPILERS_ASSERT(s->m_return_var)
            a_args.push_back(al, s->m_return_var);
            ASR::FunctionType_t* s_func_type = ASR::down_cast<ASR::FunctionType_t>(s->m_function_signature);
            ASR::asr_t* s_sub_asr = ASRUtils::make_Function_t_util(al, s->base.base.loc,
                s->m_symtab, s->m_name, s->m_dependencies, s->n_dependencies,
                a_args.p, a_args.size(), s->m_body, s->n_body,
                nullptr, s_func_type->m_abi, s->m_access, s_func_type->m_deftype,
                nullptr, false, false, false, s_func_type->m_inline, s_func_type->m_static,
                s_func_type->m_type_params, s_func_type->n_type_params, s_func_type->m_restrictions,
                s_func_type->n_restrictions, s_func_type->m_is_restriction, s->m_deterministic,
                s->m_side_effect_free);
            ASR::symbol_t* s_sub = ASR::down_cast<ASR::symbol_t>(s_sub_asr);
            return s_sub;
        }

        // TODO: Only Program and While is processed, we need to process all calls
        // to visit_stmt().
        // TODO: Only TranslationUnit's and Program's symbol table is processed
        // for transforming function->subroutine if they return arrays
        void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_global_scope;
            std::vector<std::pair<std::string, ASR::symbol_t*>> replace_vec;
            // Transform functions returning arrays to subroutines
            for (auto &item : x.m_global_scope->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    if (s->m_return_var) {
                        /*
                        * A function which returns an array will be converted
                        * to a subroutine with the destination array as the last
                        * argument. This helps in avoiding deep copies and the
                        * destination memory directly gets filled inside the subroutine.
                        */
                        if( PassUtils::is_array(s->m_return_var) ) {
                            ASR::symbol_t* s_sub = create_subroutine_from_function(s);
                            replace_vec.push_back(std::make_pair(item.first, s_sub));
                        }
                    }
                }
            }

            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::TranslationUnit_t &xx = const_cast<ASR::TranslationUnit_t&>(x);
            // Updating the symbol table so that now the name
            // of the function (which returned array) now points
            // to the newly created subroutine.
            for( auto& item: replace_vec ) {
                xx.m_global_scope->add_symbol(item.first, item.second);
            }

            // Now visit everything else
            for (auto &item : x.m_global_scope->get_scope()) {
                this->visit_symbol(*item.second);
            }
            current_scope = current_scope_copy;
        }

        void visit_Program(const ASR::Program_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            SymbolTable* current_scope_copy = current_scope;
            current_scope = xx.m_symtab;
            std::vector<std::pair<std::string, ASR::symbol_t*> > replace_vec;

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                    if (s->m_return_var) {
                        /*
                        * A function which returns an array will be converted
                        * to a subroutine with the destination array as the last
                        * argument. This helps in avoiding deep copies and the
                        * destination memory directly gets filled inside the subroutine.
                        */
                        if( PassUtils::is_array(s->m_return_var) ) {
                            ASR::symbol_t* s_sub = create_subroutine_from_function(s);
                            replace_vec.push_back(std::make_pair(item.first, s_sub));
                            bool is_arg = false;
                            size_t arg_index = 0;
                            for( size_t i = 0; i < xx.n_body; i++ ) {
                                ASR::stmt_t* stm = xx.m_body[i];
                                if( stm->type == ASR::stmtType::SubroutineCall ) {
                                    ASR::SubroutineCall_t *subrout_call = ASR::down_cast<ASR::SubroutineCall_t>(stm);
                                    for ( size_t j = 0; j < subrout_call->n_args; j++ ) {
                                        ASR::expr_t* arg_value = subrout_call->m_args[j].m_value;
                                        if( arg_value->type == ASR::exprType::Var ) {
                                            ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(arg_value);
                                            ASR::symbol_t* sym = var->m_v;
                                            if ( sym->type == ASR::symbolType::Function ) {
                                                ASR::Function_t* subrout = ASR::down_cast<ASR::Function_t>(sym);
                                                std::string subrout_name = std::string(subrout->m_name);
                                                if ( subrout_name == item.first ) {
                                                    is_arg = true;
                                                    arg_index = j;
                                                    ASR::call_arg_t new_call_arg;
                                                    new_call_arg.loc = subrout_call->m_args[j].loc;
                                                    new_call_arg.m_value = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, var->base.base.loc, s_sub));
                                                    subrout_call->m_args[j] = new_call_arg;
                                                }
                                            }

                                        }
                                    }
                                    if ( is_arg ) {
                                        ASR::symbol_t* subrout = subrout_call->m_name;
                                        if ( subrout->type == ASR::symbolType::Function ) {
                                            ASR::Function_t* subrout_func = ASR::down_cast<ASR::Function_t>(subrout);
                                            std::string subrout_func_name = std::string(subrout_func->m_name);
                                            ASR::expr_t* arg = subrout_func->m_args[arg_index];
                                            if( arg->type == ASR::exprType::Var ) {
                                                ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(arg);
                                                ASR::symbol_t* sym = var->m_v;
                                                if ( sym->type == ASR::symbolType::Function ) {
                                                    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(sym);
                                                    ASR::symbol_t* s_func = create_subroutine_from_function(ASR::down_cast<ASR::Function_t>(sym));
                                                    subrout_func->m_symtab->add_symbol(func->m_name, s_func);
                                                    subrout_func->m_args[arg_index] = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, var->base.base.loc, s_func));
                                                }
                                            }

                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Updating the symbol table so that now the name
            // of the function (which returned array) now points
            // to the newly created subroutine.
            for( auto& item: replace_vec ) {
                current_scope->add_symbol(item.first, item.second);
            }

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::AssociateBlock_t>(*item.second)) {
                    ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                    visit_AssociateBlock(*s);
                }
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                    visit_Function(*s);
                }
            }

            transform_stmts(xx.m_body, xx.n_body);
            current_scope = current_scope_copy;
        }

        inline void visit_AssignmentUtil(const ASR::Assignment_t& x) {
            ASR::expr_t** current_expr_copy_9 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            this->call_replacer();
            current_expr = current_expr_copy_9;
            this->visit_expr(*x.m_value);
            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
            }
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            if( (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
                ASR::is_a<ASR::GetPointer_t>(*x.m_value)) ||
                (ASR::is_a<ASR::ArrayConstant_t>(*x.m_value)) ) {
                return ;
            }

            if( ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ) {
                visit_AssignmentUtil(x);
                return ;
            }

            if( PassUtils::is_array(x.m_target) ) {
                replacer.result_var = x.m_target;
                remove_original_statement = true;
            } else if( ASR::is_a<ASR::ArraySection_t>(*x.m_target) ) {
                ASR::ArraySection_t* array_ref = ASR::down_cast<ASR::ArraySection_t>(x.m_target);
                replacer.result_var = array_ref->m_v;
                remove_original_statement = true;
                result_lbound.reserve(al, array_ref->n_args);
                result_ubound.reserve(al, array_ref->n_args);
                result_inc.reserve(al, array_ref->n_args);
                ASR::expr_t *m_start, *m_end, *m_increment;
                m_start = m_end = m_increment = nullptr;
                for( int i = 0; i < (int) array_ref->n_args; i++ ) {
                    if( array_ref->m_args[i].m_step != nullptr ) {
                        if( array_ref->m_args[i].m_left == nullptr ) {
                            m_start = PassUtils::get_bound(replacer.result_var, i + 1, "lbound", al);
                        } else {
                            m_start = array_ref->m_args[i].m_left;
                        }
                        if( array_ref->m_args[i].m_right == nullptr ) {
                            m_end = PassUtils::get_bound(replacer.result_var, i + 1, "ubound", al);
                        } else {
                            m_end = array_ref->m_args[i].m_right;
                        }
                    } else {
                        m_start = array_ref->m_args[i].m_right;
                        m_end = array_ref->m_args[i].m_right;
                    }
                    m_increment = array_ref->m_args[i].m_step;
                    result_lbound.push_back(al, m_start);
                    result_ubound.push_back(al, m_end);
                    result_inc.push_back(al, m_increment);
                }
                use_custom_loop_params = true;
            }

            visit_AssignmentUtil(x);
        }

};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit,
                           const LCompilers::PassOptions& /*pass_options*/) {
    ArrayOpVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
