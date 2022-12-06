#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/arr_slice.h>
#include <libasr/pass/pass_utils.h>
#include <unordered_map>
#include <map>
#include <utility>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

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

class ArrSliceVisitor : public PassUtils::PassVisitor<ArrSliceVisitor>
{
private:

    ASR::expr_t* slice_var;
    bool create_slice_var;

    int slice_counter;

    std::string rl_path;

public:
    ArrSliceVisitor(Allocator &al, const std::string &rl_path) : PassVisitor(al, nullptr),
    slice_var(nullptr), create_slice_var(false), slice_counter(0),
    rl_path(rl_path)
    {
        pass_result.reserve(al, 1);
    }

    ASR::ttype_t* get_array_from_slice(const ASR::ArraySection_t& x, ASR::expr_t* arr_var) {
        Vec<ASR::dimension_t> m_dims;
        m_dims.reserve(al, x.n_args);
        ASR::ttype_t* int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
        ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int32_type));
        for( size_t i = 0; i < x.n_args; i++ ) {
            if( x.m_args[i].m_step != nullptr ) {
                ASR::expr_t *start = nullptr, *end = nullptr, *step = nullptr;
                if( x.m_args[i].m_left == nullptr ) {
                    start = PassUtils::get_bound(arr_var, i + 1, "lbound", al);
                } else {
                    start = x.m_args[i].m_left;
                }

                if( x.m_args[i].m_right == nullptr ) {
                    end = PassUtils::get_bound(arr_var, i + 1, "ubound", al);
                } else {
                    end = x.m_args[i].m_right;
                }

                if( x.m_args[i].m_step == nullptr ) {
                    step = const_1;
                } else {
                    step = x.m_args[i].m_step;
                }

                start = PassUtils::to_int32(start, int32_type, al);
                end = PassUtils::to_int32(end, int32_type, al);
                step = PassUtils::to_int32(step, int32_type, al);

                ASR::expr_t* gap = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x.base.base.loc,
                                                        end, ASR::binopType::Sub, start, int32_type, nullptr));
                ASR::expr_t* slice_size = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x.base.base.loc,
                                                        gap, ASR::binopType::Div, step, int32_type, nullptr));
                ASR::expr_t* actual_size = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x.base.base.loc,
                                                        slice_size, ASR::binopType::Add, const_1, int32_type, nullptr));
                ASR::dimension_t curr_dim;
                curr_dim.loc = x.base.base.loc;
                curr_dim.m_start = const_1;
                curr_dim.m_length = actual_size;
                m_dims.push_back(al, curr_dim);
            } else {
                ASR::dimension_t curr_dim;
                curr_dim.loc = x.base.base.loc;
                curr_dim.m_start = const_1;
                curr_dim.m_length = const_1;
                m_dims.push_back(al, curr_dim);
            }
        }

        ASR::ttype_t* new_type = nullptr;
        ASR::ttype_t* t2 = ASRUtils::type_get_past_pointer(x.m_type);
        switch (t2->type)
        {
            case ASR::ttypeType::Integer: {
                ASR::Integer_t* curr_type = down_cast<ASR::Integer_t>(t2);
                new_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, curr_type->m_kind,
                                                    m_dims.p, m_dims.size()));
                break;
            }
            case ASR::ttypeType::Real: {
                ASR::Real_t* curr_type = down_cast<ASR::Real_t>(t2);
                new_type = ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc, curr_type->m_kind,
                                                    m_dims.p, m_dims.size()));
                break;
            }
            case ASR::ttypeType::Complex: {
                ASR::Complex_t* curr_type = down_cast<ASR::Complex_t>(t2);
                new_type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc, curr_type->m_kind,
                                                    m_dims.p, m_dims.size()));
                break;
            }
            case ASR::ttypeType::Logical: {
                ASR::Logical_t* curr_type = down_cast<ASR::Logical_t>(t2);
                new_type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc, curr_type->m_kind,
                                                    m_dims.p, m_dims.size()));
                break;
            }
            case ASR::ttypeType::Struct: {
                ASR::Struct_t* curr_type = down_cast<ASR::Struct_t>(t2);
                new_type = ASRUtils::TYPE(ASR::make_Struct_t(al, x.base.base.loc, curr_type->m_derived_type,
                                                    m_dims.p, m_dims.size()));
                break;
            }
            default:
                break;
        }
        if (ASR::is_a<ASR::Pointer_t>(*x.m_type)) {
            new_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, x.base.base.loc, new_type));
        }

        return new_type;
    }

    void visit_ArraySection(const ASR::ArraySection_t& x) {
        if( create_slice_var ) {
            ASR::expr_t* x_arr_var = x.m_v;
            Str new_name_str;
            new_name_str.from_str(al, "~" + std::to_string(slice_counter) + "_slice");
            slice_counter += 1;
            char* new_var_name = (char*)new_name_str.c_str(al);
            ASR::asr_t* slice_asr = ASR::make_Variable_t(al, x.base.base.loc, current_scope, new_var_name, nullptr, 0,
                                                        ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                                        get_array_from_slice(x, x_arr_var), ASR::abiType::Source, ASR::accessType::Public,
                                                        ASR::presenceType::Required, false);
            ASR::symbol_t* slice_sym = ASR::down_cast<ASR::symbol_t>(slice_asr);
            current_scope->add_symbol(std::string(new_var_name), slice_sym);
            slice_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, slice_sym));
            Vec<ASR::expr_t*> idx_vars_target, idx_vars_value;
            PassUtils::create_idx_vars(idx_vars_target, x.n_args, x.base.base.loc, al, current_scope, "_t");
            PassUtils::create_idx_vars(idx_vars_value, x.n_args, x.base.base.loc, al, current_scope, "_v");
            ASR::stmt_t* doloop = nullptr;
            int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x.m_v));
            ASR::ttype_t* int_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, a_kind, nullptr, 0));
            ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int_type));
            for( int i = (int)x.n_args - 1; i >= 0; i-- ) {
                ASR::do_loop_head_t head;
                head.m_v = idx_vars_value[i];
                if( x.m_args[i].m_step != nullptr ) {
                    if( x.m_args[i].m_left == nullptr ) {
                        head.m_start = PassUtils::get_bound(x_arr_var, i + 1, "lbound", al);
                    } else {
                        head.m_start = x.m_args[i].m_left;
                    }
                    if( x.m_args[i].m_right == nullptr ) {
                        head.m_end = PassUtils::get_bound(x_arr_var, i + 1, "ubound", al);
                    } else {
                        head.m_end = x.m_args[i].m_right;
                    }
                } else {
                    head.m_start = x.m_args[i].m_right;
                    head.m_end = x.m_args[i].m_right;
                }
                head.m_increment = x.m_args[i].m_step;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* target_ref = PassUtils::create_array_ref(slice_sym, idx_vars_target, al, x.base.base.loc, x.m_type);
                    ASR::expr_t* value_ref = PassUtils::create_array_ref(x.m_v, idx_vars_value, al);
                    ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, target_ref, value_ref, nullptr));
                    doloop_body.push_back(al, assign_stmt);
                } else {
                    ASR::stmt_t* set_to_one = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_target[i+1], const_1, nullptr));
                    doloop_body.push_back(al, set_to_one);
                    doloop_body.push_back(al, doloop);
                }
                ASR::expr_t* inc_expr = LFortran::ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x.base.base.loc, idx_vars_target[i], ASR::binopType::Add, const_1, int_type, nullptr));
                ASR::stmt_t* assign_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_target[i], inc_expr, nullptr));
                doloop_body.push_back(al, assign_stmt);
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            ASR::stmt_t* set_to_one = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_target[0], const_1, nullptr));
            pass_result.push_back(al, set_to_one);
            pass_result.push_back(al, doloop);
        }
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
            ASR::is_a<ASR::GetPointer_t>(*x.m_value)) ||
            ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ) {
            return ;
        }
        this->visit_expr(*x.m_value);
        // If any slicing happened then do loop must have been created
        // So, the current assignment should be inserted into pass_result
        // so that it doesn't get ignored.
        if( pass_result.size() > 0 ) {
            pass_result.push_back(al, const_cast<ASR::stmt_t*>(&(x.base)));
        }
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
        T& xx = const_cast<T&>(x);
        create_slice_var = true;
        slice_var = nullptr;
        this->visit_expr(*x.m_left);
        if( slice_var != nullptr ) {
            xx.m_left = slice_var;
            slice_var = nullptr;
        }
        this->visit_expr(*x.m_right);
        if( slice_var != nullptr ) {
            xx.m_right = slice_var;
            slice_var = nullptr;
        }
        create_slice_var = false;
    }

    void visit_Print(const ASR::Print_t& x) {
        ASR::Print_t& xx = const_cast<ASR::Print_t&>(x);
        for( size_t i = 0; i < xx.n_values; i++ ) {
            slice_var = nullptr;
            create_slice_var = true;
            this->visit_expr(*xx.m_values[i]);
            create_slice_var = false;
            if( slice_var != nullptr ) {
                xx.m_values[i] = slice_var;
            }
        }
        // If any slicing happened then do loop must have been created
        // So, the current print should be inserted into pass_result
        // so that it doesn't get ignored.
        if( pass_result.size() > 0 ) {
            pass_result.push_back(al, const_cast<ASR::stmt_t*>(&(x.base)));
        }
    }
};

void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    ArrSliceVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LFortran
