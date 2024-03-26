#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_print_arr.h>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*
This ASR pass replaces array slice with do loops and array expression assignments.
The function `pass_replace_print_arr` transforms the ASR tree in-place.

Converts:

    print*, y(1:3)

to:

    do i = 1, 3
        print *, y(i)
    end do


Converts:
    a: not_array
    b: array
    c: not_array
    d: not_array
    print *, a, b(1:10), c, d

to:
    print *, a
    do i = 1, 10
        print *, b(i)
    end do
    print *, c, d
*/

class PrintArrVisitor : public PassUtils::PassVisitor<PrintArrVisitor>
{
private:
    std::string rl_path;
public:
    PrintArrVisitor(Allocator &al, const std::string &rl_path_) : PassVisitor(al, nullptr),
    rl_path(rl_path_) {
        pass_result.reserve(al, 1);

    }

    ASR::stmt_t* print_array_using_doloop(ASR::expr_t *arr_expr, ASR::StringFormat_t* format, const Location &loc) {
        int n_dims = PassUtils::get_rank(arr_expr);
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope);
        ASR::stmt_t* doloop = nullptr;
        ASR::stmt_t* empty_print_endl = ASRUtils::STMT(ASR::make_Print_t(al, loc,
                                            nullptr, 0, nullptr, nullptr));
        ASR::ttype_t *str_type_len_1 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 1, nullptr));
        ASR::ttype_t *str_type_len_2 = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 0, nullptr));
        ASR::expr_t *space = ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, " "), str_type_len_1));
        ASR::expr_t *empty_space = ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, ""), str_type_len_2));
        for( int i = n_dims - 1; i >= 0; i-- ) {
            ASR::do_loop_head_t head;
            head.m_v = idx_vars[i];
            head.m_start = PassUtils::get_bound(arr_expr, i + 1, "lbound", al);
            head.m_end = PassUtils::get_bound(arr_expr, i + 1, "ubound", al);
            head.m_increment = nullptr;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                ASR::expr_t* ref = PassUtils::create_array_ref(arr_expr, idx_vars, al, current_scope);
                Vec<ASR::expr_t*> print_args;
                print_args.reserve(al, 1);
                print_args.push_back(al, ref);
                ASR::stmt_t* print_stmt = nullptr;
                if (format != nullptr) {
                    ASR::expr_t* string_format = ASRUtils::EXPR(ASR::make_StringFormat_t(al, format->base.base.loc,
                    format->m_fmt, print_args.p, print_args.size(), ASR::string_format_kindType::FormatFortran,
                    format->m_type, format->m_value));
                    Vec<ASR::expr_t*> format_args;
                    format_args.reserve(al, 1);
                    format_args.push_back(al, string_format);
                    print_stmt = ASRUtils::STMT(ASR::make_Print_t(al, loc,
                        format_args.p, format_args.size(), nullptr, empty_space));
                } else {
                    print_stmt = ASRUtils::STMT(ASR::make_Print_t(al, loc,
                        print_args.p, print_args.size(), nullptr, space));
                }
                doloop_body.push_back(al, print_stmt);
            } else {
                doloop_body.push_back(al, doloop);
                doloop_body.push_back(al, empty_print_endl);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head, doloop_body.p, doloop_body.size(), nullptr, 0));
        }
        return doloop;
    }

    ASR::stmt_t* create_formatstmt(std::vector<ASR::expr_t*> &print_body, ASR::StringFormat_t* format, const Location &loc, ASR::stmtType _type) {
        Vec<ASR::expr_t*> body;
        body.reserve(al, print_body.size());
        for (size_t j=0; j<print_body.size(); j++) {
            body.push_back(al, print_body[j]);
        }
        ASR::expr_t* string_format = ASRUtils::EXPR(ASR::make_StringFormat_t(al, format->base.base.loc,
        format->m_fmt, body.p, body.size(), ASR::string_format_kindType::FormatFortran,
        format->m_type, nullptr));
        Vec<ASR::expr_t*> print_args;
        print_args.reserve(al, 1);
        print_args.push_back(al, string_format);
        ASR::stmt_t* statement = nullptr;
        if (_type == ASR::stmtType::Print) {
            statement = ASRUtils::STMT(ASR::make_Print_t(al, loc,
            print_args.p, print_args.size(), nullptr, nullptr));
        } else if (_type == ASR::stmtType::FileWrite) {
            statement = ASRUtils::STMT(ASR::make_FileWrite_t(al, loc, 0, nullptr,
                nullptr, nullptr, nullptr, print_args.p, print_args.size(), nullptr, nullptr));
        }
        print_body.clear();
        return statement;
    }

    void visit_Print(const ASR::Print_t& x) {
        std::vector<ASR::expr_t*> print_body;
        ASR::stmt_t* empty_print_endl;
        ASR::stmt_t* print_stmt;
        if (x.n_values > 0 && ASR::is_a<ASR::StringFormat_t>(*x.m_values[0])) {
            empty_print_endl = ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, 0, nullptr, nullptr));
            ASR::StringFormat_t* format = ASR::down_cast<ASR::StringFormat_t>(x.m_values[0]);
            for (size_t i=0; i<format->n_args; i++) {
                if (PassUtils::is_array(format->m_args[i])) {
                    if (print_body.size() > 0) {
                        print_stmt = create_formatstmt(print_body, format, x.base.base.loc, ASR::stmtType::Print);
                        pass_result.push_back(al, print_stmt);
                    }
                    print_stmt = print_array_using_doloop(format->m_args[i],format, x.base.base.loc);
                    pass_result.push_back(al, print_stmt);
                    pass_result.push_back(al, empty_print_endl);
                } else {
                    print_body.push_back(format->m_args[i]);
                }
            }
            if (print_body.size() > 0) {
                print_stmt = create_formatstmt(print_body, format, x.base.base.loc, ASR::stmtType::Print);
                pass_result.push_back(al, print_stmt);
            }
            return;
        }
        ASR::ttype_t *str_type_len_1 = ASRUtils::TYPE(ASR::make_Character_t(
            al, x.base.base.loc, 1, 1, nullptr));
        ASR::expr_t *space = ASRUtils::EXPR(ASR::make_StringConstant_t(
        al, x.base.base.loc, s2c(al, " "), str_type_len_1));
        ASR::expr_t *backspace = ASRUtils::EXPR(ASR::make_StringConstant_t(
        al, x.base.base.loc, s2c(al, "\b"), str_type_len_1));
        ASR::stmt_t* back = ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                            nullptr, 0, nullptr, backspace));
        for (size_t i=0; i<x.n_values; i++) {
            // TODO: This will disallow printing array pointer in Fortran
            // Pointers are treated the same as normal variables in Fortran
            // However, LPython prints the address of pointers when you do
            // print(some_pointer). Same goes for C/C++ (if we add their frontends in future).
            // So we need to figure out a way to de-couple printing support from libasr
            // or add nodes according to the frontends because each frontend will have a different
            // way of handling printing of pointers and non-pointers
            if (!ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_values[i])) &&
                PassUtils::is_array(x.m_values[i])) {
                if (print_body.size() > 0) {
                    Vec<ASR::expr_t*> body;
                    body.reserve(al, print_body.size());
                    for (size_t j=0; j<print_body.size(); j++) {
                        body.push_back(al, print_body[j]);
                    }
                    if (x.m_separator) {
                        print_stmt = ASRUtils::STMT(ASR::make_Print_t(
                            al, x.base.base.loc, body.p, body.size(),
                            x.m_separator, x.m_separator));
                    } else {
                        print_stmt = ASRUtils::STMT(ASR::make_Print_t(
                            al, x.base.base.loc, body.p, body.size(),
                            nullptr, space));
                    }
                    pass_result.push_back(al, print_stmt);
                    print_body.clear();
                }
                print_stmt = print_array_using_doloop(x.m_values[i], nullptr, x.base.base.loc);
                pass_result.push_back(al, print_stmt);
                pass_result.push_back(al, back);
                if (x.m_separator) {
                    if (i == x.n_values - 1) {
                        empty_print_endl = ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, 0, nullptr, x.m_end));
                    } else {
                        empty_print_endl = ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, 0, nullptr, x.m_separator));
                    }
                } else {
                    if (i == x.n_values - 1) {
                        empty_print_endl = ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, 0, nullptr, x.m_end));
                    } else {
                        empty_print_endl = ASRUtils::STMT(ASR::make_Print_t(al, x.base.base.loc,
                                                nullptr, 0, nullptr, nullptr));
                    }
                }
                pass_result.push_back(al, empty_print_endl);
            } else {
                print_body.push_back(x.m_values[i]);
            }
        }
        if (print_body.size() > 0) {
            Vec<ASR::expr_t*> body;
            body.reserve(al, print_body.size());
            for (size_t j=0; j<print_body.size(); j++) {
                body.push_back(al, print_body[j]);
            }
            print_stmt = ASRUtils::STMT(ASR::make_Print_t(
                al, x.base.base.loc, body.p, body.size(),
                x.m_separator, x.m_end));
            pass_result.push_back(al, print_stmt);
            print_body.clear();
        }
    }

    ASR::stmt_t* write_array_using_doloop(ASR::expr_t *arr_expr, ASR::StringFormat_t* format, const Location &loc) {
        int n_dims = PassUtils::get_rank(arr_expr);
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, current_scope);
        ASR::stmt_t* doloop = nullptr;
        ASR::ttype_t *str_type_len = ASRUtils::TYPE(ASR::make_Character_t(
            al, loc, 1, 0, nullptr));
        ASR::expr_t *empty_space = ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, ""), str_type_len));
        ASR::stmt_t* empty_file_write_endl = ASRUtils::STMT(ASR::make_FileWrite_t(al, loc,
                                            0, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr));
        for( int i = n_dims - 1; i >= 0; i-- ) {
            ASR::do_loop_head_t head;
            head.m_v = idx_vars[i];
            head.m_start = PassUtils::get_bound(arr_expr, i + 1, "lbound", al);
            head.m_end = PassUtils::get_bound(arr_expr, i + 1, "ubound", al);
            head.m_increment = nullptr;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                ASR::expr_t* ref = PassUtils::create_array_ref(arr_expr, idx_vars, al, current_scope);
                Vec<ASR::expr_t*> print_args;
                print_args.reserve(al, 1);
                print_args.push_back(al, ref);
                ASR::stmt_t* write_stmt = nullptr;
                if (format != nullptr) {
                    ASR::expr_t* string_format = ASRUtils::EXPR(ASR::make_StringFormat_t(al, format->base.base.loc,
                    format->m_fmt, print_args.p, print_args.size(), ASR::string_format_kindType::FormatFortran,
                    format->m_type, format->m_value));
                    Vec<ASR::expr_t*> format_args;
                    format_args.reserve(al, 1);
                    format_args.push_back(al, string_format);
                    write_stmt = ASRUtils::STMT(ASR::make_FileWrite_t(
                        al, loc, i, nullptr, nullptr, nullptr, nullptr,
                        format_args.p, format_args.size(), nullptr, empty_space));
                } else {
                    write_stmt = ASRUtils::STMT(ASR::make_FileWrite_t(
                        al, loc, i, nullptr, nullptr, nullptr, nullptr,
                        print_args.p, print_args.size(), nullptr, nullptr));
                }
                doloop_body.push_back(al, write_stmt);
            } else {
                doloop_body.push_back(al, doloop);
                doloop_body.push_back(al, empty_file_write_endl);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head, doloop_body.p, doloop_body.size(), nullptr, 0));
        }
        return doloop;
    }

    void print_args_apart_from_arrays(std::vector<ASR::expr_t*> &write_body, const ASR::FileWrite_t& x) {
        Vec<ASR::expr_t*> body;
        body.from_pointer_n_copy(al, write_body.data(), write_body.size());
        ASR::stmt_t* write_stmt = ASRUtils::STMT(ASR::make_FileWrite_t(
            al, x.base.base.loc, x.m_label, x.m_unit, x.m_iomsg, x.m_iostat,
            x.m_id, body.p, body.size(), x.m_separator, x.m_end));
        pass_result.push_back(al, write_stmt);
        write_body.clear();
    }

    void visit_FileWrite(const ASR::FileWrite_t& x) {
        std::vector<ASR::expr_t*> write_body;
        ASR::stmt_t* write_stmt;
        ASR::stmt_t* empty_file_write_endl = ASRUtils::STMT(ASR::make_FileWrite_t(al, x.base.base.loc,
                                            x.m_label, nullptr, nullptr, nullptr, nullptr,
                                            nullptr, 0, nullptr, nullptr));
        if(x.m_values && x.m_values[0] != nullptr && ASR::is_a<ASR::StringFormat_t>(*x.m_values[0])){
            ASR::StringFormat_t* format = ASR::down_cast<ASR::StringFormat_t>(x.m_values[0]);
            for (size_t i=0; i<format->n_args; i++) {
                if (PassUtils::is_array(format->m_args[i])) {
                    if (write_body.size() > 0) {
                        write_stmt = create_formatstmt(write_body, format, x.base.base.loc, ASR::stmtType::FileWrite);
                        pass_result.push_back(al, write_stmt);
                    }
                    write_stmt = write_array_using_doloop(format->m_args[i],format, x.base.base.loc);
                    pass_result.push_back(al, write_stmt);
                    pass_result.push_back(al, empty_file_write_endl);
                } else {
                    write_body.push_back(format->m_args[i]);
                }
            }
            if (write_body.size() > 0) {
                write_stmt = create_formatstmt(write_body, format, x.base.base.loc, ASR::stmtType::FileWrite);
                pass_result.push_back(al, write_stmt);
            }
            return;
        }
        for (size_t i=0; i<x.n_values; i++) {
            // TODO: This will disallow printing array pointer in Fortran
            // Pointers are treated the same as normal variables in Fortran
            // However, LPython prints the address of pointers when you do
            // print(some_pointer). Same goes for C/C++ (if we add their frontends in future).
            // So we need to figure out a way to de-couple printing support from libasr
            // or add nodes according to the frontends because each frontend will have a different
            // way of handling printing of pointers and non-pointers
            if (!ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_values[i])) &&
                PassUtils::is_array(x.m_values[i])) {
                if (write_body.size() > 0) {
                    print_args_apart_from_arrays(write_body, x);
                    pass_result.push_back(al, empty_file_write_endl);
                }
                write_stmt = write_array_using_doloop(x.m_values[i], nullptr, x.base.base.loc);
                pass_result.push_back(al, write_stmt);
                pass_result.push_back(al, empty_file_write_endl);
            } else {
                write_body.push_back(x.m_values[i]);
            }
        }
        if (write_body.size() > 0) {
            print_args_apart_from_arrays(write_body, x);
        }
    }

};

void pass_replace_print_arr(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    PrintArrVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
