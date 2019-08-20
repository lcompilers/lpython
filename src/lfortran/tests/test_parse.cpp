#include <tests/doctest.h>

#include <iostream>
#include <sstream>
#include <chrono>

#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>

using LFortran::parse;
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

std::vector<int> tokens(const std::string &input,
        std::vector<LFortran::YYSTYPE> *stypes=nullptr)
{
    LFortran::Tokenizer t;
    t.set_string(input);
    std::vector<int> tst;
    int token = yytokentype::END_OF_FILE + 1; // Something different from EOF
    while (token != yytokentype::END_OF_FILE) {
        LFortran::YYSTYPE y;
        token = t.lex(y);
        tst.push_back(token);
        if (stypes) stypes->push_back(y);
    }
    return tst;
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
        '=',
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::TK_NAME,
        '=',
        tt::TK_INTEGER,
        '*',
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x**3";
    ref = {
        tt::TK_INTEGER,
        '*',
        tt::TK_NAME,
        tt::TK_POW,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "(2*x**3)";
    ref = {
        '(',
        tt::TK_INTEGER,
        '*',
        tt::TK_NAME,
        tt::TK_POW,
        tt::TK_INTEGER,
        ')',
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x";
    ref = {
        tt::TK_INTEGER,
        '*',
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
        '*',
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
        '*',
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);
    s = "2*x yyyyy";
    ref = {
        tt::TK_INTEGER,
        '*',
        tt::TK_NAME,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x\n**3";
    ref = {
        tt::TK_INTEGER,
        '*',
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
        '=',
        tt::TK_INTEGER,
        tt::TK_NEWLINE,

        tt::TK_NAME,
        '=',
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::TK_NEWLINE,

        tt::TK_NAME,
        '=',
        tt::TK_INTEGER,
        '*',
        tt::TK_NAME,
        tt::TK_NEWLINE,

        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x = 1; x = y;;x = 2*y";
    ref = {
        tt::TK_NAME,
        '=',
        tt::TK_INTEGER,
        ';',

        tt::TK_NAME,
        '=',
        tt::TK_NAME,
        ';',
        ';',

        tt::TK_NAME,
        '=',
        tt::TK_INTEGER,
        '*',
        tt::TK_NAME,

        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "\n2*x\n\n;;\n**3\n";
    ref = {
        tt::TK_NEWLINE,
        tt::TK_INTEGER,
        '*',
        tt::TK_NAME,
        tt::TK_NEWLINE,
        tt::TK_NEWLINE,
        ';',
        ';',
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

    s = "2.not.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NOT,
        tt::TK_INTEGER,
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
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "1+1d0+2";
    ref = {
        tt::TK_INTEGER,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "1D-5+1.e12+2.E-10+1.E+10+1e10";
    ref = {
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3 + .3 + .3e-3";
    ref = {
        tt::TK_INTEGER,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3 + 3. + 3.e-3";
    ref = {
        tt::TK_INTEGER,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3_i + 3._dp + 3.e-3_dp + 0.3_dp + 1e5_dp";
    ref = {
        tt::TK_INTEGER,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "3_4 + 3._8 + 3.e-3_8 + 0.3_8 + 1e5_8";
    ref = {
        tt::TK_INTEGER,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        '+',
        tt::TK_REAL,
        '+',
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
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o'k", 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o''k", 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o'x'k", 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o,""k", 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o,""k", "s""")";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_STRING,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'ok', 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o"k', 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o""k', 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o"x"k', 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o,''k', 3)";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, 'o,''k', 's''')";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_STRING,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, "o,""k", "s''""''")";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
        tt::TK_STRING,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = R"(print *, somekind_"o,""k", otherKind_"s''""''")";
    ref = {
        tt::KW_PRINT,
        '*',
        ',',
        tt::TK_STRING,
        ',',
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
}
