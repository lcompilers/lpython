#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <cmath>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/exception.h>
#include <lfortran/semantics/asr_implicit_cast_rules.h>
#include <lfortran/semantics/ast_common_visitor.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>

namespace LFortran {

class BodyVisitor : public AST::BaseVisitor<BodyVisitor> {
private:
    std::map<std::string, std::string> intrinsic_procedures = {
        {"kind", "lfortran_intrinsic_kind"},
        {"selected_int_kind", "lfortran_intrinsic_kind"},
        {"selected_real_kind", "lfortran_intrinsic_kind"},
        {"size", "lfortran_intrinsic_array"},
        {"lbound", "lfortran_intrinsic_array"},
        {"ubound", "lfortran_intrinsic_array"},
        {"min", "lfortran_intrinsic_array"},
        {"max", "lfortran_intrinsic_array"},
        {"allocated", "lfortran_intrinsic_array"},
        {"minval", "lfortran_intrinsic_array"},
        {"maxval", "lfortran_intrinsic_array"},
        {"real", "lfortran_intrinsic_array"},
        {"floor", "lfortran_intrinsic_array"},
        {"sum", "lfortran_intrinsic_array"},
        {"abs", "lfortran_intrinsic_math2"},
        {"sin", "lfortran_intrinsic_trig"},
        {"sqrt", "lfortran_intrinsic_math2"},
        {"int", "lfortran_intrinsic_array"},
        {"real", "lfortran_intrinsic_array"},
        {"tiny", "lfortran_intrinsic_array"}
};

public:
    Allocator &al;
    ASR::asr_t *asr, *tmp;
    SymbolTable *current_scope;
    Vec<ASR::stmt_t*> *current_body;
    ASR::Module_t *current_module = nullptr;

    BodyVisitor(Allocator &al, ASR::asr_t *unit) : al{al}, asr{unit} {}

    void visit_Declaration(const AST::Declaration_t & /* x */){
        // Already visited this AST node in the SymbolTableVisitor
    };

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        ASR::TranslationUnit_t *unit = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
        current_scope = unit->m_global_scope;
        Vec<ASR::asr_t*> items;
        items.reserve(al, x.n_items);
        for (size_t i=0; i<x.n_items; i++) {
            tmp = nullptr;
            visit_ast(*x.m_items[i]);
            if (tmp) {
                items.push_back(al, tmp);
            }
        }
        unit->m_items = items.p;
        unit->n_items = items.size();
    }

    void visit_Open(const AST::Open_t& x) {
        ASR::expr_t *a_newunit = nullptr, *a_filename = nullptr, *a_status = nullptr;
        if( x.n_args > 1 ) {
            throw SemanticError("Number of arguments cannot be more than 1 in Open statement.",
                                x.base.base.loc);
        }
        if( x.n_args == 1 ) {
            this->visit_expr(*x.m_args[0]);
            a_newunit = LFortran::ASRUtils::EXPR(tmp);
        }
        for( std::uint32_t i = 0; i < x.n_kwargs; i++ ) {
            AST::keyword_t kwarg = x.m_kwargs[i];
            std::string m_arg_str(kwarg.m_arg);
            if( m_arg_str == std::string("newunit") ||
                m_arg_str == std::string("unit") ) {
                if( a_newunit != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `unit` found, `unit` has already been specified via argument or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_newunit = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_newunit_type = LFortran::ASRUtils::expr_type(a_newunit);
                if( ( m_arg_str == std::string("newunit") &&
                      a_newunit->type != ASR::exprType::Var ) ||
                    ( a_newunit_type->type != ASR::ttypeType::Integer &&
                    a_newunit_type->type != ASR::ttypeType::IntegerPointer ) ) {
                        throw SemanticError("`newunit`/`unit` must be a variable of type, Integer or IntegerPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("file") ) {
                if( a_filename != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `file` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_filename = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_filename_type = LFortran::ASRUtils::expr_type(a_filename);
                if( a_filename_type->type != ASR::ttypeType::Character &&
                    a_filename_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`file` must be of type, Character or CharacterPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("status") ) {
                if( a_status != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `status` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_status = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_status_type = LFortran::ASRUtils::expr_type(a_status);
                if( a_status_type->type != ASR::ttypeType::Character &&
                    a_status_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`status` must be of type, Character or CharacterPointer", x.base.base.loc);
                }
            }
        }
        if( a_newunit == nullptr ) {
            throw SemanticError("`newunit` or `unit` must be specified either in argument or keyword arguments.",
                                x.base.base.loc);
        }
        tmp = ASR::make_Open_t(al, x.base.base.loc, x.m_label,
                               a_newunit, a_filename, a_status);
    }

    void visit_Close(const AST::Close_t& x) {
        ASR::expr_t *a_unit = nullptr, *a_iostat = nullptr, *a_iomsg = nullptr;
        ASR::expr_t *a_err = nullptr, *a_status = nullptr;
        if( x.n_args > 1 ) {
            throw SemanticError("Number of arguments cannot be more than 1 in Close statement.",
                        x.base.base.loc);
        }
        if( x.n_args == 1 ) {
            this->visit_expr(*x.m_args[0]);
            a_unit = LFortran::ASRUtils::EXPR(tmp);
        }
        for( std::uint32_t i = 0; i < x.n_kwargs; i++ ) {
            AST::keyword_t kwarg = x.m_kwargs[i];
            std::string m_arg_str(kwarg.m_arg);
            if( m_arg_str == std::string("unit") ) {
                if( a_unit != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `unit` found, `unit` has already been specified via argument or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_unit = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_newunit_type = LFortran::ASRUtils::expr_type(a_unit);
                if( a_newunit_type->type != ASR::ttypeType::Integer &&
                    a_newunit_type->type != ASR::ttypeType::IntegerPointer ) {
                        throw SemanticError("`unit` must be of type, Integer or IntegerPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("iostat") ) {
                if( a_iostat != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iostat` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iostat = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_iostat_type = LFortran::ASRUtils::expr_type(a_iostat);
                if( a_iostat->type != ASR::exprType::Var ||
                    ( a_iostat_type->type != ASR::ttypeType::Integer &&
                      a_iostat_type->type != ASR::ttypeType::IntegerPointer ) ) {
                        throw SemanticError("`iostat` must be a variable of type, Integer or IntegerPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("iomsg") ) {
                if( a_iomsg != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iomsg` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iomsg = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_iomsg_type = LFortran::ASRUtils::expr_type(a_iomsg);
                if( a_iomsg->type != ASR::exprType::Var ||
                   ( a_iomsg_type->type != ASR::ttypeType::Character &&
                    a_iomsg_type->type != ASR::ttypeType::CharacterPointer ) ) {
                        throw SemanticError("`iomsg` must be of type, Character or CharacterPointer", x.base.base.loc);
                    }
            } else if( m_arg_str == std::string("status") ) {
                if( a_status != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `status` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_status = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_status_type = LFortran::ASRUtils::expr_type(a_status);
                if( a_status_type->type != ASR::ttypeType::Character &&
                    a_status_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`status` must be of type, Character or CharacterPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("err") ) {
                if( a_err != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `err` found, `err` has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                if( kwarg.m_value->type != AST::exprType::Num ) {
                    throw SemanticError("`err` must be a literal integer", x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_err = LFortran::ASRUtils::EXPR(tmp);
            }
        }
        if( a_unit == nullptr ) {
            throw SemanticError("`newunit` or `unit` must be specified either in argument or keyword arguments.",
                                x.base.base.loc);
        }
        tmp = ASR::make_Close_t(al, x.base.base.loc, x.m_label, a_unit, a_iostat, a_iomsg, a_err, a_status);
    }

    void create_read_write_ASR_node(const AST::stmt_t& read_write_stmt, AST::stmtType _type) {
        int64_t m_label = -1;
        AST::argstar_t* m_args = nullptr; size_t n_args = 0;
        AST::kw_argstar_t* m_kwargs = nullptr; size_t n_kwargs = 0;
        AST::expr_t** m_values = nullptr; size_t n_values = 0;
        const Location& loc = read_write_stmt.base.loc;
        if( _type == AST::stmtType::Write ) {
            AST::Write_t* w = (AST::Write_t*)(&read_write_stmt);
            m_label = w->m_label;
            m_args = w->m_args; n_args = w->n_args;
            m_kwargs = w->m_kwargs; n_kwargs = w->n_kwargs;
            m_values = w->m_values; n_values = w->n_values;
        } else if( _type == AST::stmtType::Read ) {
            AST::Read_t* r = (AST::Read_t*)(&read_write_stmt);
            m_label = r->m_label;
            m_args = r->m_args; n_args = r->n_args;
            m_kwargs = r->m_kwargs; n_kwargs = r->n_kwargs;
            m_values = r->m_values; n_values = r->n_values;
        }

        ASR::expr_t *a_unit, *a_fmt, *a_iomsg, *a_iostat, *a_id;
        a_unit = a_fmt = a_iomsg = a_iostat = a_id = nullptr;
        Vec<ASR::expr_t*> a_values_vec;
        a_values_vec.reserve(al, n_values);

        if( n_args > 2 ) {
            throw SemanticError("Number of arguments cannot be more than 2 in Read/Write statement.",
                                loc);
        }
        std::vector<ASR::expr_t**> args = {&a_unit, &a_fmt};
        for( std::uint32_t i = 0; i < n_args; i++ ) {
            if( m_args[i].m_value != nullptr ) {
                this->visit_expr(*m_args[i].m_value);
                *args[i] = LFortran::ASRUtils::EXPR(tmp);
            }
        }
        for( std::uint32_t i = 0; i < n_kwargs; i++ ) {
            AST::kw_argstar_t kwarg = m_kwargs[i];
            std::string m_arg_str(kwarg.m_arg);
            if( m_arg_str == std::string("unit") ) {
                if( a_unit != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `unit` found, `unit` has already been specified via argument or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_unit = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_unit_type = LFortran::ASRUtils::expr_type(a_unit);
                if( a_unit_type->type != ASR::ttypeType::Integer &&
                    a_unit_type->type != ASR::ttypeType::IntegerPointer ) {
                        throw SemanticError("`unit` must be of type, Integer or IntegerPointer", loc);
                }
            } else if( m_arg_str == std::string("iostat") ) {
                if( a_iostat != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iostat` found, unit has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iostat = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_iostat_type = LFortran::ASRUtils::expr_type(a_iostat);
                if( a_iostat->type != ASR::exprType::Var ||
                    ( a_iostat_type->type != ASR::ttypeType::Integer &&
                      a_iostat_type->type != ASR::ttypeType::IntegerPointer ) ) {
                        throw SemanticError("`iostat` must be of type, Integer or IntegerPointer", loc);
                }
            } else if( m_arg_str == std::string("iomsg") ) {
                if( a_iomsg != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iomsg` found, unit has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iomsg = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_iomsg_type = LFortran::ASRUtils::expr_type(a_iomsg);
                if( a_iomsg->type != ASR::exprType::Var ||
                   ( a_iomsg_type->type != ASR::ttypeType::Character &&
                     a_iomsg_type->type != ASR::ttypeType::CharacterPointer ) ) {
                        throw SemanticError("`iomsg` must be of type, Character or CharacterPointer", loc);
                    }
            } else if( m_arg_str == std::string("id") ) {
                if( a_id != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `id` found, unit has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_id = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_status_type = LFortran::ASRUtils::expr_type(a_id);
                if( a_status_type->type != ASR::ttypeType::Character &&
                    a_status_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`status` must be of type, Character or CharacterPointer", loc);
                }
            }
        }
        if( a_unit == nullptr && n_args < 1 ) {
            throw SemanticError("`unit` must be specified either in arguments or keyword arguments.",
                                loc);
        }
        if( a_fmt == nullptr && n_args < 2 ) {
            throw SemanticError("`fmt` must be specified either in arguments or keyword arguments.",
                                loc);
        }

        for( std::uint32_t i = 0; i < n_values; i++ ) {
            this->visit_expr(*m_values[i]);
            a_values_vec.push_back(al, LFortran::ASRUtils::EXPR(tmp));
        }
        if( _type == AST::stmtType::Write ) {
            tmp = ASR::make_Write_t(al, loc, m_label, a_unit, a_fmt,
                                    a_iomsg, a_iostat, a_id, a_values_vec.p, n_values);
        } else if( _type == AST::stmtType::Read ) {
            tmp = ASR::make_Read_t(al, loc, m_label, a_unit, a_fmt,
                                   a_iomsg, a_iostat, a_id, a_values_vec.p, n_values);
        }
    }

    void visit_Write(const AST::Write_t& x) {
        create_read_write_ASR_node(x.base, x.class_type);
    }

    void visit_Read(const AST::Read_t& x) {
        create_read_write_ASR_node(x.base, x.class_type);
    }

    void visit_Associate(const AST::Associate_t& x) {
        this->visit_expr(*(x.m_target));
        ASR::expr_t* target = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*(x.m_value));
        ASR::expr_t* value = LFortran::ASRUtils::EXPR(tmp);
        ASR::ttype_t* target_type = LFortran::ASRUtils::expr_type(target);
        ASR::ttype_t* value_type = LFortran::ASRUtils::expr_type(value);
        bool is_target_pointer = ASRUtils::is_pointer(target_type);
        bool is_value_pointer = ASRUtils::is_pointer(value_type);
        if( !(is_target_pointer && !is_value_pointer) ) {
            throw SemanticError("Only a pointer variable can be associated with a non-pointer variable.", x.base.base.loc);
        }
        if( ASRUtils::is_same_type_pointer(target_type, value_type) ) {
            tmp = ASR::make_Associate_t(al, x.base.base.loc, target, value);
        }
    }

    void visit_AssociateBlock(const AST::AssociateBlock_t& x) {
        SymbolTable* new_scope = al.make_new<SymbolTable>(current_scope);
        for( size_t i = 0; i < x.n_syms; i++ ) {
            this->visit_expr(*x.m_syms[i].m_initializer);
            ASR::asr_t *v = ASR::make_Variable_t(al, x.base.base.loc, new_scope,
                                                 x.m_syms[i].m_name, ASR::intentType::AssociateBlock,
                                                 LFortran::ASRUtils::EXPR(tmp), nullptr,
                                                 ASR::storage_typeType::Default,
                                                 nullptr, ASR::abiType::Source,
                                                 ASR::accessType::Private,
                                                 ASR::presenceType::Required);
            new_scope->scope[x.m_syms[i].m_name] = ASR::down_cast<ASR::symbol_t>(v);
        }
        SymbolTable* current_scope_copy = current_scope;
        current_scope = new_scope;
        for( size_t i = 0; i < x.n_body; i++ ) {
            this->visit_stmt(*x.m_body[i]);
            if( tmp != nullptr ) {
                current_body->push_back(al, LFortran::ASRUtils::STMT(tmp));
            }
            // To avoid last statement to be entered twice once we exit this node
            tmp = nullptr;
        }
        current_scope->scope.clear();
        current_scope = current_scope_copy;
    }

    void visit_Allocate(const AST::Allocate_t& x) {
        Vec<ASR::alloc_arg_t> alloc_args_vec;
        alloc_args_vec.reserve(al, x.n_args);
        ASR::ttype_t *int32_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                                                            4, nullptr, 0));
        ASR::expr_t* const_1 = LFortran::ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, 1, int32_type));
        for( size_t i = 0; i < x.n_args; i++ ) {
            ASR::alloc_arg_t new_arg;
            new_arg.loc = x.base.base.loc;
            this->visit_expr(*(x.m_args[i].m_end));
            // Assume that tmp is an `ArrayRef`
            ASR::expr_t* tmp_stmt = LFortran::ASRUtils::EXPR(tmp);
            ASR::ArrayRef_t* array_ref = ASR::down_cast<ASR::ArrayRef_t>(tmp_stmt);
            new_arg.m_a = array_ref->m_v;
            Vec<ASR::dimension_t> dims_vec;
            dims_vec.reserve(al, array_ref->n_args);
            for( size_t j = 0; j < array_ref->n_args; j++ ) {
                ASR::dimension_t new_dim;
                new_dim.loc = array_ref->m_args[j].loc;
                ASR::expr_t* m_left = array_ref->m_args[j].m_left;
                if( m_left != nullptr ) {
                    new_dim.m_start = m_left;
                } else {
                    new_dim.m_start = const_1;
                }
                ASR::expr_t* m_right = array_ref->m_args[j].m_right;
                new_dim.m_end = m_right;
                dims_vec.push_back(al, new_dim);
            }
            new_arg.m_dims = dims_vec.p;
            new_arg.n_dims = dims_vec.size();
            alloc_args_vec.push_back(al, new_arg);
        }

        // Only one arg should be present
        if( x.n_keywords > 1 ||
          ( x.n_keywords == 1 && std::string(x.m_keywords[0].m_arg) != "stat") ) {
            throw SemanticError("`allocate` statement only "
                                "accepts one keyword argument,"
                                "`stat`", x.base.base.loc);
        }
        ASR::expr_t* stat = nullptr;
        if( x.n_keywords == 1 ) {
            this->visit_expr(*(x.m_keywords[0].m_value));
            stat = LFortran::ASRUtils::EXPR(tmp);
        }
        tmp = ASR::make_Allocate_t(al, x.base.base.loc,
                                    alloc_args_vec.p, alloc_args_vec.size(),
                                    stat);
    }

// If there are allocatable variables in the local scope it inserts an ImplicitDeallocate node
// with their list. The ImplicitDeallocate node will deallocate them if they are allocated,
// otherwise does nothing.
    ASR::stmt_t* create_implicit_deallocate(const Location& loc) {
        Vec<ASR::symbol_t*> del_syms;
        del_syms.reserve(al, 0);
        for( auto& item: current_scope->scope ) {
            if( item.second->type == ASR::symbolType::Variable ) {
                const ASR::symbol_t* sym = LFortran::ASRUtils::symbol_get_past_external(item.second);
                ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(sym);
                if( var->m_storage == ASR::storage_typeType::Allocatable &&
                    var->m_intent == ASR::intentType::Local ) {
                    del_syms.push_back(al, item.second);
                }
            }
        }
        if( del_syms.size() == 0 ) {
            return nullptr;
        }
        return LFortran::ASRUtils::STMT(ASR::make_ImplicitDeallocate_t(al, loc,
                    del_syms.p, del_syms.size()));
    }

    void visit_Deallocate(const AST::Deallocate_t& x) {
        Vec<ASR::symbol_t*> arg_vec;
        arg_vec.reserve(al, x.n_args);
        for( size_t i = 0; i < x.n_args; i++ ) {
            this->visit_expr(*(x.m_args[i].m_end));
            ASR::expr_t* tmp_expr = LFortran::ASRUtils::EXPR(tmp);
            if( tmp_expr->type != ASR::exprType::Var ) {
                throw SemanticError("Only an allocatable variable symbol "
                                    "can be deallocated.",
                                    tmp_expr->base.loc);
            } else {
                const ASR::Var_t* tmp_var = ASR::down_cast<ASR::Var_t>(tmp_expr);
                ASR::symbol_t* tmp_sym = tmp_var->m_v;
                if( LFortran::ASRUtils::symbol_get_past_external(tmp_sym)->type != ASR::symbolType::Variable ) {
                    throw SemanticError("Only an allocatable variable symbol "
                                        "can be deallocated.",
                                        tmp_expr->base.loc);
                } else {
                    ASR::Variable_t* tmp_v = ASR::down_cast<ASR::Variable_t>(tmp_sym);
                    if( tmp_v->m_storage != ASR::storage_typeType::Allocatable ) {
                        throw SemanticError("Only an allocatable variable symbol "
                                            "can be deallocated.",
                                            tmp_expr->base.loc);
                    }
                    arg_vec.push_back(al, tmp_sym);
                }
            }
        }
        tmp = ASR::make_ExplicitDeallocate_t(al, x.base.base.loc,
                                            arg_vec.p, arg_vec.size());
    }

    void visit_Return(const AST::Return_t& x) {
        // TODO
        tmp = ASR::make_Return_t(al, x.base.base.loc);
    }

    void visit_case_stmt(const AST::case_stmt_t& x) {
        switch(x.type) {
            case AST::case_stmtType::CaseStmt: {
                AST::CaseStmt_t* Case_Stmt = (AST::CaseStmt_t*)(&(x.base));
                if (Case_Stmt->n_test == 0) {
                    throw SemanticError("Case statement must have at least one condition",
                                        x.base.loc);
                }
                if (AST::is_a<AST::CaseCondExpr_t>(*(Case_Stmt->m_test[0]))) {
                    // For now we only support a list of expressions
                    Vec<ASR::expr_t*> a_test_vec;
                    a_test_vec.reserve(al, Case_Stmt->n_test);
                    for( std::uint32_t i = 0; i < Case_Stmt->n_test; i++ ) {
                        if (!AST::is_a<AST::CaseCondExpr_t>(*(Case_Stmt->m_test[i]))) {
                            throw SemanticError("Not implemented yet: range expression not in first position",
                                                x.base.loc);
                        }
                        AST::CaseCondExpr_t *condexpr
                            = AST::down_cast<AST::CaseCondExpr_t>(Case_Stmt->m_test[i]);
                        this->visit_expr(*condexpr->m_cond);
                        ASR::expr_t* m_test_i = LFortran::ASRUtils::EXPR(tmp);
                        if( LFortran::ASRUtils::expr_type(m_test_i)->type != ASR::ttypeType::Integer ) {
                            throw SemanticError(R"""(Expression in Case selector can only be an Integer)""",
                                                x.base.loc);
                        }
                        a_test_vec.push_back(al, LFortran::ASRUtils::EXPR(tmp));
                    }
                    Vec<ASR::stmt_t*> case_body_vec;
                    case_body_vec.reserve(al, Case_Stmt->n_body);
                    for( std::uint32_t i = 0; i < Case_Stmt->n_body; i++ ) {
                        this->visit_stmt(*(Case_Stmt->m_body[i]));
                        if (tmp != nullptr) {
                            case_body_vec.push_back(al, LFortran::ASRUtils::STMT(tmp));
                        }
                    }
                    tmp = ASR::make_CaseStmt_t(al, x.base.loc, a_test_vec.p, a_test_vec.size(),
                                        case_body_vec.p, case_body_vec.size());
                    break;
                } else {
                    // For now we only support exactly one range
                    if (Case_Stmt->n_test != 1) {
                        throw SemanticError("Not implemented: more than one range condition",
                                            x.base.loc);
                    }
                    AST::CaseCondRange_t *condrange
                        = AST::down_cast<AST::CaseCondRange_t>(Case_Stmt->m_test[0]);
                    ASR::expr_t *m_start = nullptr, *m_end = nullptr;
                    if( condrange->m_start != nullptr ) {
                        this->visit_expr(*(condrange->m_start));
                        m_start = LFortran::ASRUtils::EXPR(tmp);
                        if( LFortran::ASRUtils::expr_type(m_start)->type != ASR::ttypeType::Integer ) {
                            throw SemanticError(R"""(Expression in Case selector can only be an Integer)""",
                                                x.base.loc);
                        }
                    }
                    if( condrange->m_end != nullptr ) {
                        this->visit_expr(*(condrange->m_end));
                        m_end = LFortran::ASRUtils::EXPR(tmp);
                        if( LFortran::ASRUtils::expr_type(m_end)->type != ASR::ttypeType::Integer ) {
                            throw SemanticError(R"""(Expression in Case selector can only be an Integer)""",
                                                x.base.loc);
                        }
                    }
                    Vec<ASR::stmt_t*> case_body_vec;
                    case_body_vec.reserve(al, Case_Stmt->n_body);
                    for( std::uint32_t i = 0; i < Case_Stmt->n_body; i++ ) {
                        this->visit_stmt(*(Case_Stmt->m_body[i]));
                        if (tmp != nullptr) {
                            case_body_vec.push_back(al, LFortran::ASRUtils::STMT(tmp));
                        }
                    }
                    tmp = ASR::make_CaseStmt_Range_t(al, x.base.loc, m_start, m_end,
                                        case_body_vec.p, case_body_vec.size());
                    break;
                }
            }
            default: {
                throw SemanticError(R"""(Case statement can only support a valid expression
                                    that reduces to a constant or range defined by : separator)""",
                                    x.base.loc);
            }
        }
    }

    void visit_Select(const AST::Select_t& x) {
        this->visit_expr(*(x.m_test));
        ASR::expr_t* a_test = LFortran::ASRUtils::EXPR(tmp);
        if( LFortran::ASRUtils::expr_type(a_test)->type != ASR::ttypeType::Integer ) {
            throw SemanticError(R"""(Expression in Case selector can only be an Integer)""", x.base.base.loc);
        }
        Vec<ASR::case_stmt_t*> a_body_vec;
        a_body_vec.reserve(al, x.n_body);
        Vec<ASR::stmt_t*> def_body;
        def_body.reserve(al, 1);
        for( std::uint32_t i = 0; i < x.n_body; i++ ) {
            AST::case_stmt_t *body = x.m_body[i];
            if (AST::is_a<AST::CaseStmt_Default_t>(*body)) {
                if (def_body.size() != 0) {
                    throw SemanticError("Default case present more than once",
                        x.base.base.loc);
                }
                AST::CaseStmt_Default_t *d =
                        AST::down_cast<AST::CaseStmt_Default_t>(body);
                for( std::uint32_t j = 0; j < d->n_body; j++ ) {
                    this->visit_stmt(*(d->m_body[j]));
                    if (tmp != nullptr) {
                        def_body.push_back(al,
                            ASR::down_cast<ASR::stmt_t>(tmp));
                    }
                }
            } else {
                this->visit_case_stmt(*body);
                a_body_vec.push_back(al, ASR::down_cast<ASR::case_stmt_t>(tmp));
            }
        }
        tmp = ASR::make_Select_t(al, x.base.base.loc, a_test, a_body_vec.p,
                           a_body_vec.size(), def_body.p, def_body.size());
    }

    void visit_Module(const AST::Module_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Module_t *v = ASR::down_cast<ASR::Module_t>(t);
        current_scope = v->m_symtab;
        current_module = v;

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        current_module = nullptr;
        tmp = nullptr;
    }

    void visit_Program(const AST::Program_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Program_t *v = ASR::down_cast<ASR::Program_t>(t);
        current_scope = v->m_symtab;

        Vec<ASR::stmt_t*> body;
        current_body = &body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                ASR::stmt_t* tmp_stmt = LFortran::ASRUtils::STMT(tmp);
                if( tmp_stmt->type == ASR::stmtType::SubroutineCall ) {
                    ASR::stmt_t* impl_decl = create_implicit_deallocate_subrout_call(tmp_stmt);
                    if( impl_decl != nullptr ) {
                        body.push_back(al, impl_decl);
                    }
                }
                body.push_back(al, tmp_stmt);
            }
        }
        ASR::stmt_t* impl_del = create_implicit_deallocate(x.base.base.loc);
        if( impl_del != nullptr ) {
            body.push_back(al, impl_del);
        }
        v->m_body = body.p;
        v->n_body = body.size();

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        tmp = nullptr;
    }

    ASR::stmt_t* create_implicit_deallocate_subrout_call(ASR::stmt_t* x) {
        ASR::SubroutineCall_t* subrout_call = ASR::down_cast<ASR::SubroutineCall_t>(x);
        const ASR::symbol_t* subrout_sym = LFortran::ASRUtils::symbol_get_past_external(subrout_call->m_name);
        if( subrout_sym->type != ASR::symbolType::Subroutine ) {
            return nullptr;
        }
        ASR::Subroutine_t* subrout = ASR::down_cast<ASR::Subroutine_t>(subrout_sym);
        Vec<ASR::symbol_t*> del_syms;
        del_syms.reserve(al, 1);
        for( size_t i = 0; i < subrout_call->n_args; i++ ) {
            if( subrout_call->m_args[i]->type == ASR::exprType::Var ) {
                const ASR::Var_t* arg_var = ASR::down_cast<ASR::Var_t>(subrout_call->m_args[i]);
                const ASR::symbol_t* sym = LFortran::ASRUtils::symbol_get_past_external(arg_var->m_v);
                if( sym->type == ASR::symbolType::Variable ) {
                    ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(sym);
                    const ASR::Var_t* orig_arg_var = ASR::down_cast<ASR::Var_t>(subrout->m_args[i]);
                    const ASR::symbol_t* orig_sym = LFortran::ASRUtils::symbol_get_past_external(orig_arg_var->m_v);
                    ASR::Variable_t* orig_var = ASR::down_cast<ASR::Variable_t>(orig_sym);
                    if( var->m_storage == ASR::storage_typeType::Allocatable &&
                        orig_var->m_intent == ASR::intentType::Out ) {
                        del_syms.push_back(al, arg_var->m_v);
                    }
                }
            }
        }
        if( del_syms.size() == 0 ) {
            return nullptr;
        }
        return LFortran::ASRUtils::STMT(ASR::make_ImplicitDeallocate_t(al, x->base.loc,
                    del_syms.p, del_syms.size()));
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
    // TODO: add SymbolTable::lookup_symbol(), which will automatically return
    // an error
    // TODO: add SymbolTable::get_symbol(), which will only check in Debug mode
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Subroutine_t *v = ASR::down_cast<ASR::Subroutine_t>(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                ASR::stmt_t* tmp_stmt = LFortran::ASRUtils::STMT(tmp);
                if( tmp_stmt->type == ASR::stmtType::SubroutineCall ) {
                    ASR::stmt_t* impl_decl = create_implicit_deallocate_subrout_call(tmp_stmt);
                    if( impl_decl != nullptr ) {
                        body.push_back(al, impl_decl);
                    }
                }
                body.push_back(al, tmp_stmt);
            }
        }
        ASR::stmt_t* impl_del = create_implicit_deallocate(x.base.base.loc);
        if( impl_del != nullptr ) {
            body.push_back(al, impl_del);
        }
        v->m_body = body.p;
        v->n_body = body.size();

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Function(const AST::Function_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Function_t *v = ASR::down_cast<ASR::Function_t>(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                ASR::stmt_t* tmp_stmt = LFortran::ASRUtils::STMT(tmp);
                if( tmp_stmt->type == ASR::stmtType::SubroutineCall ) {
                    ASR::stmt_t* impl_decl = create_implicit_deallocate_subrout_call(tmp_stmt);
                    if( impl_decl != nullptr ) {
                        body.push_back(al, impl_decl);
                    }
                }
                body.push_back(al, tmp_stmt);
            }
        }
        ASR::stmt_t* impl_del = create_implicit_deallocate(x.base.base.loc);
        if( impl_del != nullptr ) {
            body.push_back(al, impl_del);
        }
        v->m_body = body.p;
        v->n_body = body.size();

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Assignment(const AST::Assignment_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = LFortran::ASRUtils::EXPR(tmp);
        ASR::ttype_t *target_type = LFortran::ASRUtils::expr_type(target);
        if( target->type != ASR::exprType::Var &&
            target->type != ASR::exprType::ArrayRef &&
            target->type != ASR::exprType::DerivedRef )
        {
            throw SemanticError(
                "The LHS of assignment can only be a variable or an array reference",
                x.base.base.loc
            );
        }

        this->visit_expr(*x.m_value);
        ASR::expr_t *value = LFortran::ASRUtils::EXPR(tmp);
        ASR::ttype_t *value_type = LFortran::ASRUtils::expr_type(value);
        if( target->type == ASR::exprType::Var && !ASRUtils::is_array(target_type) &&
            value->type == ASR::exprType::ConstantArray ) {
            throw SemanticError("ArrayInitalizer expressions can only be assigned array references", x.base.base.loc);
        }
        if (target->type == ASR::exprType::Var ||
            target->type == ASR::exprType::ArrayRef) {

            ImplicitCastRules::set_converted_value(al, x.base.base.loc, &value,
                                                    value_type, target_type);

        }
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value);
    }

    Vec<ASR::expr_t*> visit_expr_list(AST::fnarg_t *ast_list, size_t n) {
        Vec<ASR::expr_t*> asr_list;
        asr_list.reserve(al, n);
        for (size_t i=0; i<n; i++) {
            LFORTRAN_ASSERT(ast_list[i].m_end != nullptr);
            visit_expr(*ast_list[i].m_end);
            ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
            asr_list.push_back(al, expr);
        }
        return asr_list;
    }

    void visit_SubroutineCall(const AST::SubroutineCall_t &x) {
        SymbolTable* scope = current_scope;
        std::string sub_name = x.m_name;
        ASR::symbol_t *original_sym;
        ASR::expr_t *v_expr = nullptr;
        // If this is a type bound procedure (in a class) it won't be in the
        // main symbol table. Need to check n_member.
        if (x.n_member == 1) {
            ASR::symbol_t *v = current_scope->resolve_symbol(x.m_member[0].m_name);
            ASR::asr_t *v_var = ASR::make_Var_t(al, x.base.base.loc, v);
            v_expr = LFortran::ASRUtils::EXPR(v_var);
            original_sym = resolve_deriv_type_proc(x.base.base.loc, x.m_name,
                x.m_member[0].m_name, scope);
        } else {
            original_sym = current_scope->resolve_symbol(sub_name);
        }
        if (!original_sym) {
            throw SemanticError("Subroutine '" + sub_name + "' not declared", x.base.base.loc);
        }
        Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
        ASR::symbol_t *final_sym=nullptr;
        switch (original_sym->type) {
            case (ASR::symbolType::Subroutine) : {
                final_sym=original_sym;
                original_sym = nullptr;
                break;
            }
            case (ASR::symbolType::GenericProcedure) : {
                ASR::GenericProcedure_t *p = ASR::down_cast<ASR::GenericProcedure_t>(original_sym);
                int idx = select_generic_procedure(args, *p, x.base.base.loc);
                final_sym = p->m_procs[idx];
                break;
            }
            case (ASR::symbolType::ClassProcedure) : {
                final_sym = original_sym;
                original_sym = nullptr;
                break;
            }
            case (ASR::symbolType::ExternalSymbol) : {
                ASR::ExternalSymbol_t *p = ASR::down_cast<ASR::ExternalSymbol_t>(original_sym);
                final_sym = p->m_external;
                // Enforced by verify(), but we ensure anyway that
                // ExternalSymbols are not chained:
                LFORTRAN_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*final_sym))
                if (ASR::is_a<ASR::GenericProcedure_t>(*final_sym)) {
                    ASR::GenericProcedure_t *g = ASR::down_cast<ASR::GenericProcedure_t>(final_sym);
                    int idx = select_generic_procedure(args, *g, x.base.base.loc);
                    // FIXME
                    // Create ExternalSymbol for the final subroutine here
                    final_sym = g->m_procs[idx];
                    if (!ASR::is_a<ASR::Subroutine_t>(*final_sym)) {
                        throw SemanticError("ExternalSymbol must point to a Subroutine", x.base.base.loc);
                    }
                    // We mangle the new ExternalSymbol's local name as:
                    //   generic_procedure_local_name @
                    //     specific_procedure_remote_name
                    std::string local_sym = std::string(p->m_name) + "@"
                        + LFortran::ASRUtils::symbol_name(final_sym);
                    if (current_scope->scope.find(local_sym)
                        == current_scope->scope.end()) {
                        Str name;
                        name.from_str(al, local_sym);
                        char *cname = name.c_str(al);
                        ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                            al, p->base.base.loc,
                            /* a_symtab */ current_scope,
                            /* a_name */ cname,
                            final_sym,
                            p->m_module_name, LFortran::ASRUtils::symbol_name(final_sym),
                            ASR::accessType::Private
                            );
                        final_sym = ASR::down_cast<ASR::symbol_t>(sub);
                        current_scope->scope[local_sym] = final_sym;
                    } else {
                        final_sym = current_scope->scope[local_sym];
                    }
                } else {
                    if (!ASR::is_a<ASR::Subroutine_t>(*final_sym)) {
                        throw SemanticError("ExternalSymbol must point to a Subroutine", x.base.base.loc);
                    }
                    final_sym=original_sym;
                    original_sym = nullptr;
                }
                break;
            }
            default : {
                throw SemanticError("Symbol type not supported", x.base.base.loc);
            }
        }
        tmp = ASR::make_SubroutineCall_t(al, x.base.base.loc,
                final_sym, original_sym, args.p, args.size(), v_expr);
    }

    int select_generic_procedure(const Vec<ASR::expr_t*> &args,
            const ASR::GenericProcedure_t &p, Location loc) {
        for (size_t i=0; i < p.n_procs; i++) {
            if (ASR::is_a<ASR::Subroutine_t>(*p.m_procs[i])) {
                ASR::Subroutine_t *sub
                    = ASR::down_cast<ASR::Subroutine_t>(p.m_procs[i]);
                if (argument_types_match(args, *sub)) {
                    return i;
                }
            } else if (ASR::is_a<ASR::Function_t>(*p.m_procs[i])) {
                ASR::Function_t *fn
                    = ASR::down_cast<ASR::Function_t>(p.m_procs[i]);
                if (argument_types_match(args, *fn)) {
                    return i;
                }
            } else {
                throw SemanticError("Only Subroutine supported in generic procedure", loc);
            }
        }
        throw SemanticError("Arguments do not match", loc);
    }

    template <typename T>
    bool argument_types_match(const Vec<ASR::expr_t*> &args,
            const T &sub) {
        if (args.size() == sub.n_args) {
            for (size_t i=0; i < args.size(); i++) {
                ASR::Variable_t *v = LFortran::ASRUtils::EXPR2VAR(sub.m_args[i]);
                ASR::ttype_t *arg1 = LFortran::ASRUtils::expr_type(args[i]);
                ASR::ttype_t *arg2 = v->m_type;
                if (!types_equal(*arg1, *arg2)) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    bool types_equal(const ASR::ttype_t &a, const ASR::ttype_t &b) {
        if (a.type == b.type) {
            // TODO: check dims
            switch (a.type) {
                case (ASR::ttypeType::Real) : {
                    ASR::Real_t *a2 = ASR::down_cast<ASR::Real_t>(&a);
                    ASR::Real_t *b2 = ASR::down_cast<ASR::Real_t>(&b);
                    if (a2->m_kind == b2->m_kind) {
                        return true;
                    } else {
                        return false;
                    }
                    break;
                }
                default : return true;
            }
        }
        return false;
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_Compare(al, x, left, right, tmp);
    }

    void visit_BoolOp(const AST::BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_BoolOp(al, x, left, right, tmp);
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_BinOp(al, x, left, right, tmp);
    }

    void visit_StrOp(const AST::StrOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_StrOp(al, x, left, right, tmp);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_UnaryOp(al, x, operand, tmp);
    }

    ASR::asr_t* resolve_variable(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string var_name = id;
        ASR::symbol_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            throw SemanticError("Variable '" + var_name + "' not declared", loc);
        }
        if( v->type == ASR::symbolType::Variable ) {
            ASR::Variable_t* v_var = ASR::down_cast<ASR::Variable_t>(v);
            if( v_var->m_type == nullptr && 
                v_var->m_intent == ASR::intentType::AssociateBlock ) {
                return (ASR::asr_t*)(v_var->m_symbolic_value);
            }
        }
        return ASR::make_Var_t(al, loc, v);
    }

    ASR::asr_t* getDerivedRef_t(const Location& loc, ASR::asr_t* v_var, ASR::symbol_t* member) {
        ASR::Variable_t* member_variable = ((ASR::Variable_t*)(&(member->base)));
        ASR::ttype_t* member_type = member_variable->m_type;
        switch( member_type->type ) {
            case ASR::ttypeType::Derived: {
                ASR::Derived_t* der = (ASR::Derived_t*)(&(member_type->base));
                ASR::DerivedType_t* der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
                if( der_type->m_symtab->counter != current_scope->counter ) {
                    ASR::symbol_t* der_ext;
                    char* module_name = (char*)"nullptr";
                    ASR::symbol_t* m_external = der->m_derived_type;
                    if( m_external->type == ASR::symbolType::ExternalSymbol ) {
                        ASR::ExternalSymbol_t* m_ext = (ASR::ExternalSymbol_t*)(&(m_external->base));
                        m_external = m_ext->m_external;
                        module_name = m_ext->m_module_name;
                    }
                    Str mangled_name;
                    mangled_name.from_str(al, "1_" +
                                              std::string(module_name) + "_" +
                                              std::string(der_type->m_name));
                    char* mangled_name_char = mangled_name.c_str(al);
                    if( current_scope->scope.find(mangled_name.str()) == current_scope->scope.end() ) {
                        bool make_new_ext_sym = true;
                        ASR::symbol_t* der_tmp = nullptr;
                        if( current_scope->scope.find(std::string(der_type->m_name)) != current_scope->scope.end() ) {
                            der_tmp = current_scope->scope[std::string(der_type->m_name)];
                            if( der_tmp->type == ASR::symbolType::ExternalSymbol ) {
                                ASR::ExternalSymbol_t* der_ext_tmp = (ASR::ExternalSymbol_t*)(&(der_tmp->base));
                                if( der_ext_tmp->m_external == m_external ) {
                                    make_new_ext_sym = false;
                                }
                            }
                        }
                        if( make_new_ext_sym ) {
                            der_ext = (ASR::symbol_t*)ASR::make_ExternalSymbol_t(al, loc, current_scope, mangled_name_char, m_external,
                                                                                module_name, der_type->m_name, ASR::accessType::Public);
                            current_scope->scope[mangled_name.str()] = der_ext;
                        } else {
                            LFORTRAN_ASSERT(der_tmp != nullptr);
                            der_ext = der_tmp;
                        }
                    } else {
                        der_ext = current_scope->scope[mangled_name.str()];
                    }
                    ASR::asr_t* der_new = ASR::make_Derived_t(al, loc, der_ext, der->m_dims, der->n_dims);
                    member_type = (ASR::ttype_t*)(der_new);
                }
                break;
            }
            default :
                break;
        }
        return ASR::make_DerivedRef_t(al, loc, LFortran::ASRUtils::EXPR(v_var), member, member_type, nullptr);
    }

    ASR::asr_t* resolve_variable2(const Location &loc, const char* id,
            const char* derived_type_id, SymbolTable*& scope) {
        std::string var_name = id;
        std::string dt_name = derived_type_id;
        ASR::symbol_t *v = scope->resolve_symbol(dt_name);
        if (!v) {
            throw SemanticError("Variable '" + dt_name + "' not declared", loc);
        }
        ASR::Variable_t* v_variable = ((ASR::Variable_t*)(&(v->base)));
        if ( v_variable->m_type->type == ASR::ttypeType::Derived ||
             v_variable->m_type->type == ASR::ttypeType::DerivedPointer ||
             v_variable->m_type->type == ASR::ttypeType::Class ) {
            ASR::ttype_t* v_type = v_variable->m_type;
            ASR::Derived_t* der = (ASR::Derived_t*)(&(v_type->base));
            ASR::DerivedType_t* der_type;
            if( der->m_derived_type->type == ASR::symbolType::ExternalSymbol ) {
                ASR::ExternalSymbol_t* der_ext = (ASR::ExternalSymbol_t*)(&(der->m_derived_type->base));
                ASR::symbol_t* der_sym = der_ext->m_external;
                if( der_sym == nullptr ) {
                    throw SemanticError("'" + std::string(der_ext->m_name) + "' isn't a Derived type.", loc);
                } else {
                    der_type = (ASR::DerivedType_t*)(&(der_sym->base));
                }
            } else {
                der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
            }
            scope = der_type->m_symtab;
            ASR::symbol_t* member = der_type->m_symtab->resolve_symbol(var_name);
            if( member != nullptr ) {
                ASR::asr_t* v_var = ASR::make_Var_t(al, loc, v);
                return getDerivedRef_t(loc, v_var, member);
            } else {
                throw SemanticError("Variable '" + dt_name + "' doesn't have any member named, '" + var_name + "'.", loc);
            }
        } else {
            throw SemanticError("Variable '" + dt_name + "' is not a derived type", loc);
        }
    }

    ASR::symbol_t* resolve_deriv_type_proc(const Location &loc, const char* id,
            const char* derived_type_id, SymbolTable*& scope) {
        std::string var_name = id;
        std::string dt_name = derived_type_id;
        ASR::symbol_t *v = scope->resolve_symbol(dt_name);
        if (!v) {
            throw SemanticError("Variable '" + dt_name + "' not declared", loc);
        }
        ASR::Variable_t* v_variable = ((ASR::Variable_t*)(&(v->base)));
        if ( v_variable->m_type->type == ASR::ttypeType::Derived ||
             v_variable->m_type->type == ASR::ttypeType::DerivedPointer ||
             v_variable->m_type->type == ASR::ttypeType::Class ) {
            ASR::ttype_t* v_type = v_variable->m_type;
            ASR::Derived_t* der = (ASR::Derived_t*)(&(v_type->base));
            ASR::DerivedType_t* der_type;
            if( der->m_derived_type->type == ASR::symbolType::ExternalSymbol ) {
                ASR::ExternalSymbol_t* der_ext = (ASR::ExternalSymbol_t*)(&(der->m_derived_type->base));
                ASR::symbol_t* der_sym = der_ext->m_external;
                if( der_sym == nullptr ) {
                    throw SemanticError("'" + std::string(der_ext->m_name) + "' isn't a Derived type.", loc);
                } else {
                    der_type = (ASR::DerivedType_t*)(&(der_sym->base));
                }
            } else {
                der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
            }
            scope = der_type->m_symtab;
            ASR::symbol_t* member = der_type->m_symtab->resolve_symbol(var_name);
            if( member != nullptr ) {
                return member;
            } else {
                throw SemanticError("Variable '" + dt_name + "' doesn't have any member named, '" + var_name + "'.", loc);
            }
        } else {
            throw SemanticError("Variable '" + dt_name + "' is not a derived type", loc);
        }
    }


    void visit_Name(const AST::Name_t &x) {
        if (x.n_member == 0) {
            tmp = resolve_variable(x.base.base.loc, x.m_id);
        } else if (x.n_member == 1 && x.m_member[0].n_args == 0) {
            SymbolTable* scope = current_scope;
            tmp = resolve_variable2(x.base.base.loc, x.m_id,
                x.m_member[0].m_name, scope);
        } else {
            SymbolTable* scope = current_scope;
            tmp = resolve_variable2(x.base.base.loc, x.m_member[1].m_name, x.m_member[0].m_name, scope);
            ASR::DerivedRef_t* tmp2;
            std::uint32_t i;
            for( i = 2; i < x.n_member; i++ ) {
                tmp2 = (ASR::DerivedRef_t*)resolve_variable2(x.base.base.loc,
                                            x.m_member[i].m_name, x.m_member[i - 1].m_name, scope);
                tmp = ASR::make_DerivedRef_t(al, x.base.base.loc, LFortran::ASRUtils::EXPR(tmp), tmp2->m_m, tmp2->m_type, nullptr);
            }
            i = x.n_member - 1;
            tmp2 = (ASR::DerivedRef_t*)resolve_variable2(x.base.base.loc, x.m_id, x.m_member[i].m_name, scope);
            tmp = ASR::make_DerivedRef_t(al, x.base.base.loc, LFortran::ASRUtils::EXPR(tmp), tmp2->m_m, tmp2->m_type, nullptr);
        }
    }

    void symbol_resolve_generic_procedure(
            ASR::symbol_t *v,
            const AST::FuncCallOrArray_t &x
            ) {
        ASR::ExternalSymbol_t *p = ASR::down_cast<ASR::ExternalSymbol_t>(v);
        ASR::symbol_t *f2 = ASR::down_cast<ASR::ExternalSymbol_t>(v)->m_external;
        ASR::GenericProcedure_t *g = ASR::down_cast<ASR::GenericProcedure_t>(f2);
        Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
        int idx = select_generic_procedure(args, *g, x.base.base.loc);
        ASR::symbol_t *final_sym;
        final_sym = g->m_procs[idx];
        if (!ASR::is_a<ASR::Function_t>(*final_sym)) {
            throw SemanticError("ExternalSymbol must point to a Function", x.base.base.loc);
        }
        ASR::ttype_t *return_type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(final_sym)->m_return_var)->m_type;
        // Create ExternalSymbol for the final subroutine:
        // We mangle the new ExternalSymbol's local name as:
        //   generic_procedure_local_name @
        //     specific_procedure_remote_name
        std::string local_sym = std::string(p->m_name) + "@"
            + LFortran::ASRUtils::symbol_name(final_sym);
        if (current_scope->scope.find(local_sym)
            == current_scope->scope.end()) {
            Str name;
            name.from_str(al, local_sym);
            char *cname = name.c_str(al);
            ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                al, g->base.base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                final_sym,
                p->m_module_name, LFortran::ASRUtils::symbol_name(final_sym),
                ASR::accessType::Private
                );
            final_sym = ASR::down_cast<ASR::symbol_t>(sub);
            current_scope->scope[local_sym] = final_sym;
        } else {
            final_sym = current_scope->scope[local_sym];
        }
        ASR::expr_t *value = nullptr;
        tmp = ASR::make_FunctionCall_t(al, x.base.base.loc,
            final_sym, v, args.p, args.size(), nullptr, 0, return_type,
            value, nullptr);
    }

    void visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x) {
        std::vector<std::string> all_intrinsics = {
            "cos",  "tan",  "sinh",  "cosh",  "tanh",
            "asin", "acos", "atan", "asinh", "acosh", "atanh"};
        SymbolTable *scope = current_scope;
        std::string var_name = x.m_func;
        ASR::symbol_t *v = nullptr;
        ASR::expr_t *v_expr = nullptr;
        // If this is a type bound procedure (in a class) it won't be in the
        // main symbol table. Need to check n_member.
        if (x.n_member == 1) {
            ASR::symbol_t *obj = current_scope->resolve_symbol(x.m_member[0].m_name);
            ASR::asr_t *obj_var = ASR::make_Var_t(al, x.base.base.loc, obj);
            v_expr = LFortran::ASRUtils::EXPR(obj_var);
            v = resolve_deriv_type_proc(x.base.base.loc, x.m_func,
                x.m_member[0].m_name, scope);
        } else {
            v = current_scope->resolve_symbol(var_name);
        }
        if (!v) {
            std::string remote_sym = to_lower(var_name);
            if (intrinsic_procedures.find(remote_sym)
                        != intrinsic_procedures.end()) {
                std::string module_name = intrinsic_procedures[remote_sym];
                bool shift_scope = false;
                if (current_scope->parent->parent) {
                    current_scope = current_scope->parent;
                    shift_scope = true;
                }
                ASR::Module_t *m = LFortran::ASRUtils::load_module(al, current_scope->parent, module_name,
                    x.base.base.loc, true);
                if (shift_scope) current_scope = scope;

                ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
                if (!t) {
                    throw SemanticError("The symbol '" + remote_sym
                        + "' not found in the module '" + module_name + "'",
                        x.base.base.loc);
                }
                if (ASR::is_a<ASR::GenericProcedure_t>(*t)) {
                    ASR::GenericProcedure_t *gp = ASR::down_cast<ASR::GenericProcedure_t>(t);
                    ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                        al, gp->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ gp->m_name,
                        (ASR::symbol_t*)gp,
                        m->m_name, gp->m_name,
                        ASR::accessType::Private
                        );
                    std::string sym = gp->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                    symbol_resolve_generic_procedure(
                        ASR::down_cast<ASR::symbol_t>(fn), x);
                    return;
                }

                if (!ASR::is_a<ASR::Function_t>(*t)) {
                    throw SemanticError("The symbol '" + remote_sym
                        + "' found in the module '" + module_name + "', "
                        + "but it is not a function.",
                        x.base.base.loc);
                }

                ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
                ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                    al, mfn->base.base.loc,
                    /* a_symtab */ current_scope,
                    /* a_name */ mfn->m_name,
                    (ASR::symbol_t*)mfn,
                    m->m_name, mfn->m_name,
                    ASR::accessType::Private
                    );
                std::string sym = mfn->m_name;
                current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                v = ASR::down_cast<ASR::symbol_t>(fn);
                if (current_module) {
                    // Add the module `m` to current module dependencies
                    Vec<char*> vec;
                    vec.from_pointer_n_copy(al, current_module->m_dependencies,
                                current_module->n_dependencies);
                    if (!present(vec, m->m_name)) {
                        vec.push_back(al, m->m_name);
                        current_module->m_dependencies = vec.p;
                        current_module->n_dependencies = vec.size();
                    }
                }
            } else if (to_lower(var_name) == "present") {
                // Intrinsic function present(), add it to the global scope
                ASR::TranslationUnit_t *unit = (ASR::TranslationUnit_t *)asr;
                const char *fn_name_orig = "present";
                char *fn_name = (char *)fn_name_orig;
                SymbolTable *fn_scope =
                    al.make_new<SymbolTable>(unit->m_global_scope);
                ASR::ttype_t *type;
                type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
                ASR::asr_t *return_var = ASR::make_Variable_t(
                    al, x.base.base.loc, fn_scope, fn_name, LFortran::ASRUtils::intent_return_var,
                    nullptr, nullptr, ASR::storage_typeType::Default, type,
                    ASR::abiType::Source,
                    ASR::Public, ASR::presenceType::Required);
                fn_scope->scope[std::string(fn_name)] =
                    ASR::down_cast<ASR::symbol_t>(return_var);
                ASR::asr_t *return_var_ref = ASR::make_Var_t(
                    al, x.base.base.loc, ASR::down_cast<ASR::symbol_t>(return_var));
                ASR::asr_t *fn =
                    ASR::make_Function_t(al, x.base.base.loc,
                                       /* a_symtab */ fn_scope,
                                       /* a_name */ fn_name,
                                       /* a_args */ nullptr,
                                       /* n_args */ 0,
                                       /* a_body */ nullptr,
                                       /* n_body */ 0,
                                       /* a_return_var */ LFortran::ASRUtils::EXPR(return_var_ref),
                                       ASR::abiType::Source,
                                       ASR::Public, ASR::deftypeType::Implementation);
                std::string sym_name = fn_name;
                unit->m_global_scope->scope[sym_name] =
                    ASR::down_cast<ASR::symbol_t>(fn);
                v = ASR::down_cast<ASR::symbol_t>(fn);
            } else {
                auto find_intrinsic =
                    std::find(all_intrinsics.begin(), all_intrinsics.end(),
                        to_lower(var_name));
                if (find_intrinsic == all_intrinsics.end()) {
                    throw SemanticError("Function or array '" + var_name +
                                        "' not declared",
                                    x.base.base.loc);
                } else {
                    int intrinsic_index =
                      std::distance(all_intrinsics.begin(), find_intrinsic);
                    // Intrinsic function, add it to the global scope
                    ASR::TranslationUnit_t *unit = (ASR::TranslationUnit_t *)asr;
                    Str s;
                    s.from_str_view(all_intrinsics[intrinsic_index]);
                    char *fn_name = s.c_str(al);
                    SymbolTable *fn_scope =
                        al.make_new<SymbolTable>(unit->m_global_scope);
                    ASR::ttype_t *type;

                    // Arguments
                    Vec<ASR::expr_t *> args;
                    args.reserve(al, 1);
                    type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc, 4, nullptr, 0));
                    const char *arg0_s_orig = "x";
                    char *arg0_s = (char *)arg0_s_orig;
                    ASR::asr_t *arg0 = ASR::make_Variable_t(
                        al, x.base.base.loc, fn_scope, arg0_s, LFortran::ASRUtils::intent_in, nullptr, nullptr,
                        ASR::storage_typeType::Default, type,
                        ASR::abiType::Source,
                        ASR::Public, ASR::presenceType::Required);
                    ASR::symbol_t *var = ASR::down_cast<ASR::symbol_t>(arg0);
                    fn_scope->scope[std::string(arg0_s)] = var;
                    args.push_back(al, LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var)));

                    // Return value
                    type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc, 4, nullptr, 0));
                    ASR::asr_t *return_var = ASR::make_Variable_t(
                        al, x.base.base.loc, fn_scope, fn_name, LFortran::ASRUtils::intent_return_var,
                        nullptr, nullptr, ASR::storage_typeType::Default, type,
                        ASR::abiType::Source,
                        ASR::Public, ASR::presenceType::Required);
                    fn_scope->scope[std::string(fn_name)] =
                        ASR::down_cast<ASR::symbol_t>(return_var);
                    ASR::asr_t *return_var_ref = ASR::make_Var_t(
                        al, x.base.base.loc, ASR::down_cast<ASR::symbol_t>(return_var));
                    ASR::asr_t *fn =
                        ASR::make_Function_t(al, x.base.base.loc,
                                             /* a_symtab */ fn_scope,
                                             /* a_name */ fn_name,
                                             /* a_args */ args.p,
                                             /* n_args */ args.n,
                                             /* a_body */ nullptr,
                                             /* n_body */ 0,
                                             /* a_return_var */ LFortran::ASRUtils::EXPR(return_var_ref),
                                             ASR::abiType::Intrinsic,
                                             ASR::Public, ASR::deftypeType::Implementation);
                    std::string sym_name = fn_name;
                    unit->m_global_scope->scope[sym_name] =
                        ASR::down_cast<ASR::symbol_t>(fn);
                    v = ASR::down_cast<ASR::symbol_t>(fn);
                }
            }
        }
        switch (v->type) {
            case ASR::symbolType::ClassProcedure : {
                Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
                ASR::ttype_t *type = nullptr;
                ASR::ClassProcedure_t *v_class_proc = ASR::down_cast<ASR::ClassProcedure_t>(v);
                type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(v_class_proc->m_proc)->m_return_var)->m_type;
                tmp = ASR::make_FunctionCall_t(al, x.base.base.loc,
                        v, nullptr, args.p, args.size(), nullptr, 0, type, nullptr,
                        v_expr);
                break;
            }
            case ASR::symbolType::Function : {
                Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
                ASR::ttype_t *type;
                type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(v)->m_return_var)->m_type;
                tmp = ASR::make_FunctionCall_t(al, x.base.base.loc,
                    v, nullptr, args.p, args.size(), nullptr, 0, type, nullptr,
                    v_expr);
                break;
            }
            case (ASR::symbolType::GenericProcedure) : {
                ASR::GenericProcedure_t *p = ASR::down_cast<ASR::GenericProcedure_t>(v);
                Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
                int idx = select_generic_procedure(args, *p, x.base.base.loc);
                ASR::symbol_t *final_sym = p->m_procs[idx];

                ASR::ttype_t *type;
                type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(final_sym)->m_return_var)->m_type;
                tmp = ASR::make_FunctionCall_t(al, x.base.base.loc,
                    final_sym, v, args.p, args.size(), nullptr, 0, type, nullptr,
                    v_expr);
                break;
            }
            case (ASR::symbolType::ExternalSymbol) : {
                ASR::symbol_t *f2 = ASR::down_cast<ASR::ExternalSymbol_t>(v)->m_external;
                LFORTRAN_ASSERT(f2);
                if (ASR::is_a<ASR::Function_t>(*f2)) {
                    Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
                    ASR::ttype_t *return_type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(f2)->m_return_var)->m_type;
                    // Populate value
                    ASR::expr_t* value = nullptr;
                    std::string func_name = x.m_func;
                    // Only populate for supported intrinsic functions
                    if (intrinsic_procedures.find(func_name)
                        != intrinsic_procedures.end()) { // Got an intrinsic, now try to assign value
                        ASR::expr_t* func_expr = args[0];
                        ASR::ttype_t *func_type = LFortran::ASRUtils::expr_type(func_expr);
                        // TODO: This ordering is terrible; falls through in an arbitrary manner
                        // Should be in a hash table or something
                        if (func_name == "tiny") {
                            if (args.n == 1) {
                                if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
                                    if (LFortran::ASRUtils::is_array(func_type)){
                                        throw SemanticError("Array values not implemented yet",
                                                            x.base.base.loc);
                                    }
                                    int tiny_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(func_type);
                                    if (tiny_kind == 4){
                                        float low_val = std::numeric_limits<float>::min();
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, x.base.base.loc, low_val, func_type));
                                    } else {
                                        double low_val = std::numeric_limits<float>::min();
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, x.base.base.loc, low_val, func_type));
                                    }
                                }
                                else {
                                    throw SemanticError("Argument for tiny must be Real", x.base.base.loc);
                                }
                            } else {
                                throw SemanticError("tiny must have only one argument", x.base.base.loc);
                            }
                        }
                        else if (func_name == "int") {
                            if (args.n == 1) {
                                ASR::expr_t* func_expr = args[0];
                                int func_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(func_type);
                                if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
                                    if (func_kind == 4){
                                        float rr = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(func_expr))->m_r;
                                        int64_t ival = static_cast<int64_t>(rr);
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, ival, func_type));
                                    } else {
                                        double rr = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(func_expr))->m_r;
                                        int64_t ival = static_cast<int64_t>(rr);
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, ival, func_type));
                                    }
                                }
                                else if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*func_type)) {
                                    if (func_kind == 4){
                                        int64_t ival = ASR::down_cast<ASR::ConstantInteger_t>(
                                            LFortran::ASRUtils::expr_value(func_expr))->m_n;
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, ival, func_type));
                                    } else {
                                        int64_t ival = ASR::down_cast<ASR::ConstantInteger_t>(
                                            LFortran::ASRUtils::expr_value(func_expr))->m_n;
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, x.base.base.loc, ival, func_type));
                                    }
                                }
                                // TODO: Handle BOZ later
                                // else if () {

                                // }
                            } else {
                                throw SemanticError("int must have only one argument", x.base.base.loc);
                            }
                        }
                        else if (func_name == "real") {
                            if (args.n == 1) {
                                ASR::expr_t* real_expr = args[0];
                                int real_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(func_type);
                                if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
                                    if (real_kind == 4){
                                        float rr = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(real_expr))->m_r;
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, x.base.base.loc, rr, func_type));
                                    } else {
                                        double rr = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(real_expr))->m_r;
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, x.base.base.loc, rr, func_type));
                                    }
                                }
                                else if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*func_type)) {
                                    if (real_kind == 4){
                                        int64_t rv = ASR::down_cast<ASR::ConstantInteger_t>(
                                            LFortran::ASRUtils::expr_value(real_expr))->m_n;
                                        float rr = static_cast<float>(rv);
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, x.base.base.loc, rr, func_type));
                                    } else {
                                        int64_t rv = ASR::down_cast<ASR::ConstantInteger_t>(LFortran::ASRUtils::expr_value(real_expr))->m_n;
                                        double rr = static_cast<double>(rv);
                                        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, x.base.base.loc, rr, func_type));
                                    }
                                }
                                // TODO: Handle BOZ later
                                // else if () {

                                // }
                            } else {
                                throw SemanticError("real must have only one argument", x.base.base.loc);
                            }
                        }
                        else if (var_name=="floor") {
                            // TODO: Implement optional kind; J3/18-007r1 --> FLOOR(A, [KIND])
                            if (args.n==1) {
                            ASR::expr_t* func_expr = args[0];
                            ASR::ttype_t* func_type = LFortran::ASRUtils::expr_type(func_expr);
                            int func_kind = ASRUtils::extract_kind_from_ttype_t(func_type);
                            int64_t ival {0};
                            if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
                                if (func_kind == 4){
                                    float rv = ASR::down_cast<ASR::ConstantReal_t>(
                                        LFortran::ASRUtils::expr_value(func_expr))->m_r;
                                    if (rv<0) {
                                        // negative number
                                        // floor -> integer(|x|+1)
                                        ival = static_cast<int64_t>(rv-1);
                                    } else {
                                        // positive, floor -> integer(x)
                                        ival = static_cast<int64_t>(rv);
                                    }
                                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, ival,func_type));
                                } else {
                                    double rv = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(func_expr))->m_r;
                                    int64_t ival = static_cast<int64_t>(rv);
                                    if (rv<0) {
                                        // negative number
                                        // floor -> integer(x+1)
                                        ival = static_cast<int64_t>(rv+1);
                                    } else {
                                        // positive, floor -> integer(x)
                                        ival = static_cast<int64_t>(rv);
                                    }
                                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, ival,func_type));
                                }
                            } else {
                                throw SemanticError("floor must have one real argument", x.base.base.loc);
                            }
                            } else {
                                throw SemanticError("floor with optional kind not yet implemented;\n will only work with one real argument", x.base.base.loc);
                            }
                        }
                        else if (func_name == "kind") {
                            if (args.n == 1) {
                                int64_t kind_val {4};
                                if (ASR::is_a<ASR::ConstantLogical_t>(*func_expr)){
                                    kind_val = ASR::down_cast<ASR::Logical_t>(ASR::down_cast<ASR::ConstantLogical_t>(func_expr)->m_type)->m_kind;
                                }
                                else if (ASR::is_a<ASR::ConstantReal_t>(*func_expr)){
                                    kind_val = ASR::down_cast<ASR::Real_t>(ASR::down_cast<ASR::ConstantReal_t>(func_expr)->m_type)->m_kind;
                                }
                                else if (ASR::is_a<ASR::ConstantInteger_t>(*func_expr)){
                                    kind_val = ASR::down_cast<ASR::Integer_t>(ASR::down_cast<ASR::ConstantInteger_t>(func_expr)->m_type)->m_kind;
                                }
                                else if (ASR::is_a<ASR::Var_t>(*func_expr)) {
                                    kind_val = ASRUtils::extract_kind(func_expr, x.base.base.loc);
                                }
                                else {
                                    throw SemanticError("kind supports Real, Integer, Logical and things which reduce to the same", x.base.base.loc);
                                }
                                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, kind_val, func_type));
                            } else {
                                throw SemanticError("kind must have only one argument", x.base.base.loc);
                            }
                        }
                        else if (func_name == "selected_int_kind") {
                            if (args.n == 1 && ASR::is_a<ASR::Integer_t>(*func_type)) {
                                int64_t kind_val {4}, R {4};
                                R = ASR::down_cast<ASR::ConstantInteger_t>(ASRUtils::expr_value(func_expr))->m_n;
                                if (R >= 10) { // > ?
                                   kind_val = 8;
                                }
                                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, kind_val, func_type));
                            } else {
                                throw SemanticError(func_name + " must have only one integer argument", x.base.base.loc);
                            }
                        }
                        else if (func_name == "selected_real_kind") {
                            // TODO: Be more standards compliant 16.9.170
                            // e.g. selected_real_kind(6, 70)
                            if (args.n == 1 && ASR::is_a<ASR::Integer_t>(*func_type)) {
                                int64_t kind_val {4}, R {4};
                                R = ASR::down_cast<ASR::ConstantInteger_t>(ASRUtils::expr_value(func_expr))->m_n;
                                if (R >= 7) { // > ?
                                   kind_val = 8;
                                }
                                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, kind_val, func_type));
                            } else {
                                throw SemanticError(func_name + " must have only one integer argument", x.base.base.loc);
                            }
                        }
                    }
                    tmp = ASR::make_FunctionCall_t(al, x.base.base.loc,
                        v, nullptr, args.p, args.size(), nullptr, 0, return_type,
                        value, nullptr);
                } else if (ASR::is_a<ASR::Variable_t>(*f2)) {
                    Vec<ASR::array_index_t> args;
                    args.reserve(al, x.n_args);
                    for (size_t i=0; i<x.n_args; i++) {
                        ASR::array_index_t ai;
                        if (x.m_args[i].m_start == nullptr && x.m_args[i].m_end) {
                            visit_expr(*x.m_args[i].m_end);
                            ai.m_left = nullptr;
                            ai.m_right = LFortran::ASRUtils::EXPR(tmp);
                            ai.m_step = nullptr;
                            ai.loc = ai.m_right->base.loc;
                        } else if (x.m_args[i].m_start == nullptr
                                && x.m_args[i].m_end == nullptr) {
                            ai.m_left = nullptr;
                            ai.m_right = nullptr;
                            ai.m_step = nullptr;
                            ai.loc = x.base.base.loc;
                        } else {
                            throw SemanticError("Argument type not implemented yet",
                                x.base.base.loc);
                        }
                        args.push_back(al, ai);
                    }

                    ASR::ttype_t *type;
                    type = ASR::down_cast<ASR::Variable_t>(f2)->m_type;
                    tmp = ASR::make_ArrayRef_t(al, x.base.base.loc,
                        v, args.p, args.size(), type, nullptr);
                } else if(ASR::is_a<ASR::DerivedType_t>(*f2)) {
                    Vec<ASR::expr_t*> vals = visit_expr_list(x.m_args, x.n_args);
                    ASR::ttype_t* der = LFortran::ASRUtils::TYPE(
                                        ASR::make_Derived_t(al, x.base.base.loc, v,
                                                            nullptr, 0));
                    tmp = ASR::make_DerivedTypeConstructor_t(al, x.base.base.loc,
                            v, vals.p, vals.size(), der);
                } else if (ASR::is_a<ASR::GenericProcedure_t>(*f2)) {
                    symbol_resolve_generic_procedure(v, x);
                } else {
                    throw SemanticError("Unimplemented", x.base.base.loc);
                }
                break;
            }
            case (ASR::symbolType::Variable) : {
                Vec<ASR::array_index_t> args;
                args.reserve(al, x.n_args);
                for (size_t i=0; i<x.n_args; i++) {
                    ASR::array_index_t ai;
                    ai.loc = x.base.base.loc;
                    ASR::expr_t *m_start, *m_end, *m_step;
                    m_start = m_end = m_step = nullptr;
                    if( x.m_args[i].m_start != nullptr ) {
                        visit_expr(*(x.m_args[i].m_start));
                        m_start = LFortran::ASRUtils::EXPR(tmp);
                        ai.loc = m_start->base.loc;
                    }
                    if( x.m_args[i].m_end != nullptr ) {
                        visit_expr(*(x.m_args[i].m_end));
                        m_end = LFortran::ASRUtils::EXPR(tmp);
                        ai.loc = m_end->base.loc;
                    }
                    if( x.m_args[i].m_step != nullptr ) {
                        visit_expr(*(x.m_args[i].m_step));
                        m_step = LFortran::ASRUtils::EXPR(tmp);
                        ai.loc = m_step->base.loc;
                    }
                    ai.m_left = m_start;
                    ai.m_right = m_end;
                    ai.m_step = m_step;
                    args.push_back(al, ai);
                }

                ASR::ttype_t *type;
                type = ASR::down_cast<ASR::Variable_t>(v)->m_type;
                ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(v);
                if( var->m_type == nullptr &&
                    var->m_intent == ASR::intentType::AssociateBlock ) {
                    ASR::expr_t* orig_expr = var->m_symbolic_value;
                    ASR::Var_t* orig_Var = ASR::down_cast<ASR::Var_t>(orig_expr);
                    v = orig_Var->m_v;
                    type = ASR::down_cast<ASR::Variable_t>(v)->m_type;
                }
                tmp = ASR::make_ArrayRef_t(al, x.base.base.loc,
                    v, args.p, args.size(), type, nullptr);
                break;
            }
            default : throw SemanticError("Symbol '" + var_name
                    + "' is not a function or an array", x.base.base.loc);
            }
    }

    void visit_Num(const AST::Num_t &x) {
        int ikind = 4;
        if (x.m_kind) {
            ikind = std::atoi(x.m_kind);
            if (ikind == 0) {
                std::string var_name = x.m_kind;
                ASR::symbol_t *v = current_scope->resolve_symbol(var_name);
                if (v) {
                    const ASR::symbol_t *v3 = LFortran::ASRUtils::symbol_get_past_external(v);
                    if (ASR::is_a<ASR::Variable_t>(*v3)) {
                        ASR::Variable_t *v2 = ASR::down_cast<ASR::Variable_t>(v3);
                        if (v2->m_value) {
                            if (ASR::is_a<ASR::ConstantInteger_t>(*v2->m_value)) {
                                ikind = ASR::down_cast<ASR::ConstantInteger_t>(v2->m_value)->m_n;
                            } else {
                                throw SemanticError("Variable '" + var_name + "' is constant but not an integer",
                                    x.base.base.loc);
                            }
                        } else {
                            throw SemanticError("Variable '" + var_name + "' is not constant",
                                x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("Symbol '" + var_name + "' is not a variable",
                            x.base.base.loc);
                    }
                } else {
                    throw SemanticError("Variable '" + var_name + "' not declared",
                        x.base.base.loc);
                }
            }
        }
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al,
                x.base.base.loc, ikind, nullptr, 0));
        if (BigInt::is_int_ptr(x.m_n)) {
            throw SemanticError("Integer constants larger than 2^62-1 are not implemented yet", x.base.base.loc);
        } else {
            LFORTRAN_ASSERT(!BigInt::is_int_ptr(x.m_n));
            tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, x.m_n, type);
        }
    }

    void visit_Parenthesis(const AST::Parenthesis_t &x) {
        visit_expr(*x.m_operand);
    }

    void visit_Logical(const AST::Logical_t &x) {
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_ConstantLogical_t(al, x.base.base.loc, x.m_value, type);
    }

    void visit_String(const AST::String_t &x) {
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_ConstantString_t(al, x.base.base.loc, x.m_s, type);
    }

    void visit_Real(const AST::Real_t &x) {
        double r = ASRUtils::extract_real(x.m_n);
        char* s_kind;
        int r_kind = ASRUtils::extract_kind_str(x.m_n, s_kind);
        if (r_kind == 0) {
            std::string var_name = s_kind;
            ASR::symbol_t *v = current_scope->resolve_symbol(var_name);
            if (v) {
                const ASR::symbol_t *v3 = LFortran::ASRUtils::symbol_get_past_external(v);
                if (ASR::is_a<ASR::Variable_t>(*v3)) {
                    ASR::Variable_t *v2 = ASR::down_cast<ASR::Variable_t>(v3);
                    if (v2->m_value) {
                        if (ASR::is_a<ASR::ConstantInteger_t>(*v2->m_value)) {
                            r_kind = ASR::down_cast<ASR::ConstantInteger_t>(v2->m_value)->m_n;
                        } else {
                            throw SemanticError("Variable '" + var_name + "' is constant but not an integer",
                                x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("Variable '" + var_name + "' is not constant",
                            x.base.base.loc);
                    }
                } else {
                    throw SemanticError("Symbol '" + var_name + "' is not a variable",
                        x.base.base.loc);
                }
            } else {
                throw SemanticError("Variable '" + var_name + "' not declared",
                    x.base.base.loc);
            }
        }
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc,
                r_kind, nullptr, 0));
        tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, r, type);
    }

    void visit_Complex(const AST::Complex_t &x) {
        this->visit_expr(*x.m_re);
        ASR::expr_t *re = LFortran::ASRUtils::EXPR(tmp);
        int a_kind_r = LFortran::ASRUtils::extract_kind_from_ttype_t(LFortran::ASRUtils::expr_type(re));
        this->visit_expr(*x.m_im);
        ASR::expr_t *im = LFortran::ASRUtils::EXPR(tmp);
        int a_kind_i = LFortran::ASRUtils::extract_kind_from_ttype_t(LFortran::ASRUtils::expr_type(im));
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                std::max(a_kind_r, a_kind_i), nullptr, 0));
        tmp = ASR::make_ConstantComplex_t(al, x.base.base.loc,
                re, im, type);
    }

    void visit_ArrayInitializer(const AST::ArrayInitializer_t &x) {
        Vec<ASR::expr_t*> body;
        body.reserve(al, x.n_args);
        ASR::ttype_t *type = nullptr;
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
            if (type == nullptr) {
                type = LFortran::ASRUtils::expr_type(expr);
            } else {
                if (LFortran::ASRUtils::expr_type(expr)->type != type->type) {
                    throw SemanticError("Type mismatch in array initializer",
                        x.base.base.loc);
                }
            }
            body.push_back(al, expr);
        }
        tmp = ASR::make_ConstantArray_t(al, x.base.base.loc, body.p,
            body.size(), type);
    }

    void visit_Print(const AST::Print_t &x) {
        Vec<ASR::expr_t*> body;
        body.reserve(al, x.n_values);
        for (size_t i=0; i<x.n_values; i++) {
            visit_expr(*x.m_values[i]);
            ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
            body.push_back(al, expr);
        }
        tmp = ASR::make_Print_t(al, x.base.base.loc, nullptr,
            body.p, body.size());
    }

    void visit_If(const AST::If_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = LFortran::ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, LFortran::ASRUtils::STMT(tmp));
            }
        }
        Vec<ASR::stmt_t*> orelse;
        orelse.reserve(al, x.n_orelse);
        for (size_t i=0; i<x.n_orelse; i++) {
            visit_stmt(*x.m_orelse[i]);
            if (tmp != nullptr) {
                orelse.push_back(al, LFortran::ASRUtils::STMT(tmp));
            }
        }
        tmp = ASR::make_If_t(al, x.base.base.loc, test, body.p,
                body.size(), orelse.p, orelse.size());
    }

    void visit_WhileLoop(const AST::WhileLoop_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = LFortran::ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, LFortran::ASRUtils::STMT(tmp));
            }
        }
        tmp = ASR::make_WhileLoop_t(al, x.base.base.loc, test, body.p,
                body.size());
    }

    void visit_ImpliedDoLoop(const AST::ImpliedDoLoop_t& x) {
        Vec<ASR::expr_t*> a_values_vec;
        ASR::expr_t *a_start, *a_end, *a_increment;
        a_start = a_end = a_increment = nullptr;
        a_values_vec.reserve(al, x.n_values);
        for( size_t i = 0; i < x.n_values; i++ ) {
            this->visit_expr(*(x.m_values[i]));
            a_values_vec.push_back(al, LFortran::ASRUtils::EXPR(tmp));
        }
        this->visit_expr(*(x.m_start));
        a_start = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*(x.m_end));
        a_end = LFortran::ASRUtils::EXPR(tmp);
        if( x.m_increment != nullptr ) {
            this->visit_expr(*(x.m_increment));
            a_increment = LFortran::ASRUtils::EXPR(tmp);
        }
        ASR::expr_t** a_values = a_values_vec.p;
        size_t n_values = a_values_vec.size();
        // std::string a_var_name = std::to_string(iloop_counter) + std::string(x.m_var);
        // iloop_counter += 1;
        // Str a_var_name_f;
        // a_var_name_f.from_str(al, a_var_name);
        // ASR::asr_t* a_variable = ASR::make_Variable_t(al, x.base.base.loc, current_scope, a_var_name_f.c_str(al),
        //                                                 ASR::intentType::Local, nullptr,
        //                                                 ASR::storage_typeType::Default, LFortran::ASRUtils::expr_type(a_start),
        //                                                 ASR::abiType::Source, ASR::Public);
        LFORTRAN_ASSERT(current_scope->scope.find(std::string(x.m_var)) != current_scope->scope.end());
        ASR::symbol_t* a_sym = current_scope->scope[std::string(x.m_var)];
        // current_scope->scope[a_var_name] = a_sym;
        ASR::expr_t* a_var = LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, a_sym));
        tmp = ASR::make_ImpliedDoLoop_t(al, x.base.base.loc, a_values, n_values,
                                            a_var, a_start, a_end, a_increment,
                                            LFortran::ASRUtils::expr_type(a_start), nullptr);
    }

    void visit_DoLoop(const AST::DoLoop_t &x) {
        if (! x.m_var) {
            throw SemanticError("Do loop: loop variable is required for now",
                x.base.base.loc);
        }
        if (! x.m_start) {
            throw SemanticError("Do loop: start condition required for now",
                x.base.base.loc);
        }
        if (! x.m_end) {
            throw SemanticError("Do loop: end condition required for now",
                x.base.base.loc);
        }
        ASR::expr_t *var = LFortran::ASRUtils::EXPR(resolve_variable(x.base.base.loc, x.m_var));
        visit_expr(*x.m_start);
        ASR::expr_t *start = LFortran::ASRUtils::EXPR(tmp);
        visit_expr(*x.m_end);
        ASR::expr_t *end = LFortran::ASRUtils::EXPR(tmp);
        ASR::expr_t *increment;
        if (x.m_increment) {
            visit_expr(*x.m_increment);
            increment = LFortran::ASRUtils::EXPR(tmp);
        } else {
            increment = nullptr;
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, LFortran::ASRUtils::STMT(tmp));
            }
        }
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        head.loc = head.m_v->base.loc;
        tmp = ASR::make_DoLoop_t(al, x.base.base.loc, head, body.p,
                body.size());
    }

    void visit_DoConcurrentLoop(const AST::DoConcurrentLoop_t &x) {
        if (x.n_control != 1) {
            throw SemanticError("Do concurrent: exactly one control statement is required for now",
            x.base.base.loc);
        }
        AST::ConcurrentControl_t &h = *(AST::ConcurrentControl_t*) x.m_control[0];
        if (! h.m_var) {
            throw SemanticError("Do loop: loop variable is required for now",
                x.base.base.loc);
        }
        if (! h.m_start) {
            throw SemanticError("Do loop: start condition required for now",
                x.base.base.loc);
        }
        if (! h.m_end) {
            throw SemanticError("Do loop: end condition required for now",
                x.base.base.loc);
        }
        ASR::expr_t *var = LFortran::ASRUtils::EXPR(resolve_variable(x.base.base.loc, h.m_var));
        visit_expr(*h.m_start);
        ASR::expr_t *start = LFortran::ASRUtils::EXPR(tmp);
        visit_expr(*h.m_end);
        ASR::expr_t *end = LFortran::ASRUtils::EXPR(tmp);
        ASR::expr_t *increment;
        if (h.m_increment) {
            visit_expr(*h.m_increment);
            increment = LFortran::ASRUtils::EXPR(tmp);
        } else {
            increment = nullptr;
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, LFortran::ASRUtils::STMT(tmp));
            }
        }
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        head.loc = head.m_v->base.loc;
        tmp = ASR::make_DoConcurrentLoop_t(al, x.base.base.loc, head, body.p,
                body.size());
    }

    void visit_Exit(const AST::Exit_t &x) {
        // TODO: add a check here that we are inside a While loop
        tmp = ASR::make_Exit_t(al, x.base.base.loc);
    }

    void visit_Cycle(const AST::Cycle_t &x) {
        // TODO: add a check here that we are inside a While loop
        tmp = ASR::make_Cycle_t(al, x.base.base.loc);
    }

    void visit_Continue(const AST::Continue_t &/*x*/) {
        // TODO: add a check here that we are inside a While loop
        // Nothing to generate, we return a null pointer
        tmp = nullptr;
    }

    void visit_Stop(const AST::Stop_t &x) {
        ASR::expr_t *code;
        if (x.m_code) {
            visit_expr(*x.m_code);
            code = LFortran::ASRUtils::EXPR(tmp);
        } else {
            code = nullptr;
        }
        tmp = ASR::make_Stop_t(al, x.base.base.loc, code);
    }

    void visit_ErrorStop(const AST::ErrorStop_t &x) {
        ASR::expr_t *code;
        if (x.m_code) {
            visit_expr(*x.m_code);
            code = LFortran::ASRUtils::EXPR(tmp);
        } else {
            code = nullptr;
        }
        tmp = ASR::make_ErrorStop_t(al, x.base.base.loc, code);
    }

};

ASR::TranslationUnit_t *body_visitor(Allocator &al,
        AST::TranslationUnit_t &ast, ASR::asr_t *unit)
{
    BodyVisitor b(al, unit);
    b.visit_TranslationUnit(ast);
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    return tu;
}

} // namespace LFortran
