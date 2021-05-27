#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/pass_utils.h>

namespace LFortran {

    namespace PassUtils {

        void get_dims(ASR::expr_t* x, Vec<dimension_descriptor>& result, Allocator& al) {
            result.reserve(al, 0);
            if( x->type == ASR::exprType::Var ) {
                ASR::Var_t* x_var = ASR::down_cast<ASR::Var_t>(x);
                ASR::symbol_t* x_sym = (ASR::symbol_t*)symbol_get_past_external(x_var->m_v);
                if( x_sym->type != ASR::symbolType::Variable ) {
                    return ;
                }
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(x_sym);
                ASR::ttype_t* x_type = expr_type(x);
                ASR::dimension_t* m_dims = nullptr;
                int n_dims = 0;
                switch( v->m_type->type ) {
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
                if( n_dims > 0 ) {
                    for( int i = 0; i < n_dims; i++ ) {
                        int lbound = -1, ubound = -1;
                        if( m_dims[i].m_start->type == ASR::exprType::ConstantInteger ) {
                            ASR::ConstantInteger_t* m_start = ASR::down_cast<ASR::ConstantInteger_t>(m_dims[i].m_start);
                            lbound = m_start->m_n;
                        }
                        if( m_dims[i].m_end->type == ASR::exprType::ConstantInteger ) {
                            ASR::ConstantInteger_t* m_end = ASR::down_cast<ASR::ConstantInteger_t>(m_dims[i].m_end);
                            ubound = m_end->m_n;
                        } 
                        if( lbound != -1 && ubound != -1 ) {
                            dimension_descriptor new_dim;
                            new_dim.lbound = lbound;
                            new_dim.ubound = ubound;
                            result.push_back(al, new_dim);
                        } else {
                            break;
                        }
                    }
                }
            }
        }   

        bool is_array(ASR::expr_t* x, Allocator& al) {
            Vec<dimension_descriptor> result;
            get_dims(x, result, al);
            return result.size() > 0;
        }

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars, Allocator& al) {
            ASR::Var_t* arr_var = ASR::down_cast<ASR::Var_t>(arr_expr);
            ASR::symbol_t* arr = arr_var->m_v;
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
            ASR::expr_t* array_ref = EXPR(ASR::make_ArrayRef_t(al, arr_expr->base.loc, arr, 
                                                                args.p, args.size(), 
                                                                expr_type(EXPR((ASR::asr_t*)arr_var))));
            return array_ref;
        }

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc, Allocator& al, ASR::TranslationUnit_t& unit) {
            idx_vars.reserve(al, n_dims);
            for( int i = 1; i <= n_dims; i++ ) {
                Str str_name;
                str_name.from_str(al, std::to_string(i) + std::string("_k"));
                const char* const_idx_var_name = str_name.c_str(al);
                char* idx_var_name = (char*)const_idx_var_name;
                ASR::expr_t* idx_var = nullptr;
                ASR::ttype_t* int32_type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
                if( unit.m_global_scope->scope.find(std::string(idx_var_name)) == unit.m_global_scope->scope.end() ) {
                    ASR::asr_t* idx_sym = ASR::make_Variable_t(al, loc, unit.m_global_scope, idx_var_name, 
                                                            ASR::intentType::Local, nullptr, ASR::storage_typeType::Default, 
                                                            int32_type, ASR::abiType::Source, ASR::accessType::Public);
                    unit.m_global_scope->scope[std::string(idx_var_name)] = ASR::down_cast<ASR::symbol_t>(idx_sym);
                    idx_var = EXPR(ASR::make_Var_t(al, loc, ASR::down_cast<ASR::symbol_t>(idx_sym)));
                } else {
                    ASR::symbol_t* idx_sym = unit.m_global_scope->scope[std::string(idx_var_name)];
                    idx_var = EXPR(ASR::make_Var_t(al, loc, idx_sym));
                }
                idx_vars.push_back(al, idx_var);
            }
        }

    }

}