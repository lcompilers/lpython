#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/arr_slice.h>
#include <lfortran/pass/pass_utils.h>
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

class ArrSliceVisitor : public ASR::BaseWalkVisitor<ArrSliceVisitor>
{
private:
    Allocator &al;
    ASR::TranslationUnit_t &unit;
    Vec<ASR::stmt_t*> arr_slice_result;

    ASR::expr_t* slice_var;

    int slice_counter;

    SymbolTable* current_scope;

public:
    ArrSliceVisitor(Allocator &al, ASR::TranslationUnit_t &unit) : al{al}, unit{unit}, 
    slice_var{nullptr}, slice_counter{0}, current_scope{nullptr} {
        arr_slice_result.reserve(al, 1);
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            arr_slice_result.n = 0;
            visit_stmt(*m_body[i]);
            if (arr_slice_result.size() > 0) {
                for (size_t j=0; j<arr_slice_result.size(); j++) {
                    body.push_back(al, arr_slice_result[j]);
                }
                arr_slice_result.n = 0;
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

    bool is_slice_present(const ASR::ArrayRef_t& x) {
        bool slice_present = false;
        for( size_t i = 0; i < x.n_args; i++ ) {
            if( !(x.m_args[i].m_left == nullptr && 
                  x.m_args[i].m_right != nullptr && 
                  x.m_args[i].m_step == nullptr) ) {
                slice_present = true;
                break;
            }
        }
        return slice_present;
    }

    void visit_ArrayRef(const ASR::ArrayRef_t& x) {
        if( is_slice_present(x) ) {
            Str new_name_str;
            new_name_str.from_str(al, std::to_string(slice_counter) + "_slice");
            slice_counter += 1;
            char* new_var_name = (char*)new_name_str.c_str(al);
            ASR::asr_t* slice_asr = ASR::make_Variable_t(al, x.base.base.loc, current_scope, new_var_name, 
                                                        ASR::intentType::Local, nullptr, ASR::storage_typeType::Default, 
                                                        x.m_type, ASR::abiType::Source, ASR::accessType::Public);
            ASR::symbol_t* slice_sym = ASR::down_cast<ASR::symbol_t>(slice_asr);
            current_scope->scope[std::string(new_var_name)] = slice_sym;
            slice_var = EXPR(ASR::make_Var_t(al, x.base.base.loc, slice_sym));
            // std::cout<<"Inside ArrayRef "<<slice_var<<std::endl;
            Vec<ASR::expr_t*> idx_vars_target, idx_vars_value;
            PassUtils::create_idx_vars(idx_vars_target, x.n_args, x.base.base.loc, al, unit, "_t");
            PassUtils::create_idx_vars(idx_vars_value, x.n_args, x.base.base.loc, al, unit, "_v");
            ASR::stmt_t* doloop = nullptr;
            ASR::ttype_t* int32_type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
            ASR::expr_t* const_1 = EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, 1, int32_type));
            for( int i = (int)x.n_args - 1; i >= 0; i-- ) {
                ASR::do_loop_head_t head;
                head.m_v = idx_vars_value[i];
                head.m_start = x.m_args[i].m_left;
                head.m_end = x.m_args[i].m_right;
                head.m_increment = x.m_args[i].m_step;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* target_ref = PassUtils::create_array_ref(slice_sym, idx_vars_target, al, x.base.base.loc, x.m_type);
                    ASR::expr_t* value_ref = PassUtils::create_array_ref(x.m_v, idx_vars_value, al, x.base.base.loc, x.m_type);
                    ASR::stmt_t* assign_stmt = STMT(ASR::make_Assignment_t(al, x.base.base.loc, target_ref, value_ref));
                    doloop_body.push_back(al, assign_stmt);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                ASR::expr_t* inc_expr = EXPR(ASR::make_BinOp_t(al, x.base.base.loc, idx_vars_target[i], ASR::binopType::Add, const_1, int32_type));
                ASR::stmt_t* assign_stmt = STMT(ASR::make_Assignment_t(al, x.base.base.loc, idx_vars_target[i], inc_expr));
                doloop_body.push_back(al, assign_stmt);
                doloop = STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            arr_slice_result.push_back(al, doloop);
        }
    }

    void visit_Print(const ASR::Print_t& x) {
        ASR::Print_t& xx = const_cast<ASR::Print_t&>(x);
        // std::cout<<"Inside Slice Print "<<xx.n_values<<std::endl;
        for( size_t i = 0; i < xx.n_values; i++ ) {        
            slice_var = nullptr;
            this->visit_expr(*xx.m_values[i]);
            // std::cout<<"Inside Print "<<slice_var<<std::endl;
            if( slice_var != nullptr ) {
                xx.m_values[i] = slice_var;
                // std::cout<<"Inside Print "<<x.m_values[i]<<std::endl;
            }
        }
        if( arr_slice_result.size() > 0 ) {
            arr_slice_result.push_back(al, const_cast<ASR::stmt_t*>(&(x.base)));
        }
    }
};

void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrSliceVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
