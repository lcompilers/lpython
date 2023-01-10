#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/print_list.h>

namespace LCompilers {

/*
This ASR pass replaces print list with print every value,
comma_space, brackets and newline. The function
`pass_replace_print_list` transforms the ASR tree in-place.

Converts:

    print(l, sep="pqr", end="xyz") # l is a list

to:

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
*/

class PrintListVisitor
    : public PassUtils::PassVisitor<PrintListVisitor> {
   private:
    std::string rl_path;

   public:
    PrintListVisitor(Allocator &al, const std::string &rl_path_)
        : PassVisitor(al, nullptr), rl_path(rl_path_) {
        pass_result.reserve(al, 1);
    }

    void visit_Print(const ASR::Print_t &x) {
        if (x.n_values == 1 &&
            ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_values[0]))) {
            ASR::List_t *listC =
                ASR::down_cast<ASR::List_t>(ASRUtils::expr_type(x.m_values[0]));

            ASR::ttype_t *int_type = ASRUtils::TYPE(
                ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
            ASR::ttype_t *bool_type = ASRUtils::TYPE(
                ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
            ASR::ttype_t *str_type_len_0 = ASRUtils::TYPE(ASR::make_Character_t(
                al, x.base.base.loc, 1, 0, nullptr, nullptr, 0));
            ASR::ttype_t *str_type_len_1 = ASRUtils::TYPE(ASR::make_Character_t(
                al, x.base.base.loc, 1, 1, nullptr, nullptr, 0));
            ASR::ttype_t *str_type_len_2 = ASRUtils::TYPE(ASR::make_Character_t(
                al, x.base.base.loc, 1, 2, nullptr, nullptr, 0));
            ASR::expr_t *comma_space =
                ASRUtils::EXPR(ASR::make_StringConstant_t(
                    al, x.base.base.loc, s2c(al, ", "), str_type_len_2));
            ASR::expr_t *single_quote =
                ASRUtils::EXPR(ASR::make_StringConstant_t(
                    al, x.base.base.loc, s2c(al, "'"), str_type_len_1));
            ASR::expr_t *empty_str = ASRUtils::EXPR(ASR::make_StringConstant_t(
                al, x.base.base.loc, s2c(al, ""), str_type_len_0));
            ASR::expr_t *open_bracket =
                ASRUtils::EXPR(ASR::make_StringConstant_t(
                    al, x.base.base.loc, s2c(al, "["), str_type_len_1));
            ASR::expr_t *close_bracket =
                ASRUtils::EXPR(ASR::make_StringConstant_t(
                    al, x.base.base.loc, s2c(al, "]"), str_type_len_1));

            std::string list_iter_var_name;
            ASR::symbol_t *list_iter_variable;
            ASR::expr_t *list_iter_var;
            {
                list_iter_var_name =
                    current_scope->get_unique_name("__list_iterator");
                list_iter_variable =
                    ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                        al, x.base.base.loc, current_scope,
                        s2c(al, list_iter_var_name), nullptr, 0, ASR::intentType::Local,
                        nullptr, nullptr, ASR::storage_typeType::Default,
                        int_type, ASR::abiType::Source, ASR::accessType::Public,
                        ASR::presenceType::Required, false));

                current_scope->add_symbol(list_iter_var_name,
                                          list_iter_variable);
                list_iter_var = ASRUtils::EXPR(
                    ASR::make_Var_t(al, x.base.base.loc, list_iter_variable));
            }

            ASR::expr_t *list_item = ASRUtils::EXPR(
                ASR::make_ListItem_t(al, x.base.base.loc, x.m_values[0],
                                     list_iter_var, listC->m_type, nullptr));
            ASR::expr_t *list_len = ASRUtils::EXPR(ASR::make_ListLen_t(
                al, x.base.base.loc, x.m_values[0], int_type, nullptr));
            ASR::expr_t *constant_one = ASRUtils::EXPR(
                ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int_type));
            ASR::expr_t *list_len_minus_one =
                ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                    al, x.base.base.loc, list_len, ASR::binopType::Sub,
                    constant_one, int_type, nullptr));
            ASR::expr_t *compare_cond =
                ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                    al, x.base.base.loc, list_iter_var, ASR::cmpopType::Lt,
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
                ASR::make_Print_t(al, x.base.base.loc, nullptr, v1.p, v1.size(),
                                  nullptr, empty_str));
            ASR::stmt_t *print_comma_space = ASRUtils::STMT(
                ASR::make_Print_t(al, x.base.base.loc, nullptr, v4.p, v4.size(),
                                  empty_str, empty_str));
            ASR::stmt_t *print_item = ASRUtils::STMT(
                ASR::make_Print_t(al, x.base.base.loc, nullptr, v2.p, v2.size(),
                                  empty_str, empty_str));
            ASR::stmt_t *print_close_bracket = ASRUtils::STMT(
                ASR::make_Print_t(al, x.base.base.loc, nullptr, v3.p, v3.size(),
                                  x.m_separator, x.m_end));

            Vec<ASR::stmt_t *> if_body;
            if_body.reserve(al, 1);
            if_body.push_back(al, print_comma_space);

            ASR::stmt_t *if_cond = ASRUtils::STMT(
                ASR::make_If_t(al, x.base.base.loc, compare_cond, if_body.p,
                               if_body.size(), nullptr, 0));

            ASR::do_loop_head_t loop_head;
            Vec<ASR::stmt_t *> loop_body;
            {
                loop_head.loc = x.base.base.loc;
                loop_head.m_v = list_iter_var;
                loop_head.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                    al, x.base.base.loc, 0, int_type));
                loop_head.m_end = list_len_minus_one;
                loop_head.m_increment =
                    ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                        al, x.base.base.loc, 1, int_type));

                if (!ASR::is_a<ASR::List_t>(*listC->m_type)) {
                    loop_body.reserve(al, 2);
                    loop_body.push_back(al, print_item);
                } else {
                    visit_Print(*ASR::down_cast<ASR::Print_t>(print_item));
                    loop_body.from_pointer_n_copy(al, pass_result.p, pass_result.size());
                    pass_result.n = 0;
                }
                loop_body.push_back(al, if_cond);
            }

            ASR::stmt_t *loop = ASRUtils::STMT(ASR::make_DoLoop_t(
                al, x.base.base.loc, loop_head, loop_body.p, loop_body.size()));

            {
                pass_result.push_back(al, print_open_bracket);
                pass_result.push_back(al, loop);
                pass_result.push_back(al, print_close_bracket);
            }
        }
    }
};

void pass_replace_print_list(
    Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions &pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    PrintListVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
}

}  // namespace LCompilers
