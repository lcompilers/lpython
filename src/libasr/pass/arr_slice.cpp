#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/arr_slice.h>

#include <vector>
#include <utility>


namespace LCompilers {

/*
This ASR pass replaces array slice with do loops and array expression assignments.
The function `pass_replace_arr_slice` transforms the ASR tree in-place.

Converts:

    x = y(1:3)

to:

    do i = 1, 3
        x(i) = y(i)
    end do
*/

class ReplaceArraySection: public ASR::BaseExprReplacer<ReplaceArraySection> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;
    size_t slice_counter;

    public:

    SymbolTable* current_scope;

    ReplaceArraySection(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_) :
    al(al_), pass_result(pass_result_), slice_counter(0), current_scope(nullptr) {}

    ASR::ttype_t* get_array_from_slice(ASR::ArraySection_t* x, ASR::expr_t* arr_var) {
        Vec<ASR::dimension_t> m_dims;
        m_dims.reserve(al, x->n_args);
        ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, 4, nullptr, 0));
        ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x->base.base.loc, 1, int32_type));
        for( size_t i = 0; i < x->n_args; i++ ) {
            if( x->m_args[i].m_step != nullptr ) {
                ASR::expr_t *start = nullptr, *end = nullptr, *step = nullptr;
                if( x->m_args[i].m_left == nullptr ) {
                    start = PassUtils::get_bound(arr_var, i + 1, "lbound", al);
                } else {
                    start = x->m_args[i].m_left;
                }

                if( x->m_args[i].m_right == nullptr ) {
                    end = PassUtils::get_bound(arr_var, i + 1, "ubound", al);
                } else {
                    end = x->m_args[i].m_right;
                }

                if( x->m_args[i].m_step == nullptr ) {
                    step = const_1;
                } else {
                    step = x->m_args[i].m_step;
                }

                start = PassUtils::to_int32(start, int32_type, al);
                end = PassUtils::to_int32(end, int32_type, al);
                step = PassUtils::to_int32(step, int32_type, al);

                ASR::expr_t* gap = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x->base.base.loc,
                                                        end, ASR::binopType::Sub, start, int32_type, nullptr));
                ASR::expr_t* slice_size = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x->base.base.loc,
                                                        gap, ASR::binopType::Div, step, int32_type, nullptr));
                ASR::expr_t* actual_size = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x->base.base.loc,
                                                        slice_size, ASR::binopType::Add, const_1, int32_type, nullptr));
                ASR::dimension_t curr_dim;
                curr_dim.loc = x->base.base.loc;
                curr_dim.m_start = const_1;
                curr_dim.m_length = actual_size;
                m_dims.push_back(al, curr_dim);
            }
        }

        ASR::ttype_t* t2 = ASRUtils::type_get_past_pointer(x->m_type);
        ASR::ttype_t* new_type = ASRUtils::duplicate_type(al, t2, &m_dims);
        if (ASR::is_a<ASR::Pointer_t>(*x->m_type)) {
            new_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, x->base.base.loc, new_type));
        }

        return new_type;
    }

    void replace_ArraySection(ASR::ArraySection_t* x) {
        LCOMPILERS_ASSERT(current_scope != nullptr);
        ASR::expr_t* x_arr_var = x->m_v;
        std::string new_name = "~" + std::to_string(slice_counter) + "_slice";
        slice_counter += 1;
        char* new_var_name = s2c(al, new_name);
        ASR::ttype_t* slice_asr_type = get_array_from_slice(x, x_arr_var);
        ASR::asr_t* slice_asr = ASR::make_Variable_t(al, x->base.base.loc, current_scope, new_var_name, nullptr, 0,
                                    ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                    slice_asr_type, ASR::abiType::Source, ASR::accessType::Public,
                                    ASR::presenceType::Required, false);
        ASR::symbol_t* slice_sym = ASR::down_cast<ASR::symbol_t>(slice_asr);
        current_scope->add_symbol(std::string(new_var_name), slice_sym);
        *current_expr = ASRUtils::EXPR(ASR::make_Var_t(al, x->base.base.loc, slice_sym));
        Vec<ASR::expr_t*> idx_vars_target, idx_vars_value;
        int sliced_rank = ASRUtils::extract_n_dims_from_ttype(slice_asr_type);
        PassUtils::create_idx_vars(idx_vars_target, sliced_rank, x->base.base.loc, al, current_scope, "_t");
        std::vector<int> value_indices;
        PassUtils::create_idx_vars(idx_vars_value, x->m_args, x->n_args, value_indices,
                                   x->base.base.loc, al, current_scope, "_v");
        ASR::stmt_t* doloop = nullptr;
        for( int i = (int) sliced_rank - 1; i >= 0; i-- ) {
            ASR::do_loop_head_t head;
            int  j = value_indices[i];
            head.m_v = idx_vars_value[j];
            if( x->m_args[j].m_left == nullptr ) {
                head.m_start = PassUtils::get_bound(x_arr_var, j + 1, "lbound", al);
            } else {
                head.m_start = x->m_args[j].m_left;
            }
            if( x->m_args[j].m_right == nullptr ) {
                head.m_end = PassUtils::get_bound(x_arr_var, j + 1, "ubound", al);
            } else {
                head.m_end = x->m_args[j].m_right;
            }
            head.m_increment = x->m_args[j].m_step;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                ASR::expr_t* target_ref = PassUtils::create_array_ref(slice_sym, idx_vars_target, al, x->base.base.loc, x->m_type);
                ASR::expr_t* value_ref = PassUtils::create_array_ref(x->m_v, idx_vars_value, al);
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, x->base.base.loc, target_ref, value_ref, nullptr));
                doloop_body.push_back(al, assign_stmt);
            } else {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(idx_vars_target[i+1]));
                ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, a_kind, nullptr, 0));
                ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x->base.base.loc, 1, int_type));
                ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(al, x->base.base.loc, idx_vars_target[i+1], const_1, nullptr));
                doloop_body.push_back(al, set_to_one);
                doloop_body.push_back(al, doloop);
            }
            int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(idx_vars_target[i]));
            ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, a_kind, nullptr, 0));
            ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x->base.base.loc, 1, int_type));
            ASR::expr_t* inc_expr = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x->base.base.loc, idx_vars_target[i], ASR::binopType::Add,
                                        const_1, int_type, nullptr));
            ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, x->base.base.loc, idx_vars_target[i], inc_expr, nullptr));
            doloop_body.push_back(al, assign_stmt);
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, x->base.base.loc, head, doloop_body.p, doloop_body.size()));
        }
        int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(idx_vars_target[0]));
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, a_kind, nullptr, 0));
        ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x->base.base.loc, 1, int_type));
        ASR::stmt_t* set_to_one = ASRUtils::STMT(ASR::make_Assignment_t(al, x->base.base.loc, idx_vars_target[0], const_1, nullptr));
        pass_result.push_back(al, set_to_one);
        pass_result.push_back(al, doloop);
    }

};

class ArraySectionVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArraySectionVisitor>
{
    private:

        Allocator& al;
        ReplaceArraySection replacer;
        Vec<ASR::stmt_t*> pass_result;

    public:

        ArraySectionVisitor(Allocator& al_) :
        al(al_), replacer(al_, pass_result) {
            pass_result.reserve(al_, 1);
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            for (size_t i=0; i<n_body; i++) {
                pass_result.n = 0;
                visit_stmt(*m_body[i]);
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                body.push_back(al, m_body[i]);
            }
            m_body = body.p;
            n_body = body.size();
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            ASR::expr_t** current_expr_copy_9 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            this->call_replacer();
            current_expr = current_expr_copy_9;
            this->visit_expr(*x.m_value);
            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
            }
    }

};

void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& /*pass_options*/) {
    ArraySectionVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}

} // namespace LCompilers
