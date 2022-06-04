#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>

namespace LFortran {

    namespace PassUtils {

        void get_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims) {
            ASR::ttype_t* t2 = ASRUtils::type_get_past_pointer(x_type);
            switch( t2->type ) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* x_type_ref = ASR::down_cast<ASR::Integer_t>(t2);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* x_type_ref = ASR::down_cast<ASR::Real_t>(t2);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* x_type_ref = ASR::down_cast<ASR::Complex_t>(t2);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* x_type_ref = ASR::down_cast<ASR::Derived_t>(t2);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* x_type_ref = ASR::down_cast<ASR::Logical_t>(t2);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* x_type_ref = ASR::down_cast<ASR::Character_t>(t2);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
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
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* x_type_ref = ASR::down_cast<ASR::Integer_t>(t2);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* x_type_ref = ASR::down_cast<ASR::Real_t>(t2);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* x_type_ref = ASR::down_cast<ASR::Complex_t>(t2);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Complex_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* x_type_ref = ASR::down_cast<ASR::Derived_t>(t2);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Derived_t(*al, x_type->base.loc, x_type_ref->m_derived_type,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* x_type_ref = ASR::down_cast<ASR::Logical_t>(t2);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* x_type_ref = ASR::down_cast<ASR::Character_t>(t2);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(*al, x_type->base.loc, x_type_ref->m_kind,
                            x_type_ref->m_len, nullptr,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                default:
                    break;
            }

            if (ASR::is_a<ASR::Pointer_t>(*x_type)) {
                new_type = ASRUtils::TYPE(ASR::make_Pointer_t(*al, x_type->base.loc, new_type));
            }

            return new_type;
        }

        int get_rank(ASR::expr_t* x) {
            int n_dims = 0;
            if( x->type == ASR::exprType::Var ) {
                const ASR::symbol_t* x_sym = LFortran::ASRUtils::symbol_get_past_external(
                                                ASR::down_cast<ASR::Var_t>(x)->m_v);
                if( x_sym->type == ASR::symbolType::Variable ) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(x_sym);
                    ASR::ttype_t* x_type = v->m_type;
                    ASR::dimension_t* m_dims;
                    get_dim_rank(x_type, m_dims, n_dims);
                }
            }
            return n_dims;
        }

        bool is_array(ASR::expr_t* x) {
            return get_rank(x) > 0;
        }

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars, Allocator& al) {
            ASR::Var_t* arr_var = ASR::down_cast<ASR::Var_t>(arr_expr);
            ASR::symbol_t* arr = arr_var->m_v;
            return create_array_ref(arr, idx_vars, al, arr_expr->base.loc, LFortran::ASRUtils::expr_type(LFortran::ASRUtils::EXPR((ASR::asr_t*)arr_var)));
        }

        ASR::expr_t* create_array_ref(ASR::symbol_t* arr, Vec<ASR::expr_t*>& idx_vars, Allocator& al,
                                      const Location& loc, ASR::ttype_t* _type) {
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
            ASR::expr_t* array_ref = LFortran::ASRUtils::EXPR(ASR::make_ArrayRef_t(al, loc, arr,
                                                                args.p, args.size(),
                                                                _type, nullptr));
            return array_ref;
        }

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc, Allocator& al,
                             SymbolTable*& current_scope, std::string suffix) {
            idx_vars.reserve(al, n_dims);
            for( int i = 1; i <= n_dims; i++ ) {
                Str str_name;
                str_name.from_str(al, std::to_string(i) + suffix);
                const char* const_idx_var_name = str_name.c_str(al);
                char* idx_var_name = (char*)const_idx_var_name;
                ASR::expr_t* idx_var = nullptr;
                ASR::ttype_t* int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
                if( current_scope->get_symbol(std::string(idx_var_name)) == nullptr ) {
                    ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, idx_var_name,
                                                            ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                                            int32_type, ASR::abiType::Source, ASR::accessType::Public,
                                                            ASR::presenceType::Required, false);
                    current_scope->add_symbol(std::string(idx_var_name), ASR::down_cast<ASR::symbol_t>(idx_sym));
                    idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
                } else {
                    ASR::symbol_t* idx_sym = current_scope->get_symbol(std::string(idx_var_name));
                    idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));

                }
                idx_vars.push_back(al, idx_var);
            }
        }

        ASR::symbol_t* import_generic_procedure(std::string func_name, std::string module_name,
                                       Allocator& al, ASR::TranslationUnit_t& unit,
                                       const std::string &rl_path,
                                       SymbolTable*& current_scope, Location& loc) {
            ASR::symbol_t *v;
            std::string remote_sym = func_name;
            SymbolTable* current_scope_copy = current_scope;
            current_scope = unit.m_global_scope;
            // We tell `load_module` not to run verify, since the ASR might
            // not be in valid state. We run verify at the end of this pass
            // anyway, so verify will be run no matter what.
            ASR::Module_t *m = LFortran::ASRUtils::load_module(al, current_scope,
                                            module_name, loc, true,
                                            rl_path, false,
                                            [&](const std::string &msg, const Location &) { throw LFortranException(msg); }
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
            current_scope->add_symbol(sym, ASR::down_cast<ASR::symbol_t>(fn));
            v = ASR::down_cast<ASR::symbol_t>(fn);
            current_scope = current_scope_copy;
            return v;
        }

        ASR::symbol_t* import_function(std::string func_name, std::string module_name,
                                       Allocator& al, ASR::TranslationUnit_t& unit,
                                       const std::string &rl_path,
                                       SymbolTable*& current_scope, Location& loc) {
            ASR::symbol_t *v;
            std::string remote_sym = func_name;
            SymbolTable* current_scope_copy = current_scope;
            current_scope = unit.m_global_scope;
            // We tell `load_module` not to run verify, since the ASR might
            // not be in valid state. We run verify at the end of this pass
            // anyway, so verify will be run no matter what.
            ASR::Module_t *m = LFortran::ASRUtils::load_module(al, current_scope,
                                            module_name, loc, true,
                                            rl_path, false,
                                            [&](const std::string &msg, const Location &) { throw LFortranException(msg); });

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
            SymbolTable* current_scope2 = unit.m_global_scope;

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


        ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim, std::string bound,
                                Allocator& al) {
            ASR::ttype_t* int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4, nullptr, 0));
            ASR::expr_t* dim_expr = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_expr->base.loc, dim, int32_type));
            ASR::arrayboundType bound_type = ASR::arrayboundType::LBound;
            if( bound == "ubound" ) {
                bound_type = ASR::arrayboundType::UBound;
            }
            return LFortran::ASRUtils::EXPR(ASR::make_ArrayBound_t(al, arr_expr->base.loc, arr_expr, dim_expr,
                        int32_type, bound_type, nullptr));
        }


        ASR::stmt_t* get_flipsign(ASR::expr_t* arg0, ASR::expr_t* arg1,
                              Allocator& al, ASR::TranslationUnit_t& unit,
                              const std::string& rl_path,
                              SymbolTable*& current_scope,
                              const std::function<void (const std::string &, const Location &)> err) {
            ASR::symbol_t *v = import_generic_procedure("flipsign", "lfortran_intrinsic_optimization",
                                                        al, unit, rl_path, current_scope, arg0->base.loc);
            Vec<ASR::call_arg_t> args;
            args.reserve(al, 2);
            ASR::call_arg_t arg0_, arg1_;
            arg0_.loc = arg0->base.loc, arg0_.m_value = arg0;
            args.push_back(al, arg0_);
            arg1_.loc = arg1->base.loc, arg1_.m_value = arg1;
            args.push_back(al, arg1_);
            return ASRUtils::STMT(
                    ASRUtils::symbol_resolve_external_generic_procedure_without_eval(
                        arg0->base.loc, v, args, current_scope, al,
                        err));
        }

        ASR::expr_t* to_int32(ASR::expr_t* x, ASR::ttype_t* int64type, Allocator& al) {
            int cast_kind = -1;
            switch( LFortran::ASRUtils::expr_type(x)->type ) {
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
                return LFortran::ASRUtils::EXPR(ASR::make_Cast_t(al, x->base.loc, x, (ASR::cast_kindType)cast_kind, int64type, nullptr));
            } else {
                throw LFortranException("Array indices can only be of type real or integer.");
            }
            return nullptr;
        }

        bool is_slice_present(const ASR::ArrayRef_t& x) {
            bool slice_present = false;
            for( size_t i = 0; i < x.n_args; i++ ) {
                if( x.m_args[i].m_step != nullptr ) {
                    slice_present = true;
                    break;
                }
            }
            return slice_present;
        }

        bool is_slice_present(const ASR::expr_t* x) {
            if( x == nullptr || x->type != ASR::exprType::ArrayRef ) {
                return false;
            }
            ASR::ArrayRef_t* array_ref = ASR::down_cast<ASR::ArrayRef_t>(x);
            return is_slice_present(*array_ref);
        }

        ASR::expr_t* create_auxiliary_variable_for_expr(ASR::expr_t* expr, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::stmt_t*& assign_stmt) {
            ASR::asr_t* expr_sym = ASR::make_Variable_t(al, expr->base.loc, current_scope, s2c(al, name),
                                                    ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                                    ASRUtils::expr_type(expr), ASR::abiType::Source, ASR::accessType::Public,
                                                    ASR::presenceType::Required, false);
            if( current_scope->get_symbol(name) == nullptr ) {
                current_scope->add_symbol(name, ASR::down_cast<ASR::symbol_t>(expr_sym));
            } else {
                throw LFortranException("Symbol with " + name + " is already present in " + std::to_string(current_scope->counter));
            }
            ASR::expr_t* var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, expr->base.loc, ASR::down_cast<ASR::symbol_t>(expr_sym)));
            assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, var->base.loc, var, expr, nullptr));
            return var;
        }

        ASR::expr_t* create_auxiliary_variable(Location& loc, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::ttype_t* var_type) {
            ASR::asr_t* expr_sym = ASR::make_Variable_t(al, loc, current_scope, s2c(al, name),
                                                    ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                                                    var_type, ASR::abiType::Source, ASR::accessType::Public,
                                                    ASR::presenceType::Required, false);
            if( current_scope->get_symbol(name) == nullptr ) {
                current_scope->add_symbol(name, ASR::down_cast<ASR::symbol_t>(expr_sym));
            } else {
                throw LFortranException("Symbol with " + name + " is already present in " + std::to_string(current_scope->counter));
            }
            ASR::expr_t* var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(expr_sym)));
            return var;
        }

        ASR::expr_t* get_fma(ASR::expr_t* arg0, ASR::expr_t* arg1, ASR::expr_t* arg2,
            Allocator& al, ASR::TranslationUnit_t& unit, std::string& rl_path,
            SymbolTable*& current_scope, Location& loc,
            const std::function<void (const std::string &, const Location &)> err) {
            ASR::symbol_t *v = import_generic_procedure("fma", "lfortran_intrinsic_optimization",
                                                        al, unit, rl_path, current_scope, arg0->base.loc);
            Vec<ASR::call_arg_t> args;
            args.reserve(al, 3);
            ASR::call_arg_t arg0_, arg1_, arg2_;
            arg0_.loc = arg0->base.loc, arg0_.m_value = arg0;
            args.push_back(al, arg0_);
            arg1_.loc = arg1->base.loc, arg1_.m_value = arg1;
            args.push_back(al, arg1_);
            arg2_.loc = arg2->base.loc, arg2_.m_value = arg2;
            args.push_back(al, arg2_);
            return ASRUtils::EXPR(
                        ASRUtils::symbol_resolve_external_generic_procedure_without_eval(
                        loc, v, args, current_scope, al, err));
        }

        ASR::expr_t* get_sign_from_value(ASR::expr_t* arg0, ASR::expr_t* arg1,
            Allocator& al, ASR::TranslationUnit_t& unit, std::string& rl_path,
            SymbolTable*& current_scope, Location& loc,
            const std::function<void (const std::string &, const Location &)> err) {
            ASR::symbol_t *v = import_generic_procedure("sign_from_value", "lfortran_intrinsic_optimization",
                                                        al, unit, rl_path, current_scope, arg0->base.loc);
            Vec<ASR::call_arg_t> args;
            args.reserve(al, 2);
            ASR::call_arg_t arg0_, arg1_;
            arg0_.loc = arg0->base.loc, arg0_.m_value = arg0;
            args.push_back(al, arg0_);
            arg1_.loc = arg1->base.loc, arg1_.m_value = arg1;
            args.push_back(al, arg1_);
            return ASRUtils::EXPR(
                        ASRUtils::symbol_resolve_external_generic_procedure_without_eval(
                        loc, v, args, current_scope, al, err));
        }

        Vec<ASR::stmt_t*> replace_doloop(Allocator &al, const ASR::DoLoop_t &loop) {
            Location loc = loop.base.base.loc;
            ASR::expr_t *a=loop.m_head.m_start;
            ASR::expr_t *b=loop.m_head.m_end;
            ASR::expr_t *c=loop.m_head.m_increment;
            ASR::expr_t *cond = nullptr;
            ASR::stmt_t *inc_stmt = nullptr;
            ASR::stmt_t *stmt1 = nullptr;
            if( !a && !b && !c ) {
                ASR::ttype_t *cond_type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0));
                cond = LFortran::ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, true, cond_type));
            } else {
                LFORTRAN_ASSERT(a);
                LFORTRAN_ASSERT(b);
                if (!c) {
                    ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
                    c = LFortran::ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, type));
                }
                LFORTRAN_ASSERT(c);
                int increment;
                if (c->type == ASR::exprType::IntegerConstant) {
                    increment = ASR::down_cast<ASR::IntegerConstant_t>(c)->m_n;
                } else if (c->type == ASR::exprType::IntegerUnaryMinus) {
                    ASR::IntegerUnaryMinus_t *u = ASR::down_cast<ASR::IntegerUnaryMinus_t>(c);
                    increment = - ASR::down_cast<ASR::IntegerConstant_t>(u->m_arg)->m_n;
                } else if (c->type == ASR::exprType::UnaryOp) {
                    // TODO: remove once we remove UnaryOp
                    ASR::UnaryOp_t *u = ASR::down_cast<ASR::UnaryOp_t>(c);
                    LFORTRAN_ASSERT(u->m_op == ASR::unaryopType::USub);
                    LFORTRAN_ASSERT(u->m_operand->type == ASR::exprType::IntegerConstant);
                    increment = - ASR::down_cast<ASR::IntegerConstant_t>(u->m_operand)->m_n;
                } else {
                    throw LFortranException("Do loop increment type not supported");
                }
                ASR::cmpopType cmp_op;
                if (increment > 0) {
                    cmp_op = ASR::cmpopType::LtE;
                } else {
                    cmp_op = ASR::cmpopType::GtE;
                }
                ASR::expr_t *target = loop.m_head.m_v;
                ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
                stmt1 = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
                    LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, a, ASR::binopType::Sub, c, type, nullptr, nullptr)),
                    nullptr));

                cond = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc,
                    LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, target, ASR::binopType::Add, c, type, nullptr, nullptr)),
                    cmp_op, b, type, nullptr, nullptr));

                inc_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
                            LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, target, ASR::binopType::Add, c, type, nullptr, nullptr)),
                        nullptr));
            }
            Vec<ASR::stmt_t*> body;
            body.reserve(al, loop.n_body + (inc_stmt != nullptr));
            if( inc_stmt ) {
                body.push_back(al, inc_stmt);
            }
            for (size_t i=0; i<loop.n_body; i++) {
                body.push_back(al, loop.m_body[i]);
            }
            ASR::stmt_t *stmt2 = LFortran::ASRUtils::STMT(ASR::make_WhileLoop_t(al, loc, cond,
                body.p, body.size()));
            Vec<ASR::stmt_t*> result;
            result.reserve(al, 2);
            if( stmt1 ) {
                result.push_back(al, stmt1);
            }
            result.push_back(al, stmt2);

            return result;
        }

    }

}
