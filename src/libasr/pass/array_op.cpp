#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/array_op.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

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

class ArrayOpVisitor : public PassUtils::PassVisitor<ArrayOpVisitor>
{
private:

    /*
        This pointer stores the result of a node.
        Specifically if a node or its child contains
        operations performed over arrays then it points
        to the new variable which will store the result
        of that operation once the code is compiled.
    */
    ASR::expr_t *tmp_val;

    /*
        This pointer is intened to be a signal for the current
        node to create a new variable for storing the result of
        array operation or store it in a variable available from
        the parent node. For example, if BinOp is a child of the
        Assignment node then, the following will point to the target
        attribute of the assignment node. This helps in avoiding
        unnecessary do loop passes.
    */
    ASR::expr_t *result_var;
    Vec<ASR::expr_t*> result_lbound, result_ubound, result_inc;
    bool use_custom_loop_params;

    /*
        This integer just maintains the count of new result
        variables created. It helps in uniquely identifying
        the variables and avoids clash with already existing
        variables defined by the user.
    */
    int result_var_num;

    std::string rl_path;

public:
    ArrayOpVisitor(Allocator &al,
        const std::string &rl_path) : PassVisitor(al, nullptr),
    tmp_val(nullptr), result_var(nullptr), use_custom_loop_params(false),
    result_var_num(0), rl_path(rl_path)
    {
        pass_result.reserve(al, 1);
        result_lbound.reserve(al, 1);
        result_ubound.reserve(al, 1);
        result_inc.reserve(al, 1);
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
        LFORTRAN_ASSERT(s->m_return_var)
        a_args.push_back(al, s->m_return_var);
        ASR::asr_t* s_sub_asr = ASR::make_Function_t(al, s->base.base.loc,
            s->m_symtab,
            s->m_name, s->m_dependencies, s->n_dependencies,
            a_args.p, a_args.size(), s->m_body, s->n_body,
            nullptr,
            s->m_abi, s->m_access, s->m_deftype, nullptr, false, false,
            false, s->m_inline, s->m_static,
            s->m_type_params, s->n_type_params,
            s->m_restrictions, s->n_restrictions,
            s->m_is_restriction);
        ASR::symbol_t* s_sub = ASR::down_cast<ASR::symbol_t>(s_sub_asr);
        return s_sub;
    }

    // TODO: Only Program and While is processed, we need to process all calls
    // to visit_stmt().
    // TODO: Only TranslationUnit's and Program's symbol table is processed
    // for transforming function->subroutine if they return arrays

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
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
    }

    void visit_Program(const ASR::Program_t &x) {
        std::vector<std::pair<std::string, ASR::symbol_t*> > replace_vec;
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
        current_scope = xx.m_symtab;

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

        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);

    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
             ASR::is_a<ASR::GetPointer_t>(*x.m_value)) ) {
            return ;
        }
        if( ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ) {
            this->visit_expr(*x.m_value);
            return ;
        }
        if( PassUtils::is_array(x.m_target) ) {
            result_var = x.m_target;
            this->visit_expr(*(x.m_value));
        } else if( ASR::is_a<ASR::ArraySection_t>(*x.m_target) ) {
            ASR::ArraySection_t* array_ref = ASR::down_cast<ASR::ArraySection_t>(x.m_target);
            result_var = array_ref->m_v;
            result_lbound.reserve(al, array_ref->n_args);
            result_ubound.reserve(al, array_ref->n_args);
            result_inc.reserve(al, array_ref->n_args);
            ASR::expr_t *m_start, *m_end, *m_increment;
            m_start = m_end = m_increment = nullptr;
            for( int i = 0; i < (int) array_ref->n_args; i++ ) {
                if( array_ref->m_args[i].m_step != nullptr ) {
                    if( array_ref->m_args[i].m_left == nullptr ) {
                        m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al);
                    } else {
                        m_start = array_ref->m_args[i].m_left;
                    }
                    if( array_ref->m_args[i].m_right == nullptr ) {
                        m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al);
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
            this->visit_expr(*(x.m_value));
        }
        result_var = nullptr;
    }

    void visit_ArrayReshape(const ASR::ArrayReshape_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
        if( ASRUtils::is_array(ASRUtils::expr_type(x.m_array)) &&
            !ASR::is_a<ASR::Var_t>(*x.m_array)) {
            result_var = nullptr;
            this->visit_expr(*x.m_array);
            if( tmp_val ) {
                ASR::ArrayReshape_t& xx = const_cast<ASR::ArrayReshape_t&>(x);
                xx.m_array = tmp_val;
                retain_original_stmt = true;
                remove_original_stmt = false;
            }
        }
    }

    ASR::ttype_t* get_matching_type(ASR::expr_t* sibling) {
        ASR::ttype_t* sibling_type = LFortran::ASRUtils::expr_type(sibling);
        if( sibling->type != ASR::exprType::Var ) {
            return sibling_type;
        }
        ASR::dimension_t* m_dims;
        int ndims;
        PassUtils::get_dim_rank(sibling_type, m_dims, ndims);
        for( int i = 0; i < ndims; i++ ) {
            if( m_dims[i].m_start != nullptr ||
                m_dims[i].m_length != nullptr ) {
                return sibling_type;
            }
        }
        Vec<ASR::dimension_t> new_m_dims;
        new_m_dims.reserve(al, ndims);
        for( int i = 0; i < ndims; i++ ) {
            ASR::dimension_t new_m_dim;
            new_m_dim.loc = m_dims[i].loc;
            new_m_dim.m_start = PassUtils::get_bound(sibling, i + 1, "lbound", al);
            new_m_dim.m_length = ASRUtils::compute_length_from_start_end(al, new_m_dim.m_start,
                                     PassUtils::get_bound(sibling, i + 1, "ubound", al));
            new_m_dims.push_back(al, new_m_dim);
        }
        return PassUtils::set_dim_rank(sibling_type, new_m_dims.p, ndims, true, &al);
    }

    ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                            ASR::ttype_t* var_type) {
        ASR::expr_t* idx_var = nullptr;
        Str str_name;
        str_name.from_str(al, "~" + std::to_string(counter) + suffix);
        const char* const_idx_var_name = str_name.c_str(al);
        char* idx_var_name = (char*)const_idx_var_name;

        if( current_scope->get_symbol(std::string(idx_var_name)) == nullptr ) {
            ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, idx_var_name, nullptr, 0,
                                                    ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                                    var_type, ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required,
                                                    false);
            current_scope->add_symbol(std::string(idx_var_name), ASR::down_cast<ASR::symbol_t>(idx_sym));
            idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
        } else {
            ASR::symbol_t* idx_sym = current_scope->get_symbol(std::string(idx_var_name));
            idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));
        }

        return idx_var;
    }

    ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                            ASR::expr_t* sibling) {
        ASR::ttype_t* var_type = get_matching_type(sibling);
        return create_var(counter, suffix, loc, var_type);
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_ComplexConstant(const ASR::ComplexConstant_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_RealConstant(const ASR::RealConstant_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void fix_dimension(const ASR::Cast_t& x, ASR::expr_t* arg_expr) {
        ASR::ttype_t* x_type = const_cast<ASR::ttype_t*>(x.m_type);
        ASR::ttype_t* arg_type = LFortran::ASRUtils::expr_type(arg_expr);
        ASR::dimension_t* m_dims;
        int ndims;
        PassUtils::get_dim_rank(arg_type, m_dims, ndims);
        PassUtils::set_dim_rank(x_type, m_dims, ndims);
    }

    void visit_Cast(const ASR::Cast_t& x) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_arg));
        result_var = result_var_copy;
        if( tmp_val != nullptr && PassUtils::is_array(tmp_val) ) {
            if( result_var == nullptr ) {
                fix_dimension(x, tmp_val);
                result_var = create_var(result_var_num, std::string("_implicit_cast_res"), x.base.base.loc, const_cast<ASR::expr_t*>(&(x.base)));
                result_var_num += 1;
            }
            int n_dims = PassUtils::get_rank(result_var);
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
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
                    ASR::expr_t* impl_cast_el_wise = LFortran::ASRUtils::EXPR(ASR::make_Cast_t(al, x.base.base.loc, ref, x.m_kind, x.m_type, nullptr));
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, impl_cast_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
            tmp_val = result_var;
        } else {
            tmp_val = const_cast<ASR::expr_t*>(&(x.base));
        }
    }

    void visit_Var(const ASR::Var_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
        if( result_var != nullptr && PassUtils::is_array(result_var) ) {
            int rank_var = PassUtils::get_rank(tmp_val);
            int n_dims = rank_var;
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
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
                        ref = PassUtils::create_array_ref(tmp_val, idx_vars, al);
                    } else {
                        ref = tmp_val;
                    }
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, ref, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
            tmp_val = nullptr;
        }
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        handle_UnaryOp(x, 0);
    }
    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        handle_UnaryOp(x, 1);
    }
    void visit_ComplexUnaryMinus(const ASR::ComplexUnaryMinus_t &x) {
        handle_UnaryOp(x, 2);
    }
    void visit_IntegerBitNot(const ASR::IntegerBitNot_t &x) {
        handle_UnaryOp(x, 3);
    }
    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        handle_UnaryOp(x, 4);
    }

    template<typename T>
    void handle_UnaryOp(const T& x, int unary_type) {
        std::string res_prefix = "_unary_op_res";
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_arg));
        ASR::expr_t* operand = tmp_val;
        int rank_operand = PassUtils::get_rank(operand);
        if( rank_operand == 0 ) {
            tmp_val = const_cast<ASR::expr_t*>(&(x.base));
            return ;
        }
        if( rank_operand > 0 ) {
            result_var = result_var_copy;
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, res_prefix,
                                        x.base.base.loc, operand);
                result_var_num += 1;
            }
            tmp_val = result_var;

            int n_dims = rank_operand;
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
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
                        op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_IntegerUnaryMinus_t(al, x.base.base.loc,
                            ref, x.m_type, nullptr));
                    } else if (unary_type == 1) {
                        op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_RealUnaryMinus_t(al, x.base.base.loc,
                            ref, x.m_type, nullptr));
                    } else if (unary_type == 2) {
                        op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_ComplexUnaryMinus_t(al, x.base.base.loc,
                            ref, x.m_type, nullptr));
                    } else if (unary_type == 3) {
                        op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_IntegerBitNot_t(al, x.base.base.loc,
                            ref, x.m_type, nullptr));
                    } else if (unary_type == 4) {
                        op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_LogicalNot_t(al, x.base.base.loc,
                            ref, x.m_type, nullptr));
                    }
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
        }
    }

    bool is_elemental(ASR::symbol_t* x) {
        x = ASRUtils::symbol_get_past_external(x);
        if( !ASR::is_a<ASR::Function_t>(*x) ) {
            return false;
        }
        return ASR::down_cast<ASR::Function_t>(x)->m_elemental;
    }

    template <typename T>
    void visit_ArrayOpCommon(const T& x, std::string res_prefix) {
        bool current_status = use_custom_loop_params;
        use_custom_loop_params = false;
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_left));
        ASR::expr_t* left = tmp_val;
        result_var = nullptr;
        this->visit_expr(*(x.m_right));
        ASR::expr_t* right = tmp_val;
        use_custom_loop_params = current_status;
        int rank_left = PassUtils::get_rank(left);
        int rank_right = PassUtils::get_rank(right);
        if( rank_left == 0 && rank_right == 0 ) {
            tmp_val = const_cast<ASR::expr_t*>(&(x.base));
            return ;
        }
        if( rank_left > 0 && rank_right > 0 ) {
            if( rank_left != rank_right ) {
                // This should be checked by verify() and thus should not happen
                throw LCompilersException("Cannot generate loop for operands of different shapes");
            }
            result_var = result_var_copy;
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, res_prefix, x.base.base.loc, left);
                result_var_num += 1;
            }
            tmp_val = result_var;

            int n_dims = rank_left;
            Vec<ASR::expr_t*> idx_vars, idx_vars_value;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope, "_t");
            PassUtils::create_idx_vars(idx_vars_value, n_dims, x.base.base.loc, al, current_scope, "_v");
            ASR::ttype_t* int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
            ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int32_type));
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
                    ASR::expr_t* op_el_wise = nullptr;
                    switch( x.class_type ) {
                        case ASR::exprType::IntegerBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                                                al, x.base.base.loc,
                                                ref_1, (ASR::binopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                        case ASR::exprType::RealBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_RealBinOp_t(
                                                al, x.base.base.loc,
                                                ref_1, (ASR::binopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                        case ASR::exprType::ComplexBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_ComplexBinOp_t(
                                                al, x.base.base.loc,
                                                ref_1, (ASR::binopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                        case ASR::exprType::LogicalBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_LogicalBinOp_t(
                                                al, x.base.base.loc,
                                                ref_1, (ASR::logicalbinopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                       case ASR::exprType::IntegerCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                                                    al, x.base.base.loc,
                                                    ref_1, (ASR::cmpopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                        case ASR::exprType::RealCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_RealCompare_t(
                                                    al, x.base.base.loc,
                                                    ref_1, (ASR::cmpopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                        case ASR::exprType::ComplexCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_ComplexCompare_t(
                                                    al, x.base.base.loc,
                                                    ref_1, (ASR::cmpopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                        case ASR::exprType::LogicalCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_LogicalCompare_t(
                                                    al, x.base.base.loc,
                                                    ref_1, (ASR::cmpopType)x.m_op, ref_2, x.m_type, nullptr));
                            break;
                        default:
                            throw LCompilersException("The desired operation is not supported yet for arrays.");
                    }
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    ASR::expr_t* idx_lb = PassUtils::get_bound(left, i + 1, "lbound", al);
                    ASR::stmt_t* set_to_one = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_value[i+1], idx_lb, nullptr));
                    doloop_body.push_back(al, set_to_one);
                    doloop_body.push_back(al, doloop);
                }
                ASR::expr_t* inc_expr = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x.base.base.loc, idx_vars_value[i],
                                                                ASR::binopType::Add, const_1, int32_type, nullptr));
                ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_value[i], inc_expr, nullptr));
                doloop_body.push_back(al, assign_stmt);
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            ASR::expr_t* idx_lb = PassUtils::get_bound(right, 1, "lbound", al);
            ASR::stmt_t* set_to_one = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_value[0], idx_lb, nullptr));
            pass_result.push_back(al, set_to_one);
            pass_result.push_back(al, doloop);
        } else if( (rank_left == 0 && rank_right > 0) ||
                   (rank_right == 0 && rank_left > 0) ) {
            result_var = result_var_copy;
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
                result_var = create_var(result_var_num, res_prefix, x.base.base.loc, arr_expr);
                result_var_num += 1;
            }
            tmp_val = result_var;

            Vec<ASR::expr_t*> idx_vars, idx_vars_value;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope, "_t");
            PassUtils::create_idx_vars(idx_vars_value, n_dims, x.base.base.loc, al, current_scope, "_v");
            ASR::ttype_t* int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
            ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int32_type));
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
                    ASR::expr_t* op_el_wise = nullptr;
                    switch( x.class_type ) {
                        case ASR::exprType::IntegerBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::binopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        case ASR::exprType::RealBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_RealBinOp_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::binopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        case ASR::exprType::ComplexBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_ComplexBinOp_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::binopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        case ASR::exprType::LogicalBinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_LogicalBinOp_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::logicalbinopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        case ASR::exprType::IntegerCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::cmpopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        case ASR::exprType::RealCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_RealCompare_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::cmpopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        case ASR::exprType::ComplexCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_ComplexCompare_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::cmpopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        case ASR::exprType::LogicalCompare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_LogicalCompare_t(
                                                    al, x.base.base.loc,
                                                    lexpr, (ASR::cmpopType)x.m_op, rexpr, x.m_type, nullptr));
                            break;
                        default:
                            throw LCompilersException("The desired operation is not supported yet for arrays.");
                    }
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    ASR::expr_t* op_expr = nullptr;
                    if( rank_left > 0 ) {
                        op_expr = left;
                    } else {
                        op_expr = right;
                    }
                    ASR::expr_t* idx_lb = PassUtils::get_bound(op_expr, i + 2, "lbound", al);
                    ASR::stmt_t* set_to_one = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc,
                                                idx_vars_value[i + 1], idx_lb, nullptr));
                    doloop_body.push_back(al, set_to_one);
                    doloop_body.push_back(al, doloop);
                }
                ASR::expr_t* inc_expr = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x.base.base.loc, idx_vars_value[i],
                                                                ASR::binopType::Add, const_1, int32_type, nullptr));
                ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_value[i], inc_expr, nullptr));
                doloop_body.push_back(al, assign_stmt);
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            ASR::expr_t* op_expr = nullptr;
            if( rank_left > 0 ) {
                op_expr = left;
            } else {
                op_expr = right;
            }
            ASR::expr_t* idx_lb = PassUtils::get_bound(op_expr, 1, "lbound", al);
            ASR::stmt_t* set_to_one = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc,
                                        idx_vars_value[0], idx_lb, nullptr));
            pass_result.push_back(al, set_to_one);
            pass_result.push_back(al, doloop);
        }
        result_var = nullptr;
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        visit_ArrayOpCommon<ASR::IntegerBinOp_t>(x, "_bin_op_res");
    }

    void visit_RealBinOp(const ASR::RealBinOp_t &x) {
        visit_ArrayOpCommon<ASR::RealBinOp_t>(x, "_bin_op_res");
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t &x) {
        visit_ArrayOpCommon<ASR::ComplexBinOp_t>(x, "_bin_op_res");
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t &x) {
        visit_ArrayOpCommon<ASR::LogicalBinOp_t>(x, "_bool_op_res");
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        visit_ArrayOpCommon<ASR::IntegerCompare_t>(x, "_comp_op_res");
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
        visit_ArrayOpCommon<ASR::RealCompare_t>(x, "_comp_op_res");
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t &x) {
        visit_ArrayOpCommon<ASR::ComplexCompare_t>(x, "_comp_op_res");
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
        visit_ArrayOpCommon<ASR::LogicalCompare_t>(x, "_comp_op_res");
    }

    void visit_ArraySize(const ASR::ArraySize_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
        std::string x_name;
        if( x.m_name->type == ASR::symbolType::ExternalSymbol ) {
            x_name = down_cast<ASR::ExternalSymbol_t>(x.m_name)->m_name;
        } else if( x.m_name->type == ASR::symbolType::Function ) {
            x_name = down_cast<ASR::Function_t>(x.m_name)->m_name;
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

        ASR::symbol_t *sub = current_scope->resolve_symbol(x_name);
        if (sub && ASR::is_a<ASR::Function_t>(*sub)
            && ASR::down_cast<ASR::Function_t>(sub)->m_return_var == nullptr) {
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, "_func_call_res",
                    x.base.base.loc, x.m_type);
                result_var_num += 1;
            }
            Vec<ASR::call_arg_t> s_args;
            s_args.reserve(al, x.n_args + 1);
            for( size_t i = 0; i < x.n_args; i++ ) {
                s_args.push_back(al, x.m_args[i]);
            }
            ASR::call_arg_t result_arg;
            result_arg.loc = result_var->base.loc;
            result_arg.m_value = result_var;
            s_args.push_back(al, result_arg);
            tmp_val = result_var;
            ASR::stmt_t* subrout_call = LFortran::ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc,
                                                sub, nullptr,
                                                s_args.p, s_args.size(), nullptr));
            pass_result.push_back(al, subrout_call);
        } else if( is_elemental(x.m_name) ) {
            std::vector<bool> array_mask(x.n_args, false);
            bool at_least_one_array = false;
            for( size_t iarg = 0; iarg < x.n_args; iarg++ ) {
                array_mask[iarg] = ASRUtils::is_array(
                    ASRUtils::expr_type(x.m_args[iarg].m_value));
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
            for( size_t iarg = 0; iarg < x.n_args; iarg++ ) {
                result_var = nullptr;
                this->visit_expr(*(x.m_args[iarg].m_value));
                operand = tmp_val;
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
                tmp_val = const_cast<ASR::expr_t*>(&(x.base));
                return ;
            }
            if( !are_all_rank_same ) {
                throw LCompilersException("Broadcasting support not yet available "
                                          "for different shape arrays.");
            }
            result_var = result_var_copy;
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, res_prefix,
                                        x.base.base.loc, operand);
                result_var_num += 1;
            }
            tmp_val = result_var;

            int n_dims = common_rank;
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
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
                    ref_args.reserve(al, x.n_args);
                    for( size_t iarg = 0; iarg < x.n_args; iarg++ ) {
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
                    ASR::ttype_t* dim_less_type = ASRUtils::duplicate_type(al, x.m_type, &empty_dim);
                    ASR::expr_t* op_el_wise = nullptr;
                    op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_FunctionCall_t(al, x.base.base.loc,
                                    x.m_name, x.m_original_name, ref_args.p, ref_args.size(), dim_less_type,
                                    nullptr, x.m_dt));
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, op_el_wise, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
        }
        result_var = nullptr;
    }

};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit,
                           const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    ArrayOpVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LFortran
