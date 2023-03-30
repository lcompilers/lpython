#include "for_else.h"
#include "libasr/asr_scopes.h"

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/for_all.h>
#include <libasr/pass/stmt_walk_visitor.h>

namespace LCompilers {

class ExitVisitor : public ASR::StatementWalkVisitor<ExitVisitor> {
public:
    ASR::expr_t* flag_expr;

    ExitVisitor(Allocator &al, ASR::expr_t* flag_expr) : StatementWalkVisitor(al) {
        this->flag_expr = flag_expr;
    }

    void visit_Exit(const ASR::Exit_t &x) {
        std::cerr << "Break!" << std::endl;

        // Vec<ASR::stmt_t*> result;
        // result.reserve(al, 1);

        // Location loc = x.base.base.loc;
        // ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0));
        // ASR::expr_t* false_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, false, bool_type));
        // ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, flag_expr, false_expr, nullptr));
        // result.push_back(al, assign_stmt);

        // pass_result = result;

        auto scope = current_scope->get_scope();
        for (auto it = scope.begin(); it != scope.end(); it++) {
            std::cerr << "First: " << it->first << ", second: " << it->second << std::endl;
        }

    }
};

class ForElseVisitor : public ASR::StatementWalkVisitor<ForElseVisitor>
{
public:
    ForElseVisitor(Allocator &al) : StatementWalkVisitor(al) {
        counter = 0;
    }

    int counter;

    void visit_Exit(const ASR::Exit_t &x) {
        std::cerr << "!!!! Break!" << std::endl;
    }

    void visit_ForElse(const ASR::ForElse_t &x) {
        Location loc = x.base.base.loc;

        Vec<ASR::stmt_t*> result;
        result.reserve(al, 1);

        // create a boolean flag and set it to false
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0));
        ASR::expr_t* true_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, true, bool_type));
        ASR::expr_t* false_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, false, bool_type));

        auto target_scope = current_scope; // al.make_new<SymbolTable>(current_scope);

        Str s;
        s.from_str_view(std::string("_no_break_") + std::to_string(counter));
        counter++;

        ASR::symbol_t* flag_symbol = LCompilers::ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(
                al, loc, target_scope,
                s.c_str(al), nullptr, 0, ASR::intentType::Local, nullptr, nullptr,
                ASR::storage_typeType::Default, bool_type,
                ASR::abiType::Source, ASR::Public,
                ASR::presenceType::Required, false));
        ASR::expr_t* flag_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, flag_symbol));
        target_scope->add_symbol(s.c_str(al), flag_symbol);

        ASR::stmt_t* assign_stmt = ASRUtils::STMT(
            ASR::make_Assignment_t(al, loc, flag_expr, true_expr, nullptr));
        result.push_back(al, assign_stmt);

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);

        for (size_t i = 0; i < x.n_body; i++) {
            ASR::stmt_t *stmt = x.m_body[i];

            if (stmt->type == ASR::stmtType::Exit) {
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(
                    ASR::make_Assignment_t(al, loc, flag_expr, false_expr, nullptr));
                result.push_back(al, assign_stmt);
            }

            body.push_back(al, stmt);
        }

        // convert head and body to DoLoop
        ASR::stmt_t *stmt = ASRUtils::STMT(
            ASR::make_DoLoop_t(al, loc, x.m_head, body.p, body.size())
        );
        result.push_back(al, stmt);

        // add an If block that executes the orelse statements when the flag is true
        result.push_back(
            al, ASRUtils::STMT(
                ASR::make_If_t(al, loc, flag_expr, x.m_orelse, x.n_orelse, nullptr, 0)));

        pass_result = result;
    }
};

void pass_replace_forelse(Allocator &al, ASR::TranslationUnit_t &unit,
                          const LCompilers::PassOptions& /*pass_options*/) {
    ForElseVisitor v(al);
    v.visit_TranslationUnit(unit);
    // ExitVisitor v2(al, nullptr);
    // v2.visit_TranslationUnit(unit);
}

} // namespace LCompilers
