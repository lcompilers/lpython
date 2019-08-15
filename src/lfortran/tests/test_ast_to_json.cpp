#include <tests/doctest.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <lfortran/ast_to_json.h>

TEST_CASE("Check ast_to_json()") {
    std::string s = LFortran::ast_to_json(nullptr);
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
