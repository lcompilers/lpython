#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <lfortran/ast_to_json.h>

using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::operatorType;
using LFortran::AST::BaseWalkVisitor;


namespace LFortran {

namespace {
    std::string op2str(const operatorType type)
    {
        switch (type) {
            case (operatorType::Add) : return "Add";
            case (operatorType::Sub) : return "Sub";
            case (operatorType::Mul) : return "Mul";
            case (operatorType::Div) : return "Div";
            case (operatorType::Pow) : return "Pow";
        }
        throw std::runtime_error("Unknown type");
    }
}


class JSONVisitor : public BaseWalkVisitor<JSONVisitor>
{
    rapidjson::Document d;
    rapidjson::Value j;
    rapidjson::Document::AllocatorType& al;
public:
    JSONVisitor () : al{d.GetAllocator()} {}

    void visit_BinOp(const BinOp_t &x) {
        this->visit_expr(*x.m_left);
        rapidjson::Value left = std::move(j);
        this->visit_expr(*x.m_right);
        rapidjson::Value right = std::move(j);
        j.SetObject();
        j.AddMember("type", "BinOp", al);
        j.AddMember("op", op2str(x.m_op), al);
        j.AddMember("left", left, al);
        j.AddMember("right", right, al);
    }
    void visit_Name(const Name_t &x) {
        j.SetObject();
        j.AddMember("type", "Name", al);
        j.AddMember("id", std::string(x.m_id, 1), al);
    }
    void visit_Num(const Num_t &x) {
        j.SetObject();
        j.AddMember("type", "Num", al);
        j.AddMember("n", x.m_n, al);
    }
    std::string get_str() {
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        j.Accept(writer);
        return strbuf.GetString();
    }
};

std::string ast_to_json(LFortran::AST::ast_t &ast) {
    JSONVisitor v;
    v.visit_ast(ast);
    return v.get_str();
}

}
