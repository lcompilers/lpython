#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <cmath>

#include <lpython/ast.h>
#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/exception.h>
#include <lpython/semantics/asr_implicit_cast_rules.h>
#include <lpython/semantics/ast_common_visitor.h>
#include <lpython/semantics/ast_to_asr.h>
#include <lpython/parser/parser_stype.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>

namespace LFortran {

class BodyVisitor : public CommonVisitor<BodyVisitor> {
private:

public:
    ASR::asr_t *asr;
    Vec<ASR::stmt_t*> *current_body;

    BodyVisitor(Allocator &al, ASR::asr_t *unit, diag::Diagnostics &diagnostics)
         : CommonVisitor(al, nullptr, diagnostics), asr{unit} {}

    void visit_Declaration(const AST::Declaration_t & /* x */){
        // Already visited this AST node in the SymbolTableVisitor
    };

    // Transforms statements to a list of ASR statements
    // In addition, it also inserts the following nodes if needed:
    //   * ImplicitDeallocate
    //   * GoToTarget
    // The `body` Vec must already be reserved
    void transform_stmts(Vec<ASR::stmt_t*> &body, size_t n_body, AST::stmt_t **m_body) {
        tmp = nullptr;
        for (size_t i=0; i<n_body; i++) {
            // If there is a label, create a GoToTarget node first
            int64_t label = stmt_label(m_body[i]);
            if (label != 0) {
                ASR::asr_t *l = ASR::make_GoToTarget_t(al, m_body[i]->base.loc, label);
                body.push_back(al, ASR::down_cast<ASR::stmt_t>(l));
            }
            // Visit the statement
            this->visit_stmt(*m_body[i]);
            if (tmp != nullptr) {
                ASR::stmt_t* tmp_stmt = LFortran::ASRUtils::STMT(tmp);
                if (tmp_stmt->type == ASR::stmtType::SubroutineCall) {
                    ASR::stmt_t* impl_decl = create_implicit_deallocate_subrout_call(tmp_stmt);
                    if (impl_decl != nullptr) {
                        body.push_back(al, impl_decl);
                    }
                }
                body.push_back(al, tmp_stmt);
            }
            // To avoid last statement to be entered twice once we exit this node
            tmp = nullptr;
        }
    }

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
                    ( !ASRUtils::is_integer(*a_newunit_type))
                    ) {
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
                if (!ASRUtils::is_character(*a_filename_type)) {
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
                if (!ASRUtils::is_character(*a_status_type)) {
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
                if (!ASRUtils::is_integer(*a_newunit_type)) {
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
                    (!ASRUtils::is_integer(*a_iostat_type)) ) {
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
                    (!ASRUtils::is_character(*a_iomsg_type))) {
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
                if (!ASRUtils::is_character(*a_status_type)) {
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
                if (kwarg.m_value != nullptr) {
                    this->visit_expr(*kwarg.m_value);
                    a_unit = LFortran::ASRUtils::EXPR(tmp);
                    ASR::ttype_t* a_unit_type = LFortran::ASRUtils::expr_type(a_unit);
                    if  (!ASRUtils::is_integer(*a_unit_type)) {
                            throw SemanticError("`unit` must be of type, Integer", loc);
                    }
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
                    (!ASRUtils::is_integer(*a_iostat_type)) ) {
                        throw SemanticError("`iostat` must be of type, Integer", loc);
                }
            } else if( m_arg_str == std::string("iomsg") ) {
                if( a_iomsg != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iomsg` found, it has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iomsg = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_iomsg_type = LFortran::ASRUtils::expr_type(a_iomsg);
                if( a_iomsg->type != ASR::exprType::Var ||
                   (!ASRUtils::is_character(*a_iomsg_type)) ) {
                        throw SemanticError("`iomsg` must be of type, Character", loc);
                    }
            } else if( m_arg_str == std::string("id") ) {
                if( a_id != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `id` found, it has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_id = LFortran::ASRUtils::EXPR(tmp);
                ASR::ttype_t* a_status_type = LFortran::ASRUtils::expr_type(a_id);
                if (!ASRUtils::is_character(*a_status_type)) {
                        throw SemanticError("`status` must be of type Character", loc);
                }
            } else if( m_arg_str == std::string("fmt") ) {
                if( a_fmt != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `fmt` found, it has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                if (kwarg.m_value != nullptr) {
                    tmp = nullptr;
                    this->visit_expr(*kwarg.m_value);
                    if (tmp == nullptr) {
                        throw SemanticError("?", loc);
                    }
                    a_fmt = LFortran::ASRUtils::EXPR(tmp);
                    ASR::ttype_t* a_fmt_type = LFortran::ASRUtils::expr_type(a_fmt);
                    if (!ASRUtils::is_character(*a_fmt_type)) {
                            throw SemanticError("`fmt` must be of type Character", loc);
                    }
                }
            }
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
            std::string name = to_lower(x.m_syms[i].m_name);
            char *name_c = s2c(al, name);
            ASR::asr_t *v = ASR::make_Variable_t(al, x.base.base.loc, new_scope,
                                                 name_c, ASR::intentType::AssociateBlock,
                                                 LFortran::ASRUtils::EXPR(tmp), nullptr,
                                                 ASR::storage_typeType::Default,
                                                 nullptr, ASR::abiType::Source,
                                                 ASR::accessType::Private,
                                                 ASR::presenceType::Required,
                                                 false);
            new_scope->scope[name_c] = ASR::down_cast<ASR::symbol_t>(v);
        }
        SymbolTable* current_scope_copy = current_scope;
        current_scope = new_scope;
        transform_stmts(*current_body, x.n_body, x.m_body);
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
          ( x.n_keywords == 1 && to_lower(x.m_keywords[0].m_arg) != "stat") ) {
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
                        // If it is not allocatable, it can also be a pointer
                        if (ASR::is_a<ASR::Pointer_t>(*tmp_v->m_type)) {
                            // OK
                        } else {
                            throw SemanticError("Only an allocatable or a pointer variable "
                                                "can be deallocated.",
                                                tmp_expr->base.loc);
                        }
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
                    transform_stmts(case_body_vec, Case_Stmt->n_body, Case_Stmt->m_body);
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
                    transform_stmts(case_body_vec, Case_Stmt->n_body, Case_Stmt->m_body);
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
                transform_stmts(def_body, d->n_body, d->m_body);
            } else {
                this->visit_case_stmt(*body);
                a_body_vec.push_back(al, ASR::down_cast<ASR::case_stmt_t>(tmp));
            }
        }
        tmp = ASR::make_Select_t(al, x.base.base.loc, a_test, a_body_vec.p,
                           a_body_vec.size(), def_body.p, def_body.size());
    }

    void visit_Submodule(const AST::Submodule_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[to_lower(x.m_name)];
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

    void visit_Module(const AST::Module_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[to_lower(x.m_name)];
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
        ASR::symbol_t *t = current_scope->scope[to_lower(x.m_name)];
        ASR::Program_t *v = ASR::down_cast<ASR::Program_t>(t);
        current_scope = v->m_symtab;

        Vec<ASR::stmt_t*> body;
        current_body = &body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
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
                        orig_var->m_storage == ASR::storage_typeType::Allocatable &&
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
        ASR::symbol_t *t = current_scope->scope[to_lower(x.m_name)];
        if( t->type == ASR::symbolType::GenericProcedure ) {
            std::string subrout_name = "~" + to_lower(x.m_name);
            t = current_scope->scope[subrout_name];
        }
        ASR::Subroutine_t *v = ASR::down_cast<ASR::Subroutine_t>(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
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
        ASR::symbol_t *t = current_scope->scope[to_lower(x.m_name)];
        ASR::Function_t *v = ASR::down_cast<ASR::Function_t>(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
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
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = LFortran::ASRUtils::EXPR(tmp);
        ASR::stmt_t *overloaded_stmt = nullptr;
        if( LFortran::ASRUtils::use_overloaded_assignment(target, value,
            current_scope, asr, al, x.base.base.loc) ) {
            overloaded_stmt = LFortran::ASRUtils::STMT(asr);
        }
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

        ASR::ttype_t *value_type = ASRUtils::expr_type(value);
        if( target->type == ASR::exprType::Var && !ASRUtils::is_array(target_type) &&
            value->type == ASR::exprType::ConstantArray ) {
            throw SemanticError("ArrayInitalizer expressions can only be assigned array references", x.base.base.loc);
        }
        if (target->type == ASR::exprType::Var ||
            target->type == ASR::exprType::ArrayRef) {

            ImplicitCastRules::set_converted_value(al, x.base.base.loc, &value,
                                                    value_type, target_type);

        }
        if (!ASRUtils::check_equal_type(ASRUtils::expr_type(target),
                                    ASRUtils::expr_type(value))) {
            std::string ltype = ASRUtils::type_to_str(ASRUtils::expr_type(target));
            std::string rtype = ASRUtils::type_to_str(ASRUtils::expr_type(value));
            diag.semantic_error_label(
                "Type mismatch in assignment, the types must be compatible",
                {target->base.loc, value->base.loc},
                "type mismatch (" + ltype + " and " + rtype + ")"
            );
            throw SemanticAbort();
        }
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value,
                                     overloaded_stmt, false);
    }

    void visit_SubroutineCall(const AST::SubroutineCall_t &x) {
        SymbolTable* scope = current_scope;
        std::string sub_name = to_lower(x.m_name);
        ASR::symbol_t *original_sym;
        ASR::expr_t *v_expr = nullptr;
        // If this is a type bound procedure (in a class) it won't be in the
        // main symbol table. Need to check n_member.
        if (x.n_member == 1) {
            ASR::symbol_t *v = current_scope->resolve_symbol(to_lower(x.m_member[0].m_name));
            ASR::asr_t *v_var = ASR::make_Var_t(al, x.base.base.loc, v);
            v_expr = LFortran::ASRUtils::EXPR(v_var);
            original_sym = resolve_deriv_type_proc(x.base.base.loc, to_lower(x.m_name),
                to_lower(x.m_member[0].m_name), scope);
        } else {
            original_sym = current_scope->resolve_symbol(sub_name);
        }
        if (!original_sym) {
            original_sym = resolve_intrinsic_function(x.base.base.loc, sub_name);
        }
        Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
        if (x.n_keywords > 0) {
            ASR::symbol_t* f2 = LFortran::ASRUtils::symbol_get_past_external(original_sym);
            if (ASR::is_a<ASR::Subroutine_t>(*f2)) {
                ASR::Subroutine_t *f = ASR::down_cast<ASR::Subroutine_t>(f2);
                visit_kwargs(args, x.m_keywords, x.n_keywords,
                    f->m_args, f->n_args, x.base.base.loc);
            } else {
                throw SemanticError(
                    "Keyword arguments are not implemented for generic subroutines yet",
                    x.base.base.loc);
            }
        }
        Vec<ASR::expr_t*> args_with_mdt;
        if( x.n_member == 1 ) {
            args_with_mdt.reserve(al, x.n_args + 1);
            args_with_mdt.push_back(al, v_expr);
            for( size_t i = 0; i < args.size(); i++ ) {
                args_with_mdt.push_back(al, args[i]);
            }
        }
        ASR::symbol_t *final_sym=nullptr;
        switch (original_sym->type) {
            case (ASR::symbolType::Subroutine) : {
                final_sym=original_sym;
                original_sym = nullptr;
                break;
            }
            case (ASR::symbolType::GenericProcedure) : {
                ASR::GenericProcedure_t *p = ASR::down_cast<ASR::GenericProcedure_t>(original_sym);
                int idx;
                if( x.n_member == 1 ) {
                    idx = select_generic_procedure(args_with_mdt, *p, x.base.base.loc);
                } else {
                    idx = select_generic_procedure(args, *p, x.base.base.loc);
                }
                // Create ExternalSymbol for procedures in different modules.
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
                    std::string local_sym = std::string(to_lower(p->m_name)) + "@"
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
                            p->m_module_name, nullptr, 0, LFortran::ASRUtils::symbol_name(final_sym),
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

    void visit_Print(const AST::Print_t &x) {
        Vec<ASR::expr_t*> body;
        body.reserve(al, x.n_values);
        for (size_t i=0; i<x.n_values; i++) {
            visit_expr(*x.m_values[i]);
            ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
            body.push_back(al, expr);
        }
        ASR::expr_t *fmt=nullptr;
        if (x.m_fmt != nullptr) {
            this->visit_expr(*x.m_fmt);
            fmt = ASRUtils::EXPR(tmp);
        }
        tmp = ASR::make_Print_t(al, x.base.base.loc, fmt,
            body.p, body.size());
    }

    void visit_If(const AST::If_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = LFortran::ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        Vec<ASR::stmt_t*> orelse;
        orelse.reserve(al, x.n_orelse);
        transform_stmts(orelse, x.n_orelse, x.m_orelse);
        tmp = ASR::make_If_t(al, x.base.base.loc, test, body.p,
                body.size(), orelse.p, orelse.size());
    }

    void visit_WhileLoop(const AST::WhileLoop_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = LFortran::ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
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
        std::string var_name = to_lower(x.m_var);
        if (current_scope->scope.find(var_name) == current_scope->scope.end()) {
            throw SemanticError("The implied do loop variable '" + var_name + "' is not declared", x.base.base.loc);
        };

        LFORTRAN_ASSERT(current_scope->scope.find(var_name) != current_scope->scope.end());
        ASR::symbol_t* a_sym = current_scope->scope[var_name];
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
        ASR::expr_t *var = LFortran::ASRUtils::EXPR(resolve_variable(x.base.base.loc, to_lower(x.m_var)));
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
        transform_stmts(body, x.n_body, x.m_body);
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
        ASR::expr_t *var = LFortran::ASRUtils::EXPR(resolve_variable(x.base.base.loc, to_lower(h.m_var)));
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
        transform_stmts(body, x.n_body, x.m_body);
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        head.loc = head.m_v->base.loc;
        tmp = ASR::make_DoConcurrentLoop_t(al, x.base.base.loc, head, body.p,
                body.size());
    }

    void visit_ForAllSingle(const AST::ForAllSingle_t &x) {
        if (x.n_control != 1) {
            throw SemanticError("Forall statement: exactly one control statement is required for now",
            x.base.base.loc);
        }
        AST::ConcurrentControl_t &h = *(AST::ConcurrentControl_t*) x.m_control[0];
        if (! h.m_var) {
            throw SemanticError("Forall statement: loop variable is required",
                x.base.base.loc);
        }
        if (! h.m_start) {
            throw SemanticError("Forall statement: start condition is required",
                x.base.base.loc);
        }
        if (! h.m_end) {
            throw SemanticError("Forall statement: end condition is required",
                x.base.base.loc);
        }
        ASR::expr_t *var = LFortran::ASRUtils::EXPR(
            resolve_variable(x.base.base.loc, to_lower(h.m_var))
        );
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

        ASR::stmt_t* assign_stmt;
        this->visit_stmt(*x.m_assign);
        assign_stmt = LFortran::ASRUtils::STMT(tmp);
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        head.loc = head.m_v->base.loc;
        tmp = ASR::make_ForAllSingle_t(al, x.base.base.loc, head, assign_stmt);
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

    void visit_GoTo(const AST::GoTo_t &x) {
        if (x.m_goto_label) {
            if (AST::is_a<AST::Num_t>(*x.m_goto_label)) {
                int goto_label = AST::down_cast<AST::Num_t>(x.m_goto_label)->m_n;
                tmp = ASR::make_GoTo_t(al, x.base.base.loc, goto_label);
            } else {
                throw SemanticError("A goto label must be an integer",
                    x.base.base.loc);
            }
        } else {
            throw SemanticError("Currently only 'goto INTEGER' is supported",
                x.base.base.loc);
        }
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

    void visit_Nullify(const AST::Nullify_t &x) {
        Vec<ASR::symbol_t*> arg_vec;
        arg_vec.reserve(al, x.n_args);
        for( size_t i = 0; i < x.n_args; i++ ) {
            this->visit_expr(*(x.m_args[i]));
            ASR::expr_t* tmp_expr = LFortran::ASRUtils::EXPR(tmp);
            if( tmp_expr->type != ASR::exprType::Var ) {
                throw SemanticError("Only a pointer variable symbol "
                                    "can be nullified.",
                                    tmp_expr->base.loc);
            } else {
                const ASR::Var_t* tmp_var = ASR::down_cast<ASR::Var_t>(tmp_expr);
                ASR::symbol_t* tmp_sym = tmp_var->m_v;
                if( LFortran::ASRUtils::symbol_get_past_external(tmp_sym)->type 
                    != ASR::symbolType::Variable ) {
                    throw SemanticError("Only a pointer variable symbol "
                                        "can be nullified.",
                                        tmp_expr->base.loc);
                } else {
                    ASR::Variable_t* tmp_v = ASR::down_cast<ASR::Variable_t>(tmp_sym);
                    if (ASR::is_a<ASR::Pointer_t>(*tmp_v->m_type)) {
                        arg_vec.push_back(al, tmp_sym);
                    } else {
                        throw SemanticError("Only a pointer variable symbol "
                                            "can be nullified.",
                                            tmp_expr->base.loc);
                    }
                }
            }
        }
        tmp = ASR::make_Nullify_t(al, x.base.base.loc, arg_vec.p, arg_vec.size());
    }

};

Result<ASR::TranslationUnit_t*> body_visitor(Allocator &al,
        AST::TranslationUnit_t &ast,
        diag::Diagnostics &diagnostics,
        ASR::asr_t *unit)
{
    BodyVisitor b(al, unit, diagnostics);
    try {
        b.visit_TranslationUnit(ast);
    } catch (const SemanticError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    } catch (const SemanticAbort &) {
        Error error;
        return error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    return tu;
}

} // namespace LFortran
