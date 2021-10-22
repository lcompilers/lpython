#include <tests/doctest.h>
#include <iostream>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>

#include <lfortran/ast_to_json.h>
#include <lfortran/parser/parser.h>

using namespace rapidjson;
using LFortran::TRY;
using LFortran::parse;

TEST_CASE("Check ast_to_json()") {
    Allocator al(4*1024);
    std::string s, r;
    LFortran::AST::ast_t* result;
    rapidjson::Document d1, d2;
    LFortran::diag::Diagnostics diagnostics;

    result = TRY(parse(al, "2*x", diagnostics))->m_items[0];
    s = LFortran::ast_to_json(*result);
    std::cout << s << std::endl;
    r = R"(
        {
            "left": {
                "n": 2,
                "type": "Num"
            },
            "op": "Mul",
            "right": {
                "id": "x",
                "type": "Name"
            },
            "type": "BinOp"
        }
    )";
    d1.Parse(s);
    d2.Parse(r);
    CHECK(d1 == d2);


    result = TRY(parse(al, "(2*x)", diagnostics))->m_items[0];
    s = LFortran::ast_to_json(*result);
    std::cout << s << std::endl;
    d1.Parse(s);
    CHECK(d1 == d2);


    result = TRY(parse(al, "2*x**y", diagnostics))->m_items[0];
    s = LFortran::ast_to_json(*result);
    std::cout << s << std::endl;
    r = R"(
        {
            "left": {
                "n": 2,
                "type": "Num"
            },
            "op": "Mul",
            "right": {
                "left": {
                    "id": "x",
                    "type": "Name"
                },
                "op": "Pow",
                "right": {
                    "id": "y",
                    "type": "Name"
                },
                "type": "BinOp"
            },
            "type": "BinOp"
        }
    )";
    d1.Parse(s);
    d2.Parse(r);
    CHECK(d1 == d2);
}
