#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/print_arr.h>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*
This ASR pass replaces array slice with do loops and array expression assignments.
The function `pass_replace_print_arr` transforms the ASR tree in-place.

Converts:

    print*, y(1:3)

to:

    do i = 1, 3
        print *, y(i)
    end do
*/

class PrintArrVisitor : public PassUtils::PassVisitor<PrintArrVisitor>
{
private:
    std::string rl_path;
public:
    PrintArrVisitor(Allocator &al, const std::string &rl_path_) : PassVisitor(al, nullptr),
    rl_path(rl_path_) {
        pass_result.reserve(al, 1);

    }

    void visit_Print(const ASR::Print_t& x) {
        if( x.n_values == 1 && PassUtils::is_array(x.m_values[0]) ) {
            ASR::expr_t* arr_expr = x.m_values[0];

            int n_dims = PassUtils::get_rank(arr_expr);
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, current_scope);
            ASR::stmt_t* doloop = nullptr;
            ASR::stmt_t* empty_print_endl = LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, nullptr, 0, nullptr, nullptr));
            for( int i = n_dims - 1; i >= 0; i-- ) {
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = PassUtils::get_bound(arr_expr, i + 1, "lbound", al);
                head.m_end = PassUtils::get_bound(arr_expr, i + 1, "ubound", al);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;
                Vec<ASR::stmt_t*> doloop_body;
                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(arr_expr, idx_vars, al);
                    Vec<ASR::expr_t*> print_args;
                    print_args.reserve(al, 1);
                    print_args.push_back(al, ref);
                    ASR::stmt_t* print_stmt = LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc, nullptr,
                                                                 print_args.p, print_args.size(), nullptr, nullptr));
                    doloop_body.push_back(al, print_stmt);
                } else {
                    doloop_body.push_back(al, doloop);
                    doloop_body.push_back(al, empty_print_endl);
                }
                doloop = LFortran::ASRUtils::STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
            }
            pass_result.push_back(al, doloop);
            pass_result.push_back(al, empty_print_endl);
        }
    }

};

void pass_replace_print_arr(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    PrintArrVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
}


} // namespace LFortran
