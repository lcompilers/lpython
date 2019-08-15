#include <nlohmann/json.hpp>

#include <lfortran/ast_to_json.h>

using json = nlohmann::json;

namespace LFortran {

std::string ast_to_json(LFortran::AST::expr_t *ast) {
    json j = {
        {"pi", 3.141},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {"answer", {
            {"everything", 42}
        }},
        {"list", {1, 0, 2}},
        {"object", {
            {"currency", "USD"},
            {"value", 42.99}
        }},
        {"objects", {
            {
                {"currency", "USD"},
                {"value", 42.99}
            },
            {
                {"currency", "USD"},
                {"value", 42.99}
            },
            {
                {"currency", "USD"},
                {"value", 42.99}
            },
        }}
    };
    return j.dump(4);
}

}
