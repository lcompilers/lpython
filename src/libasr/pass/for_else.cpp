#include "for_else.h"
#include "libasr/asr_scopes.h"

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/for_all.h>
#include <libasr/pass/stmt_walk_visitor.h>

#include <stack>

namespace LCompilers {

using ASR::is_a;
using ASR::down_cast;
using ASR::stmtType;

std::map<ASR::stmt_t*, ASR::symbol_t*> doLoopFlagMap;

class ExitVisitor : public ASR::StatementWalkVisitor<ExitVisitor> {
public:
    std::stack<ASR::stmt_t*> doLoopStack;

    ExitVisitor(Allocator &al) : StatementWalkVisitor(al) {
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        ASR::stmt_t *doLoopStmt = (ASR::stmt_t*)(&x);
        // std::cerr << doLoopStmt << " -- " << doLoopFlagMap[doLoopStmt] << std::endl;

        doLoopStack.push(doLoopStmt);

        ASR::DoLoop_t& xx = const_cast<ASR::DoLoop_t&>(x);
        this->transform_stmts(xx.m_body, xx.n_body);

        doLoopStack.pop();
    }

    void visit_Exit(const ASR::Exit_t &x) {
        if (doLoopStack.empty() ||
            // the current loop is not originally a ForElse loop
            doLoopFlagMap.find(doLoopStack.top()) == doLoopFlagMap.end())
            return;

        // std::cerr << "Break! inside " << doLoopStack.top() << std::endl;

        Vec<ASR::stmt_t*> result;
        result.reserve(al, 1);

        Location loc = x.base.base.loc;
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0));
        ASR::symbol_t* flag_symbol = doLoopFlagMap[doLoopStack.top()];
        ASR::expr_t* flag_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, flag_symbol));
        ASR::expr_t* false_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, false, bool_type));
        ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, flag_expr, false_expr, nullptr));
        result.push_back(al, assign_stmt);
        result.push_back(al, ASRUtils::STMT(ASR::make_Exit_t(al, loc)));

        pass_result = result;

        return;

        auto current = current_scope;
        while (current != nullptr) {
            std::cerr << "Scope " << current << std::endl;
            auto scope = current->get_scope();
            for (auto it = scope.begin(); it != scope.end(); it++) {
                if (is_a<ASR::Variable_t>(*it->second)) {
                    std::cerr << "  Variable: " << it->first << std::endl;
                } else if (is_a<ASR::Program_t>(*it->second)) {
                    std::cerr << "  Program: " << it->first << std::endl;
                } else if (is_a<ASR::Block_t>(*it->second)) {
                    std::cerr << "  Block: " << it->first << std::endl;
                    ASR::Block_t *block = down_cast<ASR::Block_t>(it->second);
                    std::cerr << "    body " << block->n_body << std::endl;
                    for (size_t i = 0; i < block->n_body; i++) {
                        std::cerr << "      " << getStmtType(block->m_body[i]->type) << std::endl;
                    }
                } else if (is_a<ASR::Function_t>(*it->second)) {
                    std::cerr << "  Function: " << it->first << std::endl;
                    ASR::Function_t *block = down_cast<ASR::Function_t>(it->second);
                    for (size_t i = 0; i < block->n_body; i++) {
                        std::cerr << "      " << getStmtType(block->m_body[i]->type) << std::endl;
                    }
                } else {
                    std::cerr << "  First: " << it->first << ", second: " << it->second->type << std::endl;
                }
            }
            current = current->parent;
        }
    }

    std::string getStmtType(stmtType t) {
        switch (t) {
        case stmtType::Allocate: return "Allocate";
        case stmtType::Assign: return "Assign";
        case stmtType::Assignment: return "Assignment";
        case stmtType::Associate: return "Associate";
        case stmtType::Cycle: return "Cycle";
        case stmtType::ExplicitDeallocate: return "ExplicitDeallocate";
        case stmtType::ImplicitDeallocate: return "ImplicitDeallocate";
        case stmtType::DoConcurrentLoop: return "DoConcurrentLoop";
        case stmtType::DoLoop: return "DoLoop";
        case stmtType::ForElse: return "ForElse";
        case stmtType::ErrorStop: return "ErrorStop";
        case stmtType::Exit: return "Exit";
        case stmtType::ForAllSingle: return "ForAllSingle";
        case stmtType::GoTo: return "GoTo";
        case stmtType::GoToTarget: return "GoToTarget";
        case stmtType::If: return "If";
        case stmtType::IfArithmetic: return "IfArithmetic";
        case stmtType::Print: return "Print";
        case stmtType::FileOpen: return "FileOpen";
        case stmtType::FileClose: return "FileClose";
        case stmtType::FileRead: return "FileRead";
        case stmtType::FileBackspace: return "FileBackspace";
        case stmtType::FileRewind: return "FileRewind";
        case stmtType::FileInquire: return "FileInquire";
        case stmtType::FileWrite: return "FileWrite";
        case stmtType::Return: return "Return";
        case stmtType::Select: return "Select";
        case stmtType::Stop: return "Stop";
        case stmtType::Assert: return "Assert";
        case stmtType::SubroutineCall: return "SubroutineCall";
        case stmtType::Where: return "Where";
        case stmtType::WhileLoop: return "WhileLoop";
        case stmtType::Nullify: return "Nullify";
        case stmtType::Flush: return "Flush";
        case stmtType::ListAppend: return "ListAppend";
        case stmtType::AssociateBlockCall: return "AssociateBlockCall";
        case stmtType::SelectType: return "SelectType";
        case stmtType::CPtrToPointer: return "CPtrToPointer";
        case stmtType::BlockCall: return "BlockCall";
        case stmtType::SetInsert: return "SetInsert";
        case stmtType::SetRemove: return "SetRemove";
        case stmtType::ListInsert: return "ListInsert";
        case stmtType::ListRemove: return "ListRemove";
        case stmtType::ListClear: return "ListClear";
        case stmtType::DictInsert: return "DictInsert";
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

    void visit_ForElse(const ASR::ForElse_t &x) {
        Location loc = x.base.base.loc;

        Vec<ASR::stmt_t*> result;
        result.reserve(al, 1);

        // create a boolean flag and set it to true
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0));
        ASR::expr_t* true_expr = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, true, bool_type));

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

        // convert head and body to DoLoop
        ASR::stmt_t *doLoopStmt = ASRUtils::STMT(
            ASR::make_DoLoop_t(al, loc, x.m_head, x.m_body, x.n_body)
        );
        result.push_back(al, doLoopStmt);

        // this DoLoop corresponds to the current flag
        doLoopFlagMap[doLoopStmt] = flag_symbol;
        // std::cerr << doLoopStmt << " -> " << flag_expr << std::endl;

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
    ExitVisitor v2(al);
    v2.visit_TranslationUnit(unit);
}

} // namespace LCompilers
