#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>

namespace LCompilers {

    namespace PassUtils {

        void get_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims) {
            ASR::ttype_t* t2 = ASRUtils::type_get_past_allocatable(
                ASRUtils::type_get_past_pointer(x_type));
            switch( t2->type ) {
                case ASR::ttypeType::Array: {
                    ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t2);
                    n_dims = array_t->n_dims;
                    m_dims = array_t->m_dims;
                    break ;
                }
                default:
                    break;
            }
        }

        ASR::ttype_t* set_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims,
                                    bool create_new, Allocator* al) {
            ASR::ttype_t* new_type = nullptr;
            ASR::ttype_t* t2 = ASRUtils::type_get_past_pointer(x_type);
            switch( t2->type ) {
                case ASR::ttypeType::Array: {
                    ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t2);
                    if( create_new ) {
                        new_type = ASRUtils::make_Array_t_util(*al,
                            x_type->base.loc, array_t->m_type, m_dims, n_dims);
                    } else {
                        array_t->n_dims = n_dims;
                        array_t->m_dims = m_dims;
                    }
                }
                default:
                    break;
            }

            if (ASR::is_a<ASR::Pointer_t>(*x_type)) {
                new_type = ASRUtils::TYPE(ASR::make_Pointer_t(*al, x_type->base.loc,
                    ASRUtils::type_get_past_allocatable(new_type)));
            }

            return new_type;
        }

        int get_rank(ASR::expr_t* x) {
            int n_dims = 0;
            if( ASR::is_a<ASR::Var_t>(*x) ) {
                const ASR::symbol_t* x_sym = ASRUtils::symbol_get_past_external(
                                                ASR::down_cast<ASR::Var_t>(x)->m_v);
                if( x_sym->type == ASR::symbolType::Variable ) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(x_sym);
                    ASR::ttype_t* x_type = v->m_type;
                    ASR::dimension_t* m_dims;
                    get_dim_rank(x_type, m_dims, n_dims);
                }
            } else if (ASR::is_a<ASR::StructInstanceMember_t>(*x)) {
                ASR::ttype_t *x_type = ASR::down_cast<ASR::StructInstanceMember_t>(x)->m_type;
                ASR::dimension_t* m_dims;
                get_dim_rank(x_type, m_dims, n_dims);
            } else if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*x)) {
                ASR::ttype_t* x_type = ASR::down_cast<ASR::ArrayPhysicalCast_t>(x)->m_type;
                ASR::dimension_t* m_dims;
                get_dim_rank(x_type, m_dims, n_dims);
            }
            return n_dims;
        }

        bool is_array(ASR::expr_t* x) {
            return get_rank(x) > 0;
        }

         #define fix_struct_type_scope() array_ref_type = ASRUtils::type_get_past_array( \
                ASRUtils::type_get_past_pointer( \
                    ASRUtils::type_get_past_allocatable(array_ref_type))); \
            if( current_scope && ASR::is_a<ASR::Struct_t>(*array_ref_type) ) { \
                ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(array_ref_type); \
                if( current_scope->get_counter() != ASRUtils::symbol_parent_symtab( \
                        struct_t->m_derived_type)->get_counter() ) { \
                    ASR::symbol_t* m_derived_type = current_scope->resolve_symbol( \
                        ASRUtils::symbol_name(struct_t->m_derived_type)); \
                    ASR::ttype_t* struct_type = ASRUtils::TYPE(ASR::make_Struct_t(al, \
                        struct_t->base.base.loc, m_derived_type)); \
                    array_ref_type = struct_type; \
                } \
            } \

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, ASR::expr_t* idx_var,
            Allocator& al, SymbolTable* current_scope, bool perform_cast,
            ASR::cast_kindType cast_kind, ASR::ttype_t* casted_type) {
            Vec<ASR::array_index_t> args;
            args.reserve(al, 1);
            ASR::array_index_t ai;
            ai.loc = arr_expr->base.loc;
            ai.m_left = nullptr;
            ai.m_right = idx_var;
            ai.m_step = nullptr;
            args.push_back(al, ai);
            ASR::ttype_t* array_ref_type = ASRUtils::duplicate_type_without_dims(
                al, ASRUtils::expr_type(arr_expr), arr_expr->base.loc);
            fix_struct_type_scope()
            ASR::expr_t* array_ref = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al,
                                        arr_expr->base.loc, arr_expr,
                                        args.p, args.size(),
                                        ASRUtils::type_get_past_array(
                                            ASRUtils::type_get_past_pointer(
                                                ASRUtils::type_get_past_allocatable(array_ref_type))),
                                        ASR::arraystorageType::RowMajor, nullptr));
            if( perform_cast ) {
                LCOMPILERS_ASSERT(casted_type != nullptr);
                array_ref = ASRUtils::EXPR(ASR::make_Cast_t(al, array_ref->base.loc,
                    array_ref, cast_kind, casted_type, nullptr));
            }
            return array_ref;
        }

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr,
            Vec<ASR::expr_t*>& idx_vars, Allocator& al, SymbolTable* current_scope,
            bool perform_cast, ASR::cast_kindType cast_kind, ASR::ttype_t* casted_type) {
            Vec<ASR::array_index_t> args;
            args.reserve(al, 1);
            for( size_t i = 0; i < idx_vars.size(); i++ ) {
                ASR::array_index_t ai;
                ai.loc = arr_expr->base.loc;
                ai.m_left = nullptr;
                ai.m_right = idx_vars[i];
                ai.m_step = nullptr;
                args.push_back(al, ai);
            }

            ASR::ttype_t* array_ref_type = ASRUtils::duplicate_type_without_dims(
                al, ASRUtils::expr_type(arr_expr), arr_expr->base.loc);
            fix_struct_type_scope()
            ASR::expr_t* array_ref = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al,
                                        arr_expr->base.loc, arr_expr,
                                        args.p, args.size(),
                                        ASRUtils::type_get_past_array(
                                            ASRUtils::type_get_past_pointer(
                                                ASRUtils::type_get_past_allocatable(array_ref_type))),
                                        ASR::arraystorageType::RowMajor, nullptr));
            if( perform_cast ) {
                LCOMPILERS_ASSERT(casted_type != nullptr);
                array_ref = ASRUtils::EXPR(ASR::make_Cast_t(al, array_ref->base.loc,
                    array_ref, cast_kind, casted_type, nullptr));
            }
            return array_ref;
        }

        ASR::expr_t* create_array_ref(ASR::ArraySection_t* array_section,
            Vec<ASR::expr_t*>& idx_vars, Allocator& al, SymbolTable* current_scope,
            bool perform_cast, ASR::cast_kindType cast_kind, ASR::ttype_t* casted_type) {
            Vec<ASR::array_index_t> args;
            args.reserve(al, 1);
            const Location& loc = array_section->base.base.loc;
            for( size_t i = 0; i < idx_vars.size(); i++ ) {
                ASR::array_index_t ai;
                ai.loc = loc;
                ai.m_left = nullptr;
                ai.m_step = nullptr;
                if( array_section->m_args[i].m_step == nullptr ) {
                    ai.m_right = array_section->m_args[i].m_right;
                } else {
                    ai.m_right = idx_vars[i];
                }
                args.push_back(al, ai);
            }
            Vec<ASR::dimension_t> empty_dims;
            empty_dims.reserve(al, 1);
            ASR::ttype_t* array_ref_type = array_section->m_type;
            array_ref_type = ASRUtils::duplicate_type_without_dims(al, array_ref_type, loc);
            fix_struct_type_scope()
            ASR::expr_t* array_ref = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al,
                                        loc, array_section->m_v,
                                        args.p, args.size(),
                                        array_ref_type,
                                        ASR::arraystorageType::RowMajor, nullptr));
            if( perform_cast ) {
                LCOMPILERS_ASSERT(casted_type != nullptr);
                array_ref = ASRUtils::EXPR(ASR::make_Cast_t(al, array_ref->base.loc,
                    array_ref, cast_kind, casted_type, nullptr));
            }
            return array_ref;
        }

        ASR::expr_t* create_array_ref(ASR::symbol_t* arr, Vec<ASR::expr_t*>& idx_vars, Allocator& al,
            const Location& loc, ASR::ttype_t* _type, SymbolTable* current_scope, bool perform_cast,
            ASR::cast_kindType cast_kind, ASR::ttype_t* casted_type) {
            Vec<ASR::array_index_t> args;
            args.reserve(al, 1);
            for( size_t i = 0; i < idx_vars.size(); i++ ) {
                ASR::array_index_t ai;
                ai.loc = loc;
                ai.m_left = nullptr;
                ai.m_right = idx_vars[i];
                ai.m_step = nullptr;
                args.push_back(al, ai);
            }
            ASR::ttype_t* array_ref_type = ASRUtils::duplicate_type_without_dims(al, _type, loc);
            fix_struct_type_scope()
            ASR::expr_t* arr_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, arr));
            ASR::expr_t* array_ref = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc, arr_var,
                                        args.p, args.size(),
                                        array_ref_type,
                                        ASR::arraystorageType::RowMajor, nullptr));
            if( perform_cast ) {
                LCOMPILERS_ASSERT(casted_type != nullptr);
                array_ref = ASRUtils::EXPR(ASR::make_Cast_t(al, array_ref->base.loc,
                    array_ref, cast_kind, casted_type, nullptr));
            }
            return array_ref;
        }

        ASR::ttype_t* get_matching_type(ASR::expr_t* sibling, Allocator& al) {
            ASR::ttype_t* sibling_type = ASRUtils::expr_type(sibling);
            ASR::dimension_t* m_dims;
            int ndims;
            PassUtils::get_dim_rank(sibling_type, m_dims, ndims);
            if( !ASRUtils::is_fixed_size_array(m_dims, ndims) &&
                !ASRUtils::is_dimension_dependent_only_on_arguments(m_dims, ndims) ) {
                return ASRUtils::TYPE(ASR::make_Allocatable_t(al, sibling_type->base.loc,
                    ASRUtils::type_get_past_allocatable(
                        ASRUtils::duplicate_type_with_empty_dims(al, sibling_type))));
            }
            for( int i = 0; i < ndims; i++ ) {
                if( m_dims[i].m_start != nullptr ||
                    m_dims[i].m_length != nullptr ) {
                    return sibling_type;
                }
            }
            Vec<ASR::dimension_t> new_m_dims;
            new_m_dims.reserve(al, ndims);
            for( int i = 0; i < ndims; i++ ) {
                ASR::dimension_t new_m_dim;
                new_m_dim.loc = m_dims[i].loc;
                new_m_dim.m_start = PassUtils::get_bound(sibling, i + 1, "lbound", al);
                new_m_dim.m_length = ASRUtils::compute_length_from_start_end(al, new_m_dim.m_start,
                                        PassUtils::get_bound(sibling, i + 1, "ubound", al));
                new_m_dims.push_back(al, new_m_dim);
            }
            return PassUtils::set_dim_rank(sibling_type, new_m_dims.p, ndims, true, &al);
    }

    ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                            ASR::ttype_t* var_type, Allocator& al, SymbolTable*& current_scope) {
        ASR::dimension_t* m_dims = nullptr;
        int ndims = 0;
        PassUtils::get_dim_rank(var_type, m_dims, ndims);
        if( !ASRUtils::is_fixed_size_array(m_dims, ndims) &&
            !ASRUtils::is_dimension_dependent_only_on_arguments(m_dims, ndims) &&
            !(ASR::is_a<ASR::Allocatable_t>(*var_type) || ASR::is_a<ASR::Pointer_t>(*var_type)) ) {
            var_type = ASRUtils::TYPE(ASR::make_Allocatable_t(al, var_type->base.loc,
                ASRUtils::type_get_past_allocatable(
                    ASRUtils::duplicate_type_with_empty_dims(al, var_type))));
        }
        ASR::expr_t* idx_var = nullptr;
        std::string str_name = "__libasr__created__var__" + std::to_string(counter) + "_" + suffix;
        char* idx_var_name = s2c(al, str_name);

        if( current_scope->get_symbol(std::string(idx_var_name)) == nullptr ) {
            ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, idx_var_name, nullptr, 0,
                                                    ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                                    var_type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                                    ASR::presenceType::Required, false);
            current_scope->add_symbol(std::string(idx_var_name), ASR::down_cast<ASR::symbol_t>(idx_sym));
            idx_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
        } else {
            ASR::symbol_t* idx_sym = current_scope->get_symbol(std::string(idx_var_name));
            idx_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));
        }

        return idx_var;
    }

    ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                            ASR::expr_t* sibling, Allocator& al, SymbolTable*& current_scope) {
        ASR::ttype_t* var_type = nullptr;
        var_type = get_matching_type(sibling, al);
        return create_var(counter, suffix, loc, var_type, al, current_scope);
    }

    void fix_dimension(ASR::Cast_t* x, ASR::expr_t* arg_expr) {
        ASR::ttype_t* x_type = const_cast<ASR::ttype_t*>(x->m_type);
        ASR::ttype_t* arg_type = ASRUtils::expr_type(arg_expr);
        ASR::dimension_t* m_dims;
        int ndims;
        PassUtils::get_dim_rank(arg_type, m_dims, ndims);
        PassUtils::set_dim_rank(x_type, m_dims, ndims);
    }

        void create_vars(Vec<ASR::expr_t*>& vars, int n_vars, const Location& loc,
                         Allocator& al, SymbolTable*& current_scope, std::string suffix,
                         ASR::intentType intent, ASR::presenceType presence) {
            vars.reserve(al, n_vars);
            for (int i = 1; i <= n_vars; i++) {
                std::string idx_var_name = "__" + std::to_string(i) + suffix;
                ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
                if( current_scope->get_symbol(idx_var_name) != nullptr ) {
                    ASR::symbol_t* idx_sym = current_scope->get_symbol(idx_var_name);
                    if( ASR::is_a<ASR::Variable_t>(*idx_sym) ) {
                        ASR::Variable_t* idx_var = ASR::down_cast<ASR::Variable_t>(idx_sym);
                        if( !(ASRUtils::check_equal_type(idx_var->m_type, int32_type) &&
                              idx_var->m_symbolic_value == nullptr) ) {
                            idx_var_name = current_scope->get_unique_name(idx_var_name, false);
                        }
                    } else {
                        idx_var_name = current_scope->get_unique_name(idx_var_name, false);
                    }
                }
                char* var_name = s2c(al, idx_var_name);;
                ASR::expr_t* var = nullptr;
                if( current_scope->get_symbol(idx_var_name) == nullptr ) {
                    ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, var_name, nullptr, 0,
                                                            intent, nullptr, nullptr, ASR::storage_typeType::Default,
                                                            int32_type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                                            presence, false);
                    current_scope->add_symbol(idx_var_name, ASR::down_cast<ASR::symbol_t>(idx_sym));
                    var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
                } else {
                    ASR::symbol_t* idx_sym = current_scope->get_symbol(idx_var_name);
                    var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));
                }
                vars.push_back(al, var);
            }
        }

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc,
                             Allocator& al, SymbolTable*& current_scope, std::string suffix) {
            create_vars(idx_vars, n_dims, loc, al, current_scope, suffix);
        }

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, ASR::array_index_t* m_args, int n_dims,
                             std::vector<int>& value_indices, const Location& loc, Allocator& al,
                             SymbolTable*& current_scope, std::string suffix) {
            idx_vars.reserve(al, n_dims);
            for (int i = 1; i <= n_dims; i++) {
                if( m_args[i - 1].m_step == nullptr ) {
                    idx_vars.push_back(al, m_args[i - 1].m_right);
                    continue;
                }
                std::string idx_var_name = "__" + std::to_string(i) + suffix;
                ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
                if( current_scope->get_symbol(idx_var_name) != nullptr ) {
                    ASR::symbol_t* idx_sym = current_scope->get_symbol(idx_var_name);
                    if( ASR::is_a<ASR::Variable_t>(*idx_sym) ) {
                        ASR::Variable_t* idx_var = ASR::down_cast<ASR::Variable_t>(idx_sym);
                        if( !(ASRUtils::check_equal_type(idx_var->m_type, int32_type) &&
                              idx_var->m_symbolic_value == nullptr) ) {
                            idx_var_name = current_scope->get_unique_name(idx_var_name, false);
                        }
                    } else {
                        idx_var_name = current_scope->get_unique_name(idx_var_name, false);
                    }
                }
                char* var_name = s2c(al, idx_var_name);;
                ASR::expr_t* var = nullptr;
                if( current_scope->get_symbol(idx_var_name) == nullptr ) {
                    ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, var_name, nullptr, 0,
                                            ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                            int32_type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                            ASR::presenceType::Required, false);
                    current_scope->add_symbol(idx_var_name, ASR::down_cast<ASR::symbol_t>(idx_sym));
                    var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
                } else {
                    ASR::symbol_t* idx_sym = current_scope->get_symbol(idx_var_name);
                    var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));
                }
                value_indices.push_back(i - 1);
                idx_vars.push_back(al, var);
            }
        }

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, Vec<ASR::expr_t*>& loop_vars,
                             std::vector<int>& loop_var_indices,
                             Vec<ASR::expr_t*>& vars, Vec<ASR::expr_t*>& incs,
                             const Location& loc, Allocator& al,
                             SymbolTable*& current_scope, std::string suffix) {
            idx_vars.reserve(al, incs.size());
            loop_vars.reserve(al, 1);
            for (size_t i = 0; i < incs.size(); i++) {
                if( incs[i] == nullptr ) {
                    idx_vars.push_back(al, vars[i]);
                    continue;
                }
                std::string idx_var_name = "__" + std::to_string(i + 1) + suffix;
                ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
                if( current_scope->get_symbol(idx_var_name) != nullptr ) {
                    ASR::symbol_t* idx_sym = current_scope->get_symbol(idx_var_name);
                    if( ASR::is_a<ASR::Variable_t>(*idx_sym) ) {
                        ASR::Variable_t* idx_var = ASR::down_cast<ASR::Variable_t>(idx_sym);
                        if( !(ASRUtils::check_equal_type(idx_var->m_type, int32_type) &&
                              idx_var->m_symbolic_value == nullptr) ) {
                            idx_var_name = current_scope->get_unique_name(idx_var_name, false);
                        }
                    } else {
                        idx_var_name = current_scope->get_unique_name(idx_var_name, false);
                    }
                }
                char* var_name = s2c(al, idx_var_name);;
                ASR::expr_t* var = nullptr;
                if( current_scope->get_symbol(idx_var_name) == nullptr ) {
                    ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, var_name, nullptr, 0,
                                            ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                            int32_type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                            ASR::presenceType::Required, false);
                    current_scope->add_symbol(idx_var_name, ASR::down_cast<ASR::symbol_t>(idx_sym));
                    var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
                } else {
                    ASR::symbol_t* idx_sym = current_scope->get_symbol(idx_var_name);
                    var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));
                }
                idx_vars.push_back(al, var);
                loop_vars.push_back(al, var);
                loop_var_indices.push_back(i);
            }
        }

        ASR::symbol_t* import_generic_procedure(std::string func_name, std::string module_name,
                                       Allocator& al, ASR::TranslationUnit_t& unit,
                                       LCompilers::PassOptions& pass_options,
                                       SymbolTable*& current_scope, Location& loc) {
            ASR::symbol_t *v;
            std::string remote_sym = func_name;
            SymbolTable* current_scope_copy = current_scope;
            current_scope = unit.m_symtab;
            // We tell `load_module` not to run verify, since the ASR might
            // not be in valid state. We run verify at the end of this pass
            // anyway, so verify will be run no matter what.
            ASR::Module_t *m = ASRUtils::load_module(al, current_scope,
                                            module_name, loc, true,
                                            pass_options, false,
                                            [&](const std::string &msg, const Location &) { throw LCompilersException(msg); }
                                            );
            ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);

            std::string sym = remote_sym;
            if( current_scope->get_symbol(sym) != nullptr ) {
                v = current_scope->get_symbol(sym);
                if( !ASRUtils::is_intrinsic_optimization<ASR::symbol_t>(v) ) {
                    sym += "@IntrinsicOptimization";
                } else {
                    current_scope = current_scope_copy;
                    return v;
                }
            }
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(al, t->base.loc, current_scope,
                                                        s2c(al, sym), t,
                                                        s2c(al, module_name), nullptr, 0, s2c(al, remote_sym),
                                                        ASR::accessType::Private);
            current_scope->add_or_overwrite_symbol(sym, ASR::down_cast<ASR::symbol_t>(fn));
            v = ASR::down_cast<ASR::symbol_t>(fn);
            current_scope = current_scope_copy;
            return v;
        }

        ASR::symbol_t* import_function(std::string func_name, std::string module_name,
                                       Allocator& al, ASR::TranslationUnit_t& unit,
                                       LCompilers::PassOptions& pass_options,
                                       SymbolTable*& current_scope, Location& loc) {
            ASR::symbol_t *v;
            std::string remote_sym = func_name;
            SymbolTable* current_scope_copy = current_scope;
            current_scope = unit.m_symtab;
            // We tell `load_module` not to run verify, since the ASR might
            // not be in valid state. We run verify at the end of this pass
            // anyway, so verify will be run no matter what.
            ASR::Module_t *m = ASRUtils::load_module(al, current_scope,
                                            module_name, loc, true,
                                            pass_options, false,
                                            [&](const std::string &msg, const Location &) { throw LCompilersException(msg); });

            ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
            ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(al, mfn->base.base.loc, current_scope,
                                                        mfn->m_name, (ASR::symbol_t*)mfn,
                                                        m->m_name, nullptr, 0, mfn->m_name, ASR::accessType::Private);
            std::string sym = mfn->m_name;
            if( current_scope->get_symbol(sym) != nullptr ) {
                v = current_scope->get_symbol(sym);
            } else {
                current_scope->add_symbol(sym, ASR::down_cast<ASR::symbol_t>(fn));
                v = ASR::down_cast<ASR::symbol_t>(fn);
            }
            current_scope = current_scope_copy;
            return v;
        }

        // Imports the function from an already loaded ASR module
        ASR::symbol_t* import_function2(std::string func_name, std::string module_name,
                                       Allocator& al, ASR::TranslationUnit_t& unit,
                                       SymbolTable*& current_scope) {
            ASR::symbol_t *v;
            std::string remote_sym = func_name;
            SymbolTable* current_scope_copy = current_scope;
            SymbolTable* current_scope2 = unit.m_symtab;

            ASR::Module_t *m;
            if (current_scope2->get_symbol(module_name) != nullptr) {
                ASR::symbol_t *sm = current_scope2->get_symbol(module_name);
                if (ASR::is_a<ASR::Module_t>(*sm)) {
                    m = ASR::down_cast<ASR::Module_t>(sm);
                } else {
                    // The symbol `module_name` is not a module
                    return nullptr;
                }
            } else {
                // The module `module_name` is not in ASR
                return nullptr;
            }
            ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
            if (!t) return nullptr;
            ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(al, mfn->base.base.loc, current_scope2,
                                                        mfn->m_name, (ASR::symbol_t*)mfn,
                                                        m->m_name, nullptr, 0, mfn->m_name, ASR::accessType::Private);
            std::string sym = mfn->m_name;
            if( current_scope2->get_symbol(sym) != nullptr ) {
                v = current_scope2->get_symbol(sym);
            } else {
                current_scope2->add_symbol(sym, ASR::down_cast<ASR::symbol_t>(fn));
                v = ASR::down_cast<ASR::symbol_t>(fn);
            }
            current_scope2 = current_scope_copy;
            return v;
        }

        ASR::expr_t* create_compare_helper(Allocator &al, const Location &loc, ASR::expr_t* left, ASR::expr_t* right,
                                                ASR::cmpopType op) {
            ASR::ttype_t* type = ASRUtils::expr_type(left);
            ASR::ttype_t* cmp_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
            // TODO: compute `value`:
            if (ASRUtils::is_integer(*type)) {
                return ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc, left, op, right, cmp_type, nullptr));
            } else if (ASRUtils::is_real(*type)) {
                return ASRUtils::EXPR(ASR::make_RealCompare_t(al, loc, left, op, right, cmp_type, nullptr));
            } else if (ASRUtils::is_complex(*type)) {
                return ASRUtils::EXPR(ASR::make_ComplexCompare_t(al, loc, left, op, right, cmp_type, nullptr));
            } else if (ASRUtils::is_logical(*type)) {
                return ASRUtils::EXPR(ASR::make_LogicalCompare_t(al, loc, left, op, right, cmp_type, nullptr));
            } else if (ASRUtils::is_character(*type)) {
                return ASRUtils::EXPR(ASR::make_StringCompare_t(al, loc, left, op, right, cmp_type, nullptr));
            } else {
                throw LCompilersException("Type not supported");
            }
        }

        ASR::expr_t* create_binop_helper(Allocator &al, const Location &loc, ASR::expr_t* left, ASR::expr_t* right,
                                                ASR::binopType op) {
            ASR::ttype_t* type = ASRUtils::expr_type(left);
            ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
            // TODO: compute `value`:
            if (ASRUtils::is_integer(*type)) {
                return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, left, op, right, type, nullptr));
            } else if (ASRUtils::is_real(*type)) {
                return ASRUtils::EXPR(ASR::make_RealBinOp_t(al, loc, left, op, right, type, nullptr));
            } else if (ASRUtils::is_complex(*type)) {
                return ASRUtils::EXPR(ASR::make_ComplexBinOp_t(al, loc, left, op, right, type, nullptr));
            } else {
                throw LCompilersException("Type not supported");
            }
        }

        ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim, std::string bound,
                                Allocator& al) {
            ASR::ttype_t* x_mv_type = ASRUtils::expr_type(arr_expr);
            ASR::dimension_t* m_dims;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(x_mv_type, m_dims);
            bool is_data_only_array = ASRUtils::is_fixed_size_array(m_dims, n_dims) && ASRUtils::get_asr_owner(arr_expr) &&
                                    ASR::is_a<ASR::StructType_t>(*ASRUtils::get_asr_owner(arr_expr));
            ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4));
            if (is_data_only_array) {
                const Location& loc = arr_expr->base.loc;
                ASR::expr_t* zero = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 0, int32_type));
                ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, int32_type));
                if( bound == "ubound" ) {
                    return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                        al, arr_expr->base.loc, m_dims[dim - 1].m_length, ASR::binopType::Sub, one, int32_type, nullptr));
                }
                if ( m_dims[dim - 1].m_start != nullptr ) {
                    return m_dims[dim - 1].m_start;
                }
                return  zero;
            }
            ASR::expr_t* dim_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_expr->base.loc, dim, int32_type));
            ASR::arrayboundType bound_type = ASR::arrayboundType::LBound;
            if( bound == "ubound" ) {
                bound_type = ASR::arrayboundType::UBound;
            }
            return ASRUtils::EXPR(ASR::make_ArrayBound_t(al, arr_expr->base.loc, arr_expr, dim_expr,
                        int32_type, bound_type, nullptr));
        }

        bool skip_instantiation(PassOptions pass_options, int64_t id) {
            if (!pass_options.skip_optimization_func_instantiation.empty()) {
                for (size_t i=0; i<pass_options.skip_optimization_func_instantiation.size(); i++) {
                    if (pass_options.skip_optimization_func_instantiation[i] == id) {
                        return true;
                    }
                }
            }
            return false;
        }

        ASR::expr_t* get_flipsign(ASR::expr_t* arg0, ASR::expr_t* arg1,
            Allocator& al, ASR::TranslationUnit_t& unit, const Location& loc,
            PassOptions& pass_options) {
            ASR::ttype_t* type = ASRUtils::expr_type(arg1);
            int64_t fp_s = static_cast<int64_t>(ASRUtils::IntrinsicScalarFunctions::FlipSign);
            if (skip_instantiation(pass_options, fp_s)) {
                Vec<ASR::expr_t*> args;
                args.reserve(al, 2);
                args.push_back(al, arg0);
                args.push_back(al, arg1);
                return ASRUtils::EXPR(ASRUtils::make_IntrinsicScalarFunction_t_util(al, loc, fp_s,
                    args.p, args.n, 0, type, nullptr));
            }
            ASRUtils::impl_function instantiate_function =
            ASRUtils::IntrinsicScalarFunctionRegistry::get_instantiate_function(
                    static_cast<int64_t>(ASRUtils::IntrinsicScalarFunctions::FlipSign));
            Vec<ASR::ttype_t*> arg_types;
            arg_types.reserve(al, 2);
            arg_types.push_back(al, ASRUtils::expr_type(arg0));
            arg_types.push_back(al, ASRUtils::expr_type(arg1));
            Vec<ASR::call_arg_t> args;
            args.reserve(al, 2);
            ASR::call_arg_t arg0_, arg1_;
            arg0_.loc = arg0->base.loc, arg0_.m_value = arg0;
            args.push_back(al, arg0_);
            arg1_.loc = arg1->base.loc, arg1_.m_value = arg1;
            args.push_back(al, arg1_);
            return instantiate_function(al, loc,
                unit.m_symtab, arg_types, type, args, 0);
        }

        ASR::expr_t* to_int32(ASR::expr_t* x, ASR::ttype_t* int64type, Allocator& al) {
            int cast_kind = -1;
            switch( ASRUtils::expr_type(x)->type ) {
                case ASR::ttypeType::Integer: {
                    cast_kind = ASR::cast_kindType::IntegerToInteger;
                    break;
                }

                case ASR::ttypeType::Real: {
                    cast_kind = ASR::cast_kindType::RealToInteger;
                    break;
                }

                default: {
                    break;
                }
            }
            if( cast_kind > 0 ) {
                return ASRUtils::EXPR(ASR::make_Cast_t(al, x->base.loc, x, (ASR::cast_kindType)cast_kind, int64type, nullptr));
            } else {
                throw LCompilersException("Array indices can only be of type real or integer.");
            }
            return nullptr;
        }

        ASR::expr_t* create_auxiliary_variable_for_expr(ASR::expr_t* expr, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::stmt_t*& assign_stmt) {
            ASR::asr_t* expr_sym = ASR::make_Variable_t(al, expr->base.loc, current_scope, s2c(al, name), nullptr, 0,
                                                    ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                                    ASRUtils::duplicate_type(al, ASRUtils::expr_type(expr)), nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                                    ASR::presenceType::Required, false);
            if( current_scope->get_symbol(name) == nullptr ) {
                current_scope->add_symbol(name, ASR::down_cast<ASR::symbol_t>(expr_sym));
            } else {
                throw LCompilersException("Symbol with " + name + " is already present in " + std::to_string(current_scope->counter));
            }
            ASR::expr_t* var = ASRUtils::EXPR(ASR::make_Var_t(al, expr->base.loc, ASR::down_cast<ASR::symbol_t>(expr_sym)));
            assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, var->base.loc, var, expr, nullptr));
            return var;
        }

        ASR::expr_t* create_auxiliary_variable(const Location& loc, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::ttype_t* var_type,
            ASR::intentType var_intent) {
            ASRUtils::import_struct_t(al, loc, var_type, var_intent, current_scope);
            ASR::asr_t* expr_sym = ASR::make_Variable_t(al, loc, current_scope, s2c(al, name), nullptr, 0,
                                                    var_intent, nullptr, nullptr, ASR::storage_typeType::Default,
                                                    var_type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                                    ASR::presenceType::Required, false);
            if( current_scope->get_symbol(name) == nullptr ) {
                current_scope->add_symbol(name, ASR::down_cast<ASR::symbol_t>(expr_sym));
            } else {
                throw LCompilersException("Symbol with " + name + " is already present in " + std::to_string(current_scope->counter));
            }
            ASR::expr_t* var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(expr_sym)));
            return var;
        }

        ASR::expr_t* get_fma(ASR::expr_t* arg0, ASR::expr_t* arg1, ASR::expr_t* arg2,
            Allocator& al, ASR::TranslationUnit_t& unit, Location& loc,
            PassOptions& pass_options) {
            int64_t fma_id = static_cast<int64_t>(ASRUtils::IntrinsicScalarFunctions::FMA);
            ASR::ttype_t* type = ASRUtils::expr_type(arg0);
            if (skip_instantiation(pass_options, fma_id)) {
                Vec<ASR::expr_t*> args;
                args.reserve(al, 3);
                args.push_back(al, arg0);
                args.push_back(al, arg1);
                args.push_back(al, arg2);
                return ASRUtils::EXPR(ASRUtils::make_IntrinsicScalarFunction_t_util(al, loc, fma_id,
                    args.p, args.n, 0, type, nullptr));
            }
            ASRUtils::impl_function instantiate_function =
            ASRUtils::IntrinsicScalarFunctionRegistry::get_instantiate_function(
                    static_cast<int64_t>(ASRUtils::IntrinsicScalarFunctions::FMA));
            Vec<ASR::ttype_t*> arg_types;
            arg_types.reserve(al, 3);
            arg_types.push_back(al, ASRUtils::expr_type(arg0));
            arg_types.push_back(al, ASRUtils::expr_type(arg1));
            arg_types.push_back(al, ASRUtils::expr_type(arg2));

            Vec<ASR::call_arg_t> args;
            args.reserve(al, 3);
            ASR::call_arg_t arg0_, arg1_, arg2_;
            arg0_.loc = arg0->base.loc, arg0_.m_value = arg0;
            args.push_back(al, arg0_);
            arg1_.loc = arg1->base.loc, arg1_.m_value = arg1;
            args.push_back(al, arg1_);
            arg2_.loc = arg2->base.loc, arg2_.m_value = arg2;
            args.push_back(al, arg2_);
            return instantiate_function(al, loc,
                unit.m_symtab, arg_types, type, args, 0);
        }

        ASR::symbol_t* insert_fallback_vector_copy(Allocator& al, ASR::TranslationUnit_t& unit,
            SymbolTable*& global_scope, std::vector<ASR::ttype_t*>& types,
            std::string prefix) {
            const int num_args = 6;
            std::string vector_copy_name = prefix;
            for( ASR::ttype_t*& type: types ) {
                vector_copy_name += ASRUtils::type_to_str_python(type, false);
            }
            vector_copy_name += "@IntrinsicOptimization";
            if( global_scope->resolve_symbol(vector_copy_name) ) {
                return global_scope->resolve_symbol(vector_copy_name);
            }
            Vec<ASR::expr_t*> arg_exprs;
            arg_exprs.reserve(al, num_args);
            SymbolTable* vector_copy_symtab = al.make_new<SymbolTable>(global_scope);
            for( int i = 0; i < num_args; i++ ) {
                std::string arg_name = "arg" + std::to_string(i);
                ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(al, unit.base.base.loc, vector_copy_symtab,
                                        s2c(al, arg_name), nullptr, 0, ASR::intentType::In, nullptr, nullptr, ASR::storage_typeType::Default,
                                        types[std::min(i, (int) types.size() - 1)], nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                        ASR::presenceType::Required, false));
                ASR::expr_t* arg_expr = ASRUtils::EXPR(ASR::make_Var_t(al, arg->base.loc, arg));
                arg_exprs.push_back(al, arg_expr);
                vector_copy_symtab->add_symbol(arg_name, arg);
            }
            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);
            ASR::do_loop_head_t do_loop_head;
            do_loop_head.m_start = arg_exprs[2];
            do_loop_head.m_end = arg_exprs[3];
            do_loop_head.m_increment = arg_exprs[4];
            do_loop_head.loc = arg_exprs[2]->base.loc;
            Vec<ASR::expr_t*> idx_vars;
            create_idx_vars(idx_vars, 1, arg_exprs[2]->base.loc,
                            al, vector_copy_symtab);
            do_loop_head.m_v = idx_vars[0];
            Vec<ASR::stmt_t*> loop_body;
            loop_body.reserve(al, 1);
            ASR::expr_t* target = create_array_ref(arg_exprs[0], idx_vars, al);
            ASR::expr_t* value = create_array_ref(arg_exprs[1], idx_vars, al);
            ASR::stmt_t* copy_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, target->base.loc,
                                        target, value, nullptr));
            loop_body.push_back(al, copy_stmt);
            ASR::stmt_t* fallback_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, do_loop_head.loc,
                                            nullptr, do_loop_head, loop_body.p, loop_body.size()));
            Vec<ASR::stmt_t*> fallback_while_loop = replace_doloop(al, *ASR::down_cast<ASR::DoLoop_t>(fallback_loop),
                                                                  (int) ASR::cmpopType::Lt);
            for( size_t i = 0; i < fallback_while_loop.size(); i++ ) {
                body.push_back(al, fallback_while_loop[i]);
            }
            ASR::asr_t* vector_copy_asr = ASRUtils::make_Function_t_util(al,
                unit.base.base.loc,
                vector_copy_symtab,
                s2c(al, vector_copy_name), nullptr, 0, arg_exprs.p, arg_exprs.size(),
                /* nullptr, 0, */ body.p, body.size(), nullptr,
                ASR::abiType::Source, ASR::accessType::Public,
                ASR::deftypeType::Implementation,
                nullptr, false, false, false, false, false,
                nullptr, 0, false, false, false);
            global_scope->add_symbol(vector_copy_name, ASR::down_cast<ASR::symbol_t>(vector_copy_asr));
            return ASR::down_cast<ASR::symbol_t>(vector_copy_asr);
        }

        ASR::stmt_t* get_vector_copy(ASR::expr_t* array0, ASR::expr_t* array1, ASR::expr_t* start,
            ASR::expr_t* end, ASR::expr_t* step, ASR::expr_t* vector_length,
            Allocator& al, ASR::TranslationUnit_t& unit,
            SymbolTable*& global_scope, Location& loc) {
            ASR::ttype_t* array0_type = ASRUtils::expr_type(array0);
            ASR::ttype_t* array1_type = ASRUtils::expr_type(array1);
            ASR::ttype_t* index_type = ASRUtils::expr_type(start);
            std::vector<ASR::ttype_t*> types = {array0_type, array1_type, index_type};
            ASR::symbol_t *v = insert_fallback_vector_copy(al, unit, global_scope,
                                                           types,
                                                           "vector_copy_");
            Vec<ASR::call_arg_t> args;
            args.reserve(al, 6);
            ASR::call_arg_t arg0_, arg1_, arg2_, arg3_, arg4_, arg5_;
            ASR::expr_t* array0_expr = array0;
            arg0_.loc = array0->base.loc, arg0_.m_value = array0_expr;
            args.push_back(al, arg0_);
            ASR::expr_t* array1_expr = array1;
            arg1_.loc = array1->base.loc, arg1_.m_value = array1_expr;
            args.push_back(al, arg1_);
            arg2_.loc = start->base.loc, arg2_.m_value = start;
            args.push_back(al, arg2_);
            arg3_.loc = end->base.loc, arg3_.m_value = end;
            args.push_back(al, arg3_);
            arg4_.loc = step->base.loc, arg4_.m_value = step;
            args.push_back(al, arg4_);
            arg5_.loc = vector_length->base.loc, arg5_.m_value = vector_length;
            args.push_back(al, arg5_);
            return ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, loc, v,
                                                             nullptr, args.p, args.size(),
                                                             nullptr, nullptr, false));
        }

        ASR::expr_t* get_sign_from_value(ASR::expr_t* arg0, ASR::expr_t* arg1,
            Allocator& al, ASR::TranslationUnit_t& unit, Location& loc,
            PassOptions& pass_options) {
            int64_t sfv_id = static_cast<int64_t>(ASRUtils::IntrinsicScalarFunctions::SignFromValue);
            ASR::ttype_t* type = ASRUtils::expr_type(arg0);
            if (skip_instantiation(pass_options, sfv_id)) {
                Vec<ASR::expr_t*> args;
                args.reserve(al, 2);
                args.push_back(al, arg0);
                args.push_back(al, arg1);
                return ASRUtils::EXPR(ASRUtils::make_IntrinsicScalarFunction_t_util(al, loc, sfv_id,
                    args.p, args.n, 0, type, nullptr));
            }
            ASRUtils::impl_function instantiate_function =
            ASRUtils::IntrinsicScalarFunctionRegistry::get_instantiate_function(
                    static_cast<int64_t>(ASRUtils::IntrinsicScalarFunctions::FMA));
            Vec<ASR::ttype_t*> arg_types;
            arg_types.reserve(al, 2);
            arg_types.push_back(al, ASRUtils::expr_type(arg0));
            arg_types.push_back(al, ASRUtils::expr_type(arg1));

            Vec<ASR::call_arg_t> args;
            args.reserve(al, 3);
            ASR::call_arg_t arg0_, arg1_;
            arg0_.loc = arg0->base.loc, arg0_.m_value = arg0;
            args.push_back(al, arg0_);
            arg1_.loc = arg1->base.loc, arg1_.m_value = arg1;
            args.push_back(al, arg1_);
            return instantiate_function(al, loc,
                unit.m_symtab, arg_types, type, args, 0);
        }

        Vec<ASR::stmt_t*> replace_doloop(Allocator &al, const ASR::DoLoop_t &loop,
                                         int comp, bool use_loop_variable_after_loop) {
            Location loc = loop.base.base.loc;
            ASR::expr_t *a=loop.m_head.m_start;
            ASR::expr_t *b=loop.m_head.m_end;
            ASR::expr_t *c=loop.m_head.m_increment;
            ASR::expr_t *cond = nullptr;
            ASR::stmt_t *inc_stmt = nullptr;
            ASR::stmt_t *loop_init_stmt = nullptr;
            ASR::stmt_t *stmt_add_c_after_loop = nullptr;
            if( !a && !b && !c ) {
                int a_kind = 4;
                if( loop.m_head.m_v ) {
                    a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(loop.m_head.m_v));
                }
                ASR::ttype_t *cond_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, a_kind));
                cond = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, true, cond_type));
            } else {
                LCOMPILERS_ASSERT(a);
                LCOMPILERS_ASSERT(b);
                if (!c) {
                    int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(loop.m_head.m_v));
                    ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, a_kind));
                    c = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, type));
                }
                LCOMPILERS_ASSERT(c);
                ASR::cmpopType cmp_op;

                if( comp == -1 ) {
                    int increment;
                    bool not_constant_inc = false;
                    if (!ASRUtils::is_integer(*ASRUtils::expr_type(c))) {
                        throw LCompilersException("Do loop increment type should be an integer");
                    }
                    if (c->type == ASR::exprType::IntegerConstant) {
                        increment = ASR::down_cast<ASR::IntegerConstant_t>(c)->m_n;
                    } else if (c->type == ASR::exprType::IntegerUnaryMinus) {
                        ASR::IntegerUnaryMinus_t *u = ASR::down_cast<ASR::IntegerUnaryMinus_t>(c);
                        increment = - ASR::down_cast<ASR::IntegerConstant_t>(u->m_arg)->m_n;
                    } else {
                        // This is the case when increment operator is not a
                        // constant, and so we need some conditions to check
                        // in the backend and generate while loop according
                        // to avoid infinite loops.
                        not_constant_inc = true;
                    }

                    if (not_constant_inc) {
                        ASR::expr_t *target = loop.m_head.m_v;
                        int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(target));
                        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, a_kind));

                        ASR::ttype_t *log_type = ASRUtils::TYPE(
                            ASR::make_Logical_t(al, loc, 4));
                        ASR::expr_t *const_zero = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                    loc, 0, int_type));

                        // test1: c > 0
                        ASR::expr_t *test1 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loop.base.base.loc,
                            c, ASR::cmpopType::Gt, const_zero, log_type, nullptr));
                        // test2: c <= 0
                        ASR::expr_t *test2 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loop.base.base.loc,
                            c, ASR::cmpopType::LtE, const_zero, log_type, nullptr));

                        // test11: target + c <= b
                        ASR::expr_t *test11 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                            ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, target,
                            ASR::binopType::Add, c, int_type, nullptr)), ASR::cmpopType::LtE, b, log_type, nullptr));

                        // test22: target + c >= b
                        ASR::expr_t *test22 = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                            ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, target,
                            ASR::binopType::Add, c, int_type, nullptr)), ASR::cmpopType::GtE, b, log_type, nullptr));

                        // cond1: test1 && test11
                        ASR::expr_t *cond1 = ASRUtils::EXPR(make_LogicalBinOp_t(al, loc,
                            test1, ASR::logicalbinopType::And, test11, log_type, nullptr));

                        // cond2: test2 && test22
                        ASR::expr_t *cond2 = ASRUtils::EXPR(make_LogicalBinOp_t(al, loc,
                            test2, ASR::logicalbinopType::And, test22, log_type, nullptr));

                        // cond: cond1 || cond2
                        cond = ASRUtils::EXPR(make_LogicalBinOp_t(al, loc,
                            cond1, ASR::logicalbinopType::Or, cond2, log_type, nullptr));
                        // TODO: is cmp_op uninitialized here?
                        cmp_op = ASR::cmpopType::LtE; // silence a warning
                    } else if (increment > 0) {
                        cmp_op = ASR::cmpopType::LtE;
                    } else {
                        cmp_op = ASR::cmpopType::GtE;
                    }
                } else {
                    cmp_op = (ASR::cmpopType) comp;
                }

                ASR::expr_t *target = loop.m_head.m_v;
                int a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(target));
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, a_kind));

                loop_init_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
                    ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, a,
                            ASR::binopType::Sub, c, type, nullptr)), nullptr));
                if (use_loop_variable_after_loop) {
                    stmt_add_c_after_loop = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
                        ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, target,
                                ASR::binopType::Add, c, type, nullptr)), nullptr));
                }

                inc_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
                            ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, target,
                                ASR::binopType::Add, c, type, nullptr)), nullptr));
                if (cond == nullptr) {
                    ASR::ttype_t *log_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
                    cond = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                        ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, target,
                            ASR::binopType::Add, c, type, nullptr)), cmp_op, b, log_type, nullptr));
                }
            }
            Vec<ASR::stmt_t*> body;
            body.reserve(al, loop.n_body + (inc_stmt != nullptr));
            if( inc_stmt ) {
                body.push_back(al, inc_stmt);
            }
            for (size_t i=0; i<loop.n_body; i++) {
                body.push_back(al, loop.m_body[i]);
            }
            ASR::stmt_t *while_loop_stmt = ASRUtils::STMT(ASR::make_WhileLoop_t(al, loc,
                loop.m_name, cond, body.p, body.size()));
            Vec<ASR::stmt_t*> result;
            result.reserve(al, 2);
            if( loop_init_stmt ) {
                result.push_back(al, loop_init_stmt);
            }
            result.push_back(al, while_loop_stmt);
            if (stmt_add_c_after_loop && use_loop_variable_after_loop) {
                result.push_back(al, stmt_add_c_after_loop);
            }

            return result;
        }

    namespace ReplacerUtils {
        void visit_ArrayConstant(ASR::ArrayConstant_t* x, Allocator& al,
            ASR::expr_t* arr_var, Vec<ASR::stmt_t*>* result_vec,
            ASR::expr_t* idx_var, SymbolTable* current_scope,
            bool perform_cast, ASR::cast_kindType cast_kind, ASR::ttype_t* casted_type) {
                #define increment_by_one(var, body) ASR::expr_t* inc_by_one = builder.ElementalAdd(var, \
                    make_ConstantWithType(make_IntegerConstant_t, 1, \
                        ASRUtils::expr_type(var), loc), loc); \
                    ASR::stmt_t* assign_inc = builder.Assignment(var, inc_by_one); \
                    body->push_back(al, assign_inc); \

            const Location& loc = arr_var->base.loc;
            ASRUtils::ASRBuilder builder(al, loc);
            for( size_t k = 0; k < x->n_args; k++ ) {
                ASR::expr_t* curr_init = x->m_args[k];
                if( ASR::is_a<ASR::ImpliedDoLoop_t>(*curr_init) ) {
                    ASR::ImpliedDoLoop_t* idoloop = ASR::down_cast<ASR::ImpliedDoLoop_t>(curr_init);
                    create_do_loop(al, idoloop, arr_var, result_vec, idx_var, perform_cast, cast_kind);
                } else if( ASR::is_a<ASR::ArrayConstant_t>(*curr_init) ) {
                    ASR::ArrayConstant_t* array_constant_t = ASR::down_cast<ASR::ArrayConstant_t>(curr_init);
                    visit_ArrayConstant(array_constant_t, al, arr_var, result_vec,
                                        idx_var, current_scope, perform_cast, cast_kind);
                } else if( ASR::is_a<ASR::Var_t>(*curr_init) ) {
                    ASR::ttype_t* element_type = ASRUtils::expr_type(curr_init);
                    if( ASRUtils::is_array(element_type) ) {
                        Vec<ASR::expr_t*> idx_vars;
                        Vec<ASR::stmt_t*> doloop_body;
                        int n_dims = ASRUtils::extract_n_dims_from_ttype(element_type);
                        create_do_loop(al, loc, n_dims, curr_init, idx_vars, doloop_body,
                            [=, &idx_vars, &doloop_body, &builder, &al, &perform_cast, &cast_kind, &casted_type] () {
                            ASR::expr_t* ref = PassUtils::create_array_ref(curr_init, idx_vars, al,
                                current_scope, perform_cast, cast_kind, casted_type);
                            ASR::expr_t* res = PassUtils::create_array_ref(arr_var, idx_var, al, current_scope);
                            ASR::stmt_t* assign = builder.Assignment(res, ref);
                            doloop_body.push_back(al, assign);
                            increment_by_one(idx_var, (&doloop_body))
                        }, current_scope, result_vec);
                    } else {
                        ASR::expr_t* res = PassUtils::create_array_ref(arr_var, idx_var, al, current_scope);
                        if( perform_cast ) {
                            curr_init = ASRUtils::EXPR(ASR::make_Cast_t(
                                al, curr_init->base.loc, curr_init, cast_kind, casted_type, nullptr));
                        }
                        ASR::stmt_t* assign = builder.Assignment(res, curr_init);
                        result_vec->push_back(al, assign);
                        increment_by_one(idx_var, result_vec)
                    }
                } else if( ASR::is_a<ASR::ArraySection_t>(*curr_init) ) {
                    ASR::ArraySection_t* array_section = ASR::down_cast<ASR::ArraySection_t>(curr_init);
                    Vec<ASR::expr_t*> idx_vars;
                    Vec<ASR::stmt_t*> doloop_body;
                    create_do_loop(al, loc, array_section, idx_vars, doloop_body,
                        [=, &idx_vars, &doloop_body, &builder, &al] () {
                        ASR::expr_t* ref = PassUtils::create_array_ref(array_section, idx_vars,
                            al, current_scope, perform_cast, cast_kind, casted_type);
                        ASR::expr_t* res = PassUtils::create_array_ref(arr_var, idx_var, al, current_scope);
                        ASR::stmt_t* assign = builder.Assignment(res, ref);
                        doloop_body.push_back(al, assign);
                        increment_by_one(idx_var, (&doloop_body))
                    }, current_scope, result_vec);
                } else {
                    ASR::expr_t* res = PassUtils::create_array_ref(arr_var, idx_var,
                        al, current_scope);
                    if( perform_cast ) {
                        curr_init = ASRUtils::EXPR(ASR::make_Cast_t(
                            al, curr_init->base.loc, curr_init, cast_kind, casted_type, nullptr));
                    }
                    ASR::stmt_t* assign = builder.Assignment(res, curr_init);
                    result_vec->push_back(al, assign);
                    increment_by_one(idx_var, result_vec)
                }
            }
        }
    }

    } // namespace PassUtils

} // namespace LCompilers
