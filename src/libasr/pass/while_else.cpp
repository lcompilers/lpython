#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/while_else.h>
#include <libasr/pass/stmt_walk_visitor.h>
#include <libasr/pass/pass_utils.h>
#include <stack>
#include <unordered_map>

namespace LCompilers {


class ExitVisitor : public ASR::StatementWalkVisitor<ExitVisitor> {
public:

    std::unordered_map<ASR::stmt_t*, ASR::symbol_t*> flag_map;
    std::stack<ASR::stmt_t*> loop_stack;
    
    ExitVisitor(Allocator &al)
        : StatementWalkVisitor(al) {}

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

class WhileLoopVisitor : public ASR::StatementWalkVisitor<WhileLoopVisitor>
{
private:
    int counter;

public:
    std::unordered_map<ASR::stmt_t*, ASR::symbol_t*> flag_map;

    WhileLoopVisitor(Allocator &al) : StatementWalkVisitor(al) {
        counter = 0;
        flag_map = {};
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        Location loc = x.base.base.loc;
        auto target_scope = current_scope;

        /*
        Creating a flag variable in case of a while-else loop
        Creates an if statement after the loop to check if the flag was changed
        */
        if (x.n_orelse > 0) {
            Vec<ASR::stmt_t*> result;
            result.reserve(al, 3);

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

            ASR::stmt_t *while_stmt = (ASR::stmt_t*)(&x);
            flag_map[while_stmt] = flag_symbol;

            result.push_back(al, assign_stmt);
            result.push_back(al, while_stmt);
            result.push_back(al, ASRUtils::STMT(
                        ASR::make_If_t(al, loc, flag_expr, x.m_orelse, x.n_orelse, nullptr, 0)));
            pass_result = result;
        } else {
            Vec<ASR::stmt_t*> result;
            result.reserve(al, 1);
            result.push_back(al, (ASR::stmt_t*)(&x));
            pass_result = result;
        }
    }
};

void pass_while_else(Allocator &al, ASR::TranslationUnit_t &unit,
                           const LCompilers::PassOptions& /*pass_options*/) {
    WhileLoopVisitor v(al);
    ExitVisitor e(al);
    v.visit_TranslationUnit(unit);
    e.flag_map = v.flag_map;
    e.visit_TranslationUnit(unit);
}


} // namespace LCompilers
