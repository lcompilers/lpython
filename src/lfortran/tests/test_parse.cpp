#include <tests/doctest.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <string>

#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>
#include <lfortran/bigint.h>

using LFortran::parse;
using LFortran::parse2;
using LFortran::tokens;
using LFortran::AST::ast_t;
using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::BaseWalkVisitor;

using LFortran::BigInt::is_int_ptr;
using LFortran::BigInt::ptr_to_int;
using LFortran::BigInt::int_to_ptr;
using LFortran::BigInt::string_to_largeint;
using LFortran::BigInt::largeint_to_string;
using LFortran::BigInt::MAX_SMALL_INT;
using LFortran::BigInt::MIN_SMALL_INT;

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
    void visit_Name(const Name_t & /* x */) { c_ += 1; }
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
    auto result = parse(al, text)->m_items[0];
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

TEST_CASE("Test lex_int") {
    unsigned char *s;
    uint64_t u;
    LFortran::Str suffix;

    // Test ints
    s = (unsigned char*)"15";
    CHECK(strlen((char*)s) == 2);
    REQUIRE(lex_int(s, s+strlen((char*)s), u, suffix));
    CHECK(u == 15);
    CHECK(suffix.str() == "");

    s = (unsigned char*)"1";
    REQUIRE(lex_int(s, s+strlen((char*)s), u, suffix));
    CHECK(u == 1);
    CHECK(suffix.str() == "");

    s = (unsigned char*)"9223372036854775807"; // 2^63-1
    REQUIRE(lex_int(s, s+strlen((char*)s), u, suffix));
    CHECK(u == 9223372036854775807LL);
    CHECK(suffix.str() == "");

    s = (unsigned char*)"9223372036854775808"; // 2^63
    REQUIRE(lex_int(s, s+strlen((char*)s), u, suffix));
    CHECK(u == 9223372036854775808ULL);
    CHECK(suffix.str() == "");

    s = (unsigned char*)"18446744073709551615"; // 2^64-1
    REQUIRE(lex_int(s, s+strlen((char*)s), u, suffix));
    CHECK(u == 18446744073709551615ULL);
    CHECK(suffix.str() == "");

    s = (unsigned char*)"18446744073709551616"; // 2^64
    REQUIRE(!lex_int(s, s+strlen((char*)s), u, suffix));

    // Suffixes
    s = (unsigned char*)"15_int64";
    REQUIRE(lex_int(s, s+strlen((char*)s), u, suffix));
    CHECK(u == 15);
    CHECK(suffix.str() == "int64");

    s = (unsigned char*)"1234_int64_15_3";
    REQUIRE(lex_int(s, s+strlen((char*)s), u, suffix));
    CHECK(u == 1234);
    CHECK(suffix.str() == "int64_15_3");
}

TEST_CASE("Test Big Int") {
    int64_t i;
    void *p, *p2;

    /* Integer tests */
    i = 0;
    CHECK(!is_int_ptr(i));

    i = 5;
    CHECK(!is_int_ptr(i));

    i = -5;
    CHECK(!is_int_ptr(i));

    // Largest integer that is allowed is 2^62-1
    i = 4611686018427387903LL;
    CHECK(i == MAX_SMALL_INT);
    CHECK(!is_int_ptr(i)); // this is an integer
    i = 4611686018427387904LL;
    CHECK(is_int_ptr(i)); // this is a pointer

    // Smallest integer that is allowed is -2^63
    i = -9223372036854775808ULL;
    CHECK(i == MIN_SMALL_INT);
    CHECK(!is_int_ptr(i)); // this is an integer
    i = -9223372036854775809ULL; // This does not fit into a signed 64bit int
    CHECK(is_int_ptr(i)); // this is a pointer

    /* Pointer tests */
    // Smallest pointer value is 0 (nullptr)
    p = nullptr;
    i = ptr_to_int(p);
    CHECK(is_int_ptr(i));
    p2 = int_to_ptr(i);
    CHECK(p == p2);

    // Second smallest pointer value aligned to 4 is 4
    p = (void*)4;
    i = ptr_to_int(p);
    CHECK(is_int_ptr(i));
    p2 = int_to_ptr(i);
    CHECK(p == p2);

    // Maximum pointer value aligned to 4 is (2^64-1)-3
    p = (void*)18446744073709551612ULL;
    i = ptr_to_int(p);
    CHECK(is_int_ptr(i));
    p2 = int_to_ptr(i);
    CHECK(p == p2);

    /* Big int tests */
    Allocator al(1024);
    LFortran::Str s;
    char *cs;

    s.from_str(al, "123");
    i = string_to_largeint(al, s);
    CHECK(is_int_ptr(i));
    cs = largeint_to_string(i);
    CHECK(std::string(cs) == "123");

    s.from_str(al, "123567890123456789012345678901234567890");
    i = string_to_largeint(al, s);
    CHECK(is_int_ptr(i));
    cs = largeint_to_string(i);
    CHECK(std::string(cs) == "123567890123456789012345678901234567890");
}

TEST_CASE("Test LFortran::Vec") {
    Allocator al(1024);
    LFortran::Vec<int> v;

    v.reserve(al, 2);
    CHECK(v.size() == 0);
    CHECK(v.capacity() == 2);

    v.push_back(al, 1);
    CHECK(v.size() == 1);
    CHECK(v.capacity() == 2);
    CHECK(v.p[0] == 1);
    CHECK(v[0] == 1);

    v.push_back(al, 2);
    CHECK(v.size() == 2);
    CHECK(v.capacity() == 2);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);

    v.push_back(al, 3);
    CHECK(v.size() == 3);
    CHECK(v.capacity() == 4);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v.p[2] == 3);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);

    v.push_back(al, 4);
    CHECK(v.size() == 4);
    CHECK(v.capacity() == 4);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v.p[2] == 3);
    CHECK(v.p[3] == 4);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);
    CHECK(v[3] == 4);

    v.push_back(al, 5);
    CHECK(v.size() == 5);
    CHECK(v.capacity() == 8);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v.p[2] == 3);
    CHECK(v.p[3] == 4);
    CHECK(v.p[4] == 5);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);
    CHECK(v[3] == 4);
    CHECK(v[4] == 5);


    std::vector<int> sv = v.as_vector();
    CHECK(sv.size() == 5);
    CHECK(&(sv[0]) != &(v.p[0]));
    CHECK(sv[0] == 1);
    CHECK(sv[1] == 2);
    CHECK(sv[2] == 3);
    CHECK(sv[3] == 4);
    CHECK(sv[4] == 5);
}

TEST_CASE("LFortran::Vec iterators") {
    Allocator al(1024);
    LFortran::Vec<int> v;

    v.reserve(al, 2);
    v.push_back(al, 1);
    v.push_back(al, 2);
    v.push_back(al, 3);
    v.push_back(al, 4);

    // Check reference (auto)
    int i = 0;
    for (auto &a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check reference (must be const)
    i = 0;
    for (const int &a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check direct type (auto)
    i = 0;
    for (auto a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check direct type (const)
    i = 0;
    for (const int a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check direct type (non const)
    i = 0;
    for (int a : v) {
        i += a;
    }
    CHECK(i == 10);
}

TEST_CASE("Test LFortran::Str") {
    Allocator al(1024);
    LFortran::Str s;
    const char *data = "Some string.";

    s.p = const_cast<char*>(data);
    s.n = 2;
    CHECK(s.size() == 2);
    CHECK(s.p == data);
    CHECK(s.str() == "So");

    std::string scopy = s.str();
    CHECK(s.p != &scopy[0]);
    CHECK(scopy == "So");
    CHECK(scopy[0] == 'S');
    CHECK(scopy[1] == 'o');
    CHECK(scopy[2] == '\x00');

    char *copy = s.c_str(al);
    CHECK(s.p != copy);
    CHECK(copy[0] == 'S');
    CHECK(copy[1] == 'o');
    CHECK(copy[2] == '\x00');
}

TEST_CASE("Test LFortran::Allocator") {
    Allocator al(32);
    // Size is what we asked (32) plus alignment (8) = 40
    CHECK(al.size_total() == 40);

    // Fits in the pre-allocated chunk
    al.alloc(32);
    CHECK(al.size_total() == 40);

    // Chunk doubles
    al.alloc(32);
    CHECK(al.size_total() == 80);

    // Chunk doubles
    al.alloc(90);
    CHECK(al.size_total() == 160);

    // We asked more than can fit in the doubled chunk (2*160),
    // so the chunk will be equal to what we asked (1024) plus alignment (8)
    al.alloc(1024);
    CHECK(al.size_total() == 1032);
}

TEST_CASE("Test LFortran::Allocator 2") {
    Allocator al(32);
    int *p = al.allocate<int>();
    p[0] = 5;

    p = al.allocate<int>(3);
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;

    std::vector<int> *v = al.make_new<std::vector<int>>(5);
    CHECK(v->size() == 5);
    // Must manually call the destructor:
    v->~vector<int>();
}

using tt = yytokentype;

TEST_CASE("Tokenizer") {
    Allocator al(1024);
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
    CHECK(tokens(al, s) == ref);

    s = "2*x**3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_POW,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

    s = "2*x";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2*??";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*@";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*#";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*$";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*^";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*&";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*~";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*`";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*\\";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "2*4294967295"; // 2**32-1, works everywhere
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s, &stypes) == ref);
    CHECK(stypes[0].int_suffix.int_n.n == 2);
    unsigned long nref = 4294967295U;
    CHECK(stypes[2].int_suffix.int_n.n == nref);

    s = "2*18446744073709551616"; // 2**64, too large, will throw an exception
    stypes.clear();
    CHECK(tokens(al, s, &stypes) == ref);
    LFortran::BigInt::BigInt n = stypes[2].int_suffix.int_n;
    CHECK(n.is_large());
    CHECK(n.str() == "18446744073709551616");

    // The tokenizer will only go to the first null character
    s = "2*x\0yyyyy";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);
    s = "2*x yyyyy";
    ref = {
        tt::TK_INTEGER,
        tt::TK_STAR,
        tt::TK_NAME,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

    s = "exit subroutine";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "Exit Subroutine";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "EXIT SUBROUTINE";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "exIT SuBrOuTiNe";
    ref = {
        tt::KW_EXIT,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "exITt SuBrOuTiNe";
    ref = {
        tt::TK_NAME,
        tt::KW_SUBROUTINE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "exitsubroutine";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "x 2";
    ref = {
        tt::TK_NAME,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "x2";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2 x";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2x";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "x_2";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "x_";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "_x";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "not";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = ".not.";
    ref = {
        tt::TK_NOT,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = ".nnot.";
    ref = {
        tt::TK_DEF_OP,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2.nnot.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_DEF_OP,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2 .nnot. 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_DEF_OP,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2.not.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NOT,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2 .not. 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_NOT,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2..nnot..3";
    ref = {
        tt::TK_REAL,
        tt::TK_DEF_OP,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2. .nnot. .3";
    ref = {
        tt::TK_REAL,
        tt::TK_DEF_OP,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2 == 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2==3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2 .eq. 3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2.eq.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_EQ,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2.==.3";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2..eq..3";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2..eq.3.";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2._dp.eq.3._dp";
    ref = {
        tt::TK_REAL,
        tt::TK_EQ,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2. .not. .3";
    ref = {
        tt::TK_REAL,
        tt::TK_NOT,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2..nnot..3";
    ref = {
        tt::TK_REAL,
        tt::TK_DEF_OP,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2..not..3";
    ref = {
        tt::TK_REAL,
        tt::TK_NOT,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2.e.3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_DEF_OP,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "2.e5.3";
    ref = {
        tt::TK_REAL,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "nnot";
    ref = {
        tt::TK_NAME,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "1+1.0+2";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "1+1d0+2";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_INTEGER,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

    s = "3 + .3 + .3e-3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = "3 + 3. + 3.e-3";
    ref = {
        tt::TK_INTEGER,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::TK_PLUS,
        tt::TK_REAL,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

    s = ".true. .and. .false.";
    ref = {
        tt::TK_TRUE,
        tt::TK_AND,
        tt::TK_FALSE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = ".true._lp .and. .false._8";
    ref = {
        tt::TK_TRUE,
        tt::TK_AND,
        tt::TK_FALSE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = ".true._lp .and._lp .false._8";
    ref = {
        tt::TK_TRUE,
        tt::TK_AND,
        tt::TK_NAME,
        tt::TK_FALSE,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

    s = ".and .false.";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = "and. .false.";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

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
    CHECK(tokens(al, s) == ref);

    s = R"(print *, "o,'"k", "s''""''")";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = R"(x ")";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);

    s = R"(x ')";
    CHECK_THROWS_AS(tokens(al, s), LFortran::TokenizerError);


    s = R"(if (x) then
               y = 5
           end if)";
    ref = {
        tt::KW_IF, tt::TK_LPAREN, tt::TK_NAME, tt::TK_RPAREN, tt::KW_THEN, tt::TK_NEWLINE,
        tt::TK_NAME, tt::TK_EQUAL, tt::TK_INTEGER, tt::TK_NEWLINE,
        tt::KW_END_IF,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = R"(if (x) then
               y = 5
           enD  iF)";
    ref = {
        tt::KW_IF, tt::TK_LPAREN, tt::TK_NAME, tt::TK_RPAREN, tt::KW_THEN, tt::TK_NEWLINE,
        tt::TK_NAME, tt::TK_EQUAL, tt::TK_INTEGER, tt::TK_NEWLINE,
        tt::KW_END_IF,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);

    s = R"(if (x) then
               y = 5
           endif)";
    ref = {
        tt::KW_IF, tt::TK_LPAREN, tt::TK_NAME, tt::TK_RPAREN, tt::KW_THEN, tt::TK_NEWLINE,
        tt::TK_NAME, tt::TK_EQUAL, tt::TK_INTEGER, tt::TK_NEWLINE,
        tt::KW_ENDIF,
        tt::END_OF_FILE,
    };
    CHECK(tokens(al, s) == ref);
}

#define cast(type, p) (LFortran::AST::type##_t*) (p)

TEST_CASE("Location") {
    std::string input = R"(subroutine f
    x = y
    x = 213*yz
    end subroutine)";

    Allocator al(1024*1024);
    LFortran::AST::ast_t* result = parse(al, input)->m_items[0];
    CHECK(result->loc.first_line == 1);
    CHECK(result->loc.first_column == 1);
    CHECK(result->loc.last_line == 4);
    CHECK(result->loc.last_column == 19);
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
    result = parse(al, input)->m_items[0];
    CHECK(result->loc.first_line == 1);
    CHECK(result->loc.first_column == 1);
    CHECK(result->loc.last_line == 4);
    CHECK(result->loc.last_column == 17);

    input = R"(program f
    x = y
    x = 213*yz
    end program)";
    result = parse(al, input)->m_items[0];
    CHECK(result->loc.first_line == 1);
    CHECK(result->loc.first_column == 1);
    CHECK(result->loc.last_line == 4);
    CHECK(result->loc.last_column == 16);
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
        std::cerr << format_syntax_error("input", input, e.loc, e.token);
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
        std::cerr << format_syntax_error("input", input, e.loc, e.token);
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
        std::cerr << format_syntax_error("input", input, e.loc, e.token);
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
        std::cerr << format_syntax_error("input", input, e.loc, e.token);
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
        CHECK(e.token == yytokentype::TK_NEWLINE);
        std::cerr << format_syntax_error("input", input, e.loc, e.token);
        CHECK(e.loc.first_line == 1);
        CHECK(e.loc.first_column == 11);
        CHECK(e.loc.last_line == 1);
        CHECK(e.loc.last_column == 11);
    }

    input = "1 + x allocate y";
    try {
        parse(al, input);
        CHECK(false);
    } catch (const LFortran::ParserError &e) {
        CHECK(e.msg() == "syntax error");
        CHECK(e.token == yytokentype::KW_ALLOCATE);
        std::cerr << format_syntax_error("input", input, e.loc, e.token);
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
        std::cerr << format_syntax_error("input", input, e.loc, -1, &e.token);
        CHECK(e.loc.first_line == 1);
        CHECK(e.loc.first_column == 3);
        CHECK(e.loc.last_line == 1);
        CHECK(e.loc.last_column == 3);
    }
    CHECK_THROWS_AS(parse2(al, input), LFortran::TokenizerError);
}
