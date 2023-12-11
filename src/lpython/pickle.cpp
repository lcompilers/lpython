#include <string>

#include <lpython/pickle.h>
#include <lpython/bigint.h>
#include <lpython/python_ast.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/location.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>

namespace LCompilers::LPython {

/********************** AST Pickle *******************/
class PickleVisitor : public AST::PickleBaseVisitor<PickleVisitor>
{
public:
    std::string get_str() {
        return s;
    }
};

std::string pickle_python(AST::ast_t &ast, bool colors, bool indent) {
    PickleVisitor v;
    v.use_colors = colors;
    v.indent = indent;
    v.visit_ast(ast);
    return v.get_str();
}

/********************** AST Pickle Tree *******************/
class ASTTreeVisitor : public AST::TreeBaseVisitor<ASTTreeVisitor>
{
public:
    std::string get_str() {
        return s;
    }
};

std::string pickle_tree_python(AST::ast_t &ast, bool colors) {
    ASTTreeVisitor v;
    v.use_colors = colors;
    v.visit_ast(ast);
    return v.get_str();
}

/********************** AST Pickle Json *******************/
class ASTJsonVisitor :
    public LPython::AST::JsonBaseVisitor<ASTJsonVisitor>
{
public:
    using LPython::AST::JsonBaseVisitor<ASTJsonVisitor>::JsonBaseVisitor;

    std::string get_str() {
        return s;
    }
};

std::string pickle_json(LPython::AST::ast_t &ast, LocationManager &lm) {
    ASTJsonVisitor v(lm);
    v.visit_ast(ast);
    return v.get_str();
}

}
