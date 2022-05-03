#ifndef LPYTHON_ATTRIBUTE_EVAL_H
#define LPYTHON_ATTRIBUTE_EVAL_H


#include <libasr/asr.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>

namespace LFortran {

struct AttributeHandler {

    typedef ASR::asr_t* (*attribute_eval_callback)(ASR::symbol_t*, Allocator &, const Location &, Vec<ASR::expr_t*> &);

    std::map<std::string, attribute_eval_callback> attribute_map;

    AttributeHandler() {
        attribute_map = {
            {"list@append", &eval_list_append}
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
            Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
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
            return cb(s, al, loc, args);
        } else {
            throw SemanticError(class_name + "." + attr_name + " is not implemented yet",
                loc);
        }
    }

    static ASR::asr_t* eval_list_append(ASR::symbol_t *s, Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args) {
        if (args.size() != 1) {
            throw SemanticError("append() takes exactly one argument",
                loc);
        }
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
        ASR::ttype_t *ele_type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::check_equal_type(ele_type, list_type)) {
            throw SemanticError("Type mismatch while appending to a list, found ('" +
                ASRUtils::type_to_str_python(ele_type) + "' and '" +
                ASRUtils::type_to_str_python(list_type) + "').", loc);
        }
        return make_ListAppend_t(al, loc, s, args[0]);
    }



}; // AttributeHandler

} // namespace LFortran

#endif /* LPYTHON_ATTRIBUTE_EVAL_H */
