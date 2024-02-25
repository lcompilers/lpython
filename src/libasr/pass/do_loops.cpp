#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_do_loops.h>
#include <libasr/pass/stmt_walk_visitor.h>
#include <libasr/pass/pass_utils.h>
#include <stack>

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
        flag_map = {};
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        Location loc = x.base.base.loc;
        auto target_scope = current_scope;

        /*
        Creating a flag variable in case of a for-else loop
        Creates an if statement after the loop to check if the flag was changed
        */
        if (x.n_orelse > 0) {
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
            target_scope-> add_symbol(s.c_str(al), flag_symbol);
            ASR::expr_t* flag_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, flag_symbol));

            ASR::stmt_t *assign_stmt = ASRUtils::STMT(
                    ASR::make_Assignment_t(al, loc, flag_expr, true_expr, nullptr));

            result.push_back(al, assign_stmt);

            for (const auto &stmt : loop) {
                result.push_back(al, stmt);
            }

            ASR::stmt_t *while_stmt = result[2];
            flag_map[while_stmt] = flag_symbol;

            result.push_back(al, ASRUtils::STMT(
                        ASR::make_If_t(al, loc, flag_expr, x.m_orelse, x.n_orelse, nullptr, 0)));
            pass_result = result;
        } else {
            pass_result = PassUtils::replace_doloop(al, x, -1, use_loop_variable_after_loop);
        }
    }

    std::unordered_map<ASR::stmt_t*, ASR::symbol_t*> flag_map;


private:
    int counter;
};

class ExitVisitor : public ASR::StatementWalkVisitor<ExitVisitor> {
public:

    std::unordered_map<ASR::stmt_t*, ASR::symbol_t*> flag_map;
    std::stack<ASR::stmt_t*> loop_stack;
    
    ExitVisitor(Allocator &al)
        : StatementWalkVisitor(al) {}

    //void visit_DoLoop(const ASR::DoLoop_t &x) {
        //ASR::stmt_t *do_loop_stmt = (ASR::stmt_t*)(&x);
            
        //loop_stack.push(do_loop_stmt);
        //ASR::DoLoop_t &xx = const_cast<ASR::DoLoop_t&>(x);
        //this->transform_stmts(xx.m_body, xx.n_body);

        //loop_stack.pop();
    //}

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        ASR::stmt_t *while_stmt = (ASR::stmt_t*)(&x);
            
        loop_stack.push(while_stmt);
        ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);

        loop_stack.pop();
    }

    void visit_Exit(const ASR::Exit_t &x) {
        if (loop_stack.empty() ||
            flag_map.find(loop_stack.top()) == flag_map.end())
            return;

        Vec<ASR::stmt_t*> result;
        result.reserve(al, 2);

        Location loc = x.base.base.loc;
        ASR::ttype_t *bool_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
        ASR::expr_t *false_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, false, bool_type));
        ASR::expr_t *flag_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, flag_map.at(loop_stack.top())));
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
    while ( v.asr_changed ) {
        v.asr_changed = false;
        v.visit_TranslationUnit(unit);
    }
    e.flag_map = v.flag_map;
    e.visit_TranslationUnit(unit);
}


} // namespace LCompilers
