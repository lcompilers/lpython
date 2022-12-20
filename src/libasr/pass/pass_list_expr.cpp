#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_list_expr.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LFortran {

using ASR::down_cast;

/*
 * This ASR pass handles all the high level list features.
 */


class ListExprReplacer : public ASR::BaseExprReplacer<ListExprReplacer>
{
private:

    Allocator& al;
    ASR::TranslationUnit_t &unit;
    std::map<std::string, ASR::symbol_t*> &list_concat_func_map;
    std::map<std::string, ASR::symbol_t*> &list_section_func_map;

public:
    ListExprReplacer(Allocator &al_, ASR::TranslationUnit_t &unit_,
                      std::map<std::string, ASR::symbol_t*> &list_concat_func_map_,
                      std::map<std::string, ASR::symbol_t*> &list_section_func_map_) :
        al(al_), unit(unit_),
        list_concat_func_map(list_concat_func_map_),
        list_section_func_map(list_section_func_map_)
        { }

    void create_while_loop(Location &loc, SymbolTable* symtab,
                           ASR::expr_t* a_list, ASR::expr_t* start,
                           ASR::expr_t* end, ASR::expr_t* step,
                           ASR::expr_t* res_list,
                           Vec<ASR::stmt_t*>& body, ASR::ttype_t* item_type) {
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, 1, loc, al, symtab);
        ASR::stmt_t* loop_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], start, nullptr));
        body.push_back(al, loop_stmt);

        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(
            al, loc, 4, nullptr, 0));
        ASR::expr_t *const_zero = ASRUtils::EXPR(
                    ASR::make_IntegerConstant_t(al, loc, 0, int_type));

        ASR::expr_t* loop_test_1 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, step, ASR::cmpopType::Gt, const_zero, bool_type, nullptr));
        ASR::expr_t* loop_test_2 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, idx_vars[0], ASR::cmpopType::GtE, start, bool_type, nullptr));
        ASR::expr_t* loop_test_3 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, idx_vars[0], ASR::cmpopType::Lt, end, bool_type, nullptr));
        ASR::expr_t *loop_test_11 = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al,
                loc, loop_test_1, ASR::logicalbinopType::And, loop_test_2, bool_type, nullptr));
        loop_test_11 = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al,
                loc, loop_test_11, ASR::logicalbinopType::And, loop_test_3, bool_type, nullptr));

        ASR::expr_t* loop_test_4 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, step, ASR::cmpopType::Lt, const_zero, bool_type, nullptr));
        ASR::expr_t* loop_test_5 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, idx_vars[0], ASR::cmpopType::LtE, start, bool_type, nullptr));
        ASR::expr_t* loop_test_6 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, idx_vars[0], ASR::cmpopType::Gt, end, bool_type, nullptr));
        ASR::expr_t *loop_test_22 = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al,
                loc, loop_test_4, ASR::logicalbinopType::And, loop_test_5, bool_type, nullptr));
        loop_test_22 = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al,
                loc, loop_test_22, ASR::logicalbinopType::And, loop_test_6, bool_type, nullptr));

        ASR::expr_t *loop_test = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al,
                loc, loop_test_11, ASR::logicalbinopType::Or, loop_test_22, bool_type, nullptr));

        Vec<ASR::stmt_t*> loop_body;
        loop_body.reserve(al, 2);
        loop_stmt = ASRUtils::STMT(ASR::make_ListAppend_t(
            al, loc, res_list, ASRUtils::EXPR(ASR::make_ListItem_t(
                al, loc, a_list, idx_vars[0], item_type, nullptr))));
        loop_body.push_back(al, loop_stmt);

        loop_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                al, loc, idx_vars[0], ASR::binopType::Add, step,
                int_type, nullptr)), nullptr));
        loop_body.push_back(al, loop_stmt);

        loop_stmt = ASRUtils::STMT(ASR::make_WhileLoop_t(
            al, loc, loop_test, loop_body.p, loop_body.n));
        body.push_back(al, loop_stmt);
    }

    #define create_args(x, type, symtab) { \
        ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>( \
            ASR::make_Variable_t(al, loc, symtab, \
            s2c(al, x), nullptr, 0, ASR::intentType::In, nullptr, nullptr, \
            ASR::storage_typeType::Default, type, \
            ASR::abiType::Source, ASR::accessType::Public, \
            ASR::presenceType::Required, false)); \
        ASR::expr_t* arg_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg)); \
        arg_exprs.push_back(al, arg_expr); \
        symtab->add_symbol(x, arg); \
    }

    void create_list_section_func(Location& loc,
                SymbolTable*& global_scope, ASR::ttype_t* list_type) {
        /*
            def _lcompilers_list_section(a_list: list[i32], start: i32,
                    end: i32, step: i32, is_start_present: bool,
                    is_end_present: bool): -> list[i32]:
                result_list: list[i32]
                result_list = []

                a_len = len(a)
                if start < 0:
                    start += a_len
                if end < 0:
                    end += a_len

                if not is_start_present:
                    if step > 0:
                        start = 0
                    else:
                        start = a_len - 1

                if not is_end_present:
                    if step > 0:
                        end = a_len
                    else:
                        end = -1

                if step > 0:
                    if end > a_len:
                        end = a_len
                else:
                    if start >= a_len:
                        start = a_len - 1
                __1_k: i32 = start
                while ((step > 0 and __1_k >= start and __1_k < end) or
                    (step < 0 and __1_k <= start and __1_k > end)):
                    result_list.append(a_list[__1_k])
                    __1_k = __1_k + step
                return result_list
        */

        SymbolTable* list_section_symtab = al.make_new<SymbolTable>(global_scope);
        std::string list_type_name = ASRUtils::type_to_str_python(list_type);
        std::string fn_name = global_scope->get_unique_name("_lcompilers_list_section_" + list_type_name);
        ASR::ttype_t* item_type = ASR::down_cast<ASR::List_t>(list_type)->m_type;
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));

        Vec<ASR::expr_t*> arg_exprs;
        arg_exprs.reserve(al, 4);

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 4);

        // Declare `a_list`, `start`, `end` and `step`
        create_args("a_list", list_type, list_section_symtab)
        create_args("start", int_type, list_section_symtab)
        create_args("end", int_type, list_section_symtab)
        create_args("step", int_type, list_section_symtab)
        create_args("is_start_present", bool_type, list_section_symtab)
        create_args("is_end_present", bool_type, list_section_symtab)

        // Declare `result_list`
        ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, list_section_symtab,
            s2c(al, "result_list"), nullptr, 0, ASR::intentType::Local, nullptr, nullptr,
            ASR::storage_typeType::Default, list_type,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false));
        ASR::expr_t* res_list = ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg));
        list_section_symtab->add_symbol("result_list", arg);

        // Empty List
        ASR::expr_t* value = ASRUtils::EXPR(ASR::make_ListConstant_t(
            al, loc, nullptr, 0, list_type));
        // Initialize `result_list` with `empty value`
        ASR::stmt_t* list_section_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, res_list, value, nullptr));
        body.push_back(al, list_section_stmt);

        ASR::expr_t *a_len = ASRUtils::EXPR(ASR::make_ListLen_t(
                            al, loc, arg_exprs[0], int_type, nullptr));
        ASR::expr_t *const_one = ASRUtils::EXPR(
                    ASR::make_IntegerConstant_t(al, loc, 1, int_type));
        ASR::expr_t *const_zero = ASRUtils::EXPR(
                    ASR::make_IntegerConstant_t(al, loc, 0, int_type));
        ASR::expr_t *a_len_1 = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al,
                    loc, a_len, ASR::binopType::Sub, const_one, int_type, nullptr));
        ASR::expr_t *minus_one = ASRUtils::EXPR(
                    ASR::make_IntegerConstant_t(al, loc, -1, int_type));

        for (int i=1; i<3; i++)
        {
            ASR::expr_t* a_test = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                al, loc, arg_exprs[i], ASR::cmpopType::Lt, const_zero,
                bool_type, nullptr));

            Vec<ASR::stmt_t*> if_body;
            if_body.reserve(al, 1);
            ASR::stmt_t* if_body_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, arg_exprs[i], ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al,
                loc, arg_exprs[i], ASR::binopType::Add, a_len,
                int_type, nullptr)), nullptr));
            if_body.push_back(al, if_body_stmt);

            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test,
                if_body.p, if_body.n, nullptr, 0));
            body.push_back(al, list_section_stmt);
        }

        // If statement
        {
            ASR::expr_t* a_test = ASRUtils::EXPR(ASR::make_LogicalNot_t(al,
                loc, arg_exprs[4], bool_type, nullptr));

            Vec<ASR::stmt_t*> if_body, if_body_1, else_body_1;
            if_body_1.reserve(al, 1);
            if_body.reserve(al, 1);
            else_body_1.reserve(al, 1);
            ASR::expr_t* a_test_1 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                    arg_exprs[3], ASR::cmpopType::Gt, const_zero, bool_type, nullptr));
            ASR::stmt_t* if_body_stmt_1 = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, arg_exprs[1], const_zero, nullptr));
            if_body_1.push_back(al, if_body_stmt_1);

            ASR::stmt_t* else_body_stmt_1 = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, arg_exprs[1], a_len_1, nullptr));
            else_body_1.push_back(al, else_body_stmt_1);

            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test_1,
                if_body_1.p, if_body_1.n, else_body_1.p, else_body_1.n));
            if_body.push_back(al, list_section_stmt);
            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test,
                if_body.p, if_body.n, nullptr, 0));
            body.push_back(al, list_section_stmt);
        }

        // If statement
        {
            ASR::expr_t* a_test = ASRUtils::EXPR(ASR::make_LogicalNot_t(al,
                loc, arg_exprs[5], bool_type, nullptr));

            Vec<ASR::stmt_t*> if_body, if_body_1, else_body_1;
            if_body_1.reserve(al, 1);
            if_body.reserve(al, 1);
            else_body_1.reserve(al, 1);
            ASR::expr_t* a_test_1 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                    arg_exprs[3], ASR::cmpopType::Gt, const_zero, bool_type, nullptr));
            ASR::stmt_t* if_body_stmt_1 = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, arg_exprs[2], a_len, nullptr));
            if_body_1.push_back(al, if_body_stmt_1);

            ASR::stmt_t* else_body_stmt_1 = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, arg_exprs[2], minus_one, nullptr));
            else_body_1.push_back(al, else_body_stmt_1);

            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test_1,
                if_body_1.p, if_body_1.n, else_body_1.p, else_body_1.n));
            if_body.push_back(al, list_section_stmt);
            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test,
                if_body.p, if_body.n, nullptr, 0));
            body.push_back(al, list_section_stmt);
        }

        // If statement
        {
            ASR::expr_t* a_test = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                    arg_exprs[3], ASR::cmpopType::Gt, const_zero, bool_type, nullptr));

            Vec<ASR::stmt_t*> if_body, if_body_1, else_body, if_body_2;
            if_body_1.reserve(al, 1);
            if_body_2.reserve(al, 1);
            if_body.reserve(al, 1);
            else_body.reserve(al, 1);
            ASR::expr_t* a_test_1 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                    arg_exprs[2], ASR::cmpopType::Gt, a_len, bool_type, nullptr));
            ASR::stmt_t* if_body_stmt_1 = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, arg_exprs[2], a_len, nullptr));
            if_body_1.push_back(al, if_body_stmt_1);
            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test_1,
                if_body_1.p, if_body_1.n, nullptr, 0));
            if_body.push_back(al, list_section_stmt);

            ASR::expr_t* a_test_2 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                    arg_exprs[1], ASR::cmpopType::GtE, a_len, bool_type, nullptr));
            ASR::stmt_t* if_body_stmt_2 = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, arg_exprs[1], a_len_1, nullptr));
            if_body_2.push_back(al, if_body_stmt_2);
            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test_2,
                if_body_2.p, if_body_2.n, nullptr, 0));
            else_body.push_back(al, list_section_stmt);

            list_section_stmt = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test,
                if_body.p, if_body.n, else_body.p, else_body.n));
            body.push_back(al, list_section_stmt);
        }


        // Copy a section from the list
        // Declare `__1_k` for iterations and assign `start` value
        create_while_loop(loc, list_section_symtab, arg_exprs[0], arg_exprs[1],
            arg_exprs[2], arg_exprs[3], res_list, body, item_type);

        // Return
        list_section_stmt = ASRUtils::STMT(ASR::make_Return_t(al, loc));
        body.push_back(al, list_section_stmt);

        ASR::asr_t *fn = ASR::make_Function_t(
            al, loc,
            /* a_symtab */ list_section_symtab,
            /* a_name */ s2c(al, fn_name),
            nullptr, 0,
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
        ASR::symbol_t *fn_sym = ASR::down_cast<ASR::symbol_t>(fn);
        global_scope->add_symbol(fn_name, fn_sym);
        list_section_func_map[list_type_name] = fn_sym;
    }

/*
    This function replaces `ListSection` with a `FunctionCall` where
    the a list along with left, right and step of the `array_index` node
    are passed as arguments to the function.
    Then, the function returns the section of the list as a result.

    Converts:
        y = x[1:3] // lists

    to:
        y = _lcompilers_list_section(x, 1, 3, 1)
*/

    void replace_ListSection(const ASR::ListSection_t* x) {
        Location loc = x->base.base.loc;
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));
        Vec<ASR::call_arg_t> args;
        args.reserve(al, 4);
        ASR::call_arg_t call_arg;
        call_arg.loc = x->m_a->base.loc;
        call_arg.m_value = x->m_a;
        args.push_back(al, call_arg);
        ASR::expr_t *is_start_present, *is_end_present;
        if (x->m_section.m_left != nullptr) {
            call_arg.loc = x->m_section.m_left->base.loc;
            call_arg.m_value = x->m_section.m_left;
            is_start_present = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al,
                    loc, true, bool_type));
        } else {
            call_arg.loc = loc;
            call_arg.m_value = ASRUtils::EXPR(make_IntegerConstant_t(
                al, loc, 0, int_type));
            is_start_present = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al,
                    loc, false, bool_type));
        }
        args.push_back(al, call_arg);
        if (x->m_section.m_right != nullptr) {
            call_arg.loc = x->m_section.m_right->base.loc;
            call_arg.m_value = x->m_section.m_right;
            is_end_present = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al,
                    loc, true, bool_type));
        } else {
            call_arg.loc = loc;
            call_arg.m_value = ASRUtils::EXPR(ASR::make_ListLen_t(
                al, loc, x->m_a, int_type, nullptr));
            is_end_present = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al,
                    loc, false, bool_type));
        }
        args.push_back(al, call_arg);
        if (x->m_section.m_step != nullptr) {
            call_arg.loc = x->m_section.m_step->base.loc;
            call_arg.m_value = x->m_section.m_step;
        } else {
            call_arg.loc = loc;
            call_arg.m_value = ASRUtils::EXPR(make_IntegerConstant_t(
                al, loc, 1, int_type));
        }
        args.push_back(al, call_arg);
        call_arg.loc = x->m_a->base.loc;
        call_arg.m_value = is_start_present;
        args.push_back(al, call_arg);
        call_arg.loc = x->m_a->base.loc;
        call_arg.m_value = is_end_present;
        args.push_back(al, call_arg);

        std::string list_type_name = ASRUtils::type_to_str_python(x->m_type);
        if (list_section_func_map.find(list_type_name) == list_section_func_map.end()) {
            create_list_section_func(unit.base.base.loc,
                unit.m_global_scope, x->m_type);
        }
        ASR::symbol_t *fn_sym = list_section_func_map[list_type_name];
        *current_expr = ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
            fn_sym, nullptr, args.p, args.n, x->m_type, nullptr, nullptr));
    }

    void create_concat_function(Location& loc,
            SymbolTable*& global_scope, ASR::ttype_t* list_type) {
        /*
            def _lcompilers_list_concat(left_list: list[i32],
                    right_list: list[i32]) -> list[i32]:
                result_list: list[i32]
                result_list = []
                __1_k: i32 = 0
                while len(left_list) > __1_k:
                    result_list.append(left_list[__1_k])
                    __1_k += 1
                __1_k = 0
                while len(right_list) > __1_k:
                    result_list.append(right_list[__1_k])
                    __1_k += 1
                return result_list
        */
        SymbolTable* list_concat_symtab = al.make_new<SymbolTable>(global_scope);
        std::string list_type_name = ASRUtils::type_to_str_python(list_type);
        std::string fn_name = global_scope->get_unique_name("_lcompilers_list_concat_" + list_type_name);

        Vec<ASR::expr_t*> arg_exprs;
        arg_exprs.reserve(al, 2);

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 4);

        // Declare `left_list` and `right_list`
        create_args("left_list", list_type, list_concat_symtab)
        create_args("right_list", list_type, list_concat_symtab)

        // Declare `result_list`
        ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, list_concat_symtab,
            s2c(al, "result_list"), nullptr, 0, ASR::intentType::Local, nullptr, nullptr,
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
        ASR::ttype_t* item_type = ASR::down_cast<ASR::List_t>(list_type)->m_type;

        // Declare `__1_k` for iterations and assign `0`
        // Copy `left_list` contents
        create_while_loop(loc, list_concat_symtab, arg_exprs[0], ASRUtils::EXPR(
            make_IntegerConstant_t(al, loc, 0, int_type)), ASRUtils::EXPR(
                ASR::make_ListLen_t(al, loc, arg_exprs[0], int_type, nullptr)
            ), ASRUtils::EXPR(make_IntegerConstant_t(al, loc, 1, int_type)),
            res_list, body, item_type);

        // Copy `right_list` contents
        create_while_loop(loc, list_concat_symtab, arg_exprs[1], ASRUtils::EXPR(
            make_IntegerConstant_t(al, loc, 0, int_type)), ASRUtils::EXPR(
                ASR::make_ListLen_t(al, loc, arg_exprs[1], int_type, nullptr)
            ), ASRUtils::EXPR(make_IntegerConstant_t(al, loc, 1, int_type)),
            res_list, body, item_type);

        // Return
        list_concat_stmt = ASRUtils::STMT(ASR::make_Return_t(al, loc));
        body.push_back(al, list_concat_stmt);

        ASR::asr_t *fn = ASR::make_Function_t(
            al, loc,
            /* a_symtab */ list_concat_symtab,
            /* a_name */ s2c(al, fn_name),
            nullptr, 0,
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
        ASR::symbol_t *fn_sym = ASR::down_cast<ASR::symbol_t>(fn);
        global_scope->add_symbol(fn_name, fn_sym);
        list_concat_func_map[list_type_name] = fn_sym;
    }

/*
    This function replaces ListConcat with a FunctionCall where
    the two lists are passed as arguments to the function.
    Then, the function returns the concatenated list as a result.

    Converts:
        x = i + j // lists

    to:
        x = _lcompilers_list_concat(i, j)
*/

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
        std::string list_type_name = ASRUtils::type_to_str_python(x->m_type);
        if (list_concat_func_map.find(list_type_name) == list_concat_func_map.end()) {
            create_concat_function(unit.base.base.loc,
                unit.m_global_scope, x->m_type);
        }
        ASR::symbol_t *fn_sym = list_concat_func_map[list_type_name];
        *current_expr = ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
            fn_sym, nullptr, args.p, 2, x->m_type, nullptr, nullptr));
    }

};

class ListExprVisitor : public ASR::CallReplacerOnExpressionsVisitor<ListExprVisitor>
{
private:

    ListExprReplacer replacer;
    std::map<std::string, ASR::symbol_t*> list_concat_func_map, list_section_func_map;

public:

    ListExprVisitor(Allocator& al_, ASR::TranslationUnit_t &unit_) :
        replacer(al_, unit_, list_concat_func_map, list_section_func_map)
        { }

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.replace_expr(*current_expr);
    }

};

void pass_list_expr(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/) {
    ListExprVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LFortran
