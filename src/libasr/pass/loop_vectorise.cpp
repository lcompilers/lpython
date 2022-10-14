#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/loop_vectorise.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>
#include <cmath>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass converts loops over arrays with a call to internal
optimization routine for loop vectorisation. This allows backend
specific code generation for vector operations resulting in optimized
code.

Converts:

    i: i32
    for i in range(9216):
        a[i] = b[i]

to:

    for i in range(0, 9216, 8):
        vector_copy(8, a[i*8:(i+1)*8], b[i*8:(i+1)*8])

*/
class LoopVectoriseVisitor : public PassUtils::SkipOptimizationFunctionVisitor<LoopVectoriseVisitor>
{
private:
    ASR::TranslationUnit_t &unit;

    std::string rl_path;

    // To make sure that FMA is applied only for
    // the nodes implemented in this class
    bool from_loop_vectorise;

    const int64_t avx512 = 512;

public:
    LoopVectoriseVisitor(Allocator &al_, ASR::TranslationUnit_t &unit_,
                         const std::string& rl_path_) : SkipOptimizationFunctionVisitor(al_),
    unit(unit_), rl_path(rl_path_), from_loop_vectorise(false)
    {
        pass_result.reserve(al, 1);
    }

    bool is_loop(const ASR::stmt_t& x) {
        return (ASR::is_a<ASR::DoLoop_t>(x) ||
                ASR::is_a<ASR::WhileLoop_t>(x));
    }

    bool is_vector_copy(ASR::stmt_t* x, Vec<ASR::expr_t*>& arrays) {
        if( !ASR::is_a<ASR::Assignment_t>(*x) ) {
            return false;
        }
        ASR::Assignment_t* x_assignment = ASR::down_cast<ASR::Assignment_t>(x);
        ASR::expr_t* target = x_assignment->m_target;
        ASR::expr_t* value = x_assignment->m_value;
        if( !ASR::is_a<ASR::ArrayItem_t>(*target) ||
            !ASR::is_a<ASR::ArrayItem_t>(*value) ) {
            return false;
        }
        ASR::ArrayItem_t* target_array_ref = ASR::down_cast<ASR::ArrayItem_t>(target);
        ASR::ArrayItem_t* value_array_ref = ASR::down_cast<ASR::ArrayItem_t>(value);
        if( target_array_ref->m_args->m_left ||
            target_array_ref->m_args->m_step ||
            value_array_ref->m_args->m_left ||
            value_array_ref->m_args->m_step ) {
            return false;
        }
        arrays.push_back(al, target_array_ref->m_v);
        arrays.push_back(al, value_array_ref->m_v);
        return true;
    }

    int64_t select_vector_instruction_size() {
        return avx512;
    }

    int64_t get_vector_length(ASR::ttype_t* type) {
        int kind = ASRUtils::extract_kind_from_ttype_t(type);
        int64_t instruction_length = select_vector_instruction_size();
        // TODO: Add check for divisibility and raise error if needed.
        return instruction_length/(kind * 8);
    }

    void get_vector_intrinsic(ASR::stmt_t* loop_stmt, ASR::expr_t* index,
                              ASR::expr_t*& vector_length,
                              Vec<ASR::stmt_t*>& vectorised_loop_body) {
        LFORTRAN_ASSERT(vectorised_loop_body.reserve_called);
        Vec<ASR::expr_t*> arrays;
        arrays.reserve(al, 2);
        if( is_vector_copy(loop_stmt, arrays) ) {
            ASR::expr_t *target_sym = arrays[0], *value_sym = arrays[1];
            ASR::ttype_t* target_type = ASRUtils::expr_type(target_sym);
            int64_t vector_length_int = get_vector_length(target_type);
            vector_length = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loop_stmt->base.loc,
                                vector_length_int, ASRUtils::expr_type(index)));
            ASR::expr_t* start = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loop_stmt->base.loc, index, ASR::binopType::Mul,
                                        vector_length, ASRUtils::expr_type(index), nullptr));
            ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loop_stmt->base.loc,
                                    1, ASRUtils::expr_type(index)));
            ASR::expr_t* next_index = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loop_stmt->base.loc, index, ASR::binopType::Add,
                                            one, ASRUtils::expr_type(index), nullptr));
            ASR::expr_t* end = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loop_stmt->base.loc, next_index, ASR::binopType::Mul,
                                        vector_length, ASRUtils::expr_type(index), nullptr));
            vectorised_loop_body.push_back(al, PassUtils::get_vector_copy(target_sym, value_sym, start,
                end, one, vector_length,
                al, unit,
                current_scope, loop_stmt->base.loc));
            return ;
        }
    }

    void vectorise_loop(const ASR::DoLoop_t& x) {
        // Do Vectorisation of Loop inside this function
        ASR::expr_t* loop_start = x.m_head.m_start;
        ASR::expr_t* loop_end = x.m_head.m_end;
        ASR::expr_t* loop_inc = x.m_head.m_increment;
        ASR::expr_t* loop_start_value = ASRUtils::expr_value(loop_start);
        ASR::expr_t* loop_end_value = ASRUtils::expr_value(loop_end);
        ASR::expr_t* loop_inc_value = ASRUtils::expr_value(loop_inc);
        if( !ASRUtils::is_value_constant(loop_start_value) ||
            !ASRUtils::is_value_constant(loop_end_value) ||
            !ASRUtils::is_value_constant(loop_inc_value) ) {
            // Skip vectorisation of variable sized loops
            return ;
        }
        ASR::stmt_t* loop_stmt = x.m_body[0];
        int64_t loop_start_int = -1, loop_end_int = -1, loop_inc_int = -1;
        ASRUtils::extract_value(loop_start_value, loop_start_int);
        ASRUtils::extract_value(loop_end_value, loop_end_int);
        ASRUtils::extract_value(loop_inc_value, loop_inc_int);
        int64_t loop_size = (loop_end_int - loop_start_int) / loop_inc_int + 1;
        ASR::expr_t* vector_length = nullptr;
        Vec<ASR::stmt_t*> vectorised_loop_body;
        vectorised_loop_body.reserve(al, x.n_body);
        get_vector_intrinsic(loop_stmt, x.m_head.m_v,
                             vector_length, vectorised_loop_body);
        if( !vector_length ) {
            return ;
        }
        ASR::do_loop_head_t vectorised_loop_head;
        vectorised_loop_head.m_start = loop_start;
        int64_t vector_length_int = -1;
        ASRUtils::extract_value(vector_length, vector_length_int);
        // TODO: Add tests for loop sizes not divisible by vector_length.
        vectorised_loop_head.m_end = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.m_head.m_v->base.loc,
                                        loop_size/vector_length_int - 1, ASRUtils::expr_type(x.m_head.m_v)));
        // TODO: Update this for increments other 1.
        ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loop_stmt->base.loc,
                                    1, ASRUtils::expr_type(x.m_head.m_v)));
        vectorised_loop_head.m_increment = one;
        vectorised_loop_head.m_v = x.m_head.m_v;
        vectorised_loop_head.loc = x.m_head.loc;

        ASR::stmt_t* vectorised_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc,
                                            vectorised_loop_head, vectorised_loop_body.p,
                                            vectorised_loop_body.size()));
        pass_result.push_back(al, vectorised_loop);
    }

    void visit_DoLoop(const ASR::DoLoop_t& x) {
        from_loop_vectorise = true;
        if( x.n_body == 1 ) {
            // Vectorise
            if( is_loop(*x.m_body[0]) ) {
                from_loop_vectorise = false;
                visit_stmt(*x.m_body[0]);
                return ;
            }
            vectorise_loop(x);
        }
        from_loop_vectorise = false;
    }

};

void pass_loop_vectorise(Allocator &al, ASR::TranslationUnit_t &unit,
                         const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    LoopVectoriseVisitor v(al, unit, rl_path);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
