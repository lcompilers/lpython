#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/print_list_constant.h>

namespace LFortran {

using LFortran::ASRUtils::expr_type;

/*
This ASR pass replaces print list constant with print every value, comma,
brackets and newline. The function `pass_replace_print_list_constant` transforms
the ASR tree in-place.

Converts:

    print(l) # l is a list

to:

    print("[", end="")
    for i in range(len(l)):
        if i != 0: print(", ", end="")
        if isinstance(l[i], str): print("'", end="") # single quote
        print(l[i], end="")
        if isinstance(l[i], str): print("'", end="") # single quote
    print("]")
*/

class PrintListConstantVisitor
    : public PassUtils::PassVisitor<PrintListConstantVisitor> {
private:
  std::string rl_path;

public:
  PrintListConstantVisitor(Allocator &al, const std::string &rl_path_)
      : PassVisitor(al, nullptr), rl_path(rl_path_) {
    pass_result.reserve(al, 1);
  }

  void visit_Print(const ASR::Print_t &x) {
    if (x.n_values == 1 && ASR::is_a<ASR::ListConstant_t>(*x.m_values[0])) {
      ASR::ListConstant_t *listC =
          ASR::down_cast<ASR::ListConstant_t>(x.m_values[0]);

      Vec<ASR::expr_t *> v1, v2, v3, v4;
      v1.reserve(al, 1);
      v2.reserve(al, 1);
      v3.reserve(al, 1);
      v4.reserve(al, 1);

      ASR::ttype_t *arg_type;
      ASR::ttype_t *str_type1 = ASRUtils::TYPE(ASR::make_Character_t(
          al, x.base.base.loc, 1, 0, nullptr, nullptr, 0));
      ASR::ttype_t *str_type2 = ASRUtils::TYPE(ASR::make_Character_t(
          al, x.base.base.loc, 1, 1, nullptr, nullptr, 0));
      ASR::ttype_t *str_type3 = ASRUtils::TYPE(ASR::make_Character_t(
          al, x.base.base.loc, 1, 2, nullptr, nullptr, 0));
      ASR::expr_t *comma = ASRUtils::EXPR(ASR::make_StringConstant_t(
          al, x.base.base.loc, s2c(al, ", "), str_type3));
      ASR::expr_t *single_quote = ASRUtils::EXPR(ASR::make_StringConstant_t(
          al, x.base.base.loc, s2c(al, "'"), str_type2));
      ASR::expr_t *empty_str = ASRUtils::EXPR(ASR::make_StringConstant_t(
          al, x.base.base.loc, s2c(al, ""), str_type1));
      ASR::expr_t *open_bracket = ASRUtils::EXPR(ASR::make_StringConstant_t(
          al, x.base.base.loc, s2c(al, "["), str_type2));
      ASR::expr_t *close_bracket = ASRUtils::EXPR(ASR::make_StringConstant_t(
          al, x.base.base.loc, s2c(al, "]"), str_type2));
      v1.push_back(al, comma);
      v2.push_back(al, single_quote);
      v3.push_back(al, open_bracket);
      v4.push_back(al, close_bracket);

      ASR::stmt_t *print_comma = LFortran::ASRUtils::STMT(ASR::make_Print_t(
          al, x.base.base.loc, nullptr, v1.p, v1.size(), nullptr, empty_str));
      ASR::stmt_t *print_open_bracket = LFortran::ASRUtils::STMT(
          ASR::make_Print_t(al, x.base.base.loc, nullptr, v3.p, v3.size(),
                            nullptr, empty_str));
      ASR::stmt_t *print_close_bracket = LFortran::ASRUtils::STMT(
          ASR::make_Print_t(al, x.base.base.loc, nullptr, v4.p, v4.size(),
                            nullptr, empty_str));
      ASR::stmt_t *print_single_quote = LFortran::ASRUtils::STMT(
          ASR::make_Print_t(al, x.base.base.loc, nullptr, v2.p, v2.size(),
                            nullptr, empty_str));
      ASR::stmt_t *print_empty_endl =
          LFortran::ASRUtils::STMT(ASR::make_Print_t(
              al, x.base.base.loc, nullptr, nullptr, 0, nullptr, nullptr));

      pass_result.push_back(al, print_open_bracket);
      for (size_t j = 0; j < listC->n_args; ++j) {
        if (j == 0) {
          arg_type = expr_type(
              listC->m_args[j]); // current the arg_type in List is same
        } else {
          pass_result.push_back(al, print_comma);
        }
        if (ASRUtils::is_character(*arg_type)) {
          pass_result.push_back(al, print_single_quote);
          pass_result.push_back(al,
                                LFortran::ASRUtils::STMT(ASR::make_Print_t(
                                    al, x.base.base.loc, nullptr,
                                    &listC->m_args[j], 1, nullptr, empty_str)));
          pass_result.push_back(al, print_single_quote);
        } else if (ASRUtils::is_real(*arg_type) ||
                   ASRUtils::is_integer(*arg_type)) {
          pass_result.push_back(al,
                                LFortran::ASRUtils::STMT(ASR::make_Print_t(
                                    al, x.base.base.loc, nullptr,
                                    &listC->m_args[j], 1, nullptr, empty_str)));
        } else {
          throw LCompilersException("Printing ConstantList is only support for"
                                    "integer, float, char and string now.");
        }
      }
      pass_result.push_back(al, print_close_bracket);
      pass_result.push_back(al, print_empty_endl);
    }
  }
};

void pass_replace_print_list_constant(Allocator &al,
                                      ASR::TranslationUnit_t &unit,
                                      const std::string &rl_path) {
  PrintListConstantVisitor v(al, rl_path);
  v.visit_TranslationUnit(unit);
  LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran