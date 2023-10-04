#include <cmath>

#include <libasr/asr_utils.h>
#include <libasr/asr.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/semantic_exception.h>

namespace LCompilers {

class SymbolInstantiator : public ASR::BaseExprStmtDuplicator<SymbolInstantiator>
{
public:
    SymbolTable *func_scope;            // the instantiate scope
    SymbolTable *current_scope;         // the new function scope
    SymbolTable *template_scope;        // the template scope, where the environment is
    std::map<std::string, std::string>& context_map;
    std::map<std::string, ASR::ttype_t*> type_subs;
    std::map<std::string, ASR::symbol_t*> symbol_subs;
    std::string new_sym_name;
    SetChar dependencies;

    SymbolInstantiator(Allocator &al, std::map<std::string, std::string>& context_map,
            std::map<std::string, ASR::ttype_t*> type_subs,
            std::map<std::string, ASR::symbol_t*> symbol_subs, SymbolTable *func_scope,
            SymbolTable *template_scope, std::string new_sym_name):
        BaseExprStmtDuplicator(al),
        func_scope{func_scope},
        template_scope{template_scope},
        context_map{context_map},
        type_subs{type_subs},
        symbol_subs{symbol_subs},
        new_sym_name{new_sym_name}
        {}

    ASR::symbol_t* instantiate_symbol(ASR::symbol_t *x) {
        switch (x->type) {
            case (ASR::symbolType::Function) : {
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(x);
                return instantiate_Function(f);
            }
            case (ASR::symbolType::StructType) : {
                ASR::StructType_t *s = ASR::down_cast<ASR::StructType_t>(x);
                return instantiate_StructType(s);
            }
            default : {
                std::string sym_name = ASRUtils::symbol_name(x);
                throw new SemanticError("Instantiation of " + sym_name
                    + " symbol is not supported", x->base.loc);
            };
        }
    }

    ASR::symbol_t* instantiate_body(ASR::Function_t *new_f, ASR::Function_t *f) {
        current_scope = new_f->m_symtab;

        for (auto const &sym_pair: f->m_symtab->get_scope()) {
            if (new_f->m_symtab->resolve_symbol(sym_pair.first) == nullptr) {
                ASR::symbol_t *sym = sym_pair.second;
                if (ASR::is_a<ASR::ExternalSymbol_t>(*sym)) {
                    ASR::ExternalSymbol_t *ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(sym);
                    std::string m_name = ext_sym->m_module_name;
                    if (context_map.find(m_name) != context_map.end()) {
                        std::string new_m_name = context_map[m_name];
                        std::string member_name = ext_sym->m_original_name;
                        std::string new_x_name = "1_" + new_m_name + "_" + member_name;

                        ASR::symbol_t* new_x = current_scope->get_symbol(new_x_name);
                        if (new_x) { return new_x; }

                        ASR::symbol_t* new_sym = current_scope->resolve_symbol(new_m_name);
                        ASR::symbol_t* member_sym = ASRUtils::symbol_symtab(new_sym)->resolve_symbol(member_name);

                        new_x = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                            al, ext_sym->base.base.loc, current_scope, s2c(al, new_x_name), member_sym,
                        s2c(al, new_m_name), nullptr, 0, s2c(al, member_name), ext_sym->m_access));
                        current_scope->add_symbol(new_x_name, new_x);
                        context_map[ext_sym->m_name] = new_x_name;
                    } else {
                        ASRUtils::SymbolDuplicator dupl(al);
                        dupl.duplicate_symbol(sym, current_scope);
                    }
                }
            }
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, f->n_body);
        for (size_t i=0; i<f->n_body; i++) {
            ASR::stmt_t *new_body = this->duplicate_stmt(f->m_body[i]);
            if (new_body != nullptr) {
                body.push_back(al, new_body);
            }
        }

        SetChar deps_vec;
        deps_vec.reserve(al, new_f->n_dependencies + dependencies.size());
        for (size_t i=0; i<new_f->n_dependencies; i++) {
            char* dep = new_f->m_dependencies[i];
            deps_vec.push_back(al, dep);
        }
        for (size_t i=0; i<dependencies.size(); i++) {
            char* dep = dependencies[i];
            deps_vec.push_back(al, dep);
        }

        new_f->m_body = body.p;
        new_f->n_body = body.size();
        new_f->m_dependencies = deps_vec.p;
        new_f->n_dependencies = deps_vec.size();

        ASR::symbol_t *t = current_scope->resolve_symbol(new_sym_name);
        return t;
    }

    ASR::symbol_t* instantiate_Function(ASR::Function_t *x) {
        dependencies.clear(al);
        current_scope = al.make_new<SymbolTable>(func_scope);

        Vec<ASR::expr_t*> args;
        args.reserve(al, x->n_args);
        for (size_t i=0; i<x->n_args; i++) {
            ASR::expr_t *new_arg = duplicate_expr(x->m_args[i]);
            args.push_back(al, new_arg);
        }

        ASR::expr_t *new_return_var_ref = nullptr;
        if (x->m_return_var != nullptr) {
            new_return_var_ref = duplicate_expr(x->m_return_var);
        }

        // Rebuild the symbol table
        for (auto const &sym_pair: x->m_symtab->get_scope()) {
            duplicate_symbol(sym_pair.second);
        }

        ASR::abiType func_abi = ASRUtils::get_FunctionType(x)->m_abi;
        ASR::accessType func_access = x->m_access;
        ASR::deftypeType func_deftype = ASRUtils::get_FunctionType(x)->m_deftype;
        char *bindc_name = ASRUtils::get_FunctionType(x)->m_bindc_name;

        bool func_elemental =  ASRUtils::get_FunctionType(x)->m_elemental;
        bool func_pure = ASRUtils::get_FunctionType(x)->m_pure;
        bool func_module = ASRUtils::get_FunctionType(x)->m_module;

        SetChar deps_vec;
        deps_vec.reserve(al, dependencies.size());
        for( size_t i = 0; i < dependencies.size(); i++ ) {
            char* dep = dependencies[i];
            deps_vec.push_back(al, dep);
        }

        ASR::asr_t *result = ASRUtils::make_Function_t_util(
            al, x->base.base.loc,
            current_scope, s2c(al, new_sym_name),
            deps_vec.p, deps_vec.size(),
            args.p, args.size(),
            nullptr, 0,
            new_return_var_ref,
            func_abi, func_access, func_deftype, bindc_name,
            func_elemental, func_pure, func_module, ASRUtils::get_FunctionType(x)->m_inline,
            ASRUtils::get_FunctionType(x)->m_static, ASRUtils::get_FunctionType(x)->m_restrictions,
            ASRUtils::get_FunctionType(x)->n_restrictions, false, false, false);

        ASR::symbol_t *t = ASR::down_cast<ASR::symbol_t>(result);
        func_scope->add_symbol(new_sym_name, t);
        context_map[x->m_name] = new_sym_name;

        return t;
    }

    ASR::symbol_t* instantiate_StructType(ASR::StructType_t *x) {
        current_scope = al.make_new<SymbolTable>(func_scope);
        for (auto const &sym_pair: x->m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*sym_pair.second)) {
                duplicate_symbol(sym_pair.second);
            }
        }

        Vec<char*> data_member_names;
        data_member_names.reserve(al, x->n_members);
        for (size_t i=0; i<x->n_members; i++) {
            data_member_names.push_back(al, x->m_members[i]);
        }

        ASR::expr_t *m_alignment = duplicate_expr(x->m_alignment);

        ASR::asr_t *result = ASR::make_StructType_t(al, x->base.base.loc,
            current_scope, s2c(al, new_sym_name),
            nullptr, 0,
            data_member_names.p, data_member_names.size(),
            x->m_abi, x->m_access, x->m_is_packed, x->m_is_abstract,
            nullptr, 0, m_alignment, nullptr);

        ASR::symbol_t *t = ASR::down_cast<ASR::symbol_t>(result);
        func_scope->add_symbol(new_sym_name, t);
        context_map[x->m_name] = new_sym_name;

        /*
        for (auto const &sym_pair: x->m_symtab->get_scope()) {
            ASR::symbol_t *sym = sym_pair.second;
            if (ASR::is_a<ASR::ClassProcedure_t>(*sym)) {
                ASR::symbol_t *new_sym = duplicate_ClassProcedure(sym);
                current_scope->add_symbol(ASRUtils::symbol_name(new_sym), new_sym);
            }
        }
        */
        for (auto const &sym_pair: x->m_symtab->get_scope()) {
            if (ASR::is_a<ASR::ClassProcedure_t>(*sym_pair.second)) {
                duplicate_symbol(sym_pair.second);
            }
        }

        return t;
    }

    ASR::symbol_t* duplicate_symbol(ASR::symbol_t* x) {
        std::string sym_name = ASRUtils::symbol_name(x);

        if (symbol_subs.find(sym_name) != symbol_subs.end()) {
            return symbol_subs[sym_name];
        }

        if (current_scope->get_symbol(sym_name) != nullptr) {
            return current_scope->get_symbol(sym_name);
        }

        ASR::symbol_t* new_symbol = nullptr;
        switch (x->type) {
            case ASR::symbolType::Variable: {
                new_symbol = duplicate_Variable(ASR::down_cast<ASR::Variable_t>(x));
                break;
            }
            case ASR::symbolType::ExternalSymbol: {
                new_symbol = duplicate_ExternalSymbol(ASR::down_cast<ASR::ExternalSymbol_t>(x));
                break;
            }
            case ASR::symbolType::ClassProcedure: {
                new_symbol = duplicate_ClassProcedure(ASR::down_cast<ASR::ClassProcedure_t>(x));
                break;
            }
            default: {
                throw LCompilersException("Unsupported symbol for template instantiation");
            }
        }

        return new_symbol;
    }

    ASR::symbol_t* duplicate_Variable(ASR::Variable_t *x) {
        ASR::ttype_t *new_type = substitute_type(x->m_type);

        SetChar variable_dependencies_vec;
        variable_dependencies_vec.reserve(al, 1);
        ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, new_type);

        ASR::symbol_t* s = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(al,
            x->base.base.loc, current_scope, s2c(al, x->m_name), variable_dependencies_vec.p,
            variable_dependencies_vec.size(), x->m_intent, nullptr, nullptr, x->m_storage,
            new_type, nullptr, x->m_abi, x->m_access, x->m_presence, x->m_value_attr));
        current_scope->add_symbol(x->m_name, s);

        return s;
    }

    ASR::symbol_t* duplicate_ExternalSymbol(ASR::ExternalSymbol_t *x) {
        std::string m_name = x->m_module_name;
        if (context_map.find(m_name) != context_map.end()) {
            std::string new_m_name = context_map[m_name];
            std::string member_name = x->m_original_name;
            std::string new_x_name = "1_" + new_m_name + "_" + member_name;

            ASR::symbol_t* new_x = current_scope->get_symbol(new_x_name);
            if (new_x) { return new_x; }

            ASR::symbol_t* new_sym = current_scope->resolve_symbol(new_m_name);
            ASR::symbol_t* member_sym = ASRUtils::symbol_symtab(new_sym)->resolve_symbol(member_name);

            new_x = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                al, x->base.base.loc, current_scope, s2c(al, new_x_name), member_sym,
                s2c(al, new_m_name), nullptr, 0, s2c(al, member_name), x->m_access));
            current_scope->add_symbol(new_x_name, new_x);
            context_map[x->m_name] = new_x_name;

            return new_x;
        }

        return ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
            al, x->base.base.loc, x->m_parent_symtab, x->m_name, x->m_external,
            x->m_module_name, x->m_scope_names, x->n_scope_names, x->m_original_name, x->m_access));
    }

    // ASR::symbol_t* duplicate_ClassProcedure(ASR::symbol_t *s) {
    ASR::symbol_t* duplicate_ClassProcedure(ASR::ClassProcedure_t *x) {
        std::string new_cp_name = func_scope->get_unique_name("__asr_" + new_sym_name + "_" + x->m_name, false);
        ASR::symbol_t *cp_proc = template_scope->get_symbol(x->m_name);
        SymbolInstantiator cp_t(al, context_map, type_subs, symbol_subs,
            func_scope, template_scope, new_cp_name);
        ASR::symbol_t *new_cp_proc = cp_t.instantiate_symbol(cp_proc);

        ASR::symbol_t *new_x = ASR::down_cast<ASR::symbol_t>(ASR::make_ClassProcedure_t(
            al, x->base.base.loc, current_scope, x->m_name, x->m_self_argument,
            s2c(al, new_cp_name), new_cp_proc, x->m_abi, x->m_is_deferred));
        current_scope->add_symbol(x->m_name, new_x);

        return new_x;
    }



    ASR::asr_t* duplicate_Var(ASR::Var_t *x) {
        std::string sym_name = ASRUtils::symbol_name(x->m_v);
        ASR::symbol_t* sym = duplicate_symbol(x->m_v);
        return ASR::make_Var_t(al, x->base.base.loc, sym);
    }

    ASR::asr_t* duplicate_ArrayItem(ASR::ArrayItem_t *x) {
        ASR::expr_t *m_v = duplicate_expr(x->m_v);
        ASR::expr_t *m_value = duplicate_expr(x->m_value);

        Vec<ASR::array_index_t> args;
        args.reserve(al, x->n_args);
        for (size_t i=0; i<x->n_args; i++) {
            args.push_back(al, duplicate_array_index(x->m_args[i]));
        }

        ASR::ttype_t *type = substitute_type(x->m_type);

        return ASRUtils::make_ArrayItem_t_util(al, x->base.base.loc, m_v, args.p, x->n_args,
            ASRUtils::type_get_past_allocatable(type), x->m_storage_format, m_value);
    }

    ASR::asr_t* duplicate_ArrayConstant(ASR::ArrayConstant_t *x) {
        Vec<ASR::expr_t*> m_args;
        m_args.reserve(al, x->n_args);
        for (size_t i = 0; i < x->n_args; i++) {
            m_args.push_back(al, self().duplicate_expr(x->m_args[i]));
        }
        ASR::ttype_t* m_type = substitute_type(x->m_type);
        return make_ArrayConstant_t(al, x->base.base.loc, m_args.p, x->n_args, m_type, x->m_storage_format);
    }

    ASR::asr_t* duplicate_ListItem(ASR::ListItem_t *x) {
        ASR::expr_t *m_a = duplicate_expr(x->m_a);
        ASR::expr_t *m_pos = duplicate_expr(x->m_pos);
        ASR::ttype_t *type = substitute_type(x->m_type);
        ASR::expr_t *m_value = duplicate_expr(x->m_value);

        return ASR::make_ListItem_t(al, x->base.base.loc,
            m_a, m_pos, type, m_value);
    }

    ASR::array_index_t duplicate_array_index(ASR::array_index_t x) {
        ASR::expr_t *left = duplicate_expr(x.m_left);
        ASR::expr_t *right = duplicate_expr(x.m_right);
        ASR::expr_t *step = duplicate_expr(x.m_step);
        ASR::array_index_t result;
        result.m_left = left;
        result.m_right = right;
        result.m_step = step;
        return result;
    }

    ASR::asr_t* duplicate_Assignment(ASR::Assignment_t *x) {
        ASR::expr_t *target = duplicate_expr(x->m_target);
        ASR::expr_t *value = duplicate_expr(x->m_value);
        ASR::stmt_t *overloaded = duplicate_stmt(x->m_overloaded);
        return ASR::make_Assignment_t(al, x->base.base.loc, target, value, overloaded);
    }

    ASR::asr_t* duplicate_DoLoop(ASR::DoLoop_t *x) {
        Vec<ASR::stmt_t*> m_body;
        m_body.reserve(al, x->n_body);
        for (size_t i=0; i<x->n_body; i++) {
            m_body.push_back(al, duplicate_stmt(x->m_body[i]));
        }
        ASR::do_loop_head_t head;
        head.m_v = duplicate_expr(x->m_head.m_v);
        head.m_start = duplicate_expr(x->m_head.m_start);
        head.m_end = duplicate_expr(x->m_head.m_end);
        head.m_increment = duplicate_expr(x->m_head.m_increment);
        head.loc = x->m_head.m_v->base.loc;
        return ASR::make_DoLoop_t(al, x->base.base.loc, x->m_name, head, m_body.p, x->n_body);
    }

    ASR::asr_t* duplicate_Cast(ASR::Cast_t *x) {
        ASR::expr_t *arg = duplicate_expr(x->m_arg);
        ASR::ttype_t *type = substitute_type(ASRUtils::expr_type(x->m_arg));
        if (ASRUtils::is_real(*type)) {
            return (ASR::asr_t*) arg;
        }
        return ASRUtils::make_Cast_t_value(al, x->base.base.loc, arg, ASR::cast_kindType::IntegerToReal, x->m_type);
    }

    ASR::asr_t* duplicate_FunctionCall(ASR::FunctionCall_t *x) {
        Vec<ASR::call_arg_t> args;
        args.reserve(al, x->n_args);
        for (size_t i=0; i<x->n_args; i++) {
            ASR::call_arg_t new_arg;
            new_arg.loc = x->m_args[i].loc;
            new_arg.m_value = duplicate_expr(x->m_args[i].m_value);
            args.push_back(al, new_arg);
        }

        ASR::ttype_t* type = substitute_type(x->m_type);
        ASR::expr_t* value = duplicate_expr(x->m_value);
        ASR::expr_t* dt = duplicate_expr(x->m_dt);

        std::string call_name = ASRUtils::symbol_name(x->m_name);
        ASR::symbol_t *name = template_scope->get_symbol(call_name);

        if (ASRUtils::is_requirement_function(name)) {
            name = symbol_subs[call_name];
        } else if (context_map.find(call_name) != context_map.end()) {
            name = current_scope->resolve_symbol(context_map[call_name]);
        } else if (ASRUtils::is_generic_function(name)) {
            ASR::symbol_t *search_sym = current_scope->resolve_symbol(call_name);
            if (search_sym != nullptr) {
                name = search_sym;
            } else {
                ASR::symbol_t* name2 = ASRUtils::symbol_get_past_external(name);
                std::string nested_func_name = current_scope->get_unique_name("__asr_" + call_name, false);
                SymbolInstantiator nested(al, context_map, type_subs, symbol_subs, func_scope, template_scope, nested_func_name);
                name = nested.instantiate_symbol(name2);
                name = nested.instantiate_body(ASR::down_cast<ASR::Function_t>(name), ASR::down_cast<ASR::Function_t>(name2));
                context_map[call_name] = nested_func_name;
            }
        } else {
            name = current_scope->get_symbol(call_name);
            if (!name) {
                throw LCompilersException("Cannot handle instantiation for the function call " + call_name);
            }
        }
<<<<<<< HEAD
        if (ASRUtils::symbol_parent_symtab(name)->get_counter() != current_scope->get_counter()) {
            dependencies.push_back(al, ASRUtils::symbol_name(name));
        }
=======

        dependencies.push_back(al, ASRUtils::symbol_name(name));

>>>>>>> main
        return ASRUtils::make_FunctionCall_t_util(al, x->base.base.loc, name, x->m_original_name,
            args.p, args.size(), type, value, dt);
    }

    ASR::asr_t* duplicate_SubroutineCall(ASR::SubroutineCall_t *x) {
        Vec<ASR::call_arg_t> args;
        args.reserve(al, x->n_args);
        for (size_t i=0; i<x->n_args; i++) {
            ASR::call_arg_t new_arg;
            new_arg.loc = x->m_args[i].loc;
            new_arg.m_value = duplicate_expr(x->m_args[i].m_value);
            args.push_back(al, new_arg);
        }

        ASR::expr_t* dt = duplicate_expr(x->m_dt);

        std::string call_name = ASRUtils::symbol_name(x->m_name);
        ASR::symbol_t *name = template_scope->get_symbol(call_name);

        if (ASRUtils::is_requirement_function(name)) {
            name = symbol_subs[call_name];
        } else if (context_map.find(call_name) != context_map.end()) {
            name = current_scope->resolve_symbol(context_map[call_name]);
        } else if (ASRUtils::is_generic_function(name)) {
            ASR::symbol_t *search_sym = current_scope->resolve_symbol(call_name);
            if (search_sym != nullptr) {
                name = search_sym;
            } else {
                ASR::symbol_t* name2 = ASRUtils::symbol_get_past_external(name);
                std::string nested_func_name = current_scope->get_unique_name("__asr_" + call_name, false);
                SymbolInstantiator nested(al, context_map, type_subs, symbol_subs, func_scope, template_scope, nested_func_name);
                name = nested.instantiate_symbol(name2);
                name = nested.instantiate_body(ASR::down_cast<ASR::Function_t>(name), ASR::down_cast<ASR::Function_t>(name2));
                context_map[call_name] = nested_func_name;
            }
        } else {
            name = current_scope->get_symbol(call_name);
            if (!name) {
                throw LCompilersException("Cannot handle instantiation for the function call " + call_name);
            }
        }
<<<<<<< HEAD
        if (ASRUtils::symbol_parent_symtab(name)->get_counter() != current_scope->get_counter()) {
            dependencies.push_back(al, ASRUtils::symbol_name(name));
        }
        return ASRUtils::make_SubroutineCall_t_util(al, x->base.base.loc, name /* change this */,
=======

        dependencies.push_back(al, ASRUtils::symbol_name(name));

        return ASRUtils::make_SubroutineCall_t_util(al, x->base.base.loc, name,
>>>>>>> main
            x->m_original_name, args.p, args.size(), dt, nullptr, false);
    }

    ASR::asr_t* duplicate_StructInstanceMember(ASR::StructInstanceMember_t *x) {
        ASR::expr_t *v = duplicate_expr(x->m_v);
        ASR::ttype_t *t = substitute_type(x->m_type);
        ASR::expr_t *value = duplicate_expr(x->m_value);
        ASR::symbol_t *s = duplicate_symbol(x->m_m);
        return ASR::make_StructInstanceMember_t(al, x->base.base.loc,
            v, s, t, value);
    }

    ASR::ttype_t* substitute_type(ASR::ttype_t *ttype) {
        switch (ttype->type) {
            case (ASR::ttypeType::TypeParameter) : {
                ASR::TypeParameter_t *param = ASR::down_cast<ASR::TypeParameter_t>(ttype);
                ASR::ttype_t *t = type_subs[param->m_param];
                switch (t->type) {
                    case ASR::ttypeType::Integer: {
                        ASR::Integer_t* tnew = ASR::down_cast<ASR::Integer_t>(t);
                        t = ASRUtils::TYPE(ASR::make_Integer_t(al, t->base.loc, tnew->m_kind));
                        break;
                    }
                    case ASR::ttypeType::Real: {
                        ASR::Real_t* tnew = ASR::down_cast<ASR::Real_t>(t);
                        t = ASRUtils::TYPE(ASR::make_Real_t(al, t->base.loc, tnew->m_kind));
                        break;
                    }
                    case ASR::ttypeType::Character: {
                        ASR::Character_t* tnew = ASR::down_cast<ASR::Character_t>(t);
                        t = ASRUtils::TYPE(ASR::make_Character_t(al, t->base.loc,
                                    tnew->m_kind, tnew->m_len, tnew->m_len_expr));
                        break;
                    }
                    case ASR::ttypeType::Complex: {
                        ASR::Complex_t* tnew = ASR::down_cast<ASR::Complex_t>(t);
                        t = ASRUtils::TYPE(ASR::make_Complex_t(al, t->base.loc, tnew->m_kind));
                        break;
                    }
                    case ASR::ttypeType::TypeParameter: {
                        ASR::TypeParameter_t* tnew = ASR::down_cast<ASR::TypeParameter_t>(t);
                        t = ASRUtils::TYPE(ASR::make_TypeParameter_t(al, t->base.loc, tnew->m_param));
                        break;
                    }
                    default: {
                        LCOMPILERS_ASSERT(false);
                    }
                }
                return t;
            }
            case (ASR::ttypeType::List) : {
                ASR::List_t *tlist = ASR::down_cast<ASR::List_t>(ttype);
                return ASRUtils::TYPE(ASR::make_List_t(al, ttype->base.loc,
                    substitute_type(tlist->m_type)));
            }
            case (ASR::ttypeType::Struct) : {
                ASR::Struct_t *s = ASR::down_cast<ASR::Struct_t>(ttype);
                std::string struct_name = ASRUtils::symbol_name(s->m_derived_type);
                if (context_map.find(struct_name) != context_map.end()) {
                    std::string new_struct_name = context_map[struct_name];
                    ASR::symbol_t *sym = func_scope->resolve_symbol(new_struct_name);
                    return ASRUtils::TYPE(
                        ASR::make_Struct_t(al, s->base.base.loc, sym));
                } else {
                    return ttype;
                }
            }
            case (ASR::ttypeType::Array) : {
                ASR::Array_t *a = ASR::down_cast<ASR::Array_t>(ttype);
                ASR::ttype_t *t = substitute_type(a->m_type);
                ASR::dimension_t* m_dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(ttype, m_dims);
                Vec<ASR::dimension_t> new_dims;
                new_dims.reserve(al, n_dims);
                for (size_t i = 0; i < n_dims; i++) {
                    ASR::dimension_t old_dim = m_dims[i];
                    ASR::dimension_t new_dim;
                    new_dim.loc = old_dim.loc;
                    new_dim.m_start = duplicate_expr(old_dim.m_start);
                    new_dim.m_length = duplicate_expr(old_dim.m_length);
                    new_dims.push_back(al, new_dim);
                }
                return ASRUtils::make_Array_t_util(al, t->base.loc,
                    t, new_dims.p, new_dims.size());
            }
            case (ASR::ttypeType::Allocatable): {
                ASR::Allocatable_t *a = ASR::down_cast<ASR::Allocatable_t>(ttype);
                return ASRUtils::TYPE(ASR::make_Allocatable_t(al, ttype->base.loc,
                    substitute_type(a->m_type)));
            }
            case (ASR::ttypeType::Class): {
                ASR::Class_t *c = ASR::down_cast<ASR::Class_t>(ttype);
                std::string c_name = ASRUtils::symbol_name(c->m_class_type);
                if (context_map.find(c_name) != context_map.end()) {
                    std::string new_c_name = context_map[c_name];
                    return ASRUtils::TYPE(ASR::make_Class_t(al,
                        ttype->base.loc, func_scope->get_symbol(new_c_name)));
                }
                return ttype;
            }
            default : return ttype;
        }
    }

};

ASR::symbol_t* pass_instantiate_symbol(Allocator &al,
        std::map<std::string, std::string>& context_map,
        std::map<std::string, ASR::ttype_t*> type_subs,
        std::map<std::string, ASR::symbol_t*> symbol_subs,
        SymbolTable *current_scope, SymbolTable* template_scope,
        std::string new_sym_name, ASR::symbol_t *sym) {
    ASR::symbol_t* sym2 = ASRUtils::symbol_get_past_external(sym);
    SymbolInstantiator t(al, context_map, type_subs, symbol_subs,
        current_scope, template_scope, new_sym_name);
    return t.instantiate_symbol(sym2);
}

ASR::symbol_t* pass_instantiate_function_body(Allocator &al,
        std::map<std::string, std::string>& context_map,
        std::map<std::string, ASR::ttype_t*> type_subs,
        std::map<std::string, ASR::symbol_t*> symbol_subs,
        SymbolTable *current_scope, SymbolTable *template_scope,
        ASR::Function_t *new_f, ASR::Function_t *f) {
    SymbolInstantiator t(al, context_map, type_subs, symbol_subs,
        current_scope, template_scope, new_f->m_name);
    return t.instantiate_body(new_f, f);
}

void check_restriction(std::map<std::string, ASR::ttype_t*> type_subs,
        std::map<std::string, ASR::symbol_t*> &symbol_subs,
        ASR::Function_t *f, ASR::symbol_t *sym_arg, const Location& loc,
        diag::Diagnostics &diagnostics) {
    std::string f_name = f->m_name;
    ASR::Function_t *arg = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(sym_arg));
    std::string arg_name = arg->m_name;
    if (f->n_args != arg->n_args) {
        std::string f_narg = std::to_string(f->n_args);
        std::string arg_narg = std::to_string(arg->n_args);
        diagnostics.add(diag::Diagnostic(
            "Number of arguments mismatch, restriction expects a function with " + f_narg
                + " parameters, but a function with " + arg_narg + " parameters is provided",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label(arg_name + " has " + arg_narg + " parameters",
                            {loc, arg->base.base.loc}),
                    diag::Label(f_name + " has " + f_narg + " parameters",
                            {f->base.base.loc})
                }
        ));
        throw SemanticAbort();
    }
    for (size_t i = 0; i < f->n_args; i++) {
        ASR::ttype_t *f_param = ASRUtils::expr_type(f->m_args[i]);
        ASR::ttype_t *arg_param = ASRUtils::expr_type(arg->m_args[i]);
        if (ASR::is_a<ASR::TypeParameter_t>(*f_param)) {
            ASR::TypeParameter_t *f_tp
                = ASR::down_cast<ASR::TypeParameter_t>(f_param);
            if (!ASRUtils::check_equal_type(type_subs[f_tp->m_param],
                                            arg_param)) {
                std::string rtype = ASRUtils::type_to_str(type_subs[f_tp->m_param]);
                std::string rvar = ASRUtils::symbol_name(
                                    ASR::down_cast<ASR::Var_t>(f->m_args[i])->m_v);
                std::string atype = ASRUtils::type_to_str(arg_param);
                std::string avar = ASRUtils::symbol_name(
                                    ASR::down_cast<ASR::Var_t>(arg->m_args[i])->m_v);
                diagnostics.add(diag::Diagnostic(
                    "Restriction type mismatch with provided function argument",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("", {loc}),
                        diag::Label("Restriction's parameter " + rvar + " of type " + rtype,
                                {f->m_args[i]->base.loc}),
                        diag::Label("Function's parameter " + avar + " of type " + atype,
                                {arg->m_args[i]->base.loc})
                    }
                ));
                throw SemanticAbort();
            }
        }
    }
    if (f->m_return_var) {
        if (!arg->m_return_var) {
            std::string msg = "The restriction argument " + arg_name
                + " should have a return value";
            throw SemanticError(msg, loc);
        }
        ASR::ttype_t *f_ret = ASRUtils::expr_type(f->m_return_var);
        ASR::ttype_t *arg_ret = ASRUtils::expr_type(arg->m_return_var);
        if (ASR::is_a<ASR::TypeParameter_t>(*f_ret)) {
            ASR::TypeParameter_t *return_tp
                = ASR::down_cast<ASR::TypeParameter_t>(f_ret);
            if (!ASRUtils::check_equal_type(type_subs[return_tp->m_param], arg_ret)) {
                std::string rtype = ASRUtils::type_to_str(type_subs[return_tp->m_param]);
                std::string atype = ASRUtils::type_to_str(arg_ret);
                diagnostics.add(diag::Diagnostic(
                    "Restriction type mismatch with provided function argument",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("", {loc}),
                        diag::Label("Restriction's return type " + rtype,
                            {f->m_return_var->base.loc}),
                        diag::Label("Function's return type " + atype,
                            {arg->m_return_var->base.loc})
                    }
                ));
                throw SemanticAbort();
            }
        }
    } else {
        if (arg->m_return_var) {
            std::string msg = "The restriction argument " + arg_name
                + " should not have a return value";
            throw SemanticError(msg, loc);
        }
    }
    symbol_subs[f_name] = sym_arg;
}

} // namespace LCompilers
