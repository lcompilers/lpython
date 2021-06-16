#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/array_op.h>


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
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body);

        // Transform nested functions and subroutines
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }
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

    inline int get_rank(ASR::expr_t* x) {
        int n_dims = 0;
        ASR::ttype_t* x_type = expr_type(x);
        if( x->type == ASR::exprType::Var ) {
            ASR::Var_t* x_var = ASR::down_cast<ASR::Var_t>(x);
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(symbol_get_past_external(x_var->m_v));
            switch( v->m_type->type ) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* x_type_ref = down_cast<ASR::Integer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::IntegerPointer: {
                    ASR::IntegerPointer_t* x_type_ref = down_cast<ASR::IntegerPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* x_type_ref = down_cast<ASR::Real_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::RealPointer: {
                    ASR::RealPointer_t* x_type_ref = down_cast<ASR::RealPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* x_type_ref = down_cast<ASR::Complex_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::ComplexPointer: {
                    ASR::ComplexPointer_t* x_type_ref = down_cast<ASR::ComplexPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* x_type_ref = down_cast<ASR::Derived_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::DerivedPointer: {
                    ASR::DerivedPointer_t* x_type_ref = down_cast<ASR::DerivedPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* x_type_ref = down_cast<ASR::Logical_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::LogicalPointer: {
                    ASR::LogicalPointer_t* x_type_ref = down_cast<ASR::LogicalPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* x_type_ref = down_cast<ASR::Character_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::CharacterPointer: {
                    ASR::CharacterPointer_t* x_type_ref = down_cast<ASR::CharacterPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                default:
                    break;
            }
        }
        return n_dims;
    }

    inline bool is_array(ASR::expr_t* x) {
        return get_rank(x) > 0;
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( is_array(x.m_target) ) {
            result_var = x.m_target;
            this->visit_expr(*(x.m_value));
        }
    }

    ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars) {
        ASR::Var_t* arr_var = down_cast<ASR::Var_t>(arr_expr);
        ASR::symbol_t* arr = arr_var->m_v;
        Vec<ASR::array_index_t> args;
        args.reserve(al, 1);
        for( size_t i = 0; i < idx_vars.size(); i++ ) {
            ASR::array_index_t ai;
            ai.loc = arr_expr->base.loc;
            ai.m_left = nullptr;
            ai.m_right = idx_vars[i];
            ai.m_step = nullptr;
            args.push_back(al, ai);
        }
        ASR::expr_t* array_ref = EXPR(ASR::make_ArrayRef_t(al, arr_expr->base.loc, arr, 
                                                            args.p, args.size(), 
                                                            expr_type(EXPR((ASR::asr_t*)arr_var))));
        return array_ref;
    }

    ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                            ASR::ttype_t* var_type) {
        ASR::expr_t* idx_var = nullptr;

        Str str_name;
        str_name.from_str(al, std::to_string(counter) + suffix);
        const char* const_idx_var_name = str_name.c_str(al);
        char* idx_var_name = (char*)const_idx_var_name;

        if( current_scope->scope.find(std::string(idx_var_name)) == current_scope->scope.end() ) {
            ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, idx_var_name, 
                                                    ASR::intentType::Local, nullptr, ASR::storage_typeType::Default, 
                                                    var_type, ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required);
            current_scope->scope[std::string(idx_var_name)] = ASR::down_cast<ASR::symbol_t>(idx_sym);
            idx_var = EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
        } else {
            ASR::symbol_t* idx_sym = current_scope->scope[std::string(idx_var_name)];
            idx_var = EXPR(ASR::make_Var_t(al, loc, idx_sym));
        }

        return idx_var;
    }

    void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc, std::string suffix=std::string("_k")) {
        idx_vars.reserve(al, n_dims);
        ASR::ttype_t* int32_type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        for( int i = 1; i <= n_dims; i++ ) {
            ASR::expr_t* idx_var = create_var(i, suffix, loc, int32_type);
            idx_vars.push_back(al, idx_var);
        }
    }

    ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim, std::string bound) {
        ASR::symbol_t *v;
        std::string remote_sym = bound;
        std::string module_name = "lfortran_intrinsic_array";
        SymbolTable* current_scope_copy = current_scope;
        current_scope = unit.m_global_scope;
        ASR::Module_t *m = load_module(al, current_scope,
                                        module_name, arr_expr->base.loc, true);

        ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
        ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
        ASR::asr_t *fn = ASR::make_ExternalSymbol_t(al, mfn->base.base.loc, current_scope,
                                                    mfn->m_name, (ASR::symbol_t*)mfn,
                                                    m->m_name, mfn->m_name, ASR::accessType::Private);
        std::string sym = mfn->m_name;
        if( current_scope->scope.find(sym) != current_scope->scope.end() ) {
            v = current_scope->scope[sym];
        } else {
            current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
            v = ASR::down_cast<ASR::symbol_t>(fn);
        }
        Vec<ASR::expr_t*> args;
        args.reserve(al, 2);
        args.push_back(al, arr_expr);
        ASR::expr_t* const_1 = EXPR(ASR::make_ConstantInteger_t(al, arr_expr->base.loc, dim, expr_type(mfn->m_args[1])));
        args.push_back(al, const_1);
        ASR::ttype_t *type = EXPR2VAR(ASR::down_cast<ASR::Function_t>(
                                     symbol_get_past_external(v))->m_return_var)->m_type;
        current_scope = current_scope_copy;
        return EXPR(ASR::make_FunctionCall_t(al, arr_expr->base.loc, v, nullptr,
                                             args.p, args.size(), nullptr, 0, type));
    }

    void visit_ConstantInteger(const ASR::ConstantInteger_t& x) {
        tmp_val = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_ImplicitCast(const ASR::ImplicitCast_t& x) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_arg));
        result_var = result_var_copy;
        if( is_array(tmp_val) ) {
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, std::string("_implicit_cast_res"), x.base.base.loc, x.m_type);
                result_var_num += 1;
            }
            int n_dims = get_rank(result_var);
            Vec<ASR::expr_t*> idx_vars;
            create_idx_vars(idx_vars, n_dims, x.base.base.loc);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = get_bound(result_var, i + 1, "lbound");
                head.m_end = get_bound(result_var, i + 1, "ubound");
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = create_array_ref(tmp_val, idx_vars);
                    ASR::expr_t* res = create_array_ref(result_var, idx_vars);
                    ASR::expr_t* impl_cast_el_wise = EXPR(ASR::make_ImplicitCast_t(al, x.base.base.loc, ref, x.m_kind, x.m_type));
                    ASR::stmt_t* assign = STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, impl_cast_el_wise));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
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

    void visit_BinOp(const ASR::BinOp_t &x) {
        ASR::expr_t* result_var_copy = result_var;
        result_var = nullptr;
        this->visit_expr(*(x.m_left));
        ASR::expr_t* left = tmp_val;
        result_var = nullptr;
        this->visit_expr(*(x.m_right));
        ASR::expr_t* right = tmp_val;
        int rank_left = get_rank(left);
        int rank_right = get_rank(right);
        if( rank_left == 0 && rank_right == 0 ) {
            tmp_val = const_cast<ASR::expr_t*>(&(x.base));
            return ;
        }
        if( rank_left > 0 && rank_right > 0 ) {
            if( rank_left != rank_right ) {
                throw SemanticError("Cannot generate loop operands of different shapes", 
                                    x.base.base.loc);
            }
            result_var = result_var_copy;
            if( result_var == nullptr ) {
                result_var = create_var(result_var_num, std::string("_bin_op_res"), x.base.base.loc, expr_type(left));
                result_var_num += 1;
            }
            tmp_val = result_var;

            int n_dims = rank_left;
            Vec<ASR::expr_t*> idx_vars;
            create_idx_vars(idx_vars, n_dims, x.base.base.loc);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = get_bound(result_var, i + 1, "lbound");
                head.m_end = get_bound(result_var, i + 1, "ubound");
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref_1 = create_array_ref(left, idx_vars);
                    ASR::expr_t* ref_2 = create_array_ref(right, idx_vars);
                    ASR::expr_t* res = create_array_ref(result_var, idx_vars);
                    ASR::expr_t* bin_op_el_wise = EXPR(ASR::make_BinOp_t(al, x.base.base.loc, ref_1, x.m_op, ref_2, x.m_type));
                    ASR::stmt_t* assign = STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, bin_op_el_wise));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
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
                result_var = create_var(result_var_num, std::string("_bin_op_res"), x.base.base.loc, expr_type(arr_expr));
                result_var_num += 1;
            }
            tmp_val = result_var;

            Vec<ASR::expr_t*> idx_vars;
            create_idx_vars(idx_vars, n_dims, x.base.base.loc);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = get_bound(result_var, i + 1, "lbound");
                head.m_end = get_bound(result_var, i + 1, "ubound");
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = create_array_ref(arr_expr, idx_vars);
                    ASR::expr_t* res = create_array_ref(result_var, idx_vars);
                    ASR::expr_t* bin_op_el_wise = EXPR(ASR::make_BinOp_t(al, x.base.base.loc, ref, x.m_op, other_expr, x.m_type));
                    ASR::stmt_t* assign = STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, bin_op_el_wise));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            array_op_result.push_back(al, doloop);
        }
    }
};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrayOpVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
