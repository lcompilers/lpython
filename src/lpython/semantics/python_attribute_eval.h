#ifndef LPYTHON_ATTRIBUTE_EVAL_H
#define LPYTHON_ATTRIBUTE_EVAL_H


#include <libasr/asr.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>

namespace LFortran {

struct AttributeHandler {

    typedef ASR::asr_t* (*attribute_eval_callback)(ASR::symbol_t*, Allocator &,
                                const Location &, Vec<ASR::expr_t*> &, diag::Diagnostics &);

    std::map<std::string, attribute_eval_callback> attribute_map;

    AttributeHandler() {
        attribute_map = {
            {"list@append", &eval_list_append},
            {"list@remove", &eval_list_remove},
            {"list@insert", &eval_list_insert},
            {"set@add", &eval_set_add},
            {"set@remove", &eval_set_remove},
            {"dict@get", &eval_dict_get}
        };
    }

    std::string get_type_name(ASR::ttype_t *t) {
        if (ASR::is_a<ASR::List_t>(*t)) {
            return "list";
        } else if (ASR::is_a<ASR::Set_t>(*t)) {
            return "set";
        } else if (ASR::is_a<ASR::Dict_t>(*t)) {
            return "dict";
        }
        return "";
    }

    ASR::asr_t* get_attribute(ASR::symbol_t *s, std::string attr_name,
            Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        std::string class_name = get_type_name(type);
        if (class_name == "") {
            throw SemanticError("Type name is not implemented yet.", loc);
        }
        std::string key = get_type_name(type) + "@" + attr_name;
        auto search = attribute_map.find(key);
        if (search != attribute_map.end()) {
            attribute_eval_callback cb = search->second;
            return cb(s, al, loc, args, diag);
        } else {
            throw SemanticError(class_name + "." + attr_name + " is not implemented yet",
                loc);
        }
    }

    static ASR::asr_t* eval_list_append(ASR::symbol_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("append() takes exactly one argument",
                loc);
        }
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
        ASR::ttype_t *ele_type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::check_equal_type(ele_type, list_type)) {
            std::string fnd = ASRUtils::type_to_str_python(ele_type);
            std::string org = ASRUtils::type_to_str_python(list_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in 'append', the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (found: '" + fnd + "', expected: '" + org + "')",
                            {args[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }
        return make_ListAppend_t(al, loc, s, args[0]);
    }

    static ASR::asr_t* eval_list_remove(ASR::symbol_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("remove() takes exactly one argument",
                loc);
        }
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
        ASR::ttype_t *ele_type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::check_equal_type(ele_type, list_type)) {
            std::string fnd = ASRUtils::type_to_str_python(ele_type);
            std::string org = ASRUtils::type_to_str_python(list_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in 'remove', the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (found: '" + fnd + "', expected: '" + org + "')",
                            {args[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }
        return make_ListRemove_t(al, loc, s, args[0]);
    }

    static ASR::asr_t* eval_list_insert(ASR::symbol_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
            if (args.size() != 2) {
                throw SemanticError("insert() takes exactly two arguments",
                        loc);
            }
            ASR::ttype_t *pos_type = ASRUtils::expr_type(args[0]);
            ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                                        4, nullptr, 0));
            if (!ASRUtils::check_equal_type(pos_type, int_type)) {
                throw SemanticError("List index should be of integer type",
                                    args[0]->base.loc);
            }

            ASR::ttype_t *ele_type = ASRUtils::expr_type(args[1]);
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
            ASR::ttype_t *type = v->m_type;
            ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
            if (!ASRUtils::check_equal_type(ele_type, list_type)) {
                std::string fnd = ASRUtils::type_to_str_python(ele_type);
                std::string org = ASRUtils::type_to_str_python(list_type);
                diag.add(diag::Diagnostic(
                    "Type mismatch in 'insert', the types must be compatible",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch (found: '" + fnd + "', expected: '" + org + "')",
                                {args[0]->base.loc})
                    })
                );
                throw SemanticAbort();
            }
            return make_ListInsert_t(al, loc, s, args[0], args[1]);
    }

    static ASR::asr_t* eval_set_add(ASR::symbol_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("add() takes exactly one argument", loc);
        }

        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        ASR::ttype_t *set_type = ASR::down_cast<ASR::Set_t>(type)->m_type;
        ASR::ttype_t *ele_type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::check_equal_type(ele_type, set_type)) {
            std::string fnd = ASRUtils::type_to_str_python(ele_type);
            std::string org = ASRUtils::type_to_str_python(set_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in 'add', the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (found: '" + fnd + "', expected: '" + org + "')",
                            {args[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }

        return make_SetInsert_t(al, loc, s, args[0]);
    }

    static ASR::asr_t* eval_set_remove(ASR::symbol_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("remove() takes exactly one argument", loc);
        }

        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        ASR::ttype_t *set_type = ASR::down_cast<ASR::Set_t>(type)->m_type;
        ASR::ttype_t *ele_type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::check_equal_type(ele_type, set_type)) {
            std::string fnd = ASRUtils::type_to_str_python(ele_type);
            std::string org = ASRUtils::type_to_str_python(set_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in 'remove', the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (found: '" + fnd + "', expected: '" + org + "')",
                            {args[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }

        return make_SetRemove_t(al, loc, s, args[0]);
    }

    static ASR::asr_t* eval_dict_get(ASR::symbol_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        ASR::expr_t *def = nullptr;
        if (args.size() > 2 || args.size() < 1) {
            throw SemanticError("'get' takes atleast 1 and atmost 2 arguments",
                    loc);
        }
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        ASR::ttype_t *key_type = ASR::down_cast<ASR::Dict_t>(type)->m_key_type;
        ASR::ttype_t *value_type = ASR::down_cast<ASR::Dict_t>(type)->m_value_type;
        if (args.size() == 2) {
            def = args[1];
            if (!ASRUtils::check_equal_type(ASRUtils::expr_type(def), value_type)) {
                std::string vtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(def));
                std::string totype = ASRUtils::type_to_str_python(value_type);
                diag.add(diag::Diagnostic(
                    "Type mismatch in get's default value, the types must be compatible",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch (found: '" + vtype + "', expected: '" + totype + "')",
                                {def->base.loc})
                    })
                );
                throw SemanticAbort();
            }
        }
        if (!ASRUtils::check_equal_type(ASRUtils::expr_type(args[0]), key_type)) {
            std::string ktype = ASRUtils::type_to_str_python(ASRUtils::expr_type(args[0]));
            std::string totype = ASRUtils::type_to_str_python(key_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in get's key value, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (found: '" + ktype + "', expected: '" + totype + "')",
                            {args[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }
        return make_DictItem_t(al, loc, s, args[0], def, value_type);
    }


}; // AttributeHandler

} // namespace LFortran

#endif /* LPYTHON_ATTRIBUTE_EVAL_H */
