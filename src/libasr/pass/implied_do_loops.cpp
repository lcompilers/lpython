#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/implied_do_loops.h>
#include <libasr/pass/pass_utils.h>


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

class ImpliedDoLoopVisitor : public PassUtils::PassVisitor<ImpliedDoLoopVisitor>
{
private:
    bool contains_array;
    std::string rl_path;
public:
    // Public to surpress a warning
    ASR::TranslationUnit_t &unit;

    ImpliedDoLoopVisitor(Allocator &al, ASR::TranslationUnit_t& unit,
        const std::string &rl_path) :
             PassVisitor(al, nullptr), contains_array{false}, rl_path{rl_path},
             unit{unit} {
        pass_result.reserve(al, 1);
    }

    void visit_Var(const ASR::Var_t& x) {
        ASR::expr_t* x_expr = (ASR::expr_t*)(&(x.base));
        contains_array = PassUtils::is_array(x_expr);
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t&) {
        contains_array = false;
    }

    void visit_RealConstant(const ASR::RealConstant_t&) {
        contains_array = false;
    }

    void visit_ComplexConstant(const ASR::ComplexConstant_t&) {
        contains_array = false;
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t&) {
        contains_array = false;
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_RealBinOp(const ASR::RealBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t& x) {
        handle_BinOp(x);
    }

    template <typename T>
    void handle_BinOp(const T& x) {
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
        ASR::ttype_t *_type = LFortran::ASRUtils::expr_type(idoloop->m_start);
        ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_var->base.base.loc, 1, _type));
        ASR::expr_t *const_n, *offset, *num_grps, *grp_start;
        const_n = offset = num_grps = grp_start = nullptr;
        if( arr_idx == nullptr ) {
            const_n = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_var->base.base.loc, idoloop->n_values, _type));
            offset = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.base.loc, idoloop->m_var, ASR::binopType::Sub, idoloop->m_start, _type, nullptr));
            num_grps = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.base.loc, offset, ASR::binopType::Mul, const_n, _type, nullptr));
            grp_start = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.base.loc, num_grps, ASR::binopType::Add, const_1, _type, nullptr));
        }
        for( size_t i = 0; i < idoloop->n_values; i++ ) {
            Vec<ASR::array_index_t> args;
            ASR::array_index_t ai;
            ai.loc = arr_var->base.base.loc;
            ai.m_left = nullptr;
            if( arr_idx == nullptr ) {
                ASR::expr_t* const_i = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_var->base.base.loc, i, _type));
                ASR::expr_t* idx = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.base.loc,
                                                            grp_start, ASR::binopType::Add, const_i, _type, nullptr));
                ai.m_right = idx;
            } else {
                ai.m_right = arr_idx;
            }
            ai.m_step = nullptr;
            args.reserve(al, 1);
            args.push_back(al, ai);
            ASR::ttype_t* array_ref_type = ASRUtils::expr_type(ASRUtils::EXPR((ASR::asr_t*)arr_var));
            Vec<ASR::dimension_t> empty_dims;
            empty_dims.reserve(al, 1);
            array_ref_type = ASRUtils::duplicate_type(al, array_ref_type, &empty_dims);
            ASR::expr_t* array_ref = LFortran::ASRUtils::EXPR(ASR::make_ArrayItem_t(al, arr_var->base.base.loc,
                                                              ASRUtils::EXPR((ASR::asr_t*)arr_var),
                                                              args.p, args.size(),
                                                              array_ref_type, ASR::arraystorageType::RowMajor,
                                                              nullptr));
            if( idoloop->m_values[i]->type == ASR::exprType::ImpliedDoLoop ) {
                throw LCompilersException("Pass for nested ImpliedDoLoop nodes isn't implemented yet."); // idoloop->m_values[i]->base.loc
            }
            ASR::stmt_t* doloop_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, array_ref, idoloop->m_values[i], nullptr));
            doloop_body.push_back(al, doloop_stmt);
            if( arr_idx != nullptr ) {
                ASR::expr_t* increment = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.base.loc, arr_idx, ASR::binopType::Add, const_1, LFortran::ASRUtils::expr_type(arr_idx), nullptr));
                ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, arr_idx, increment, nullptr));
                doloop_body.push_back(al, assign_stmt);
            }
        }
        ASR::stmt_t* doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, arr_var->base.base.loc, head, doloop_body.p, doloop_body.size()));
        pass_result.push_back(al, doloop);
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        if( (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
            ASR::is_a<ASR::GetPointer_t>(*x.m_value)) ||
            ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ) {
            return ;
        }

        if( ASR::is_a<ASR::ArrayConstant_t>(*x.m_value) ) {
            ASR::ArrayConstant_t* arr_init = ASR::down_cast<ASR::ArrayConstant_t>(x.m_value);
            if( arr_init->n_args == 0 ) {
                remove_original_stmt = true;
                return ;
            }

            LFORTRAN_ASSERT_MSG(PassUtils::get_rank(x.m_target) == 1,
                                "Initialisation using ArrayConstant is "
                                "supported only for single dimensional arrays.")
            ASR::Var_t* arr_var = ASR::down_cast<ASR::Var_t>(x.m_target);
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, 1, x.base.base.loc, al, current_scope);
            ASR::expr_t* idx_var = idx_vars[0];
            ASR::expr_t* lb = PassUtils::get_bound(x.m_target, 1, "lbound", al);
            ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1,
                                        ASRUtils::expr_type(idx_var)));
            ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al,
                                            arr_var->base.base.loc, idx_var, lb, nullptr));
            pass_result.push_back(al, assign_stmt);
            for( size_t k = 0; k < arr_init->n_args; k++ ) {
                ASR::expr_t* curr_init = arr_init->m_args[k];
                if( ASR::is_a<ASR::ImpliedDoLoop_t>(*curr_init) ) {
                    ASR::ImpliedDoLoop_t* idoloop = ASR::down_cast<ASR::ImpliedDoLoop_t>(curr_init);
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
                    ASR::ttype_t* array_ref_type = ASRUtils::expr_type(ASRUtils::EXPR((ASR::asr_t*)arr_var));
                    Vec<ASR::dimension_t> empty_dims;
                    empty_dims.reserve(al, 1);
                    array_ref_type = ASRUtils::duplicate_type(al, array_ref_type, &empty_dims);
                    ASR::expr_t* array_ref = LFortran::ASRUtils::EXPR(ASR::make_ArrayItem_t(al, arr_var->base.base.loc,
                                                                        ASRUtils::EXPR((ASR::asr_t*)arr_var),
                                                                        args.p, args.size(),
                                                                        array_ref_type, ASR::arraystorageType::RowMajor,
                                                                        nullptr));
                    ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, array_ref, arr_init->m_args[k], nullptr));
                    pass_result.push_back(al, assign_stmt);
                    ASR::expr_t* increment = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.base.loc, idx_var, ASR::binopType::Add, const_1, LFortran::ASRUtils::expr_type(idx_var), nullptr));
                    assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.base.loc, idx_var, increment, nullptr));
                    pass_result.push_back(al, assign_stmt);
                }
            }
        } else if( !ASR::is_a<ASR::ArrayConstant_t>(*x.m_value) &&
                   !ASR::is_a<ASR::FunctionCall_t>(*x.m_value) && // This will be converted to SubroutineCall in array_op.cpp
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
                head.m_start = PassUtils::get_bound(x.m_target, n_dims, "lbound", al);
                head.m_end = PassUtils::get_bound(x.m_target, n_dims, "ubound", al);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(x.m_target, idx_vars, al);
                    ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, ref, x.m_value, nullptr));
                    doloop_body.push_back(al, assign);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
        }
    }
};

void pass_replace_implied_do_loops(Allocator &al, ASR::TranslationUnit_t &unit,
                                   const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    ImpliedDoLoopVisitor v(al, unit, rl_path);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
