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
        tt::IDENTIFIER,
        '=',
        tt::IDENTIFIER,
        tt::TK_NEWLINE,
        tt::IDENTIFIER,
        '=',
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::TK_NEWLINE,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x**3";
    ref = {
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::TK_POW,
        tt::NUMERIC,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "(2*x**3)";
    ref = {
        '(',
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::TK_POW,
        tt::NUMERIC,
        ')',
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x";
    ref = {
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
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

    s = "2*18446744073709551615";
    ref = {
        tt::NUMERIC,
        '*',
        tt::NUMERIC,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s, &stypes) == ref);
    CHECK(stypes[0].n == 2);
    unsigned long nref = 18446744073709551615U;
    CHECK(stypes[2].n == nref);

    s = "2*18446744073709551616";
    CHECK_THROWS_AS(tokens(s), LFortran::TokenizerError);

    // The tokenizer will only go to the first null character
    s = "2*x\0yyyyy";
    ref = {
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);
    s = "2*x yyyyy";
    ref = {
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2*x\n**3";
    ref = {
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::TK_NEWLINE,
        tt::TK_POW,
        tt::NUMERIC,
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
        tt::IDENTIFIER,
        '=',
        tt::NUMERIC,
        tt::TK_NEWLINE,

        tt::IDENTIFIER,
        '=',
        tt::IDENTIFIER,
        tt::TK_NEWLINE,
        tt::TK_NEWLINE,

        tt::IDENTIFIER,
        '=',
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::TK_NEWLINE,

        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x = 1; x = y;;x = 2*y";
    ref = {
        tt::IDENTIFIER,
        '=',
        tt::NUMERIC,
        ';',

        tt::IDENTIFIER,
        '=',
        tt::IDENTIFIER,
        ';',
        ';',

        tt::IDENTIFIER,
        '=',
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,

        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "\n2*x\n\n;;\n**3\n";
    ref = {
        tt::TK_NEWLINE,
        tt::NUMERIC,
        '*',
        tt::IDENTIFIER,
        tt::TK_NEWLINE,
        tt::TK_NEWLINE,
        ';',
        ';',
        tt::TK_NEWLINE,
        tt::TK_POW,
        tt::NUMERIC,
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
        tt::IDENTIFIER,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "exitsubroutine";
    ref = {
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x 2";
    ref = {
        tt::IDENTIFIER,
        tt::NUMERIC,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x2";
    ref = {
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2 x";
    ref = {
        tt::NUMERIC,
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "2x";
    ref = {
        tt::NUMERIC,
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x_2";
    ref = {
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "x_";
    ref = {
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);

    s = "_x";
    ref = {
        tt::IDENTIFIER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(s) == ref);
}
