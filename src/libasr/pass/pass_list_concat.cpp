#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_list_concat.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LFortran {

using ASR::down_cast;

/*
    This ASR pass replaces ListConcat with a FunctionCall where
    the two lists are passed as arguments to the function.
    Then, the function returns the concatenated list as a result.

    Converts:
        x = i + j // lists

    to:
        x = _lcompilers_list_concat(i, j)
 */


class ReplaceListConcat : public ASR::BaseExprReplacer<ReplaceListConcat>
{
private:

    Allocator& al;
    ASR::TranslationUnit_t &unit;
    ASR::symbol_t* &list_concat_func_name;

public:
    ReplaceListConcat(Allocator &al_, ASR::TranslationUnit_t &unit_,
                      ASR::symbol_t* &list_concat_func_name_) :
        al(al_), unit(unit_), list_concat_func_name(list_concat_func_name_)
        { }

    ASR::symbol_t* create_concat_function(Location& loc,
            SymbolTable*& global_scope, ASR::ttype_t* list_type) {
        /*
            def _lcompilers_list_concat(left_list: list[i32],
                    right_list: list[i32]) -> list[i32]:
                result_list: list[i32]
                result_list = []
                1_k: i32 = 0
                while len(left_list) > 1_k:
                    result_list.append(left_list[1_k])
                    1_k += 1
                1_k = 0
                while len(right_list) > 1_k:
                    result_list.append(right_list[1_k])
                    1_k += 1
                return result_list
        */
        SymbolTable* list_concat_symtab = al.make_new<SymbolTable>(global_scope);
        std::string fn_name = global_scope->get_unique_name("_lcompilers_list_concat");

        Vec<ASR::expr_t*> arg_exprs;
        arg_exprs.reserve(al, 2);

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 4);

        // Declare `left_list`
        ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, list_concat_symtab,
            s2c(al, "left_list"), ASR::intentType::In, nullptr, nullptr,
            ASR::storage_typeType::Default, list_type,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false));
        ASR::expr_t* arg_expr = ASRUtils::EXPR(
            ASR::make_Var_t(al, loc, arg));
        arg_exprs.push_back(al, arg_expr);
        list_concat_symtab->add_symbol("left_list", arg);

        // Declare `right_list`
        arg = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, list_concat_symtab,
            s2c(al, "right_list"), ASR::intentType::In, nullptr, nullptr,
            ASR::storage_typeType::Default, list_type,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false));
        arg_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg));
        arg_exprs.push_back(al, arg_expr);
        list_concat_symtab->add_symbol("right_list", arg);

        // Declare `right_list`
        arg = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, list_concat_symtab,
            s2c(al, "result_list"), ASR::intentType::Local, nullptr, nullptr,
            ASR::storage_typeType::Default, list_type,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false));
        ASR::expr_t* res_list = ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg));
        list_concat_symtab->add_symbol("result_list", arg);

        // Empty List
        ASR::expr_t* value = ASRUtils::EXPR(ASR::make_ListConstant_t(
            al, loc, nullptr, 0, list_type));
        // Initialize `result_list` with `empty value`
        ASR::stmt_t* list_concat_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, res_list, value, nullptr));
        body.push_back(al, list_concat_stmt);
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* item_type = ASR::down_cast<ASR::List_t>(list_type)->m_type;

        // Declare `1_k` for iterations and assign `0`
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, 1, loc, al, list_concat_symtab);
        list_concat_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], ASRUtils::EXPR(make_IntegerConstant_t(
                al, loc, 0, int_type)), nullptr));
        body.push_back(al, list_concat_stmt);

        // Copy `left_list` contents
        ASR::expr_t* loop_test = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, ASRUtils::EXPR(ASR::make_ListLen_t(
                al, loc, arg_exprs[0], int_type, nullptr)
            ), ASR::cmpopType::Gt, idx_vars[0], bool_type, nullptr));
        Vec<ASR::stmt_t*> loop_body;
        loop_body.reserve(al, 2);
        list_concat_stmt = ASRUtils::STMT(ASR::make_ListAppend_t(
            al, loc, res_list, ASRUtils::EXPR(ASR::make_ListItem_t(
                al, loc, arg_exprs[0], idx_vars[0], item_type, nullptr))));
        loop_body.push_back(al, list_concat_stmt);
        list_concat_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                al, loc, idx_vars[0], ASR::binopType::Add, ASRUtils::EXPR(
                    make_IntegerConstant_t(al, loc, 1, int_type)),
                int_type, nullptr)), nullptr));
        loop_body.push_back(al, list_concat_stmt);
        list_concat_stmt = ASRUtils::STMT(ASR::make_WhileLoop_t(
            al, loc, loop_test, loop_body.p, loop_body.n));
        body.push_back(al, list_concat_stmt);

        list_concat_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], ASRUtils::EXPR(make_IntegerConstant_t(
                al, loc, 0, int_type)), nullptr));
        body.push_back(al, list_concat_stmt);

        // Copy `right_list` contents
        loop_test = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, ASRUtils::EXPR(ASR::make_ListLen_t(
                al, loc, arg_exprs[1], int_type, nullptr)
            ), ASR::cmpopType::Gt, idx_vars[0], bool_type, nullptr));
        loop_body.p = nullptr, loop_body.n = 0;
        loop_body.reserve(al, 2);
        list_concat_stmt = ASRUtils::STMT(ASR::make_ListAppend_t(
            al, loc, res_list, ASRUtils::EXPR(ASR::make_ListItem_t(
                al, loc, arg_exprs[1], idx_vars[0], item_type, nullptr))));
        loop_body.push_back(al, list_concat_stmt);
        list_concat_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                al, loc, idx_vars[0], ASR::binopType::Add, ASRUtils::EXPR(
                    make_IntegerConstant_t(al, loc, 1, int_type)),
                int_type, nullptr)), nullptr));
        loop_body.push_back(al, list_concat_stmt);
        list_concat_stmt = ASRUtils::STMT(ASR::make_WhileLoop_t(
            al, loc, loop_test, loop_body.p, loop_body.n));
        body.push_back(al, list_concat_stmt);

        // Return
        list_concat_stmt = ASRUtils::STMT(ASR::make_Return_t(al, loc));
        body.push_back(al, list_concat_stmt);

        ASR::asr_t *fn = ASR::make_Function_t(
            al, loc,
            /* a_symtab */ list_concat_symtab,
            /* a_name */ s2c(al, fn_name),
            /* a_args */ arg_exprs.p,
            /* n_args */ arg_exprs.n,
            /* a_body */ body.p,
            /* n_body */ body.n,
            /* a_return_var */ res_list,
            ASR::abiType::Source,
            ASR::accessType::Public, ASR::deftypeType::Implementation,
            nullptr,
            false, false, false, false, false,
            nullptr, 0,
            nullptr, 0,
            false);
        global_scope->add_symbol(fn_name, down_cast<ASR::symbol_t>(fn));
        return ASR::down_cast<ASR::symbol_t>(fn);
    }

    void replace_ListConcat(const ASR::ListConcat_t* x) {
        Location loc = x->base.base.loc;
        Vec<ASR::call_arg_t> args;
        args.reserve(al, 2);
        ASR::call_arg_t left_list, right_list;
        left_list.loc = x->m_left->base.loc;
        left_list.m_value = x->m_left;
        args.push_back(al, left_list);
        right_list.loc = x->m_right->base.loc;
        right_list.m_value = x->m_right;
        args.push_back(al, right_list);
        if (list_concat_func_name == nullptr) {
            list_concat_func_name = create_concat_function(unit.base.base.loc,
                unit.m_global_scope, x->m_type);
        }
        *current_expr = ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
            list_concat_func_name, nullptr, args.p, 2, x->m_type, nullptr, nullptr));
    }

};

class ListConcatVisitor : public ASR::CallReplacerOnExpressionsVisitor<ListConcatVisitor>
{
private:

    ReplaceListConcat replacer;
    ASR::symbol_t* list_concat_func = nullptr;

public:

    ListConcatVisitor(Allocator& al_, ASR::TranslationUnit_t &unit_) :
        replacer(al_, unit_, list_concat_func)
        { }

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.replace_expr(*current_expr);
    }

};

void pass_list_concat(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/) {
    ListConcatVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran

