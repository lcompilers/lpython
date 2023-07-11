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
    std::map<std::string, std::string> context_map;
    std::map<std::string, ASR::ttype_t*> type_subs;
    std::map<std::string, ASR::symbol_t*> symbol_subs;
    std::string new_sym_name;
    SetChar dependencies;

    SymbolInstantiator(Allocator &al, std::map<std::string, std::string> context_map,
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
            ASR::Variable_t *param_var = ASR::down_cast<ASR::Variable_t>(
                (ASR::down_cast<ASR::Var_t>(x->m_args[i]))->m_v);
            ASR::ttype_t *param_type = ASRUtils::expr_type(x->m_args[i]);
            ASR::ttype_t *arg_type = substitute_type(param_type);

            Location loc = param_var->base.base.loc;
            std::string var_name = param_var->m_name;
            ASR::intentType s_intent = param_var->m_intent;
            ASR::expr_t *init_expr = nullptr;
            ASR::expr_t *value = nullptr;
            ASR::storage_typeType storage_type = param_var->m_storage;
            ASR::abiType abi_type = param_var->m_abi;
            ASR::accessType s_access = param_var->m_access;
            ASR::presenceType s_presence = param_var->m_presence;
            bool value_attr = param_var->m_value_attr;

            // TODO: Copying variable can be abstracted into a function
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, arg_type);
            ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), variable_dependencies_vec.p, variable_dependencies_vec.size(),
                s_intent, init_expr, value, storage_type, arg_type, nullptr,
                abi_type, s_access, s_presence, value_attr);

            current_scope->add_symbol(var_name, ASR::down_cast<ASR::symbol_t>(v));

            ASR::symbol_t *var = current_scope->get_symbol(var_name);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x->base.base.loc, var)));
        }

        ASR::expr_t *new_return_var_ref = nullptr;
        if (x->m_return_var != nullptr) {
            ASR::Variable_t *return_var = ASR::down_cast<ASR::Variable_t>(
                (ASR::down_cast<ASR::Var_t>(x->m_return_var))->m_v);
            std::string return_var_name = return_var->m_name;
            ASR::ttype_t *return_param_type = ASRUtils::expr_type(x->m_return_var);
            ASR::ttype_t *return_type = substitute_type(return_param_type);
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, return_type);
            ASR::asr_t *new_return_var = ASR::make_Variable_t(al, return_var->base.base.loc,
                current_scope, s2c(al, return_var_name),
                variable_dependencies_vec.p,
                variable_dependencies_vec.size(),
                return_var->m_intent, nullptr, nullptr,
                return_var->m_storage, return_type, return_var->m_type_declaration,
                return_var->m_abi, return_var->m_access,
                return_var->m_presence, return_var->m_value_attr);
            current_scope->add_symbol(return_var_name, ASR::down_cast<ASR::symbol_t>(new_return_var));
            new_return_var_ref = ASRUtils::EXPR(ASR::make_Var_t(al, x->base.base.loc,
                current_scope->get_symbol(return_var_name)));
        }

        // Rebuild the symbol table
        for (auto const &sym_pair: x->m_symtab->get_scope()) {
            if (current_scope->resolve_symbol(sym_pair.first) == nullptr) {
                ASR::symbol_t *sym = sym_pair.second;
                if (ASR::is_a<ASR::Variable_t>(*sym)) {
                    ASR::ttype_t *new_sym_type = substitute_type(ASRUtils::symbol_type(sym));
                    ASR::Variable_t *var_sym = ASR::down_cast<ASR::Variable_t>(sym);
                    std::string var_sym_name = var_sym->m_name;
                    SetChar variable_dependencies_vec;
                    variable_dependencies_vec.reserve(al, 1);
                    ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, new_sym_type);
                    ASR::asr_t *new_var = ASR::make_Variable_t(al, var_sym->base.base.loc,
                        current_scope, s2c(al, var_sym_name), variable_dependencies_vec.p,
                        variable_dependencies_vec.size(), var_sym->m_intent, nullptr, nullptr,
                        var_sym->m_storage, new_sym_type, var_sym->m_type_declaration, var_sym->m_abi, var_sym->m_access,
                        var_sym->m_presence, var_sym->m_value_attr);
                    current_scope->add_symbol(var_sym_name, ASR::down_cast<ASR::symbol_t>(new_var));
                }
            }
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
            ASRUtils::get_FunctionType(x)->m_static, ASRUtils::get_FunctionType(x)->m_type_params,
            ASRUtils::get_FunctionType(x)->n_type_params, ASRUtils::get_FunctionType(x)->m_restrictions,
            ASRUtils::get_FunctionType(x)->n_restrictions, false, false, false);

        ASR::symbol_t *t = ASR::down_cast<ASR::symbol_t>(result);
        func_scope->add_symbol(new_sym_name, t);

        return t;
    }

    ASR::symbol_t* instantiate_StructType(ASR::StructType_t *x) {
        current_scope = al.make_new<SymbolTable>(func_scope);
        for (auto const &sym_pair: x->m_symtab->get_scope()) {
            ASR::symbol_t *sym = sym_pair.second;
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::ttype_t *new_sym_type = substitute_type(ASRUtils::symbol_type(sym));
                ASR::Variable_t *var_sym = ASR::down_cast<ASR::Variable_t>(sym);
                std::string var_sym_name = var_sym->m_name;
                SetChar variable_dependencies_vec;
                variable_dependencies_vec.reserve(al, 1);
                ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, new_sym_type);
                ASR::asr_t *new_var = ASR::make_Variable_t(al, var_sym->base.base.loc,
                    current_scope, s2c(al, var_sym_name), variable_dependencies_vec.p,
                    variable_dependencies_vec.size(), var_sym->m_intent, nullptr, nullptr,
                    var_sym->m_storage, new_sym_type, var_sym->m_type_declaration, var_sym->m_abi, var_sym->m_access,
                    var_sym->m_presence, var_sym->m_value_attr);
                current_scope->add_symbol(var_sym_name, ASR::down_cast<ASR::symbol_t>(new_var));
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

        return t;
    }



    ASR::asr_t* duplicate_Var(ASR::Var_t *x) {
        std::string sym_name = ASRUtils::symbol_name(x->m_v);
        ASR::symbol_t *sym;
        if (symbol_subs.find(sym_name) != symbol_subs.end()) {
            sym = symbol_subs[sym_name];
        } else {
            sym = current_scope->get_symbol(sym_name);
        }
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
        std::string call_name = ASRUtils::symbol_name(x->m_name);
        ASR::symbol_t *name = template_scope->get_symbol(call_name);
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
        if (ASRUtils::is_requirement_function(name)) {
            name = symbol_subs[call_name];
        } else if (ASRUtils::is_template_function(name)) {
            std::string nested_func_name = current_scope->get_unique_name("__asr_generic_" + call_name, false);
            ASR::symbol_t* name2 = ASRUtils::symbol_get_past_external(name);
            SymbolInstantiator nested_t(al, context_map, type_subs, symbol_subs, func_scope, template_scope, nested_func_name);
            name = nested_t.instantiate_symbol(name2);
            name = nested_t.instantiate_body(ASR::down_cast<ASR::Function_t>(name),
                                             ASR::down_cast<ASR::Function_t>(name2));
            context_map[ASRUtils::symbol_name(name2)] = ASRUtils::symbol_name(name);
        }
        dependencies.push_back(al, ASRUtils::symbol_name(name));
        return ASRUtils::make_FunctionCall_t_util(al, x->base.base.loc, name, x->m_original_name,
            args.p, args.size(), type, value, dt);
    }

    ASR::asr_t* duplicate_SubroutineCall(ASR::SubroutineCall_t *x) {
        std::string call_name = ASRUtils::symbol_name(x->m_name);
        ASR::symbol_t *name = template_scope->get_symbol(call_name);
        Vec<ASR::call_arg_t> args;
        args.reserve(al, x->n_args);
        for (size_t i=0; i<x->n_args; i++) {
            ASR::call_arg_t new_arg;
            new_arg.loc = x->m_args[i].loc;
            new_arg.m_value = duplicate_expr(x->m_args[i].m_value);
            args.push_back(al, new_arg);
        }
        ASR::expr_t* dt = duplicate_expr(x->m_dt);
        if (ASRUtils::is_requirement_function(name)) {
            name = symbol_subs[call_name];
        } else {
            std::string nested_func_name = current_scope->get_unique_name("__asr_generic_" + call_name, false);
            ASR::symbol_t* name2 = ASRUtils::symbol_get_past_external(name);
            SymbolInstantiator nested_t(al, context_map, type_subs, symbol_subs, func_scope, template_scope, nested_func_name);
            name = nested_t.instantiate_symbol(name2);
            context_map[ASRUtils::symbol_name(name2)] = ASRUtils::symbol_name(name);
        }
        dependencies.push_back(al, ASRUtils::symbol_name(name));
        return ASRUtils::make_SubroutineCall_t_util(al, x->base.base.loc, name /* change this */,
            x->m_original_name, args.p, args.size(), dt, nullptr, false);
    }

    ASR::asr_t* duplicate_StructInstanceMember(ASR::StructInstanceMember_t *x) {
        ASR::expr_t *v = duplicate_expr(x->m_v);
        ASR::ttype_t *t = substitute_type(x->m_type);
        ASR::expr_t *value = duplicate_expr(x->m_value);

        ASR::symbol_t *s = x->m_m;
        if (ASR::is_a<ASR::ExternalSymbol_t>(*s)) {
            s = duplicate_ExternalSymbol(s);
        }

        return ASR::make_StructInstanceMember_t(al, x->base.base.loc,
            v, s, t, value);
    }

    ASR::symbol_t* duplicate_ExternalSymbol(ASR::symbol_t *s) {
        ASR::ExternalSymbol_t* x = ASR::down_cast<ASR::ExternalSymbol_t>(s);
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
            return new_x;
        }
        return s;
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
            default : return ttype;
        }
    }

    ASR::asr_t* make_BinOp_helper(ASR::expr_t *left, ASR::expr_t *right,
            ASR::binopType op, const Location &loc) {
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;

        if (op == ASR::binopType::Div) {
            dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
            if (ASRUtils::is_integer(*left_type)) {
                left = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type));
            }
            if (ASRUtils::is_integer(*right_type)) {
                if (ASRUtils::expr_value(right) != nullptr) {
                    int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(right))->m_n;
                    if (val == 0) {
                        throw SemanticError("division by zero is not allowed", right->base.loc);
                    }
                }
                right = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type));
            } else if (ASRUtils::is_real(*right_type)) {
                if (ASRUtils::expr_value(right) != nullptr) {
                    double val = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(right))->m_r;
                    if (val == 0.0) {
                        throw SemanticError("float division by zero is not allowed", right->base.loc);
                    }
                }
            }
        }

        if ((ASRUtils::is_integer(*left_type) || ASRUtils::is_real(*left_type)) &&
                (ASRUtils::is_integer(*right_type) || ASRUtils::is_real(*right_type))) {
            left = cast_helper(ASRUtils::expr_type(right), left);
            right = cast_helper(ASRUtils::expr_type(left), right);
            dest_type = substitute_type(ASRUtils::expr_type(left));
        }

        if (ASRUtils::is_integer(*dest_type)) {
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(left))->m_n;
                int64_t right_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(right))->m_n;
                int64_t result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    default: { LCOMPILERS_ASSERT(false); result=0; } // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, loc, result, dest_type));
            }
            return ASR::make_IntegerBinOp_t(al, loc, left, op, right, dest_type, value);
        } else if (ASRUtils::is_real(*dest_type)) {
            right = cast_helper(left_type, right);
            dest_type = ASRUtils::expr_type(right);
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                double left_value = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(left))->m_r;
                double right_value = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(right))->m_r;
                double result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    default: { LCOMPILERS_ASSERT(false); result = 0; }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(al, loc, result, dest_type));
            }
            return ASR::make_RealBinOp_t(al, loc, left, op, right, dest_type, value);
        }

        return nullptr;
    }

    ASR::expr_t *cast_helper(ASR::ttype_t *left_type, ASR::expr_t *right,
            bool is_assign=false) {
        ASR::ttype_t *right_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(right));
        if (ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type)) {
            int lkind = ASR::down_cast<ASR::Integer_t>(left_type)->m_kind;
            int rkind = ASR::down_cast<ASR::Integer_t>(right_type)->m_kind;
            if ((is_assign && (lkind != rkind)) || (lkind > rkind)) {
                return ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, right->base.loc, right, ASR::cast_kindType::IntegerToInteger,
                    left_type));
            }
        }
        return right;
    }

};

ASR::symbol_t* pass_instantiate_symbol(Allocator &al,
        std::map<std::string, std::string> context_map,
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
        std::map<std::string, std::string> context_map,
        std::map<std::string, ASR::ttype_t*> type_subs,
        std::map<std::string, ASR::symbol_t*> symbol_subs,
        SymbolTable *current_scope, SymbolTable *template_scope,
        ASR::Function_t *new_f, ASR::Function_t *f) {
    SymbolInstantiator t(al, context_map, type_subs, symbol_subs,
        current_scope, template_scope, new_f->m_name);
    return t.instantiate_body(new_f, f);
}

class FunctionInstantiator : public ASR::BaseExprStmtDuplicator<FunctionInstantiator>
{
public:
    SymbolTable *func_scope;
    SymbolTable *current_scope;
    std::map<std::string, ASR::ttype_t*> subs;
    std::map<std::string, ASR::symbol_t*> rt_subs;
    std::string new_func_name;
    std::vector<ASR::Function_t*> rts;
    SetChar dependencies;

    FunctionInstantiator(Allocator &al, std::map<std::string, ASR::ttype_t*> subs,
            std::map<std::string, ASR::symbol_t*> rt_subs, SymbolTable *func_scope,
            std::string new_func_name):
        BaseExprStmtDuplicator(al),
        func_scope{func_scope},
        subs{subs},
        rt_subs{rt_subs},
        new_func_name{new_func_name}
        {}

    ASR::asr_t* instantiate_Function(ASR::Function_t *x) {
        dependencies.clear(al);
        current_scope = al.make_new<SymbolTable>(func_scope);

        Vec<ASR::expr_t*> args;
        args.reserve(al, x->n_args);
        for (size_t i=0; i<x->n_args; i++) {
            ASR::Variable_t *param_var = ASR::down_cast<ASR::Variable_t>(
                (ASR::down_cast<ASR::Var_t>(x->m_args[i]))->m_v);
            ASR::ttype_t *param_type = ASRUtils::expr_type(x->m_args[i]);
            ASR::ttype_t *arg_type = substitute_type(param_type);

            Location loc = param_var->base.base.loc;
            std::string var_name = param_var->m_name;
            ASR::intentType s_intent = param_var->m_intent;
            ASR::expr_t *init_expr = nullptr;
            ASR::expr_t *value = nullptr;
            ASR::storage_typeType storage_type = param_var->m_storage;
            ASR::abiType abi_type = param_var->m_abi;
            ASR::accessType s_access = param_var->m_access;
            ASR::presenceType s_presence = param_var->m_presence;
            bool value_attr = param_var->m_value_attr;

            // TODO: Copying variable can be abstracted into a function
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, arg_type);
            ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), variable_dependencies_vec.p, variable_dependencies_vec.size(),
                s_intent, init_expr, value, storage_type, arg_type, nullptr,
                abi_type, s_access, s_presence, value_attr);

            current_scope->add_symbol(var_name, ASR::down_cast<ASR::symbol_t>(v));

            ASR::symbol_t *var = current_scope->get_symbol(var_name);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x->base.base.loc, var)));
        }

        ASR::expr_t *new_return_var_ref = nullptr;
        if (x->m_return_var != nullptr) {
            ASR::Variable_t *return_var = ASR::down_cast<ASR::Variable_t>(
                (ASR::down_cast<ASR::Var_t>(x->m_return_var))->m_v);
            std::string return_var_name = return_var->m_name;
            ASR::ttype_t *return_param_type = ASRUtils::expr_type(x->m_return_var);
            ASR::ttype_t *return_type = substitute_type(return_param_type);
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, return_type);
            ASR::asr_t *new_return_var = ASR::make_Variable_t(al, return_var->base.base.loc,
                current_scope, s2c(al, return_var_name),
                variable_dependencies_vec.p,
                variable_dependencies_vec.size(),
                return_var->m_intent, nullptr, nullptr,
                return_var->m_storage, return_type, return_var->m_type_declaration,
                return_var->m_abi, return_var->m_access,
                return_var->m_presence, return_var->m_value_attr);
            current_scope->add_symbol(return_var_name, ASR::down_cast<ASR::symbol_t>(new_return_var));
            new_return_var_ref = ASRUtils::EXPR(ASR::make_Var_t(al, x->base.base.loc,
                current_scope->get_symbol(return_var_name)));
        }

        // Rebuild the symbol table
        for (auto const &sym_pair: x->m_symtab->get_scope()) {
            if (current_scope->resolve_symbol(sym_pair.first) == nullptr) {
                ASR::symbol_t *sym = sym_pair.second;
                if (ASR::is_a<ASR::Variable_t>(*sym)) {
                    ASR::ttype_t *new_sym_type = substitute_type(ASRUtils::symbol_type(sym));
                    ASR::Variable_t *var_sym = ASR::down_cast<ASR::Variable_t>(sym);
                    std::string var_sym_name = var_sym->m_name;
                    SetChar variable_dependencies_vec;
                    variable_dependencies_vec.reserve(al, 1);
                    ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, new_sym_type);
                    ASR::asr_t *new_var = ASR::make_Variable_t(al, var_sym->base.base.loc,
                        current_scope, s2c(al, var_sym_name), variable_dependencies_vec.p,
                        variable_dependencies_vec.size(), var_sym->m_intent, nullptr, nullptr,
                        var_sym->m_storage, new_sym_type, var_sym->m_type_declaration, var_sym->m_abi, var_sym->m_access,
                        var_sym->m_presence, var_sym->m_value_attr);
                    current_scope->add_symbol(var_sym_name, ASR::down_cast<ASR::symbol_t>(new_var));
                }
            }
        }

        for (size_t i=0; i < ASRUtils::get_FunctionType(x)->n_restrictions; i++) {
            rts.push_back(ASR::down_cast<ASR::Function_t>(
                ASRUtils::get_FunctionType(x)->m_restrictions[i]));
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x->n_body);
        for (size_t i=0; i<x->n_body; i++) {
            ASR::stmt_t *new_body = this->duplicate_stmt(x->m_body[i]);
            if (new_body != nullptr) {
                body.push_back(al, new_body);
            }
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
            current_scope, s2c(al, new_func_name),
            deps_vec.p, deps_vec.size(),
            args.p, args.size(),
            body.p, body.size(),
            new_return_var_ref,
            func_abi, func_access, func_deftype, bindc_name,
            func_elemental, func_pure, func_module, ASRUtils::get_FunctionType(x)->m_inline,
            ASRUtils::get_FunctionType(x)->m_static, nullptr, 0, nullptr, 0, false, false, false);

        ASR::symbol_t *t = ASR::down_cast<ASR::symbol_t>(result);
        func_scope->add_symbol(new_func_name, t);

        return result;
    }

    ASR::asr_t* duplicate_Var(ASR::Var_t *x) {
        std::string sym_name = ASRUtils::symbol_name(x->m_v);
        ASR::symbol_t *sym = current_scope->get_symbol(sym_name);
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
        std::string call_name = ASRUtils::symbol_name(x->m_name);
        ASR::symbol_t *name = func_scope->get_symbol(call_name);
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
        if (ASRUtils::is_restriction_function(name)) {
            name = rt_subs[call_name];
        } else if (ASRUtils::is_generic_function(name)) {
            std::string nested_func_name = current_scope->get_unique_name("__asr_generic_" + call_name, false);
            ASR::symbol_t* name2 = ASRUtils::symbol_get_past_external(name);
            ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(name2);
            FunctionInstantiator nested_tf(al, subs, rt_subs, func_scope, nested_func_name);
            ASR::asr_t* nested_generic_func = nested_tf.instantiate_Function(func);
            name = ASR::down_cast<ASR::symbol_t>(nested_generic_func);
        }
        dependencies.push_back(al, ASRUtils::symbol_name(name));
        return ASRUtils::make_FunctionCall_t_util(al, x->base.base.loc, name, x->m_original_name,
            args.p, args.size(), type, value, dt);
    }

    ASR::asr_t* duplicate_SubroutineCall(ASR::SubroutineCall_t *x) {
        std::string call_name = ASRUtils::symbol_name(x->m_name);
        ASR::symbol_t *name = func_scope->get_symbol(call_name);
        Vec<ASR::call_arg_t> args;
        args.reserve(al, x->n_args);
        for (size_t i=0; i<x->n_args; i++) {
            ASR::call_arg_t new_arg;
            new_arg.loc = x->m_args[i].loc;
            new_arg.m_value = duplicate_expr(x->m_args[i].m_value);
            args.push_back(al, new_arg);
        }
        ASR::expr_t* dt = duplicate_expr(x->m_dt);
        if (ASRUtils::is_restriction_function(name)) {
            name = rt_subs[call_name];
        } else {
            std::string nested_func_name = current_scope->get_unique_name("__asr_generic_" + call_name, false);
            ASR::symbol_t* name2 = ASRUtils::symbol_get_past_external(name);
            ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(name2);
            FunctionInstantiator nested_tf(al, subs, rt_subs, func_scope, nested_func_name);
            ASR::asr_t* nested_generic_func = nested_tf.instantiate_Function(func);
            name = ASR::down_cast<ASR::symbol_t>(nested_generic_func);
        }
        dependencies.push_back(al, ASRUtils::symbol_name(name));
        return ASRUtils::make_SubroutineCall_t_util(al, x->base.base.loc, name /* change this */,
            x->m_original_name, args.p, args.size(), dt, nullptr, false);
    }


    ASR::ttype_t* substitute_type(ASR::ttype_t *param_type) {
        if (ASR::is_a<ASR::List_t>(*param_type)) {
            ASR::List_t *tlist = ASR::down_cast<ASR::List_t>(param_type);
            return ASRUtils::TYPE(ASR::make_List_t(al, param_type->base.loc,
                substitute_type(tlist->m_type)));
        }
        ASR::ttype_t* param_type_ = ASRUtils::type_get_past_array(param_type);
        ASR::dimension_t* m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(param_type, m_dims);
        if (ASR::is_a<ASR::TypeParameter_t>(*param_type_)) {
            ASR::TypeParameter_t *param = ASR::down_cast<ASR::TypeParameter_t>(param_type_);
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
            ASR::ttype_t *t = ASRUtils::type_get_past_array(subs[param->m_param]);
            ASR::ttype_t *t_ = nullptr;
            switch (t->type) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* tnew = ASR::down_cast<ASR::Integer_t>(t);
                    t_ = ASRUtils::TYPE(ASR::make_Integer_t(al, t->base.loc, tnew->m_kind));
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* tnew = ASR::down_cast<ASR::Real_t>(t);
                    t_ = ASRUtils::TYPE(ASR::make_Real_t(al, t->base.loc, tnew->m_kind));
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* tnew = ASR::down_cast<ASR::Character_t>(t);
                    t_ = ASRUtils::TYPE(ASR::make_Character_t(al, t->base.loc,
                                tnew->m_kind, tnew->m_len, tnew->m_len_expr));
                    break;
                }
                default: {
                    return subs[param->m_param];
                }
            }
            if( new_dims.size() > 0 ) {
                t_ = ASRUtils::make_Array_t_util(al, t->base.loc,
                    t_, new_dims.p, new_dims.size());
            }
            return t_;
        }
        return param_type;
    }

    ASR::asr_t* make_BinOp_helper(ASR::expr_t *left, ASR::expr_t *right,
            ASR::binopType op, const Location &loc) {
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;

        if (op == ASR::binopType::Div) {
            dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
            if (ASRUtils::is_integer(*left_type)) {
                left = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type));
            }
            if (ASRUtils::is_integer(*right_type)) {
                if (ASRUtils::expr_value(right) != nullptr) {
                    int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(right))->m_n;
                    if (val == 0) {
                        throw SemanticError("division by zero is not allowed", right->base.loc);
                    }
                }
                right = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type));
            } else if (ASRUtils::is_real(*right_type)) {
                if (ASRUtils::expr_value(right) != nullptr) {
                    double val = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(right))->m_r;
                    if (val == 0.0) {
                        throw SemanticError("float division by zero is not allowed", right->base.loc);
                    }
                }
            }
        }

        if ((ASRUtils::is_integer(*left_type) || ASRUtils::is_real(*left_type)) &&
                (ASRUtils::is_integer(*right_type) || ASRUtils::is_real(*right_type))) {
            left = cast_helper(ASRUtils::expr_type(right), left);
            right = cast_helper(ASRUtils::expr_type(left), right);
            dest_type = substitute_type(ASRUtils::expr_type(left));
        }

        if (ASRUtils::is_integer(*dest_type)) {
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(left))->m_n;
                int64_t right_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(right))->m_n;
                int64_t result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    default: { LCOMPILERS_ASSERT(false); result=0; } // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, loc, result, dest_type));
            }
            return ASR::make_IntegerBinOp_t(al, loc, left, op, right, dest_type, value);
        } else if (ASRUtils::is_real(*dest_type)) {
            right = cast_helper(left_type, right);
            dest_type = ASRUtils::expr_type(right);
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                double left_value = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(left))->m_r;
                double right_value = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(right))->m_r;
                double result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    default: { LCOMPILERS_ASSERT(false); result = 0; }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(al, loc, result, dest_type));
            }
            return ASR::make_RealBinOp_t(al, loc, left, op, right, dest_type, value);
        }

        return nullptr;
    }

    ASR::expr_t *cast_helper(ASR::ttype_t *left_type, ASR::expr_t *right,
            bool is_assign=false) {
        ASR::ttype_t *right_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(right));
        if (ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type)) {
            int lkind = ASR::down_cast<ASR::Integer_t>(left_type)->m_kind;
            int rkind = ASR::down_cast<ASR::Integer_t>(right_type)->m_kind;
            if ((is_assign && (lkind != rkind)) || (lkind > rkind)) {
                return ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, right->base.loc, right, ASR::cast_kindType::IntegerToInteger,
                    left_type));
            }
        }
        return right;
    }

};

ASR::symbol_t* pass_instantiate_template(Allocator &al, std::map<std::string, ASR::ttype_t*> subs,
        std::map<std::string, ASR::symbol_t*> rt_subs, SymbolTable *current_scope,
        std::string new_func_name, ASR::symbol_t *sym) {
    ASR::symbol_t* sym2 = ASRUtils::symbol_get_past_external(sym);
    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(sym2);
    FunctionInstantiator tf(al, subs, rt_subs, current_scope, new_func_name);
    ASR::asr_t *new_function = tf.instantiate_Function(func);
    return ASR::down_cast<ASR::symbol_t>(new_function);
}

} // namespace LCompilers
