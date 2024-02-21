#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_do_loops.h>
#include <libasr/pass/stmt_walk_visitor.h>
#include <libasr/pass/pass_utils.h>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces do loops with while loops. The function
`pass_replace_do_loops` transforms the ASR tree in-place.

Converts:

    do i = a, b, c
        ...
    end do

to:

    i = a-c
    do while (i+c <= b)
        i = i+c
        ...
    end do

The comparison is >= for c<0.
*/
class DoLoopVisitor : public ASR::StatementWalkVisitor<DoLoopVisitor>
{
public:
    bool use_loop_variable_after_loop = false;
    DoLoopVisitor(Allocator &al) : StatementWalkVisitor(al) {
        counter = 0;
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        Location loc = x.base.base.loc;
        auto target_scope = current_scope;

        /*
        Creating a flag variable in case of a for-else loop
        Creates an if statement after the loop to check if the flag was changed
        */
        if (x.n_orelse > 0) {
            orelse = true;
            Vec<ASR::stmt_t*> result;
            auto loop = PassUtils::replace_doloop(al, x, -1, use_loop_variable_after_loop);
            result.reserve(al, 1+loop.size());

            Str s;
            s.from_str_view(std::string("_no_break_") + std::to_string(counter));
            counter++;

            ASR::ttype_t *bool_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
            ASR::expr_t *true_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, true, bool_type));
            ASR::symbol_t *flag_symbol = LCompilers::ASR::down_cast<ASR::symbol_t>(
                ASR::make_Variable_t(
                al, loc, target_scope,
                s.c_str(al), nullptr, 0, ASR::intentType::Local, nullptr, nullptr,
                ASR::storage_typeType::Default, bool_type, nullptr,
                ASR::abiType::Source, ASR::Public,
                ASR::presenceType::Required, false));
            flag = flag_symbol;
            target_scope-> add_symbol(s.c_str(al), flag_symbol);
            ASR::expr_t* flag_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, flag_symbol));

            ASR::stmt_t *assign_stmt = ASRUtils::STMT(
                    ASR::make_Assignment_t(al, loc, flag_expr, true_expr, nullptr));

            result.push_back(al, assign_stmt);
            for (const auto &stmt : loop) {
                result.push_back(al, stmt);
            }
            result.push_back(al, ASRUtils::STMT(
                        ASR::make_If_t(al, loc, flag_expr, x.m_orelse, x.n_orelse, nullptr, 0)));
            pass_result = result;
        } else {
            pass_result = PassUtils::replace_doloop(al, x, -1, use_loop_variable_after_loop);
        }
    }

    bool orelse;
    ASR::symbol_t *flag;


private:
    int counter;
};

class ExitVisitor : public ASR::StatementWalkVisitor<ExitVisitor> {
public:

    ASR::symbol_t *flag;
    
    ExitVisitor(Allocator &al)
        : StatementWalkVisitor(al) {}

    void visit_Exit(const ASR::Exit_t &x) {
        Vec<ASR::stmt_t*> result;
        result.reserve(al, 1);

        Location loc = x.base.base.loc;
        ASR::ttype_t *bool_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
        ASR::expr_t *false_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, false, bool_type));
        ASR::expr_t *flag_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, flag));
        ASR::stmt_t *assign_stmt = ASRUtils::STMT(
                ASR::make_Assignment_t(al, loc, flag_expr, false_expr, nullptr));
        result.push_back(al, assign_stmt);
        result.push_back(al, ASRUtils::STMT(ASR::make_Exit_t(al, loc, nullptr)));

        pass_result = result;
    }
};

void pass_replace_do_loops(Allocator &al, ASR::TranslationUnit_t &unit,
                           const LCompilers::PassOptions& pass_options) {
    DoLoopVisitor v(al);
    ExitVisitor e(al);
    // Each call transforms only one layer of nested loops, so we call it twice
    // to transform doubly nested loops:
    v.asr_changed = true;
    v.use_loop_variable_after_loop = pass_options.use_loop_variable_after_loop;
    while( v.asr_changed ) {
        v.asr_changed = false;
        v.visit_TranslationUnit(unit);

        if (v.orelse) {
            e.flag = v.flag;
            e.visit_TranslationUnit(unit);
        }
    }
}


} // namespace LCompilers
