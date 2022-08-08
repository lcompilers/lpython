#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/print_list_constant.h>


namespace LFortran {

// using ASR::down_cast;
// using ASR::is_a;
using LFortran::ASRUtils::expr_type;

/*
This ASR pass replaces print list constant with print every value, comma, brackets and newline.
The function `pass_replace_print_list_constant` transforms the ASR tree in-place.

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

class PrintListConstantVisitor : public PassUtils::PassVisitor<PrintListConstantVisitor>
{
private:
    std::string rl_path;
public:
    PrintListConstantVisitor(Allocator &al, const std::string &rl_path_) : PassVisitor(al, nullptr),
    rl_path(rl_path_) {
        pass_result.reserve(al, 1);

    }

    void visit_Print(const ASR::Print_t& x) {
        if( x.n_values == 1 && ASR::is_a<ASR::ListConstant_t>(*x.m_values[0])  ) {
            ASR::ListConstant_t* listC = ASR::down_cast<ASR::ListConstant_t>(x.m_values[0]);

            ASR::ttype_t *arg_type;
            ASR::ttype_t* str_type1 = ASRUtils::TYPE(ASR::make_Character_t(al,
                x.base.base.loc, 1, 0, nullptr, nullptr, 0));
            ASR::ttype_t* str_type2 = ASRUtils::TYPE(ASR::make_Character_t(al,
                x.base.base.loc, 1, 1, nullptr, nullptr, 0));
            ASR::ttype_t* str_type3 = ASRUtils::TYPE(ASR::make_Character_t(al,
                x.base.base.loc, 1, 2, nullptr, nullptr, 0));
            ASR::expr_t *comma = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                x.base.base.loc, s2c(al, ", "), str_type3));
            ASR::expr_t *single_quote = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                x.base.base.loc, s2c(al, "'"), str_type2));
            ASR::expr_t *empty_str = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                x.base.base.loc, s2c(al, ""), str_type1));
            ASR::expr_t *open_bracket = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                x.base.base.loc, s2c(al, "["), str_type2));
            ASR::expr_t *close_bracket = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                x.base.base.loc, s2c(al, "]"), str_type2));

            ASR::stmt_t* print_comma = LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, &comma, 1, nullptr, empty_str));
            ASR::stmt_t* print_open_bracket = LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, &open_bracket, 1, nullptr, empty_str));
            ASR::stmt_t* print_close_bracket = LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, &close_bracket, 1, nullptr, empty_str));
            ASR::stmt_t* print_single_quote = LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, &single_quote, 1, nullptr, empty_str));
            ASR::stmt_t* print_empty_endl = LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, nullptr, 0, nullptr, nullptr));

            pass_result.push_back(al, print_open_bracket);
            for(size_t j = 0; j < listC->n_args; ++j) {
                if (j == 0) {
                    arg_type = expr_type(listC->m_args[j]); // current the arg_type in List is same
                }
                else {
                    pass_result.push_back(al, print_comma);
                }
                if (ASRUtils::is_character(*arg_type) ) {
                    pass_result.push_back(al, print_single_quote);
                    pass_result.push_back(al, LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, &listC->m_args[j], 1, nullptr, empty_str)));
                    pass_result.push_back(al, print_single_quote);
                }
                else if ( ASRUtils::is_real(*arg_type) || ASRUtils::is_integer(*arg_type) ){
                    pass_result.push_back(al, LFortran::ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, &listC->m_args[j], 1, nullptr, empty_str)));
                }
                else {
                    exit(1);
                }
            }
            pass_result.push_back(al, print_close_bracket);
            pass_result.push_back(al, print_empty_endl);
        }

    }

};

void pass_replace_print_list_constant(Allocator &al, ASR::TranslationUnit_t &unit,
        const std::string &rl_path) {
    PrintListConstantVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran