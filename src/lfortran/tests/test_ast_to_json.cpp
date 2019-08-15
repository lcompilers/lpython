#include <tests/doctest.h>
#include <iostream>
#include <nlohmann/json.hpp>

#include <lfortran/ast_to_json.h>
#include <lfortran/parser/parser.h>

using json = nlohmann::json;

TEST_CASE("Check ast_to_json()") {
    Allocator al(4*1024);
    std::string s;
    LFortran::AST::expr_t* result;

    result = LFortran::parse(al, "2*x");
    s = LFortran::ast_to_json(*result);
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


    result = LFortran::parse(al, "(2*x)");
    s = LFortran::ast_to_json(*result);
    std::cout << s << std::endl;
    CHECK(json::parse(s) == r);
}
