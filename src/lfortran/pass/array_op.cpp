#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/array_op.h>
#include <lfortran/pass/pass_utils.h>

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
If any child contains operations over arrays then, the a do loop
pass is added for performing the operation element wise. For stroing
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

struct dimension_descriptor
{
    int lbound, ubound;
};

class ArrayOpVisitor : public ASR::BaseWalkVisitor<ArrayOpVisitor>
{
private:
    Allocator &al;
    ASR::TranslationUnit_t &unit;
    Vec<ASR::stmt_t*> array_op_result;

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

    /*
        This integer just maintains the count of new result
        variables created. It helps in uniquely identifying
        the variables and avoids clash with already existing
        variables defined by the user.
    */
    int result_var_num;

    /*
        It stores the address of symbol table of current scope,
        which can be program, function, subroutine or even
        global scope.
    */
    SymbolTable* current_scope;

public:
    ArrayOpVisitor(Allocator &al, ASR::TranslationUnit_t &unit) : al{al}, unit{unit}, 
    tmp_val{nullptr}, result_var{nullptr}, result_var_num{0},
    current_scope{nullptr} {
        array_op_result.reserve(al, 1);
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            array_op_result.n = 0;
            visit_stmt(*m_body[i]);
            if (array_op_result.size() > 0) {
                for (size_t j=0; j<array_op_result.size(); j++) {
                    body.push_back(al, array_op_result[j]);
                }
                array_op_result.n = 0;
            } else {
                body.push_back(al, m_body[i]);
            }
        }
        m_body = body.p;
        n_body = body.size();
    }

    // TODO: Only Program and While is processed, we need to process all calls
    // to visit_stmt().

    void visit_Program(const ASR::Program_t &x) {
        std::vector<std::pair<std::string, ASR::symbol_t*>> replace_vec;
        // Transform nested functions and subroutines
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
                /*
                * A function which returns an array will be converted
                * to a subroutine with the destination array as the last
                * argument. This helps in avoiding deep copies and the
                * destination memory directly gets filled inside the subroutine.
                */
                if( PassUtils::is_array(s->m_return_var) ) {
                    for( auto& s_item: s->m_symtab->scope ) {
                        ASR::symbol_t* curr_sym = s_item.second;
                        if( curr_sym->type == ASR::symbolType::Variable ) {
                            ASR::Variable_t* var = down_cast<ASR::Variable_t>(curr_sym);
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
                    a_args.push_back(al, s->m_return_var);
                    ASR::asr_t* s_sub_asr = ASR::make_Subroutine_t(al, s->base.base.loc, s->m_symtab, 
                                                    s->m_name, a_args.p, a_args.size(), s->m_body, s->n_body, 
                                                    s->m_abi, s->m_access, s->m_deftype);
                    ASR::symbol_t* s_sub = ASR::down_cast<ASR::symbol_t>(s_sub_asr);
                    replace_vec.push_back(std::make_pair(item.first, s_sub));
                }
            }
        }

        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
        current_scope = xx.m_symtab;
        // Updating the symbol table so that the now the name
        // of the function (which returned array) now points
        // to the newly created subroutine.
        for( auto& item: replace_vec ) {
            current_scope->scope[item.first] = item.second;
        }
        transform_stmts(xx.m_body, xx.n_body);

    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Subroutine_t &xx = const_cast<ASR::Subroutine_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( PassUtils::is_array(x.m_target) ) {
            result_var = x.m_target;
            this->visit_expr(*(x.m_value));
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
                m_dims[i].m_end != nullptr ) {
                return sibling_type;
            }
        }
        Vec<ASR::dimension_t> new_m_dims;
        new_m_dims.reserve(al, ndims);
        for( int i = 0; i < ndims; i++ ) {
            ASR::dimension_t new_m_dim;
            new_m_dim.loc = m_dims[i].loc;
            new_m_dim.m_start = PassUtils::get_bound(sibling, i + 1, "lbound", 
                                                     al, unit, current_scope);
            new_m_dim.m_end = PassUtils::get_bound(sibling, i + 1, "ubound",
                                                    al, unit, current_scope);
            new_m_dims.push_back(al, new_m_dim);
        }
        return PassUtils::set_dim_rank(sibling_type, new_m_dims.p, ndims, true, &al);
    }

    ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                            ASR::expr_t* sibling) {
        ASR::expr_t* idx_var = nullptr;
        ASR::ttype_t* var_type = get_matching_type(sibling);
        Str str_name;
        str_name.from_str(al, "~" + std::to_string(counter) + suffix);
        const char* const_idx_var_name = str_name.c_str(al);
        char* idx_var_name = (char*)const_idx_var_name;

        if( current_scope->scope.find(std::string(idx_var_name)) == current_scope->scope.end() ) {
            ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, idx_var_name, 
                                                    ASR::intentType::Local, nullptr, ASR::storage_typeType::Default, 
                                                    var_type, ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required);
            current_scope->scope[std::string(idx_var_name)] = ASR::down_cast<ASR::symbol_t>(idx_sym);
            idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
        } else {
            ASR::symbol_t* idx_sym = current_scope->scope[std::string(idx_var_name)];
            idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));
        }

        return idx_var;
    }

    void visit_ConstantInteger(const ASR::ConstantInteger_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_ConstantComplex(const ASR::ConstantComplex_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_ConstantReal(const ASR::ConstantReal_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void fix_dimension(const ASR::ImplicitCast_t& x, ASR::expr_t* arg_expr) {
        ASR::ttype_t* x_type = const_cast<ASR::ttype_t*>(x.m_type);
        ASR::ttype_t* arg_type = LFortran::ASRUtils::expr_type(arg_expr);
        ASR::dimension_t* m_dims;
        int ndims;
        PassUtils::get_dim_rank(arg_type, m_dims, ndims);
        PassUtils::set_dim_rank(x_type, m_dims, ndims);
    }

    void visit_ImplicitCast(const ASR::ImplicitCast_t& x) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_arg));
        result_var = result_var_copy;
        if( PassUtils::is_array(tmp_val) ) {
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
                head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al, unit, current_scope);
                head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al, unit, current_scope);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(tmp_val, idx_vars, al);
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::expr_t* impl_cast_el_wise = LFortran::ASRUtils::EXPR(ASR::make_ImplicitCast_t(al, x.base.base.loc, ref, x.m_kind, x.m_type));
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, impl_cast_el_wise));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            array_op_result.push_back(al, doloop);
            tmp_val = result_var;
        } else {
            tmp_val = const_cast<ASR::expr_t*>(&(x.base));
        }
    }

    void visit_Var(const ASR::Var_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_UnaryOp(const ASR::UnaryOp_t& x) {
        std::string res_prefix = "_unary_op_res";
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_operand));
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
                head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al, unit, current_scope);
                head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al, unit, current_scope);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(operand, idx_vars, al);
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::expr_t* op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_UnaryOp_t(
                                                    al, x.base.base.loc, 
                                                    x.m_op, ref, x.m_type));
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, op_el_wise));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            array_op_result.push_back(al, doloop);
        }
    }

    template <typename T>
    void visit_ArrayOpCommon(const T& x, std::string res_prefix) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_left));
        ASR::expr_t* left = tmp_val;
        result_var = nullptr;
        this->visit_expr(*(x.m_right));
        ASR::expr_t* right = tmp_val;
        int rank_left = PassUtils::get_rank(left);
        int rank_right = PassUtils::get_rank(right);
        if( rank_left == 0 && rank_right == 0 ) {
            tmp_val = const_cast<ASR::expr_t*>(&(x.base));
            return ;
        }
        if( rank_left > 0 && rank_right > 0 ) {
            if( rank_left != rank_right ) {
                throw SemanticError("Cannot generate loop for operands of different shapes", 
                                    x.base.base.loc);
            }
            result_var = result_var_copy;
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, res_prefix, x.base.base.loc, left);
                result_var_num += 1;
            }
            tmp_val = result_var;

            int n_dims = rank_left;
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al, unit, current_scope);
                head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al, unit, current_scope);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref_1 = PassUtils::create_array_ref(left, idx_vars, al);
                    ASR::expr_t* ref_2 = PassUtils::create_array_ref(right, idx_vars, al);
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::expr_t* op_el_wise = nullptr;
                    switch( x.class_type ) {
                        case ASR::exprType::BinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(
                                                al, x.base.base.loc, 
                                                ref_1, (ASR::binopType)x.m_op, ref_2, x.m_type));
                            break;
                        case ASR::exprType::Compare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(
                                                al, x.base.base.loc, 
                                                ref_1, (ASR::cmpopType)x.m_op, ref_2, x.m_type));
                            break;
                        case ASR::exprType::BoolOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_BoolOp_t(
                                                al, x.base.base.loc, 
                                                ref_1, (ASR::boolopType)x.m_op, ref_2, x.m_type));
                            break;
                        default:
                            throw SemanticError("The desired operation is not supported yet for arrays.",
                                                x.base.base.loc);
                    }
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, op_el_wise));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            array_op_result.push_back(al, doloop);
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

            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = PassUtils::get_bound(result_var, i + 1, "lbound", al, unit, current_scope);
                head.m_end = PassUtils::get_bound(result_var, i + 1, "ubound", al, unit, current_scope);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(arr_expr, idx_vars, al);
                    ASR::expr_t* res = PassUtils::create_array_ref(result_var, idx_vars, al);
                    ASR::expr_t* op_el_wise = nullptr;
                    switch( x.class_type ) {
                        case ASR::exprType::BinOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(
                                                    al, x.base.base.loc, 
                                                    ref, (ASR::binopType)x.m_op, other_expr, x.m_type));
                            break;
                        case ASR::exprType::Compare:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(
                                                    al, x.base.base.loc, 
                                                    ref, (ASR::cmpopType)x.m_op, other_expr, x.m_type));
                            break;
                        case ASR::exprType::BoolOp:
                            op_el_wise = LFortran::ASRUtils::EXPR(ASR::make_BoolOp_t(
                                                    al, x.base.base.loc, 
                                                    ref, (ASR::boolopType)x.m_op, other_expr, x.m_type));
                            break;
                        default:
                            throw SemanticError("The desired operation is not supported yet for arrays.",
                                                x.base.base.loc);
                    }
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, op_el_wise));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            array_op_result.push_back(al, doloop);
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        visit_ArrayOpCommon<ASR::BinOp_t>(x, "_bin_op_res");
    }

    void visit_Compare(const ASR::Compare_t &x) {
        visit_ArrayOpCommon<ASR::Compare_t>(x, "_comp_op_res");
    }

    void visit_BoolOp(const ASR::BoolOp_t &x) {
        visit_ArrayOpCommon<ASR::BoolOp_t>(x, "_bool_op_res");
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
        if( current_scope != nullptr && 
            current_scope->scope.find(x_name) != current_scope->scope.end() &&
            current_scope->scope[x_name]->type == ASR::symbolType::Subroutine ) {
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, "_func_call_res", x.base.base.loc, x.m_args[x.n_args - 1]);
                result_var_num += 1;
            }
            Vec<ASR::expr_t*> s_args;
            s_args.reserve(al, x.n_args + 1);
            for( size_t i = 0; i < x.n_args; i++ ) {
                s_args.push_back(al, x.m_args[i]);
            }
            s_args.push_back(al, result_var);
            tmp_val = result_var;
            ASR::stmt_t* subrout_call = LFortran::ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc,
                                                current_scope->scope[x_name], nullptr, 
                                                s_args.p, s_args.size()));
            array_op_result.push_back(al, subrout_call);
        }
    }

};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrayOpVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
