#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/tokenizer.h>
#include <lfortran/parser/parser.tab.hh>

//extern int yydebug;

static inline void print_loc(const LFortran::Location &l)
{
    std::cout << "LOCATION:" << l.first_line << " " << l.first_column << " ";
    std::cout << l.last_line << " " << l.last_column << std::endl;
}


int main()
{
    Allocator al(4*1024);
    LFortran::AST::ast_t* result;
    int token;

    std::string input = R"(subroutine f
    x = y
    x = 213*y
    end subroutine)";

    LFortran::Tokenizer t;
    t.set_string(input);
    LFortran::YYSTYPE y;
    LFortran::Location l;
    while (true) {
        token = t.lex(al, y, l);
        std::cout << token << std::endl;
        if (token == yytokentype::TK_INTEGER) {
            print_loc(l);
        };
        if (token == yytokentype::END_OF_FILE) break;
    }


    //yydebug=1;
    result = LFortran::parse(al, input)->m_items[0];
    std::string p = LFortran::pickle(*result);
    std::cout << p << std::endl;

    return 0;
}
