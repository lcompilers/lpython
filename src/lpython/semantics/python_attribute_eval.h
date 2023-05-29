#ifndef LPYTHON_ATTRIBUTE_EVAL_H
#define LPYTHON_ATTRIBUTE_EVAL_H


#include <libasr/asr.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>
#include <libasr/pass/intrinsic_function_registry.h>

namespace LCompilers::LPython {

struct AttributeHandler {

    typedef ASR::asr_t* (*attribute_eval_callback)(ASR::expr_t*, Allocator &,
                                const Location &, Vec<ASR::expr_t*> &, diag::Diagnostics &);

    std::map<std::string, attribute_eval_callback> attribute_map;
    std::set<std::string> modify_attr_set;

    AttributeHandler() {
        attribute_map = {
            {"int@bit_length", &eval_int_bit_length},
            {"array@size", &eval_array_size},
            {"list@append", &eval_list_append},
            {"list@remove", &eval_list_remove},
            {"list@count", &eval_list_count},
            {"list@index", &eval_list_index},
            {"list@reverse", &eval_list_reverse},
            {"list@clear", &eval_list_clear},
            {"list@insert", &eval_list_insert},
            {"list@pop", &eval_list_pop},
            {"set@pop", &eval_set_pop},
            {"set@add", &eval_set_add},
            {"set@remove", &eval_set_remove},
            {"dict@get", &eval_dict_get},
            {"dict@pop", &eval_dict_pop}
        };

        modify_attr_set = {"list@append", "list@remove",
            "list@reverse", "list@clear", "list@insert", "list@pop",
            "set@pop", "set@add", "set@remove", "dict@pop"};
    }

    std::string get_type_name(ASR::ttype_t *t) {
        if (ASR::is_a<ASR::List_t>(*t)) {
            return "list";
        } else if (ASR::is_a<ASR::Set_t>(*t)) {
            return "set";
        } else if (ASR::is_a<ASR::Dict_t>(*t)) {
            return "dict";
        } else if (ASRUtils::is_array(t)) {
            return "array";
        } else if (ASR::is_a<ASR::Integer_t>(*t)) {
            return "int";
        }
        return "";
    }

    ASR::asr_t* get_attribute(ASR::expr_t *e, std::string attr_name,
            Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        ASR::ttype_t *type = ASRUtils::expr_type(e);
        std::string class_name = get_type_name(type);
        if (class_name == "") {
            throw SemanticError("Type name is not implemented yet.", loc);
        }
        std::string key = class_name + "@" + attr_name;
        if (modify_attr_set.find(key) != modify_attr_set.end()) {
            ASR::Variable_t* v = ASRUtils::EXPR2VAR(e);
            if (v->m_intent == ASRUtils::intent_in) {
                throw SemanticError("Modifying input function parameter `"
                            + std::string(v->m_name) + "` is not allowed", loc);
            }
        }
        auto search = attribute_map.find(key);
        if (search != attribute_map.end()) {
            attribute_eval_callback cb = search->second;
            return cb(e, al, loc, args, diag);
        } else {
            throw SemanticError(class_name + "." + attr_name + " is not implemented yet",
                loc);
        }
    }

    static ASR::asr_t* eval_int_bit_length(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &/*diag*/) {
        if (args.size() != 0) {
            throw SemanticError("int.bit_length() takes no arguments", loc);
        }
        int int_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(s));
        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                                        int_kind, nullptr, 0));
        return ASR::make_IntegerBitLen_t(al, loc, s, int_type, nullptr);
    }

    static ASR::asr_t* eval_array_size(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &/*diag*/) {
        if (args.size() != 0) {
            throw SemanticError("array.size() takes no arguments", loc);
        }
        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        return ASR::make_ArraySize_t(al, loc, s, nullptr, int_type, nullptr);
    }

    static ASR::asr_t* eval_list_append(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("append() takes exactly one argument",
                loc);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(s);
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

    static ASR::asr_t* eval_list_remove(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("remove() takes exactly one argument",
                loc);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(s);
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

    static ASR::asr_t* eval_list_count(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("count() takes exactly one argument",
                loc);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(s);
        ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
        ASR::ttype_t *ele_type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::check_equal_type(ele_type, list_type)) {
            std::string fnd = ASRUtils::type_to_str_python(ele_type);
            std::string org = ASRUtils::type_to_str_python(list_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in 'count', the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (found: '" + fnd + "', expected: '" + org + "')",
                            {args[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                                4, nullptr, 0));
        return make_ListCount_t(al, loc, s, args[0], to_type, nullptr);
    }

    static ASR::asr_t* eval_list_index(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &/*diag*/) {
        Vec<ASR::expr_t*> args_with_list;
        args_with_list.reserve(al, args.size() + 1);
        args_with_list.push_back(al, s);
        for(size_t i = 0; i < args.size(); i++) {
            args_with_list.push_back(al, args[i]);
        }
        ASRUtils::create_intrinsic_function create_function =
            ASRUtils::IntrinsicFunctionRegistry::get_create_function("list.index");
        return create_function(al, loc, args_with_list, [&](const std::string &msg, const Location &loc)
                                { throw SemanticError(msg, loc); });
    }

    static ASR::asr_t* eval_list_reverse(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &/*diag*/) {
        Vec<ASR::expr_t*> args_with_list;
        args_with_list.reserve(al, args.size() + 1);
        args_with_list.push_back(al, s);
        for(size_t i = 0; i < args.size(); i++) {
            args_with_list.push_back(al, args[i]);
        }
        ASRUtils::create_intrinsic_function create_function =
            ASRUtils::IntrinsicFunctionRegistry::get_create_function("list.reverse");
        return create_function(al, loc, args_with_list, [&](const std::string &msg, const Location &loc)
                                { throw SemanticError(msg, loc); });
    }

    static ASR::asr_t* eval_list_insert(ASR::expr_t *s, Allocator &al, const Location &loc,
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
            ASR::ttype_t *type = ASRUtils::expr_type(s);
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

    static ASR::asr_t* eval_list_pop(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
            if (args.size() > 1) {
                throw SemanticError("pop() takes atmost one argument",
                        loc);
            }
            ASR::expr_t *idx = nullptr;
            ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                                        4, nullptr, 0));
            ASR::ttype_t *type = ASRUtils::expr_type(s);
            ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
            if (args.size() == 1) {
                ASR::ttype_t *pos_type = ASRUtils::expr_type(args[0]);
                if (!ASRUtils::check_equal_type(pos_type, int_type)) {
                    std::string fnd = ASRUtils::type_to_str_python(pos_type);
                    std::string org = ASRUtils::type_to_str_python(int_type);
                    diag.add(diag::Diagnostic(
                        "Type mismatch in 'pop', List index should be of integer type",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("type mismatch (found: '" + fnd + "', expected: '" + org + "')",
                                    {args[0]->base.loc})
                        })
                    );
                    throw SemanticAbort();
                }
                idx = args[0];
            } else {
                // default is last index
                idx = (ASR::expr_t*)ASR::make_IntegerConstant_t(al, loc, -1, int_type);
            }

            return make_ListPop_t(al, loc, s, idx, list_type, nullptr);
    }

    static ASR::asr_t* eval_list_clear(ASR::expr_t *s, Allocator &al,
        const Location &loc, Vec<ASR::expr_t*> &args, diag::Diagnostics & diag) {
            if (args.size() != 0) {
                diag.add(diag::Diagnostic(
                    "Incorrect number of arguments in 'clear', it accepts no argument",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("incorrect number of arguments in clear (found: " +
                                    std::to_string(args.size()) + ", expected: 0)",
                                {loc})
                    })
                );
                throw SemanticAbort();
            }

            return make_ListClear_t(al, loc, s);
    }

    static ASR::asr_t* eval_set_pop(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &/*diag*/) {
        if (args.size() != 0) {
            throw SemanticError("pop() takes no arguments (" + std::to_string(args.size()) + " given)", loc);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(s);
        ASR::ttype_t *set_type = ASR::down_cast<ASR::Set_t>(type)->m_type;

        return make_SetPop_t(al, loc, s, set_type, nullptr);
    }

    static ASR::asr_t* eval_set_add(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("add() takes exactly one argument", loc);
        }

        ASR::ttype_t *type = ASRUtils::expr_type(s);
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

    static ASR::asr_t* eval_set_remove(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("remove() takes exactly one argument", loc);
        }

        ASR::ttype_t *type = ASRUtils::expr_type(s);
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

    static ASR::asr_t* eval_dict_get(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        ASR::expr_t *def = nullptr;
        if (args.size() > 2 || args.size() < 1) {
            throw SemanticError("'get' takes atleast 1 and atmost 2 arguments",
                    loc);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(s);
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
        return make_DictItem_t(al, loc, s, args[0], def, value_type, nullptr);
    }

    static ASR::asr_t* eval_dict_pop(ASR::expr_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
        if (args.size() != 1) {
            throw SemanticError("'pop' takes only one argument for now", loc);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(s);
        ASR::ttype_t *key_type = ASR::down_cast<ASR::Dict_t>(type)->m_key_type;
        ASR::ttype_t *value_type = ASR::down_cast<ASR::Dict_t>(type)->m_value_type;
        if (!ASRUtils::check_equal_type(ASRUtils::expr_type(args[0]), key_type)) {
            std::string ktype = ASRUtils::type_to_str_python(ASRUtils::expr_type(args[0]));
            std::string totype = ASRUtils::type_to_str_python(key_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in pop's key value, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (found: '" + ktype + "', expected: '" + totype + "')",
                            {args[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }
        return make_DictPop_t(al, loc, s, args[0], value_type, nullptr);
    }

}; // AttributeHandler

} // namespace LCompilers::LPython

#endif /* LPYTHON_ATTRIBUTE_EVAL_H */
