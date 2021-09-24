#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/implied_do_loops.h>
#include <lfortran/pass/pass_utils.h>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*
This ASR pass replaces implied do loops with do loops in array initialiser expressions. 
The function `pass_replace_implied_do_loops` transforms the ASR tree in-place.

Converts:

    x = [(i*2, i = 1, 6)]

to:

    do i = 1, 6
        x(i) = i*2
    end do
*/

class ImpliedDoLoopVisitor : public ASR::BaseWalkVisitor<ImpliedDoLoopVisitor>
{
private:
    Allocator &al;
    ASR::TranslationUnit_t &unit;
    Vec<ASR::stmt_t*> implied_do_loop_result;
    bool contains_array;
    SymbolTable* current_scope;
public:
    ImpliedDoLoopVisitor(Allocator &al, ASR::TranslationUnit_t& unit) : al{al}, unit{unit}, 
    contains_array{false}, current_scope{nullptr} {
        implied_do_loop_result.reserve(al, 1);

    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            implied_do_loop_result.n = 0;
            visit_stmt(*m_body[i]);
            if (implied_do_loop_result.size() > 0) {
                for (size_t j=0; j<implied_do_loop_result.size(); j++) {
                    body.push_back(al, implied_do_loop_result[j]);
                }
                implied_do_loop_result.n = 0;
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

    void visit_Var(const ASR::Var_t& x) {
        ASR::expr_t* x_expr = (ASR::expr_t*)(&(x.base));
        contains_array = PassUtils::is_array(x_expr);
    }

    void visit_ConstantInteger(const ASR::ConstantInteger_t&) {
        contains_array = false;
    }

    void visit_ConstantReal(const ASR::ConstantReal_t&) {
        contains_array = false;
    }

    void visit_ConstantComplex(const ASR::ConstantComplex_t&) {
        contains_array = false;
    }

    void visit_ConstantLogical(const ASR::ConstantLogical_t&) {
        contains_array = false;
    }

    void visit_BinOp(const ASR::BinOp_t& x) {
        if( contains_array ) {
            return ;
        }
        bool left_array, right_array;
        visit_expr(*(x.m_left));
        left_array = contains_array;
        visit_expr(*(x.m_right));
        right_array = contains_array;
        contains_array = left_array || right_array;
    }

    void create_do_loop(ASR::ImpliedDoLoop_t* idoloop, ASR::Var_t* arr_var, ASR::expr_t* arr_idx=nullptr) {
        ASR::do_loop_head_t head;
        head.m_v = idoloop->m_var;
        head.m_start = idoloop->m_start;
        head.m_end = idoloop->m_end;
        head.m_increment = idoloop->m_increment;
        head.loc = head.m_v->base.loc;
        Vec<ASR::stmt_t*> doloop_body;
        doloop_body.reserve(al, 1);
        ASR::symbol_t* arr = arr_var->m_v;
        ASR::ttype_t *_type = LFortran::ASRUtils::expr_type(idoloop->m_start);
        ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, arr_var->base.base.loc, 1, _type));
        ASR::expr_t *const_n, *offset, *num_grps, *grp_start;
        const_n = offset = num_grps = grp_start = nullptr;
        if( arr_idx == nullptr ) {
            const_n = LFortran::ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, arr_var->base.base.loc, idoloop->n_values, _type));
            offset = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, arr_var->base.base.loc, idoloop->m_var, ASR::binopType::Sub, idoloop->m_start, _type, nullptr, nullptr));
            num_grps = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, arr_var->base.base.loc, offset, ASR::binopType::Mul, const_n, _type, nullptr, nullptr));
            grp_start = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, arr_var->base.base.loc, num_grps, ASR::binopType::Add, const_1, _type, nullptr, nullptr));
        }
        for( size_t i = 0; i < idoloop->n_values; i++ ) {
            Vec<ASR::array_index_t> args;
            ASR::array_index_t ai;
            ai.loc = arr_var->base.base.loc;
            ai.m_left = nullptr;
            if( arr_idx == nullptr ) {
                ASR::expr_t* const_i = LFortran::ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, arr_var->base.base.loc, i, _type));
                ASR::expr_t* idx = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, arr_var->base.base.loc,
                                                            grp_start, ASR::binopType::Add, const_i, 
                                                            _type, nullptr, nullptr));
                ai.m_right = idx;
            } else {
                ai.m_right = arr_idx;
            }
            ai.m_step = nullptr;
            args.reserve(al, 1);
            args.push_back(al, ai);
            ASR::expr_t* array_ref = LFortran::ASRUtils::EXPR(ASR::make_ArrayRef_t(al, arr_var->base.base.loc, arr,
                                                                args.p, args.size(), 
                                                                LFortran::ASRUtils::expr_type(LFortran::ASRUtils::EXPR((ASR::asr_t*)arr_var)), nullptr));
            if( idoloop->m_values[i]->type == ASR::exprType::ImpliedDoLoop ) {
                throw SemanticError("Pass for nested ImpliedDoLoop nodes isn't implemented yet.", idoloop->m_values[i]->base.loc);
            }
            ASR::stmt_t* doloop_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, array_ref, idoloop->m_values[i]));
            doloop_body.push_back(al, doloop_stmt);
            if( arr_idx != nullptr ) {
                ASR::expr_t* increment = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, arr_var->base.base.loc, arr_idx, ASR::binopType::Add, const_1, LFortran::ASRUtils::expr_type(arr_idx), nullptr, nullptr));
                ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, arr_idx, increment));
                doloop_body.push_back(al, assign_stmt);
            }
        }
        ASR::stmt_t* doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, arr_var->base.base.loc, head, doloop_body.p, doloop_body.size()));
        implied_do_loop_result.push_back(al, doloop);
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        if( x.m_value->type == ASR::exprType::ConstantArray ) {
            ASR::ConstantArray_t* arr_init = ((ASR::ConstantArray_t*)(&(x.m_value->base)));
            if( arr_init->n_args == 1 && arr_init->m_args[0] != nullptr && 
                arr_init->m_args[0]->type == ASR::exprType::ImpliedDoLoop ) {
                ASR::ImpliedDoLoop_t* idoloop = ((ASR::ImpliedDoLoop_t*)(&(arr_init->m_args[0]->base)));
                ASR::Var_t* arr_var = (ASR::Var_t*)(&(x.m_target->base));
                create_do_loop(idoloop, arr_var, nullptr);
            } else if( arr_init->n_args > 1 && arr_init->m_args[0] != nullptr ) {
                ASR::Var_t* arr_var = (ASR::Var_t*)(&(x.m_target->base));
                const char* const_idx_var_name = "1_k";
                char* idx_var_name = (char*)const_idx_var_name;
                ASR::expr_t* idx_var = nullptr;
                ASR::ttype_t* idx_var_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, arr_init->base.base.loc, 4, nullptr, 0));
                ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, arr_var->base.base.loc, 1, idx_var_type));
                if( unit.m_global_scope->scope.find(std::string(idx_var_name)) == unit.m_global_scope->scope.end() ) {
                    ASR::asr_t* idx_sym = ASR::make_Variable_t(al, arr_init->base.base.loc, unit.m_global_scope, idx_var_name, 
                                                            ASR::intentType::Local, const_1, nullptr, ASR::storage_typeType::Default, 
                                                            idx_var_type, ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required,
                                                            false);
                    unit.m_global_scope->scope[std::string(idx_var_name)] = ASR::down_cast<ASR::symbol_t>(idx_sym);
                    idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
                } else {
                    ASR::symbol_t* idx_sym = unit.m_global_scope->scope[std::string(idx_var_name)];
                    idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, idx_sym));
                    ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, idx_var, const_1));
                    implied_do_loop_result.push_back(al, assign_stmt);
                }
                for( size_t k = 0; k < arr_init->n_args; k++ ) {
                    ASR::expr_t* curr_init = arr_init->m_args[k];
                    if( curr_init->type == ASR::exprType::ImpliedDoLoop ) {
                        ASR::ImpliedDoLoop_t* idoloop = ((ASR::ImpliedDoLoop_t*)(&(curr_init->base)));
                        create_do_loop(idoloop, arr_var, idx_var);
                    } else {
                        Vec<ASR::array_index_t> args;
                        ASR::array_index_t ai;
                        ai.loc = arr_var->base.base.loc;
                        ai.m_left = nullptr;
                        ai.m_right = idx_var;
                        ai.m_step = nullptr;
                        args.reserve(al, 1);
                        args.push_back(al, ai);
                        ASR::expr_t* array_ref = LFortran::ASRUtils::EXPR(ASR::make_ArrayRef_t(al, arr_var->base.base.loc, arr_var->m_v,
                                                                            args.p, args.size(), 
                                                                            LFortran::ASRUtils::expr_type(LFortran::ASRUtils::EXPR((ASR::asr_t*)arr_var)), nullptr));
                        ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, array_ref, arr_init->m_args[k]));
                        implied_do_loop_result.push_back(al, assign_stmt);
                        ASR::expr_t* increment = LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, arr_var->base.base.loc, idx_var, ASR::binopType::Add, const_1, LFortran::ASRUtils::expr_type(idx_var), nullptr, nullptr));
                        assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, idx_var, increment));
                        implied_do_loop_result.push_back(al, assign_stmt);
                    }
                }
            }
        } else if( x.m_value->type != ASR::exprType::ConstantArray &&
                   x.m_value->type != ASR::exprType::FunctionCall && // This will be converted to SubroutineCall in array_op.cpp
                   PassUtils::is_array(x.m_target)) {
            contains_array = true;
            visit_expr(*(x.m_value)); // TODO: Add support for updating contains array in all types of expressions
            if( contains_array ) {
                return ;
            }

            int n_dims = PassUtils::get_rank(x.m_target);
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
            ASR::stmt_t* doloop = nullptr;
            for( int i = n_dims - 1; i >= 0; i-- ) {
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = PassUtils::get_bound(x.m_target, n_dims, "lbound", al, unit, current_scope); 
                head.m_end = PassUtils::get_bound(x.m_target, n_dims, "ubound", al, unit, current_scope);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(x.m_target, idx_vars, al);
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, ref, x.m_value));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            implied_do_loop_result.push_back(al, doloop);
        }
    }
};

void pass_replace_implied_do_loops(Allocator &al, ASR::TranslationUnit_t &unit) {
    ImpliedDoLoopVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
