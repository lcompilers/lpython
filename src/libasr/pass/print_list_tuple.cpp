#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/print_list_tuple.h>

namespace LCompilers {

/*
This ASR pass replaces print list or print tuple with print every value,
comma_space, brackets and newline. The function
`pass_replace_print_list_tuple` transforms the ASR tree in-place.

Converts:

    print(a, b, l, sep="pqr", end="xyz") # l is a list (but not a & b)

to:
    print(a, b, sep="pqr", end="")
    print("[", end="")
    for i in range(len(l)):
        print(l[i], end="")
        if i < len(l) - 1:
            print(", ", end="")
    print("]", sep="pqr", end="xyz")

for nested lists it transforms to:

    print("[", end="")
    for i in range(len(l)):
        # print(l[i], end="") is replaced by the following code

        print("[", end="")
        for i1 in range(len(l[i])):
            print(l[i][i1], end="")
            if i1 < len(l[i]) - 1:
                print(", ", end="")
        print("]")

        if i < len(l) - 1:
            print(", ", end="")
    print("]", sep="pqr", end="xyz")

Note: In code, the variable `i` is named as `__list_iterator`

For tuples:

Converts:
    a: tuple[i32, str, f32] = (10, 'lpython', 24.04)
    print(a, sep="pqr", end="xyz")

to:
    print("(", end="")
    print(a[0], sep="", end="")
    print(", ", sep="", end="")
    print("'", a[1], "'", sep="", end="")
    print(", ", sep="", end="")
    print(a[2], sep="", end="")
    print(")", sep="pqr", end="xyz")

It also works the same way for nested lists/tuples using recursion.
*/

class PrintListTupleVisitor
    : public PassUtils::PassVisitor<PrintListTupleVisitor> {
   private:
    std::string rl_path;

   public:
   Vec<ASR::stmt_t*> print_pass_result_tmp;
    PrintListTupleVisitor(Allocator &al, const std::string &rl_path_)
        : PassVisitor(al, nullptr), rl_path(rl_path_) {
        pass_result.reserve(al, 1);
        print_pass_result_tmp.reserve(al, 1);
    }

    void print_list_helper(ASR::expr_t *list_expr, ASR::expr_t *sep_expr,
                ASR::expr_t *end_expr, const Location &loc) {
        ASR::List_t *listC =
                ASR::down_cast<ASR::List_t>(ASRUtils::expr_type(list_expr));
        ASR::ttype_t *int_type = ASRUtils::TYPE(
                ASR::make_Integer_t(al, loc, 4));
        ASR::ttype_t *bool_type = ASRUtils::TYPE(
            ASR::make_Logical_t(al, loc, 4));
        ASR::ttype_t *str_type_len_0 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 0, nullptr));
        ASR::ttype_t *str_type_len_1 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 1, nullptr));
        ASR::ttype_t *str_type_len_2 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 2, nullptr));
        ASR::expr_t *comma_space =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, ", "), str_type_len_2));
        ASR::expr_t *single_quote =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, "'"), str_type_len_1));
        ASR::expr_t *empty_str = ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, ""), str_type_len_0));
        ASR::expr_t *open_bracket =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, "["), str_type_len_1));
        ASR::expr_t *close_bracket =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, "]"), str_type_len_1));

        std::string list_iter_var_name;
        ASR::expr_t *list_iter_var;
        {
            list_iter_var_name =
                current_scope->get_unique_name("__list_iterator");
            list_iter_var = PassUtils::create_auxiliary_variable(loc,
                list_iter_var_name, al, current_scope, int_type);
        }

        ASR::expr_t *list_item = ASRUtils::EXPR(
            ASR::make_ListItem_t(al, loc, list_expr,
                                    list_iter_var, listC->m_type, nullptr));
        ASR::expr_t *list_len = ASRUtils::EXPR(ASR::make_ListLen_t(
            al, loc, list_expr, int_type, nullptr));
        ASR::expr_t *constant_one = ASRUtils::EXPR(
            ASR::make_IntegerConstant_t(al, loc, 1, int_type));
        ASR::expr_t *list_len_minus_one =
            ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                al, loc, list_len, ASR::binopType::Sub,
                constant_one, int_type, nullptr));
        ASR::expr_t *compare_cond =
            ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                al, loc, list_iter_var, ASR::cmpopType::Lt,
                list_len_minus_one, bool_type, nullptr));

        Vec<ASR::expr_t *> v1, v2, v3, v4;
        v1.reserve(al, 1);
        v3.reserve(al, 1);
        v4.reserve(al, 1);
        v1.push_back(al, open_bracket);
        v3.push_back(al, close_bracket);
        v4.push_back(al, comma_space);

        if (ASR::is_a<ASR::Character_t>(*listC->m_type)) {
            v2.reserve(al, 3);
            v2.push_back(al, single_quote);
            v2.push_back(al, list_item);
            v2.push_back(al, single_quote);
        } else {
            v2.reserve(al, 1);
            v2.push_back(al, list_item);
        }

        ASR::stmt_t *print_open_bracket = ASRUtils::STMT(
            ASR::make_Print_t(al, loc, nullptr, v1.p, v1.size(),
                                nullptr, empty_str));
        ASR::stmt_t *print_comma_space = ASRUtils::STMT(
            ASR::make_Print_t(al, loc, nullptr, v4.p, v4.size(),
                                empty_str, empty_str));
        ASR::stmt_t *print_item = ASRUtils::STMT(
            ASR::make_Print_t(al, loc, nullptr, v2.p, v2.size(),
                                empty_str, empty_str));
        ASR::stmt_t *print_close_bracket = ASRUtils::STMT(
            ASR::make_Print_t(al, loc, nullptr, v3.p, v3.size(),
                                sep_expr, end_expr));

        Vec<ASR::stmt_t *> if_body;
        if_body.reserve(al, 1);
        if_body.push_back(al, print_comma_space);

        ASR::stmt_t *if_cond = ASRUtils::STMT(
            ASR::make_If_t(al, loc, compare_cond, if_body.p,
                            if_body.size(), nullptr, 0));

        ASR::do_loop_head_t loop_head;
        Vec<ASR::stmt_t *> loop_body;
        {
            loop_head.loc = loc;
            loop_head.m_v = list_iter_var;
            loop_head.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                al, loc, 0, int_type));
            loop_head.m_end = list_len_minus_one;
            loop_head.m_increment =
                ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                    al, loc, 1, int_type));
            if (ASR::is_a<ASR::List_t>(*listC->m_type)){
                print_list_helper(list_item, nullptr, empty_str, loc);
                loop_body.from_pointer_n_copy(al, print_pass_result_tmp.p, print_pass_result_tmp.size());
                print_pass_result_tmp.n = 0;
            } else if (ASR::is_a<ASR::Tuple_t>(*listC->m_type)) {
                print_tuple_helper(list_item, nullptr, empty_str, loc);
                loop_body.from_pointer_n_copy(al, print_pass_result_tmp.p, print_pass_result_tmp.size());
                print_pass_result_tmp.n = 0;
            } else {
                loop_body.reserve(al, 2);
                loop_body.push_back(al, print_item);
            }
            loop_body.push_back(al, if_cond);
        }

        ASR::stmt_t *loop = ASRUtils::STMT(ASR::make_DoLoop_t(
            al, loc, nullptr, loop_head, loop_body.p, loop_body.size()));

        {
            print_pass_result_tmp.push_back(al, print_open_bracket);
            print_pass_result_tmp.push_back(al, loop);
            print_pass_result_tmp.push_back(al, print_close_bracket);
        }
    }

    void print_tuple_helper(ASR::expr_t *tup_expr, ASR::expr_t *sep_expr,
                ASR::expr_t *end_expr, const Location &loc) {
        ASR::Tuple_t *tup =
                ASR::down_cast<ASR::Tuple_t>(ASRUtils::expr_type(tup_expr));
        ASR::ttype_t *int_type = ASRUtils::TYPE(
                ASR::make_Integer_t(al, loc, 4));
        ASR::ttype_t *str_type_len_0 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 0, nullptr));
        ASR::ttype_t *str_type_len_1 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 1, nullptr));
        ASR::ttype_t *str_type_len_2 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 2, nullptr));
        ASR::expr_t *comma_space =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, ", "), str_type_len_2));
        ASR::expr_t *single_quote =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, "'"), str_type_len_1));
        ASR::expr_t *empty_str = ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, ""), str_type_len_0));
        ASR::expr_t *open_bracket =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, "("), str_type_len_1));
        ASR::expr_t *close_bracket =
            ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, loc, s2c(al, ")"), str_type_len_1));

        std::string tup_iter_var_name;
        ASR::expr_t *tup_iter_var, *tup_item;
        Vec<ASR::expr_t *> v1, v3, v4;
        v1.reserve(al, 1);
        v3.reserve(al, 1);
        v4.reserve(al, 1);
        v1.push_back(al, open_bracket);
        v3.push_back(al, close_bracket);
        v4.push_back(al, comma_space);

        Vec<ASR::stmt_t*> tmp_vec;
        tmp_vec.reserve(al, 3);
        ASR::stmt_t *print_open_bracket = ASRUtils::STMT(
            ASR::make_Print_t(al, loc, nullptr, v1.p, v1.size(),
                                nullptr, empty_str));
        ASR::stmt_t *print_comma_space = ASRUtils::STMT(
            ASR::make_Print_t(al, loc, nullptr, v4.p, v4.size(),
                                empty_str, empty_str));
        ASR::stmt_t *print_close_bracket = ASRUtils::STMT(
            ASR::make_Print_t(al, loc, nullptr, v3.p, v3.size(),
                                sep_expr, end_expr));

        tmp_vec.push_back(al, print_open_bracket);
        for (size_t i=0; i<tup->n_type; i++) {
            tup_iter_var = ASRUtils::EXPR(
                ASR::make_IntegerConstant_t(al, loc, i, int_type));
            tup_item = ASRUtils::EXPR(ASR::make_TupleItem_t(al, loc, tup_expr,
                                    tup_iter_var, tup->m_type[i], nullptr));
            if (ASR::is_a<ASR::List_t>(*tup->m_type[i])) {
                print_pass_result_tmp.n = 0;
                print_list_helper(tup_item, nullptr, empty_str, loc);
                for (size_t j=0; j<print_pass_result_tmp.n; j++) {
                    tmp_vec.push_back(al, print_pass_result_tmp[j]);
                }
                print_pass_result_tmp.n = 0;
            } else if (ASR::is_a<ASR::Tuple_t>(*tup->m_type[i])) {
                print_pass_result_tmp.n = 0;
                print_tuple_helper(tup_item, nullptr, empty_str, loc);
                for (size_t j=0; j<print_pass_result_tmp.n; j++) {
                    tmp_vec.push_back(al, print_pass_result_tmp[j]);
                }
                print_pass_result_tmp.n = 0;
            } else {
                Vec<ASR::expr_t *> v2;
                if (ASR::is_a<ASR::Character_t>(*tup->m_type[i])) {
                    v2.reserve(al, 3);
                    v2.push_back(al, single_quote);
                    v2.push_back(al, tup_item);
                    v2.push_back(al, single_quote);
                } else {
                    v2.reserve(al, 1);
                    v2.push_back(al, tup_item);
                }
                ASR::stmt_t *print_item = ASRUtils::STMT(
                    ASR::make_Print_t(al, loc, nullptr, v2.p, v2.size(),
                                empty_str, empty_str));
                tmp_vec.push_back(al, print_item);
            }
            if (i != tup->n_type - 1) {
                tmp_vec.push_back(al, print_comma_space);
            }
        }
        tmp_vec.push_back(al, print_close_bracket);
        print_pass_result_tmp.from_pointer_n_copy(al, tmp_vec.p, tmp_vec.size());
    }

    void visit_Print(const ASR::Print_t &x) {
        std::vector<ASR::expr_t*> print_tmp;
        ASR::ttype_t *str_type_len_1 = ASRUtils::TYPE(ASR::make_Character_t(
        al, x.base.base.loc, 1, 1, nullptr));
        ASR::expr_t *space = ASRUtils::EXPR(ASR::make_StringConstant_t(
        al, x.base.base.loc, s2c(al, " "), str_type_len_1));
        for (size_t i=0; i<x.n_values; i++) {
            if (ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_values[i])) ||
                ASR::is_a<ASR::Tuple_t>(*ASRUtils::expr_type(x.m_values[i]))) {
                if (!print_tmp.empty()) {
                    Vec<ASR::expr_t*> tmp_vec;
                    ASR::stmt_t *print_stmt;
                    tmp_vec.reserve(al, print_tmp.size());
                    for (auto &e: print_tmp) {
                        tmp_vec.push_back(al, e);
                    }
                    if (x.m_separator) {
                        print_stmt = ASRUtils::STMT(ASR::make_Print_t(al,
                            x.base.base.loc, nullptr, tmp_vec.p, tmp_vec.size(),
                            x.m_separator, x.m_separator));
                    } else {
                        print_stmt = ASRUtils::STMT(ASR::make_Print_t(al,
                            x.base.base.loc, nullptr, tmp_vec.p, tmp_vec.size(),
                            x.m_separator, space));
                    }
                    print_tmp.clear();
                    pass_result.push_back(al, print_stmt);
                }
                if (ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_values[i]))){
                    if (i == x.n_values - 1) {
                        print_list_helper(x.m_values[i], x.m_separator, x.m_end, x.base.base.loc);
                    } else {
                        print_list_helper(x.m_values[i], x.m_separator, (x.m_separator ? x.m_separator : space), x.base.base.loc);
                    }
                } else {
                    if (i == x.n_values - 1) {
                        print_tuple_helper(x.m_values[i], x.m_separator, x.m_end, x.base.base.loc);
                    } else {
                        print_tuple_helper(x.m_values[i], x.m_separator, (x.m_separator ? x.m_separator : space), x.base.base.loc);
                    }
                }
                for (size_t j=0; j<print_pass_result_tmp.n; j++)
                    pass_result.push_back(al, print_pass_result_tmp[j]);
                print_pass_result_tmp.n = 0;
            } else {
                print_tmp.push_back(x.m_values[i]);
            }
        }
        if (!print_tmp.empty()) {
            Vec<ASR::expr_t*> tmp_vec;
            tmp_vec.reserve(al, print_tmp.size());
            for (auto &e: print_tmp) {
                tmp_vec.push_back(al, e);
            }
            ASR::stmt_t *print_stmt = ASRUtils::STMT(
                ASR::make_Print_t(al, x.base.base.loc, nullptr, tmp_vec.p, tmp_vec.size(),
                            x.m_separator, x.m_end));
            print_tmp.clear();
            pass_result.push_back(al, print_stmt);
        }
    }
};

void pass_replace_print_list_tuple(
    Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions &pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    PrintListTupleVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
}

}  // namespace LCompilers
