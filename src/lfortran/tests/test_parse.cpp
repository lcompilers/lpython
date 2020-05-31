#include <tests/doctest.h>

#include <iostream>
#include <sstream>
#include <chrono>

#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>

using LFortran::parse;
using LFortran::parse2;
using LFortran::tokens;
using LFortran::AST::ast_t;
using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::BaseWalkVisitor;

// Print any vector like iterable to a string
template <class T>
inline std::ostream &print_vec(std::ostream &out, T &d)
{
    out << "[";
    for (auto p = d.begin(); p != d.end(); p++) {
        if (p != d.begin())
            out << ", ";
        out << *p;
    }
    out << "]";
    return out;
}


namespace doctest {
    // Convert std::vector<T> to string for doctest
    template<typename T> struct StringMaker<std::vector<T>> {
        static String convert(const std::vector<T> &value) {
            std::ostringstream oss;
            print_vec(oss, value);
            return oss.str().c_str();
        }
    };
}


class CountVisitor : public BaseWalkVisitor<CountVisitor>
{
    int c_;
public:
    CountVisitor() : c_{0} {}
    void visit_Name(const Name_t &x) { c_ += 1; }
    int get_count() {
        return c_;
    }
};

int count(const ast_t &b) {
    CountVisitor v;
    v.visit_ast(b);
    return v.get_count();
}


TEST_CASE("Test longer parser (N = 500)") {
    int N;
    N = 500;
    std::string text;
    std::string t0 = "(a*z+3+2*x + 3*y - x/(z**2-4) - x**(y**z))";
    text.reserve(22542);
    text = t0;
    std::cout << "Construct" << std::endl;
    for (int i = 0; i < N; i++) {
        text.append(" * " + t0);
    }
    Allocator al(1024*1024);
    std::cout << "Parse" << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto result = parse(al, text);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                     .count()
              << "ms" << std::endl;
    int c = count(*result);
    std::cout << "Count: " << c << std::endl;
    std::cout << "String size (bytes):      " << text.size() << std::endl;
    std::cout << "Allocator usage (bytes): " << al.size_current() << std::endl;
    CHECK(c == 4509);
}

using tt = yytokentype;

TEST_CASE("Tokenizer") {
    std::string s;
    std::vector<int> ref;
    std::vector<LFortran::YYSTYPE> stypes;

    s = R"(subroutine
    x = y
    x = 2*y
    subroutine)";
    ref = {
        tt::KW_SUBROUTINE,
        tt::TK_NEWLINE,
        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x**3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_POW,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "(2*x**3)";
    ref = {
        tt::TK_LPAREN,
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_POW,
        tt::TK_INTEGER,
        tt::TK_RPAREN,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*??";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*@";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*#";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*$";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*^";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*&";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*~";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*`";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*\\";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "2*4294967295"; // 2**32-1, works everywhere
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s, &stypes) == ref);
    CHECK(stypes[0].n == 2);
    unsigned long nref = 4294967295U;
    CHECK(stypes[2].n == nref);

    s = "2*18446744073709551616"; // 2**64, too large, will throw an exception
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    // The tokenizer will only go to the first null character
    s = "2*x\0yyyyy";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);
    s = "2*x yyyyy";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x\n**3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::TK_POW,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(
    x = 1
    x = y

    x = 2*y
    )";
    ref = {
        tt::TK_NEWLINE,
        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_INTEGER,
        tt::TK_NEWLINE,

        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::TK_NEWLINE,

        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_NEWLINE,

        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x = 1; x = y;;x = 2*y";
    ref = {
        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_INTEGER,
        tt::TK_SEMICOLON,

        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_NAME,
        tt::TK_SEMICOLON,
        tt::TK_SEMICOLON,

        tt::TK_NAME,
        tt::TK_EQUAL,
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,

        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "\n2*x\n\n;;\n**3\n";
    ref = {
        tt::TK_NEWLINE,
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::TK_NEWLINE,
        tt::TK_SEMICOLON,
        tt::TK_SEMICOLON,
        tt::TK_NEWLINE,
        tt::TK_POW,
        tt::TK_INTEGER,
        tt::TK_NEWLINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "exit subroutine";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "Exit Subroutine";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "EXIT SUBROUTINE";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "exIT SuBrOuTiNe";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "exITt SuBrOuTiNe";
    ref = {
        tt::TK_NAME,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "exitsubroutine";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x 2";
    ref = {
        tt::TK_NAME,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x2";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2 x";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2x";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x_2";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x_";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "_x";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "not";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = ".not.";
    ref = {
        tt::TK_NOT,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = ".nnot.";
    ref = {
        tt::TK_DEF_OP,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2.nnot.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_DEF_OP,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2 .nnot. 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_DEF_OP,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2.not.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NOT,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2 .not. 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NOT,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2..nnot..3";
    ref = {
        tt::TK_REAL,
        tt::TK_DEF_OP,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2. .nnot. .3";
    ref = {
        tt::TK_REAL,
        tt::TK_DEF_OP,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2 == 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2==3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2 .eq. 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2.eq.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2.==.3";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2..eq..3";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2..eq.3.";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2._dp.eq.3._dp";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2. .not. .3";
    ref = {
        tt::TK_REAL,
        tt::TK_NOT,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2..nnot..3";
    ref = {
        tt::TK_REAL,
        tt::TK_DEF_OP,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2..not..3";
    ref = {
        tt::TK_REAL,
        tt::TK_NOT,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2.e.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_DEF_OP,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2.e5.3";
    ref = {
        tt::TK_REAL,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "nnot";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "1+1.0+2";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "1+1d0+2";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "1D-5+1.e12+2.E-10+1.E+10+1e10";
    ref = {
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3 + .3 + .3e-3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3 + 3. + 3.e-3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3_i + 3._dp + 3.e-3_dp + 0.3_dp + 1e5_dp";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3_4 + 3._8 + 3.e-3_8 + 0.3_8 + 1e5_8";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = ".true. .and. .false.";
    ref = {
        tt::TK_TRUE,
        tt::TK_AND,
        tt::TK_FALSE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = ".true._lp .and. .false._8";
    ref = {
        tt::TK_TRUE,
        tt::TK_AND,
        tt::TK_FALSE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = ".true._lp .and._lp .false._8";
    ref = {
        tt::TK_TRUE,
        tt::TK_AND,
        tt::TK_NAME,
        tt::TK_FALSE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = ".true. _lp .and. _lp .false. _8";
    ref = {
        tt::TK_TRUE,
        tt::TK_NAME,
        tt::TK_AND,
        tt::TK_NAME,
        tt::TK_FALSE,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = ".and .false.";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = "and. .false.";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = R"(print *, "ok", 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o'k", 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o''k", 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o'x'k", 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o,""k", 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o,""k", "s""")";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'ok', 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o"k', 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o""k', 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o"x"k', 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o,''k', 3)";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o,''k', 's''')";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o,""k", "s''""''")";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, somekind_"o,""k", otherKind_"s''""''")";
    ref = {
        tt::KW_PRINT,
        tt::TK_STAR,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::TK_COMMA,
        tt::TK_STRING,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o,'"k", "s''""''")";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = R"(x ")";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    s = R"(x ')";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);


    s = R"(if (x) then
               y = 5
           end if)";
    ref = {
        tt::KW_IF, tt::TK_LPAREN, tt::TK_NAME, tt::TK_RPAREN, tt::KW_THEN, tt::TK_NEWLINE,
        tt::TK_NAME, tt::TK_EQUAL, tt::TK_INTEGER, tt::TK_NEWLINE,
        tt::KW_END_IF,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(if (x) then
               y = 5
           enD  iF)";
    ref = {
        tt::KW_IF, tt::TK_LPAREN, tt::TK_NAME, tt::TK_RPAREN, tt::KW_THEN, tt::TK_NEWLINE,
        tt::TK_NAME, tt::TK_EQUAL, tt::TK_INTEGER, tt::TK_NEWLINE,
        tt::KW_END_IF,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(if (x) then
               y = 5
           endif)";
    ref = {
        tt::KW_IF, tt::TK_LPAREN, tt::TK_NAME, tt::TK_RPAREN, tt::KW_THEN, tt::TK_NEWLINE,
        tt::TK_NAME, tt::TK_EQUAL, tt::TK_INTEGER, tt::TK_NEWLINE,
        tt::KW_ENDIF,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);
}

#define cast(type, p) (LFortran::AST::type##_t*) (p)

TEST_CASE("Location") {
    std::string input = R"(subroutine f
    x = y
    x = 213*yz
    end subroutine)";

    Allocator al(1024*1024);
    LFortran::AST::ast_t* result = parse(al, input);
    CHECK(result->loc.first_line == 1);
    CHECK(result->loc.first_column == 1);
    CHECK(result->loc.last_line == 4);
    CHECK(result->loc.last_column == 18);
    auto sub = cast(Subroutine, result);
    auto stmt = cast(Assignment, sub->m_body[1]);
    CHECK(stmt->base.base.loc.first_line == 3);
    CHECK(stmt->base.base.loc.first_column == 5);
    CHECK(stmt->base.base.loc.last_line == 3);
    CHECK(stmt->base.base.loc.last_column == 14);
    auto m = cast(BinOp, stmt->m_value);
    CHECK(m->base.base.loc.first_line == 3);
    CHECK(m->base.base.loc.first_column == 9);
    CHECK(m->base.base.loc.last_line == 3);
    CHECK(m->base.base.loc.last_column == 14);
    auto i = cast(Num, m->m_left);
    CHECK(i->m_n == 213);
    CHECK(i->base.base.loc.first_line == 3);
    CHECK(i->base.base.loc.first_column == 9);
    CHECK(i->base.base.loc.last_line == 3);
    CHECK(i->base.base.loc.last_column == 11);
    auto sym = cast(Name, m->m_right);
    CHECK(std::string(sym->m_id) == "yz");
    CHECK(sym->base.base.loc.first_line == 3);
    CHECK(sym->base.base.loc.first_column == 13);
    CHECK(sym->base.base.loc.last_line == 3);
    CHECK(sym->base.base.loc.last_column == 14);
    auto sym2 = cast(Name, stmt->m_target);
    CHECK(std::string(sym2->m_id) == "x");
    CHECK(sym2->base.base.loc.first_line == 3);
    CHECK(sym2->base.base.loc.first_column == 5);
    CHECK(sym2->base.base.loc.last_line == 3);
    CHECK(sym2->base.base.loc.last_column == 5);

    input = R"(function f()
    x = y
    x = 213*yz
    end function)";
    result = parse(al, input);
    CHECK(result->loc.first_line == 1);
    CHECK(result->loc.first_column == 1);
    CHECK(result->loc.last_line == 4);
    CHECK(result->loc.last_column == 16);

    input = R"(program f
    x = y
    x = 213*yz
    end program)";
    result = parse(al, input);
    CHECK(result->loc.first_line == 1);
    CHECK(result->loc.first_column == 1);
    CHECK(result->loc.last_line == 4);
    CHECK(result->loc.last_column == 15);
}

TEST_CASE("Errors") {
    Allocator al(1024*1024);
    std::string input;

    input = "(2+3+";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::ParserError &e) {
        CHECK(e.msg() == "syntax error");
        CHECK(e.token == yytokentype::TK_NEWLINE);
        show_syntax_error("input", input, e.loc, e.token);
        CHECK(e.loc.first_line == 1);
        CHECK(e.loc.first_column == 6);
        CHECK(e.loc.last_line == 1);
        CHECK(e.loc.last_column == 6);
    }

    input = R"(function f()
    x = y
    x = 213*yz+*
    end function)";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::ParserError &e) {
        CHECK(e.msg() == "syntax error");
        CHECK(e.token == tt::TK_STAR);
        show_syntax_error("input", input, e.loc, e.token);
        CHECK(e.loc.first_line == 3);
        CHECK(e.loc.first_column == 16);
        CHECK(e.loc.last_line == 3);
        CHECK(e.loc.last_column == 16);
    }

    input = R"(function f()
    x = y
    x = 213-*yz
    end function)";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::ParserError &e) {
        CHECK(e.msg() == "syntax error");
        CHECK(e.token == tt::TK_STAR);
        show_syntax_error("input", input, e.loc, e.token);
        CHECK(e.loc.first_line == 3);
        CHECK(e.loc.first_column == 13);
        CHECK(e.loc.last_line == 3);
        CHECK(e.loc.last_column == 13);
    }

    input = R"(function f()
    x = y xxy xx
    x = 213*yz
    end function)";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::ParserError &e) {
        CHECK(e.msg() == "syntax error");
        CHECK(e.token == yytokentype::TK_NAME);
        show_syntax_error("input", input, e.loc, e.token);
        CHECK(e.loc.first_line == 2);
        CHECK(e.loc.first_column == 11);
        CHECK(e.loc.last_line == 2);
        CHECK(e.loc.last_column == 13);
    }

    input = "1 + .notx.";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::ParserError &e) {
        CHECK(e.msg() == "syntax error");
        CHECK(e.token == yytokentype::TK_DEF_OP);
        show_syntax_error("input", input, e.loc, e.token);
        CHECK(e.loc.first_line == 1);
        CHECK(e.loc.first_column == 5);
        CHECK(e.loc.last_line == 1);
        CHECK(e.loc.last_column == 10);
    }

    input = "1 + x allocate y";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::ParserError &e) {
        CHECK(e.msg() == "syntax error");
        CHECK(e.token == yytokentype::KW_ALLOCATE);
        show_syntax_error("input", input, e.loc, e.token);
        CHECK(e.loc.first_line == 1);
        CHECK(e.loc.first_column == 7);
        CHECK(e.loc.last_line == 1);
        CHECK(e.loc.last_column == 14);
    }
    CHECK_THROWS_AS(parse2(al, input), LFortran::ParserError);

    input = "1 @ x allocate y";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::TokenizerError &e) {
        CHECK(e.token == "@");
        show_syntax_error("input", input, e.loc, -1, &e.token);
        CHECK(e.loc.first_line == 1);
        CHECK(e.loc.first_column == 3);
        CHECK(e.loc.last_line == 1);
        CHECK(e.loc.last_column == 3);
    }
    CHECK_THROWS_AS(parse2(al, input), LFortran::TokenizerError);
}
