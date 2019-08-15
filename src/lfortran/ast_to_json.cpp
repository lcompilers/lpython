#include <nlohmann/json.hpp>

#include <lfortran/ast_to_json.h>

namespace LFortran {

std::string ast_to_json(LFortran::AST::expr_t *ast) {
    return "OK!";
}

}
