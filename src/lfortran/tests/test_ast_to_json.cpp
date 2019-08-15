#include <tests/doctest.h>
#include <iostream>
#include <nlohmann/json.hpp>

#include <lfortran/ast_to_json.h>
#include <lfortran/parser/parser.h>

using json = nlohmann::json;

TEST_CASE("Check ast_to_json()") {
    std::string text = "2*x";
    Allocator al(4*1024);
    LFortran::AST::expr_t* result = LFortran::parse(al, text);
    std::string s = LFortran::ast_to_json(*result);
    std::cout << s << std::endl;
    auto r = R"(
        {
            "left": {
                "n": 50,
                "type": "Num"
            },
            "op": 2,
            "right": {
                "id": "x",
                "type": "Name"
            },
            "type": "BinOp"
        }
    )"_json;

    CHECK(json::parse(s) == r);
}
