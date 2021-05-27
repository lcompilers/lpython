#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/pass_utils.h>
#include <lfortran/pass/print_arr.h>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*
This ASR pass replaces array slice with do loops and array expression assignments. 
The function `pass_replace_print_arr` transforms the ASR tree in-place.

Converts:

    x = y(1:3)

to:

    do i = 1, 3
        x(i) = y(i)
    end do
*/

class PrintArrVisitor : public ASR::BaseWalkVisitor<PrintArrVisitor>
{
private:
    Allocator &al;
    ASR::TranslationUnit_t &unit;
    Vec<ASR::stmt_t*> print_arr_result;
public:
    PrintArrVisitor(Allocator &al, ASR::TranslationUnit_t &unit) : al{al}, unit{unit} {
        print_arr_result.reserve(al, 1);

    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            print_arr_result.n = 0;
            visit_stmt(*m_body[i]);
            if (print_arr_result.size() > 0) {
                for (size_t j=0; j<print_arr_result.size(); j++) {
                    body.push_back(al, print_arr_result[j]);
                }
                print_arr_result.n = 0;
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

    void visit_Print(const ASR::Print_t& x) {
        if( x.n_values == 1 && PassUtils::is_array(x.m_values[0], al) ) {
            ASR::expr_t* arr_expr = x.m_values[0];
            // Case #1: For a single static array in the argument
            Vec<PassUtils::dimension_descriptor> dims_vec;
            PassUtils::get_dims(arr_expr, dims_vec, al);
            if( dims_vec.size() <= 0 ) {
                return ;
            }

            PassUtils::dimension_descriptor* dims = dims_vec.p;
            int n_dims = dims_vec.size();
            ASR::ttype_t* int32_type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, unit);
            ASR::stmt_t* empty_print_stmt = STMT(ASR::make_Print_t(al, x.base.base.loc, nullptr, nullptr, 0, nullptr));
            ASR::stmt_t* doloop = nullptr;
            const char* tab = "\t";
            for( int i = n_dims - 1; i >= 0; i-- ) {
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, dims[i].lbound, int32_type)); // TODO: Replace with call to lbound
                head.m_end = EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, dims[i].ubound, int32_type)); // TODO: Replace with call to ubound
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(arr_expr, idx_vars, al);
                    Vec<ASR::expr_t*> print_args;
                    print_args.reserve(al, 1);
                    print_args.push_back(al, ref);
                    ASR::stmt_t* print_stmt = STMT(ASR::make_Print_t(al, x.base.base.loc, nullptr, 
                                                                 print_args.p, print_args.size(), (char*)tab));
                    doloop_body.push_back(al, print_stmt);
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            print_arr_result.push_back(al, doloop);
            print_arr_result.push_back(al, empty_print_stmt);
        }
    }

};

void pass_replace_print_arr(Allocator &al, ASR::TranslationUnit_t &unit) {
    PrintArrVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
