#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/pass_utils.h>

namespace LFortran {

    namespace PassUtils {

        void get_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims) {
            switch( x_type->type ) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* x_type_ref = ASR::down_cast<ASR::Integer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::IntegerPointer: {
                    ASR::IntegerPointer_t* x_type_ref = ASR::down_cast<ASR::IntegerPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* x_type_ref = ASR::down_cast<ASR::Real_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::RealPointer: {
                    ASR::RealPointer_t* x_type_ref = ASR::down_cast<ASR::RealPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* x_type_ref = ASR::down_cast<ASR::Complex_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::ComplexPointer: {
                    ASR::ComplexPointer_t* x_type_ref = ASR::down_cast<ASR::ComplexPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* x_type_ref = ASR::down_cast<ASR::Derived_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::DerivedPointer: {
                    ASR::DerivedPointer_t* x_type_ref = ASR::down_cast<ASR::DerivedPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* x_type_ref = ASR::down_cast<ASR::Logical_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::LogicalPointer: {
                    ASR::LogicalPointer_t* x_type_ref = ASR::down_cast<ASR::LogicalPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* x_type_ref = ASR::down_cast<ASR::Character_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    m_dims = x_type_ref->m_dims;
                    break;
                }
                case ASR::ttypeType::CharacterPointer: {
                    ASR::CharacterPointer_t* x_type_ref = ASR::down_cast<ASR::CharacterPointer_t>(x_type);
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
            switch( x_type->type ) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* x_type_ref = ASR::down_cast<ASR::Integer_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::IntegerPointer: {
                    ASR::IntegerPointer_t* x_type_ref = ASR::down_cast<ASR::IntegerPointer_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_IntegerPointer_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* x_type_ref = ASR::down_cast<ASR::Real_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::RealPointer: {
                    ASR::RealPointer_t* x_type_ref = ASR::down_cast<ASR::RealPointer_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_RealPointer_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* x_type_ref = ASR::down_cast<ASR::Complex_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Complex_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::ComplexPointer: {
                    ASR::ComplexPointer_t* x_type_ref = ASR::down_cast<ASR::ComplexPointer_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_ComplexPointer_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* x_type_ref = ASR::down_cast<ASR::Derived_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Derived_t(*al, x_type->base.loc, x_type_ref->m_derived_type,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::DerivedPointer: {
                    ASR::DerivedPointer_t* x_type_ref = ASR::down_cast<ASR::DerivedPointer_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_DerivedPointer_t(*al, x_type->base.loc, x_type_ref->m_derived_type,
                                                                    m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* x_type_ref = ASR::down_cast<ASR::Logical_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::LogicalPointer: {
                    ASR::LogicalPointer_t* x_type_ref = ASR::down_cast<ASR::LogicalPointer_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_LogicalPointer_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* x_type_ref = ASR::down_cast<ASR::Character_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(*al, x_type->base.loc, x_type_ref->m_kind,
                                                            m_dims, n_dims));
                    } else {
                        x_type_ref->n_dims = n_dims;
                        x_type_ref->m_dims = m_dims;
                    }
                    break;
                }
                case ASR::ttypeType::CharacterPointer: {
                    ASR::CharacterPointer_t* x_type_ref = ASR::down_cast<ASR::CharacterPointer_t>(x_type);
                    if( create_new ) {
                        new_type = LFortran::ASRUtils::TYPE(ASR::make_CharacterPointer_t(*al, x_type->base.loc, x_type_ref->m_kind,
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
                if( current_scope->scope.find(std::string(idx_var_name)) == current_scope->scope.end() ) {
                    ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, current_scope, idx_var_name, 
                                                            ASR::intentType::Local, nullptr, ASR::storage_typeType::Default, 
                                                            int32_type, ASR::abiType::Source, ASR::accessType::Public, 
                                                            ASR::presenceType::Required);
                    current_scope->scope[std::string(idx_var_name)] = ASR::down_cast<ASR::symbol_t>(idx_sym);
                    idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
                } else {
                    ASR::symbol_t* idx_sym = current_scope->scope[std::string(idx_var_name)];
                    idx_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, loc, idx_sym));
                    
                }
                idx_vars.push_back(al, idx_var);
            }
        }

        ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim, std::string bound,
                                Allocator& al, ASR::TranslationUnit_t& unit, 
                                SymbolTable*& current_scope) {
            ASR::symbol_t *v;
            std::string remote_sym = bound;
            std::string module_name = "lfortran_intrinsic_array";
            SymbolTable* current_scope_copy = current_scope;
            current_scope = unit.m_global_scope;
            ASR::Module_t *m = LFortran::ASRUtils::load_module(al, current_scope,
                                            module_name, arr_expr->base.loc, true);

            ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
            ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(al, mfn->base.base.loc, current_scope,
                                                        mfn->m_name, (ASR::symbol_t*)mfn,
                                                        m->m_name, mfn->m_name, ASR::accessType::Private);
            std::string sym = mfn->m_name;
            if( current_scope->scope.find(sym) != current_scope->scope.end() ) {
                v = current_scope->scope[sym];
            } else {
                current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                v = ASR::down_cast<ASR::symbol_t>(fn);
            }
            Vec<ASR::expr_t*> args;
            args.reserve(al, 2);
            args.push_back(al, arr_expr);
            ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, arr_expr->base.loc, dim, LFortran::ASRUtils::expr_type(mfn->m_args[1])));
            args.push_back(al, const_1);
            ASR::ttype_t *type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(
                                        LFortran::ASRUtils::symbol_get_past_external(v))->m_return_var)->m_type;
            current_scope = current_scope_copy;
            return LFortran::ASRUtils::EXPR(ASR::make_FunctionCall_t(al, arr_expr->base.loc, v, nullptr,
                                                args.p, args.size(), nullptr, 0, type, nullptr));
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
                return LFortran::ASRUtils::EXPR(ASR::make_ImplicitCast_t(al, x->base.loc, x, (ASR::cast_kindType)cast_kind, int64type, nullptr));
            } else {
                throw SemanticError("Array indices can only be of type real or integer.",
                                    x->base.loc);
            }
            return nullptr;
        }

    }

}
