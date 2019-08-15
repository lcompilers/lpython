#include <nlohmann/json.hpp>

#include <lfortran/ast_to_json.h>

using json = nlohmann::json;
using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::BaseWalkVisitor;


namespace LFortran {


class JSONVisitor : public BaseWalkVisitor<JSONVisitor>
{
    json j;
public:
    void visit_Name(const Name_t &x) { j["x"] = 1; }
    std::string get_str() {
        return j.dump(4);
    }
};

std::string ast_to_json(LFortran::AST::expr_t &ast) {
    JSONVisitor v;
    v.visit_expr(ast);
    return v.get_str();
}

}
