#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/array_op.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>

#include <vector>
#include <utility>

/*
This ASR pass replaces operations over arrays with do loops.
The function `pass_replace_array_op` transforms the ASR tree in-place.

Converts:

    c = a + b

to:

    do i = lbound(a), ubound(a)
        c(i) = a(i) + b(i)
    end do

The code below might seem intriguing because of minor but crucial
details. Generally for any node, first, its children are visited.
If any child contains operations over arrays then, the do loop
pass is added for performing the operation element wise. For storing
the result, either a new variable is created or a result variable
available from the parent node is used. Once done, this result variable
is used by the parent node in place of the child node from which it was
made available. Consider the example below for better understanding.

Say, BinOp(BinOp(Arr1 Add Arr2) Add Arr3) is the expression we want
to visit. Then, first BinOp(Arr1 Add Arr2) will be visited and its
result will be stored (not actually, just extra ASR do loop node will be added)
in a new variable, Say Result1. Then this Result1 will be used as follows,
BinOp(Result1 Add Arr3). Imagine, this overall expression is further
assigned to some Fortran variable as, Assign(Var1, BinOp(Result1 Add Arr3)).
In this case a new variable will not be created to store the result of RHS, just Var1
will be used as the final destination and a do loop pass will be added as follows,
do i = lbound(Var1), ubound(Var1)

Var1(i) = Result1(i) + Arr3(i)

end do

Note that once the control will reach the above loop, the loop for
Result1 would have already been executed.

All the nodes should be implemented using the above logic to track
array operations and perform the do loop pass. As of now, some of the
nodes are implemented and more are yet to be implemented with time.
*/

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class ReplaceArrayOp: public ASR::BaseExprReplacer<ReplaceArrayOp> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;
    size_t result_counter;
    bool& use_custom_loop_params;
    bool& apply_again;
    bool& remove_original_statement;
    Vec<ASR::expr_t*>& result_lbound;
    Vec<ASR::expr_t*>& result_ubound;
    Vec<ASR::expr_t*>& result_inc;
    ASR::dimension_t* op_dims; size_t op_n_dims;
    ASR::expr_t* op_expr;
    std::map<ASR::expr_t*, ASR::expr_t*>& resultvar2value;

    public:

    SymbolTable* current_scope;
    ASR::expr_t* result_var;
    ASR::ttype_t* result_type;

    ReplaceArrayOp(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
                   bool& use_custom_loop_params_,
                   bool& apply_again_, bool& remove_original_statement_,
                   Vec<ASR::expr_t*>& result_lbound_,
                   Vec<ASR::expr_t*>& result_ubound_,
                   Vec<ASR::expr_t*>& result_inc_,
                   std::map<ASR::expr_t*, ASR::expr_t*>& resultvar2value_) :
    al(al_), pass_result(pass_result_),
    result_counter(0), use_custom_loop_params(use_custom_loop_params_),
    apply_again(apply_again_), remove_original_statement(remove_original_statement_),
    result_lbound(result_lbound_), result_ubound(result_ubound_),
    result_inc(result_inc_), op_dims(nullptr), op_n_dims(0),
    op_expr(nullptr), resultvar2value(resultvar2value_),
    current_scope(nullptr), result_var(nullptr), result_type(nullptr) {}

    template <typename LOOP_BODY>
    void create_do_loop(const Location& loc, int var_rank, int result_rank,
        Vec<ASR::expr_t*>& idx_vars, Vec<ASR::expr_t*>& loop_vars,
        Vec<ASR::expr_t*>& idx_vars_value,
        std::vector<int>& loop_var_indices,
        Vec<ASR::stmt_t*>& doloop_body,
        ASR::expr_t* op_expr, int op_expr_dim_offset, LOOP_BODY loop_body) {
        PassUtils::create_idx_vars(idx_vars_value, var_rank, loc, al, current_scope, "_v");
        if( use_custom_loop_params ) {
            PassUtils::create_idx_vars(idx_vars, loop_vars, loop_var_indices,
                                       result_ubound, result_inc,
                                       loc, al, current_scope, "_t");
        } else {
            PassUtils::create_idx_vars(idx_vars, result_rank, loc, al, current_scope, "_t");
            loop_vars.from_pointer_n_copy(al, idx_vars.p, idx_vars.size());
        }
        ASR::stmt_t* doloop = nullptr;
        LCOMPILERS_ASSERT(result_rank >= var_rank);
        // LCOMPILERS_ASSERT(var_rank == (int) loop_vars.size());
        ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, int32_type));
        if (var_rank == (int) loop_vars.size()) {
            for( int i = var_rank - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = loop_vars[i];
                if( use_custom_loop_params ) {
                    int j = loop_var_indices[i];
                    head.m_start = result_lbound[j];
                    head.m_end = result_ubound[j];
                    head.m_increment = result_inc[j];
                } else {
                    head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
                    head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
                    head.m_increment = nullptr;
                }
                head.loc = head.m_v->base.loc;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    loop_body();
                } else {
                    if( var_rank > 0 ) {
                        ASR::expr_t* idx_lb = PassUtils::get_bound(op_expr, i + op_expr_dim_offset, "lbound", al);
                        ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(
                            al, loc, idx_vars_value[i+1], idx_lb, nullptr));
                        doloop_body.push_back(al, set_to_one);
                    }
                    doloop_body.push_back(al, doloop);
                }
                if( var_rank > 0 ) {
                    ASR::expr_t* inc_expr = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                        al, loc, idx_vars_value[i], ASR::binopType::Add, const_1, int32_type, nullptr));
                    ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
                        al, loc, idx_vars_value[i], inc_expr, nullptr));
                    doloop_body.push_back(al, assign_stmt);
                }
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head, doloop_body.p, doloop_body.size()));
            }
            if( var_rank > 0 ) {
                ASR::expr_t* idx_lb = PassUtils::get_bound(op_expr, 1, "lbound", al);
                ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, idx_vars_value[0], idx_lb, nullptr));
                pass_result.push_back(al, set_to_one);
            }
            pass_result.push_back(al, doloop);
        } else if (var_rank == 0) {
            ASR::do_loop_head_t head;
            head.m_v = loop_vars[0];
            if( use_custom_loop_params ) {
                int j = loop_var_indices[0];
                head.m_start = result_lbound[j];
                head.m_end = result_ubound[j];
                head.m_increment = result_inc[j];
            } else {
                head.m_start = PassUtils::get_bound(result_var, 1, "lbound", al);
                head.m_end = PassUtils::get_bound(result_var, 1, "ubound", al);
                head.m_increment = nullptr;
            }
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                loop_body();
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head, doloop_body.p, doloop_body.size()));
            pass_result.push_back(al, doloop);
        }

    }

    template <typename T>
    void replace_vars_helper(T* x) {
        if( op_expr == *current_expr ) {
            ASR::ttype_t* current_expr_type = ASRUtils::expr_type(*current_expr);
            op_n_dims = ASRUtils::extract_dimensions_from_ttype(current_expr_type, op_dims);
        }
        if( !(result_var != nullptr && PassUtils::is_array(result_var) &&
              resultvar2value.find(result_var) != resultvar2value.end() &&
              resultvar2value[result_var] == &(x->base)) ) {
            return ;
        }

        const Location& loc = x->base.base.loc;
        int var_rank = PassUtils::get_rank(*current_expr);
        int result_rank = PassUtils::get_rank(result_var);
        Vec<ASR::expr_t*> idx_vars, loop_vars, idx_vars_value;
        std::vector<int> loop_var_indices;
        Vec<ASR::stmt_t*> doloop_body;
        create_do_loop(loc, var_rank, result_rank, idx_vars,
        loop_vars, idx_vars_value, loop_var_indices, doloop_body,
        *current_expr, 1,
        [=, &idx_vars_value, &idx_vars, &doloop_body]() {
            ASR::expr_t* ref = nullptr;
            if( var_rank > 0 ) {
                ref = PassUtils::create_array_ref(*current_expr, idx_vars_value, al);
            } else {
                ref = *current_expr;
            }
            ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
            ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, ref, nullptr));
            doloop_body.push_back(al, assign);
        });
        *current_expr = nullptr;
        result_var = nullptr;
        use_custom_loop_params = false;
    }

    void replace_StructInstanceMember(ASR::StructInstanceMember_t* x) {
        replace_vars_helper(x);
    }

    void replace_Var(ASR::Var_t* x) {
        replace_vars_helper(x);
    }

    void replace_ArrayItem(ASR::ArrayItem_t* x) {
        replace_vars_helper(x);
    }

    template <typename LOOP_BODY>
    void create_do_loop(const Location& loc, int result_rank,
        Vec<ASR::expr_t*>& idx_vars, Vec<ASR::expr_t*>& loop_vars,
        std::vector<int>& loop_var_indices, Vec<ASR::stmt_t*>& doloop_body,
        LOOP_BODY loop_body) {
        if( use_custom_loop_params ) {
            PassUtils::create_idx_vars(idx_vars, loop_vars, loop_var_indices,
                                       result_ubound, result_inc,
                                       loc, al, current_scope, "_t");
        } else {
            PassUtils::create_idx_vars(idx_vars, result_rank, loc, al, current_scope, "_t");
            loop_vars.from_pointer_n_copy(al, idx_vars.p, idx_vars.size());
        }

        ASR::stmt_t* doloop = nullptr;
        for( int i = (int) loop_vars.size() - 1; i >= 0; i-- ) {
            // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
            ASR::do_loop_head_t head;
            head.m_v = loop_vars[i];
            if( use_custom_loop_params ) {
                int j = loop_var_indices[i];
                head.m_start = result_lbound[j];
                head.m_end = result_ubound[j];
                head.m_increment = result_inc[j];
            } else {
                head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
                head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
                head.m_increment = nullptr;
            }
            head.loc = head.m_v->base.loc;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                loop_body();
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head, doloop_body.p, doloop_body.size()));
        }
        pass_result.push_back(al, doloop);
    }

    template <typename T>
    void replace_Constant(T* x) {
        if( !(result_var != nullptr && PassUtils::is_array(result_var) &&
              resultvar2value.find(result_var) != resultvar2value.end() &&
              resultvar2value[result_var] == &(x->base)) ) {
            return ;
        }

        const Location& loc = x->base.base.loc;
        int n_dims = PassUtils::get_rank(result_var);
        Vec<ASR::expr_t*> idx_vars, loop_vars;
        std::vector<int> loop_var_indices;
        Vec<ASR::stmt_t*> doloop_body;
        create_do_loop(loc, n_dims, idx_vars,
            loop_vars, loop_var_indices, doloop_body,
            [=, &idx_vars, &doloop_body] () {
            ASR::expr_t* ref = *current_expr;
            ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
            ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, ref, nullptr));
            doloop_body.push_back(al, assign);
        });
        result_var = nullptr;
        use_custom_loop_params = false;
        *current_expr = nullptr;
    }

    void replace_IntegerConstant(ASR::IntegerConstant_t* x) {
        replace_Constant(x);
    }

    void replace_RealConstant(ASR::RealConstant_t* x) {
        replace_Constant(x);
    }

    void replace_ComplexConstant(ASR::ComplexConstant_t* x) {
        replace_Constant(x);
    }

    void replace_LogicalConstant(ASR::LogicalConstant_t* x) {
        replace_Constant(x);
    }

    template <typename T>
    ASR::expr_t* generate_element_wise_operation(const Location& loc, ASR::expr_t* left, ASR::expr_t* right, T* x) {
        switch( x->class_type ) {
            case ASR::exprType::IntegerBinOp:
                return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                            al, loc, left, (ASR::binopType)x->m_op,
                            right, x->m_type, nullptr));

            case ASR::exprType::UnsignedIntegerBinOp:
                return ASRUtils::EXPR(ASR::make_UnsignedIntegerBinOp_t(
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

            case ASR::exprType::UnsignedIntegerCompare:
                return ASRUtils::EXPR(ASR::make_UnsignedIntegerCompare_t(
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

    ASR::ttype_t* get_result_type(ASR::ttype_t* op_type,
        ASR::dimension_t* dims, size_t n_dims,
        const Location& loc, ASR::exprType class_type,
        bool& allocate) {

        Vec<ASR::dimension_t> result_dims;
        bool is_fixed_size_array = ASRUtils::is_fixed_size_array(dims, n_dims);
        if( is_fixed_size_array ) {
            result_dims.from_pointer_n(dims, n_dims);
        } else {
            allocate = true;
            result_dims.reserve(al, n_dims);
            for( size_t i = 0; i < n_dims; i++ ) {
                ASR::dimension_t result_dim;
                result_dim.loc = loc;
                result_dim.m_length = nullptr;
                result_dim.m_start = nullptr;
                result_dims.push_back(al, result_dim);
            }
        }

        op_dims = result_dims.p;
        op_n_dims = result_dims.size();

        switch( class_type ) {
            case ASR::exprType::RealCompare:
            case ASR::exprType::ComplexCompare:
            case ASR::exprType::LogicalCompare:
            case ASR::exprType::IntegerCompare: {
                return ASRUtils::TYPE(ASR::make_Logical_t(al, loc,
                            4, result_dims.p, result_dims.size()));
            }
            default: {
                if( allocate || is_fixed_size_array ) {
                    op_type = ASRUtils::type_get_past_pointer(op_type);
                }
                return ASRUtils::duplicate_type(al, op_type, &result_dims);
            }
        }
    }

    void replace_ArraySection(ASR::ArraySection_t* x) {
        Vec<ASR::dimension_t> x_dims;
        x_dims.reserve(al, x->n_args);
        const Location& loc = x->base.base.loc;
        ASRUtils::ASRBuilder builder(al, loc);
        ASR::expr_t* i32_one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
            al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0))));
        Vec<ASR::dimension_t> empty_dims;
        empty_dims.reserve(al, x->n_args);
        for( size_t i = 0; i < x->n_args; i++ ) {
            if( x->m_args[i].m_step != nullptr ) {

                ASR::dimension_t empty_dim;
                empty_dim.loc = loc;
                empty_dim.m_start = nullptr;
                empty_dim.m_length = nullptr;
                empty_dims.push_back(al, empty_dim);

                ASR::dimension_t x_dim;
                x_dim.loc = loc;
                x_dim.m_start = x->m_args[i].m_left;
                ASR::expr_t* start_value = ASRUtils::expr_value(x_dim.m_start);
                ASR::expr_t* end_value = ASRUtils::expr_value(x->m_args[i].m_right);
                ASR::expr_t* step_value = ASRUtils::expr_value(x->m_args[i].m_step);
                ASR::expr_t* length_value = nullptr;
                if( ASRUtils::is_value_constant(start_value) &&
                    ASRUtils::is_value_constant(end_value) &&
                    ASRUtils::is_value_constant(step_value) ) {
                    int64_t const_start = -1;
                    if( !ASRUtils::extract_value(start_value, const_start) ) {
                        LCOMPILERS_ASSERT(false);
                    }
                    int64_t const_end = -1;
                    if( !ASRUtils::extract_value(end_value, const_end) ) {
                        LCOMPILERS_ASSERT(false);
                    }
                    int64_t const_step = -1;
                    if( !ASRUtils::extract_value(step_value, const_step) ) {
                        LCOMPILERS_ASSERT(false);
                    }
                    length_value = make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t,
                        ((const_end - const_start)/const_step) + 1, 4, loc);
                }
                x_dim.m_length = builder.ElementalAdd(builder.ElementalDiv(
                    builder.ElementalSub(x->m_args[i].m_right, x->m_args[i].m_left, loc),
                    x->m_args[i].m_step, loc), i32_one, loc, length_value);
                x_dims.push_back(al, x_dim);
            }
        }
        if( op_expr == *current_expr ) {
            op_dims = x_dims.p;
            op_n_dims = x_dims.size();
        }

        ASR::ttype_t* x_m_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, loc,
            ASRUtils::duplicate_type(al,
            ASRUtils::type_get_past_pointer(x->m_type), &empty_dims)));

        ASR::expr_t* array_section_pointer = PassUtils::create_var(
            result_counter, "_array_section_pointer_", loc,
            x_m_type, al, current_scope);
        result_counter += 1;
        pass_result.push_back(al, ASRUtils::STMT(ASR::make_Associate_t(
            al, loc, array_section_pointer, *current_expr)));
        *current_expr = array_section_pointer;
    }

    #define allocate_result_var(op_arg, op_dims_arg, op_n_dims_arg) bool is_dimension_empty = false; \
        for( int i = 0; i < op_n_dims_arg; i++ ) { \
            if( op_dims_arg->m_length == nullptr ) { \
                is_dimension_empty = true; \
                break; \
            } \
        } \
        Vec<ASR::alloc_arg_t> alloc_args; \
        alloc_args.reserve(al, 1); \
        if( !is_dimension_empty ) { \
            ASR::alloc_arg_t alloc_arg; \
            alloc_arg.loc = loc; \
            alloc_arg.m_a = result_var; \
            alloc_arg.m_dims = op_dims_arg; \
            alloc_arg.n_dims = op_n_dims_arg; \
            alloc_args.push_back(al, alloc_arg); \
            op_dims = op_dims_arg; \
            op_n_dims = op_n_dims_arg; \
        } else { \
            Vec<ASR::dimension_t> alloc_dims; \
            alloc_dims.reserve(al, op_n_dims_arg); \
            for( int i = 0; i < op_n_dims_arg; i++ ) { \
                ASR::dimension_t alloc_dim; \
                alloc_dim.loc = loc; \
                alloc_dim.m_start = PassUtils::get_bound(op_arg, i + 1, "lbound", al); \
                alloc_dim.m_length = ASRUtils::compute_length_from_start_end(al, alloc_dim.m_start, \
                    PassUtils::get_bound(op_arg, i + 1, "ubound", al)); \
                alloc_dims.push_back(al, alloc_dim); \
            } \
            ASR::alloc_arg_t alloc_arg; \
            alloc_arg.loc = loc; \
            alloc_arg.m_a = result_var; \
            alloc_arg.m_dims = alloc_dims.p; \
            alloc_arg.n_dims = alloc_dims.size(); \
            alloc_args.push_back(al, alloc_arg); \
            op_dims = alloc_dims.p; \
            op_n_dims = alloc_dims.size(); \
        } \
        pass_result.push_back(al, ASRUtils::STMT(ASR::make_Allocate_t(al, \
            loc, alloc_args.p, alloc_args.size(), nullptr, nullptr, nullptr))); \

    template <typename T>
    void replace_ArrayOpCommon(T* x, std::string res_prefix) {
        const Location& loc = x->base.base.loc;
        bool current_status = use_custom_loop_params;
        use_custom_loop_params = false;
        ASR::dimension_t *left_dims; int rank_left;
        ASR::dimension_t *right_dims; int rank_right;
        ASR::expr_t* result_var_copy = result_var;
        ASR::dimension_t* op_dims_copy = op_dims;
        size_t op_n_dims_copy = op_n_dims;
        ASR::expr_t* op_expr_copy = op_expr;

        ASR::expr_t** current_expr_copy_35 = current_expr;
        op_dims = nullptr;
        op_n_dims = 0;
        current_expr = &(x->m_left);
        op_expr = *current_expr;
        result_var = nullptr;
        this->replace_expr(x->m_left);
        ASR::expr_t* left = *current_expr;
        left_dims = op_dims;
        rank_left = op_n_dims;
        current_expr = current_expr_copy_35;

        ASR::expr_t** current_expr_copy_36 = current_expr;
        op_dims = nullptr;
        op_n_dims = 0;
        current_expr = &(x->m_right);
        op_expr = *current_expr;
        result_var = nullptr;
        this->replace_expr(x->m_right);
        ASR::expr_t* right = *current_expr;
        right_dims = op_dims;
        rank_right = op_n_dims;
        current_expr = current_expr_copy_36;

        op_dims = op_dims_copy;
        op_n_dims = op_n_dims_copy;
        op_expr = op_expr_copy;

        use_custom_loop_params = current_status;
        result_var = result_var_copy;

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
                bool allocate = false;
                ASR::ttype_t* result_var_type = get_result_type(ASRUtils::expr_type(left),
                    left_dims, rank_left, loc, x->class_type, allocate);
                ASR::storage_typeType result_storage = ASR::storage_typeType::Default;
                if( allocate ) {
                    result_storage = ASR::storage_typeType::Allocatable;
                }
                result_var = PassUtils::create_var(result_counter, res_prefix, loc,
                                result_var_type, al, current_scope, result_storage);
                result_counter += 1;
                if( allocate ) {
                    allocate_result_var(left, left_dims, rank_left);
                }
            }
            *current_expr = result_var;

            int result_rank = PassUtils::get_rank(result_var);
            Vec<ASR::expr_t*> idx_vars, idx_vars_value, loop_vars;
            std::vector<int> loop_var_indices;
            Vec<ASR::stmt_t*> doloop_body;
            create_do_loop(loc, rank_left, result_rank, idx_vars,
                loop_vars, idx_vars_value, loop_var_indices, doloop_body, left, 1,
                [=, &left, &right, &idx_vars_value, &idx_vars, &doloop_body]() {
                ASR::expr_t* ref_1 = PassUtils::create_array_ref(left, idx_vars_value, al);
                ASR::expr_t* ref_2 = PassUtils::create_array_ref(right, idx_vars_value, al);
                ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                ASR::expr_t* op_el_wise = generate_element_wise_operation(loc, ref_1, ref_2, x);
                ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, op_el_wise, nullptr));
                doloop_body.push_back(al, assign);
            });
            use_custom_loop_params = false;
        } else if( (rank_left == 0 && rank_right > 0) ||
                   (rank_right == 0 && rank_left > 0) ) {
            ASR::expr_t *arr_expr = nullptr, *other_expr = nullptr;
            int n_dims = 0;
            ASR::dimension_t* arr_expr_dims; int arr_expr_n_dims;
            if( rank_left > 0 ) {
                arr_expr = left;
                arr_expr_dims = left_dims;
                arr_expr_n_dims = rank_left;
                other_expr = right;
                n_dims = rank_left;
            } else {
                arr_expr = right;
                arr_expr_dims = right_dims;
                arr_expr_n_dims = rank_right;
                other_expr = left;
                n_dims = rank_right;
            }
            if( result_var == nullptr ) {
                bool allocate = false;
                ASR::ttype_t* result_var_type = get_result_type(ASRUtils::expr_type(arr_expr),
                    arr_expr_dims, arr_expr_n_dims, loc, x->class_type, allocate);
                ASR::storage_typeType result_storage = ASR::storage_typeType::Default;
                if( allocate ) {
                    result_storage = ASR::storage_typeType::Allocatable;
                }
                result_var = PassUtils::create_var(result_counter, res_prefix, loc,
                                result_var_type, al, current_scope, result_storage);
                result_counter += 1;
                if( allocate ) {
                    allocate_result_var(arr_expr, arr_expr_dims, arr_expr_n_dims);
                }
            }
            *current_expr = result_var;

            ASR::expr_t* op_expr = nullptr;
            if( rank_left > 0 ) {
                op_expr = left;
            } else {
                op_expr = right;
            }

            Vec<ASR::expr_t*> idx_vars, idx_vars_value, loop_vars;
            Vec<ASR::stmt_t*> doloop_body;
            std::vector<int> loop_var_indices;
            int result_rank = PassUtils::get_rank(result_var);
            create_do_loop(loc, n_dims, result_rank, idx_vars,
                loop_vars, idx_vars_value, loop_var_indices, doloop_body,
                op_expr, 2, [=, &arr_expr, &idx_vars, &idx_vars_value, &doloop_body]() {
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
            });
            use_custom_loop_params = false;
        }
        result_var = nullptr;
    }



    void replace_Cast(ASR::Cast_t* x) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        BaseExprReplacer::replace_Cast(x);
        result_var = result_var_copy;
        ASR::expr_t* tmp_val = x->m_arg;

        bool is_arg_array = PassUtils::is_array(tmp_val);
        bool is_result_var_array = result_var && PassUtils::is_array(result_var);
        if( !is_arg_array && !is_result_var_array ) {
            result_var = nullptr;
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
        Vec<ASR::expr_t*> idx_vars, loop_vars;
        std::vector<int> loop_var_indices;
        Vec<ASR::stmt_t*> doloop_body;
        create_do_loop(loc, n_dims, idx_vars,
            loop_vars, loop_var_indices, doloop_body,
            [=, &tmp_val, &idx_vars, &is_arg_array, &doloop_body] () {
            ASR::expr_t* ref = tmp_val;
            if( is_arg_array ) {
                ref = PassUtils::create_array_ref(tmp_val, idx_vars, al);
            }
            ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
            ASR::ttype_t* x_m_type = ASRUtils::duplicate_type_without_dims(
                                        al, x->m_type, x->m_type->base.loc);
            ASR::expr_t* impl_cast_el_wise = ASRUtils::EXPR(ASR::make_Cast_t(
                al, loc, ref, x->m_kind, x_m_type, nullptr));
            ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, impl_cast_el_wise, nullptr));
            doloop_body.push_back(al, assign);
        });
        *current_expr = result_var;
        if( op_expr == &(x->base) ) {
            op_dims = nullptr;
            op_n_dims = ASRUtils::extract_dimensions_from_ttype(
                ASRUtils::expr_type(*current_expr), op_dims);
        }
        result_var = nullptr;
        use_custom_loop_params = false;
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
            const Location& loc = x->base.base.loc;
            if (result_var) {
                int n_dims = PassUtils::get_rank(result_var);
                if (n_dims != 0) {
                    Vec<ASR::expr_t*> idx_vars, loop_vars;
                    std::vector<int> loop_var_indices;
                    Vec<ASR::stmt_t*> doloop_body;
                    create_do_loop(loc, n_dims, idx_vars,
                        loop_vars, loop_var_indices, doloop_body,
                        [=, &idx_vars, &doloop_body] () {
                        ASR::expr_t* ref = ASRUtils::EXPR((ASR::asr_t*)x);
                        ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                        ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, ref, nullptr));
                        doloop_body.push_back(al, assign);
                    });
                    result_var = nullptr;
                    use_custom_loop_params = false;
                    *current_expr = nullptr;
                }
            }
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
            if( op_expr == &(x->base) ) {
                op_dims = nullptr;
                op_n_dims = ASRUtils::extract_dimensions_from_ttype(
                    ASRUtils::expr_type(*current_expr), op_dims);
            }

            Vec<ASR::expr_t*> idx_vars, loop_vars;
            std::vector<int> loop_var_indices;
            Vec<ASR::stmt_t*> doloop_body;
            create_do_loop(loc, rank_operand, idx_vars,
                loop_vars, loop_var_indices, doloop_body,
                [=, &operand, &idx_vars, &x, &doloop_body] () {
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
            });
            result_var = nullptr;
            use_custom_loop_params = false;
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

    void replace_UnsignedIntegerBinOp(ASR::UnsignedIntegerBinOp_t* x) {
        replace_ArrayOpCommon(x, "_unsigned_integer_bin_op_res");
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

    void replace_UnsignedIntegerCompare(ASR::UnsignedIntegerCompare_t* x) {
        replace_ArrayOpCommon<ASR::UnsignedIntegerCompare_t>(x, "_unsigned_integer_comp_op_res");
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

    void replace_IntrinsicFunction(ASR::IntrinsicFunction_t* x) {
        if( !ASRUtils::IntrinsicFunctionRegistry::is_elemental(x->m_intrinsic_id) ) {
            return ;
        }
        LCOMPILERS_ASSERT(current_scope != nullptr);
        const Location& loc = x->base.base.loc;
        std::vector<bool> array_mask(x->n_args, false);
        bool at_least_one_array = false;
        for( size_t iarg = 0; iarg < x->n_args; iarg++ ) {
            array_mask[iarg] = ASRUtils::is_array(
                ASRUtils::expr_type(x->m_args[iarg]));
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
            current_expr = &(x->m_args[iarg]);
            self().replace_expr(x->m_args[iarg]);
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
        if( op_expr == &(x->base) ) {
            op_dims = nullptr;
            op_n_dims = ASRUtils::extract_dimensions_from_ttype(
                ASRUtils::expr_type(*current_expr), op_dims);
        }

        Vec<ASR::expr_t*> idx_vars, loop_vars;
        std::vector<int> loop_var_indices;
        Vec<ASR::stmt_t*> doloop_body;
        create_do_loop(loc, common_rank,
            idx_vars, loop_vars, loop_var_indices, doloop_body,
            [=, &operands, &idx_vars, &doloop_body] () {
            Vec<ASR::expr_t*> ref_args;
            ref_args.reserve(al, x->n_args);
            for( size_t iarg = 0; iarg < x->n_args; iarg++ ) {
                ASR::expr_t* ref = operands[iarg];
                if( array_mask[iarg] ) {
                    ref = PassUtils::create_array_ref(operands[iarg], idx_vars, al);
                }
                ref_args.push_back(al, ref);
            }
            Vec<ASR::dimension_t> empty_dim;
            empty_dim.reserve(al, 1);
            ASR::ttype_t* dim_less_type = ASRUtils::duplicate_type(al, x->m_type, &empty_dim);
            ASR::expr_t* op_el_wise = ASRUtils::EXPR(ASR::make_IntrinsicFunction_t(al, loc,
                x->m_intrinsic_id, ref_args.p, ref_args.size(), x->m_overload_id,
                dim_less_type, nullptr));
            ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
            ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, res, op_el_wise, nullptr));
            doloop_body.push_back(al, assign);
        });
        use_custom_loop_params = false;
        result_var = nullptr;
    }

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
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
        bool is_return_var_handled = false;
        ASR::symbol_t *fn_name = ASRUtils::symbol_get_past_external(x->m_name);
        if (ASR::is_a<ASR::Function_t>(*fn_name)) {
            ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(fn_name);
            is_return_var_handled = fn->m_return_var == nullptr;
        }
        if (is_return_var_handled) {
            bool is_dimension_empty = false;
            ASR::ttype_t* result_var_type = x->m_type;
            ASR::dimension_t* m_dims = nullptr;
            size_t n_dims = ASRUtils::extract_dimensions_from_ttype(result_var_type, m_dims);
            for( size_t i = 0; i < n_dims; i++ ) {
                if( m_dims[i].m_length == nullptr ) {
                    is_dimension_empty = true;
                    break;
                }
            }
            if( result_type && is_dimension_empty ) {
                result_var_type = result_type;
            }
            bool is_allocatable = false;
            {
                ASR::storage_typeType storage = ASR::storage_typeType::Default;
                ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(fn_name);
                // Assuming the `m_return_var` is appended to the `args`.
                ASR::symbol_t *v_sym = ASR::down_cast<ASR::Var_t>(
                    fn->m_args[fn->n_args-1])->m_v;
                if (ASR::is_a<ASR::Variable_t>(*v_sym)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(v_sym);
                    storage = v->m_storage;
                    is_allocatable = v->m_storage == ASR::storage_typeType::Allocatable;
                }
                ASR::expr_t* result_var_ = PassUtils::create_var(result_counter,
                    "_func_call_res", loc, result_var_type, al, current_scope, storage);
                result_counter += 1;
                if( result_var == nullptr ) {
                    result_var = result_var_;
                    *current_expr = result_var;
                } else {
                    *current_expr = result_var_;
                }
            }
            if( op_expr == &(x->base) ) {
                op_dims = nullptr;
                op_n_dims = ASRUtils::extract_dimensions_from_ttype(
                    ASRUtils::expr_type(*current_expr), op_dims);
            }
            Vec<ASR::call_arg_t> s_args;
            s_args.reserve(al, x->n_args + 1);
            for( size_t i = 0; i < x->n_args; i++ ) {
                s_args.push_back(al, x->m_args[i]);
            }
            ASR::call_arg_t result_arg;
            result_arg.loc = result_var->base.loc;
            result_arg.m_value = *current_expr;
            s_args.push_back(al, result_arg);
            ASR::stmt_t* subrout_call = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, loc,
                                            x->m_name, nullptr, s_args.p, s_args.size(), nullptr));
            pass_result.push_back(al, subrout_call);
            if (is_allocatable && result_var != *current_expr) {
                Vec<ASR::alloc_arg_t> vec_alloc;
                vec_alloc.reserve(al, 1);
                ASR::alloc_arg_t alloc_arg;
                alloc_arg.loc = loc;
                alloc_arg.m_a = result_var;

                Vec<ASR::dimension_t> vec_dims;
                vec_dims.reserve(al, 1);
                ASR::dimension_t dim;
                dim.loc = loc;
                dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1,
                    ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0))));
                dim.m_length = PassUtils::get_bound(*current_expr, 1, "ubound", al);
                vec_dims.push_back(al, dim);

                alloc_arg.m_dims = vec_dims.p;
                alloc_arg.n_dims = vec_dims.n;
                vec_alloc.push_back(al, alloc_arg);
                pass_result.push_back(al, ASRUtils::STMT(ASR::make_Allocate_t(
                    al, loc, vec_alloc.p, 1, nullptr, nullptr, nullptr)));
            }
            apply_again = true;
            remove_original_statement = false;
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
            if( op_expr == &(x->base) ) {
                op_dims = nullptr;
                op_n_dims = ASRUtils::extract_dimensions_from_ttype(
                    ASRUtils::expr_type(*current_expr), op_dims);
            }

            Vec<ASR::expr_t*> idx_vars, loop_vars;
            std::vector<int> loop_var_indices;
            Vec<ASR::stmt_t*> doloop_body;
            create_do_loop(loc, common_rank,
                idx_vars, loop_vars, loop_var_indices, doloop_body,
                [=, &operands, &idx_vars, &doloop_body] () {
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
            });
            use_custom_loop_params = false;
        }
        result_var = nullptr;
    }


};

class ArrayOpVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArrayOpVisitor>
{
    private:

        Allocator& al;
        bool use_custom_loop_params;
        bool remove_original_statement;
        ReplaceArrayOp replacer;
        Vec<ASR::stmt_t*> pass_result;
        Vec<ASR::expr_t*> result_lbound, result_ubound, result_inc;
        Vec<ASR::stmt_t*>* parent_body;
        std::map<ASR::expr_t*, ASR::expr_t*> resultvar2value;

    public:

        bool apply_again;

        ArrayOpVisitor(Allocator& al_) :
        al(al_), use_custom_loop_params(false),
        remove_original_statement(false),
        replacer(al_, pass_result, use_custom_loop_params,
                 apply_again, remove_original_statement,
                 result_lbound, result_ubound, result_inc,
                 resultvar2value),
        parent_body(nullptr), apply_again(false) {
            pass_result.n = 0;
            result_lbound.n = 0;
            result_ubound.n = 0;
            result_inc.n = 0;
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            if( parent_body ) {
                for (size_t j=0; j < pass_result.size(); j++) {
                    parent_body->push_back(al, pass_result[j]);
                }
            }
            for (size_t i=0; i<n_body; i++) {
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                remove_original_statement = false;
                replacer.result_var = nullptr;
                replacer.result_type = nullptr;
                Vec<ASR::stmt_t*>* parent_body_copy = parent_body;
                parent_body = &body;
                visit_stmt(*m_body[i]);
                parent_body = parent_body_copy;
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
            replacer.result_type = nullptr;
            pass_result.n = 0;
        }

        // TODO: Only Program and While is processed, we need to process all calls
        // to visit_stmt().
        // TODO: Only TranslationUnit's and Program's symbol table is processed
        // for transforming function->subroutine if they return arrays
        void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_global_scope;
            // Transform functions returning arrays to subroutines
            for (auto &item : x.m_global_scope->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    PassUtils::handle_fn_return_var(al,
                        ASR::down_cast<ASR::Function_t>(item.second),
                        PassUtils::is_array);
                }
            }

            std::vector<std::string> build_order
                = ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_global_scope->get_symbol(item));
                ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                visit_symbol(*mod);
            }

            // Now visit everything else
            for (auto &item : x.m_global_scope->get_scope()) {
                if (!ASR::is_a<ASR::Module_t>(*item.second)) {
                    this->visit_symbol(*item.second);
                }
            }
            current_scope = current_scope_copy;
        }

        void visit_Module(const ASR::Module_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_symtab;
            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    PassUtils::handle_fn_return_var(al,
                        ASR::down_cast<ASR::Function_t>(item.second),
                        PassUtils::is_array);
                }
            }

            // Now visit everything else
            for (auto &item : x.m_symtab->get_scope()) {
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

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    PassUtils::handle_fn_return_var(al,
                        ASR::down_cast<ASR::Function_t>(item.second),
                        PassUtils::is_array);
                }
            }

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::AssociateBlock_t>(*item.second)) {
                    ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                    visit_AssociateBlock(*s);
                }
                if (is_a<ASR::Function_t>(*item.second)) {
                    visit_Function(*ASR::down_cast<ASR::Function_t>(
                        item.second));
                }
            }

            transform_stmts(xx.m_body, xx.n_body);
            current_scope = current_scope_copy;
        }

        inline void visit_AssignmentUtil(const ASR::Assignment_t& x) {
            ASR::expr_t** current_expr_copy_9 = current_expr;
            ASR::expr_t* original_value = x.m_value;
            resultvar2value[replacer.result_var] = original_value;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            this->call_replacer();
            current_expr = current_expr_copy_9;
            if( x.m_value == original_value ) {
                ASR::expr_t* result_var_copy = replacer.result_var;
                replacer.result_var = nullptr;
                this->visit_expr(*x.m_value);
                replacer.result_var = result_var_copy;
                remove_original_statement = false;
            } else if( x.m_value ) {
                this->visit_expr(*x.m_value);
            }
            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
            }
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            if( (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
                ASR::is_a<ASR::GetPointer_t>(*x.m_value)) ||
                (ASR::is_a<ASR::ArrayConstant_t>(*x.m_value)) ||
                (ASR::is_a<ASR::StructInstanceMember_t>(*x.m_target) &&
                 ASRUtils::is_array(ASRUtils::expr_type(x.m_value)) &&
                 ASRUtils::is_array(ASRUtils::expr_type(x.m_target))) ) { // TODO: fix for StructInstanceMember targets
                return ;
            }

            if( ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ) {
                visit_AssignmentUtil(x);
                return ;
            }

            if( PassUtils::is_array(x.m_target) ) {
                replacer.result_var = x.m_target;
                replacer.result_type = ASRUtils::expr_type(x.m_target);
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

        void visit_Associate(const ASR::Associate_t& /*x*/) {

        }

};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit,
                           const LCompilers::PassOptions& /*pass_options*/) {
    ArrayOpVisitor v(al);
    do {
        v.apply_again = false;
        v.visit_TranslationUnit(unit);
    } while( v.apply_again );
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
