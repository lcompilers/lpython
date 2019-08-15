#include <tests/doctest.h>
#include <iostream>
#include <nlohmann/json.hpp>

#include <lfortran/ast_to_json.h>
#include <lfortran/parser/parser.h>

using json = nlohmann::json;

TEST_CASE("Check ast_to_json()") {
    std::string text = "(a*z+3+2*x + 3*y - x/(z**2-4) - x**(y**z))";
    Allocator al(4*1024);
    LFortran::AST::expr_t* result = LFortran::parse(al, text);
    std::string s = LFortran::ast_to_json(*result);
    std::cout << s << std::endl;
    auto r = R"(
        {
            "answer": {
                "everything": 42
            },
            "happy": true,
            "list": [
                1,
                0,
                2
            ],
            "name": "Niels",
            "nothing": null,
            "object": {
                "currency": "USD",
                "value": 42.99
            },
            "objects": [
                {
                    "currency": "USD",
                    "value": 42.99
                },
                {
                    "currency": "USD",
                    "value": 42.99
                },
                {
                    "currency": "USD",
                    "value": 42.99
                }
            ],
            "pi": 3.141
        }
    )"_json;

    CHECK(json::parse(s) == r);
}
