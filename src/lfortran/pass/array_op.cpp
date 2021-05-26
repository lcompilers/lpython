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
    bool apply_pass;
    ASR::expr_t* m_target;

public:
    ArrayOpVisitor(Allocator &al, ASR::TranslationUnit_t &unit) : al{al}, unit{unit}, apply_pass{false}, m_target{nullptr} {
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
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    inline void get_dims(ASR::expr_t* x, Vec<dimension_descriptor>& result) {
        result.reserve(al, 0);
        ASR::dimension_t* m_dims = nullptr;
        int n_dims = 0;
        ASR::ttype_t* x_type = expr_type(x);
        if( x->type == ASR::exprType::Var ) {
            ASR::Var_t* x_var = ASR::down_cast<ASR::Var_t>(x);
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(symbol_get_past_external(x_var->m_v));
            switch( v->m_type->type ) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* x_type_ref = down_cast<ASR::Integer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::IntegerPointer: {
                    ASR::IntegerPointer_t* x_type_ref = down_cast<ASR::IntegerPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* x_type_ref = down_cast<ASR::Real_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::RealPointer: {
                    ASR::RealPointer_t* x_type_ref = down_cast<ASR::RealPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* x_type_ref = down_cast<ASR::Complex_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::ComplexPointer: {
                    ASR::ComplexPointer_t* x_type_ref = down_cast<ASR::ComplexPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* x_type_ref = down_cast<ASR::Derived_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::DerivedPointer: {
                    ASR::DerivedPointer_t* x_type_ref = down_cast<ASR::DerivedPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* x_type_ref = down_cast<ASR::Logical_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::LogicalPointer: {
                    ASR::LogicalPointer_t* x_type_ref = down_cast<ASR::LogicalPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* x_type_ref = down_cast<ASR::Character_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::CharacterPointer: {
                    ASR::CharacterPointer_t* x_type_ref = down_cast<ASR::CharacterPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                default:
                    break;
            }
            if( n_dims > 0 ) {
                for( int i = 0; i < n_dims; i++ ) {
                    int lbound = -1, ubound = -1;
                    if( m_dims[i].m_start->type == ASR::exprType::ConstantInteger ) {
                        ASR::ConstantInteger_t* m_start = down_cast<ASR::ConstantInteger_t>(m_dims[i].m_start);
                        lbound = m_start->m_n;
                    }
                    if( m_dims[i].m_end->type == ASR::exprType::ConstantInteger ) {
                        ASR::ConstantInteger_t* m_end = down_cast<ASR::ConstantInteger_t>(m_dims[i].m_end);
                        ubound = m_end->m_n;
                    } 
                    if( lbound != -1 && ubound != -1 ) {
                        dimension_descriptor new_dim;
                        new_dim.lbound = lbound;
                        new_dim.ubound = ubound;
                        result.push_back(al, new_dim);
                    } else {
                        break;
                    }
                }
            }
        }
    }

    inline bool is_array(ASR::expr_t* x) {
        Vec<dimension_descriptor> result;
        get_dims(x, result);
        return result.size() > 0;
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( is_array(x.m_target) ) {
            apply_pass = true;
            m_target = x.m_target;
            this->visit_expr(*(x.m_value));
            m_target = nullptr;
            apply_pass = false;
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

    void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc) {
        idx_vars.reserve(al, n_dims);
        for( int i = 1; i <= n_dims; i++ ) {
            Str str_name;
            str_name.from_str(al, std::to_string(i) + std::string("_k"));
            const char* const_idx_var_name = str_name.c_str(al);
            char* idx_var_name = (char*)const_idx_var_name;
            ASR::expr_t* idx_var = nullptr;
            ASR::ttype_t* int32_type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            if( unit.m_global_scope->scope.find(std::string(idx_var_name)) == unit.m_global_scope->scope.end() ) {
                ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, unit.m_global_scope, idx_var_name, 
                                                        ASR::intentType::Local, nullptr, ASR::storage_typeType::Default, 
                                                        int32_type, ASR::abiType::Source, ASR::accessType::Public);
                unit.m_global_scope->scope[std::string(idx_var_name)] = ASR::down_cast<ASR::symbol_t>(idx_sym);
                idx_var = EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
            } else {
                ASR::symbol_t* idx_sym = unit.m_global_scope->scope[std::string(idx_var_name)];
                idx_var = EXPR(ASR::make_Var_t(al, loc, idx_sym));
            }
            idx_vars.push_back(al, idx_var);
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        if( !apply_pass ) {
            return ;
        }
        // Case #1: For static arrays in both the operands
        Vec<dimension_descriptor> dims_left_vec;
        get_dims(x.m_left, dims_left_vec);
        Vec<dimension_descriptor> dims_right_vec;
        get_dims(x.m_right, dims_right_vec);
        if( dims_left_vec.size() != dims_right_vec.size() ) {
            throw SemanticError("Cannot generate loop operands of different shapes", 
                                x.base.base.loc);
        }
        if( dims_left_vec.size() <= 0 ) {
            return ;
        }

        dimension_descriptor* dims_left = dims_left_vec.p;
        int n_dims = dims_left_vec.size();
        dimension_descriptor* dims_right = dims_right_vec.p;
        ASR::ttype_t* int32_type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
        Vec<ASR::expr_t*> idx_vars;
        create_idx_vars(idx_vars, n_dims, x.base.base.loc);
        ASR::stmt_t* doloop = nullptr;
        for( int i = n_dims - 1; i >= 0; i-- ) {
            if( dims_left[i].lbound != dims_right[i].lbound ||
                dims_left[i].ubound != dims_right[i].ubound ) {
                throw SemanticError("Incompatible shapes of operand arrays", x.base.base.loc);
            }
            ASR::do_loop_head_t head;
            head.m_v = idx_vars[i];
            head.m_start = EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, dims_left[i].lbound, int32_type)); // TODO: Replace with call to lbound
            head.m_end = EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, dims_left[i].ubound, int32_type)); // TODO: Replace with call to ubound
            head.m_increment = nullptr;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                ASR::expr_t* ref_1 = create_array_ref(x.m_left, idx_vars);
                ASR::expr_t* ref_2 = create_array_ref(x.m_right, idx_vars);
                ASR::expr_t* res = create_array_ref(m_target, idx_vars);
                ASR::expr_t* bin_op_el_wise = EXPR(ASR::make_BinOp_t(al, x.base.base.loc, ref_1, x.m_op, ref_2, x.m_type));
                ASR::stmt_t* assign = STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, bin_op_el_wise));
                doloop_body.push_back(al, assign);
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
        }
        array_op_result.push_back(al, doloop);
    }
};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrayOpVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
