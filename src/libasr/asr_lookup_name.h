#ifndef ASR_LOOKUP_NAME_H
#define ASR_LOOKUP_NAME_H

#include <iostream>
#include <stdint.h>
#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/asr_builder.h>
#include <libasr/lsp_interface.h>
#include <libasr/asr_lookup_name_visitor.h>

namespace LCompilers::LFortran {
    class LookupNameVisitor : public ASR::DefaultLookupNameVisitor<LookupNameVisitor> {
        public:
            LookupNameVisitor(uint16_t pos) {
                this->pos = pos;
            }
            void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
                if ((bool&)x) { } // Suppress unused warning
                if (test_loc_and_set_span(x.base.base.loc)) {
                    const ASR::symbol_t* sym = this->symbol_get_past_external_(x.m_external);
                    this->handle_symbol(sym);
                    if ( ASR::is_a<ASR::ClassProcedure_t>(*sym) ) {
                        this->handle_symbol(ASR::down_cast<ASR::ClassProcedure_t>(sym)->m_proc);
                    }
                }
            }
            void visit_FunctionCall(const ASR::FunctionCall_t &x) {
                for (size_t i=0; i<x.n_args; i++) {
                    this->visit_call_arg(x.m_args[i]);
                }
                this->visit_ttype(*x.m_type);
                if (x.m_value)
                    this->visit_expr(*x.m_value);
                if (x.m_dt)
                    this->visit_expr(*x.m_dt);
                if (test_loc_and_set_span(x.base.base.loc)) {
                    const ASR::symbol_t* sym = this->symbol_get_past_external_(x.m_name);
                    this->handle_symbol(sym);
                    if ( ASR::is_a<ASR::ClassProcedure_t>(*sym) ) {
                        this->handle_symbol(ASR::down_cast<ASR::ClassProcedure_t>(sym)->m_proc);
                    }
                }
            }
    };


    class OccurenceCollector: public ASR::BaseWalkVisitor<OccurenceCollector> {
        public:
            std::string symbol_name;
            std::vector<document_symbols> &symbol_lists;
            LCompilers::LocationManager lm;
            OccurenceCollector(std::string symbol_name, std::vector<document_symbols> &symbol_lists,
                LCompilers::LocationManager lm) : symbol_lists(symbol_lists) {
                this->symbol_name = symbol_name;
                this->lm = lm;
            }

            void populate_document_symbol_and_push(const Location& loc, ASR::symbolType type) {
                document_symbols loc_;
                uint32_t first_line;
                uint32_t last_line;
                uint32_t first_column;
                uint32_t last_column;
                std::string filename;
                lm.pos_to_linecol(loc.first, first_line,
                    first_column, filename);
                lm.pos_to_linecol(loc.last, last_line,
                    last_column, filename);
                loc_.first_column = first_column;
                loc_.last_column = last_column + 1;
                loc_.first_line = first_line;
                loc_.last_line = last_line;
                loc_.symbol_name = symbol_name;
                loc_.filename = filename;
                loc_.symbol_type = type;
                symbol_lists.push_back(loc_);
            }

            void visit_symbol(const ASR::symbol_t& x) {
                ASR::symbol_t* sym = const_cast<ASR::symbol_t*>(&x);
                if ( ASRUtils::symbol_name(sym) == symbol_name ) {
                    if ( ASR::is_a<ASR::Function_t>(*sym) ) {
                        ASR::Function_t* f = ASR::down_cast<ASR::Function_t>(sym);
                        if ( f->m_start_name ) {
                            this->populate_document_symbol_and_push(*(f->m_start_name), x.type);
                        }
                        if ( f->m_end_name ) {
                            this->populate_document_symbol_and_push(*(f->m_end_name), x.type);
                        }
                    } else if ( ASR::is_a<ASR::Program_t>(*sym) ) {
                        ASR::Program_t* p = ASR::down_cast<ASR::Program_t>(sym);
                        if ( p->m_start_name ) {
                            this->populate_document_symbol_and_push(*(p->m_start_name), x.type);
                        }
                        if ( p->m_end_name ) {
                            this->populate_document_symbol_and_push(*(p->m_end_name), x.type);
                        }
                    } else if ( ASR::is_a<ASR::Module_t>(*sym) ) {
                        ASR::Module_t* m = ASR::down_cast<ASR::Module_t>(sym);
                        if ( m->m_start_name ) {
                            this->populate_document_symbol_and_push(*(m->m_start_name), x.type);
                        }
                        if ( m->m_end_name ) {
                            this->populate_document_symbol_and_push(*(m->m_end_name), x.type);
                        }
                    } else {
                        this->populate_document_symbol_and_push(x.base.loc, x.type);
                    }
                }
                ASR::BaseWalkVisitor<OccurenceCollector>::visit_symbol(x);
            }

            void visit_Var(const ASR::Var_t& x) {
                if ( ASRUtils::symbol_name(x.m_v) == symbol_name ) {
                    if ( ASR::is_a<ASR::Function_t>(*x.m_v) ) {
                        ASR::Function_t* f = ASR::down_cast<ASR::Function_t>(x.m_v);
                        if ( f->m_start_name ) {
                            this->populate_document_symbol_and_push(*(f->m_start_name), x.m_v->type);
                        }
                        if ( f->m_end_name ) {
                            this->populate_document_symbol_and_push(*(f->m_end_name), x.m_v->type);
                        }
                    } else if ( ASR::is_a<ASR::Program_t>(*x.m_v) ) {
                        ASR::Program_t* p = ASR::down_cast<ASR::Program_t>(x.m_v);
                        if ( p->m_start_name ) {
                            this->populate_document_symbol_and_push(*(p->m_start_name), x.m_v->type);
                        }
                        if ( p->m_end_name ) {
                            this->populate_document_symbol_and_push(*(p->m_end_name), x.m_v->type);
                        }
                    } else if ( ASR::is_a<ASR::Module_t>(*x.m_v) ) {
                        ASR::Module_t* m = ASR::down_cast<ASR::Module_t>(x.m_v);
                        if ( m->m_start_name ) {
                            this->populate_document_symbol_and_push(*(m->m_start_name), x.m_v->type);
                        }
                        if ( m->m_end_name ) {
                            this->populate_document_symbol_and_push(*(m->m_end_name), x.m_v->type);
                        }
                    } else {
                        this->populate_document_symbol_and_push(x.base.base.loc, x.m_v->type);
                    }
                }
                ASR::BaseWalkVisitor<OccurenceCollector>::visit_Var(x);
            }

            // We need this visitors because we want to use the
            // overwritten `visit_symbol` and not `this->visit_symbol`
            // in BaseWalkVisitor we have `this->visit_symbol` which
            // prevents us from using the overwritten `visit_symbol`
            void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
                for (auto &a : x.m_symtab->get_scope()) {
                    visit_symbol(*a.second);
                }
            }
            void visit_Program(const ASR::Program_t &x) {
                for (auto &a : x.m_symtab->get_scope()) {
                    visit_symbol(*a.second);
                }
                for (size_t i=0; i<x.n_body; i++) {
                    visit_stmt(*x.m_body[i]);
                }
            }
            void visit_FunctionCall(const ASR::FunctionCall_t &x) {
                for (size_t i=0; i<x.n_args; i++) {
                    this->visit_call_arg(x.m_args[i]);
                }
                if ( ASRUtils::symbol_name(x.m_name) == symbol_name ) {
                    this->populate_document_symbol_and_push(x.base.base.loc, ASR::symbolType::Function);
                }
                this->visit_ttype(*x.m_type);
                if (x.m_value && visit_compile_time_value)
                    this->visit_expr(*x.m_value);
                if (x.m_dt)
                    this->visit_expr(*x.m_dt);
            }
            void visit_Module(const ASR::Module_t &x) {
                for (auto &a : x.m_symtab->get_scope()) {
                    visit_symbol(*a.second);
                }
            }
            void visit_Function(const ASR::Function_t &x) {
                for (auto &a : x.m_symtab->get_scope()) {
                    visit_symbol(*a.second);
                }
                visit_ttype(*x.m_function_signature);
                for (size_t i=0; i<x.n_args; i++) {
                    visit_expr(*x.m_args[i]);
                }
                for (size_t i=0; i<x.n_body; i++) {
                    visit_stmt(*x.m_body[i]);
                }
                if (x.m_return_var)
                    visit_expr(*x.m_return_var);
            }
            void visit_Struct(const ASR::Struct_t &x) {
                for (auto &a : x.m_symtab->get_scope()) {
                    visit_symbol(*a.second);
                }
                for (size_t i=0; i<x.n_initializers; i++) {
                    visit_call_arg(x.m_initializers[i]);
                }
                if (x.m_alignment)
                    visit_expr(*x.m_alignment);
            }
            void visit_Enum(const ASR::Enum_t &x) {
                for (auto &a : x.m_symtab->get_scope()) {
                    visit_symbol(*a.second);
                }
                visit_ttype(*x.m_type);
            }
            void visit_Union(const ASR::Union_t &x) {
                for (auto &a : x.m_symtab->get_scope()) {
                    this->visit_symbol(*a.second);
                }
                for (size_t i=0; i<x.n_initializers; i++) {
                    visit_call_arg(x.m_initializers[i]);
                }
            }
    };

}

#endif // ASR_LOOKUP_NAME_H
