#ifndef LFORTRAN_PASS_UTILS_H
#define LFORTRAN_PASS_UTILS_H

#include <libasr/asr.h>
#include <libasr/containers.h>

namespace LFortran {

    namespace PassUtils {

        bool is_array(ASR::expr_t* x);

        void get_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims);

        ASR::ttype_t* set_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims,
                                    bool create_new=false, Allocator* al=nullptr);

        int get_rank(ASR::expr_t* x);

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars, Allocator& al);

        ASR::expr_t* create_array_ref(ASR::symbol_t* arr, Vec<ASR::expr_t*>& idx_vars, Allocator& al,
                                      const Location& loc, ASR::ttype_t* _type);

        void create_vars(Vec<ASR::expr_t*>& vars, int n_vars, const Location& loc,
                         Allocator& al, SymbolTable*& current_scope, std::string suffix="_k",
                         ASR::intentType intent=ASR::intentType::Local);

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc,
                             Allocator& al, SymbolTable*& current_scope, std::string suffix="_k");

        ASR::expr_t* create_compare_helper(Allocator &al, const Location &loc, ASR::expr_t* left, ASR::expr_t* right,
                                            ASR::cmpopType op);

        ASR::expr_t* create_binop_helper(Allocator &al, const Location &loc, ASR::expr_t* left, ASR::expr_t* right,
                                            ASR::binopType op);

        ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim, std::string bound,
                                Allocator& al);


        ASR::stmt_t* get_flipsign(ASR::expr_t* arg0, ASR::expr_t* arg1,
                                  Allocator& al, ASR::TranslationUnit_t& unit,
                                  LCompilers::PassOptions& pass_options,
                                  SymbolTable*& current_scope,
                                  const std::function<void (const std::string &, const Location &)> err);

        ASR::expr_t* to_int32(ASR::expr_t* x, ASR::ttype_t* int32type, Allocator& al);

        ASR::expr_t* create_auxiliary_variable_for_expr(ASR::expr_t* expr, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::stmt_t*& assign_stmt);

        ASR::expr_t* create_auxiliary_variable(Location& loc, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::ttype_t* var_type);

        ASR::expr_t* get_fma(ASR::expr_t* arg0, ASR::expr_t* arg1, ASR::expr_t* arg2,
                             Allocator& al, ASR::TranslationUnit_t& unit, LCompilers::PassOptions& pass_options,
                             SymbolTable*& current_scope,Location& loc,
                             const std::function<void (const std::string &, const Location &)> err);

        ASR::expr_t* get_sign_from_value(ASR::expr_t* arg0, ASR::expr_t* arg1,
                                         Allocator& al, ASR::TranslationUnit_t& unit,
                                         LCompilers::PassOptions& pass_options,
                                         SymbolTable*& current_scope, Location& loc,
                                         const std::function<void (const std::string &, const Location &)> err);

        ASR::stmt_t* get_vector_copy(ASR::expr_t* array0, ASR::expr_t* array1, ASR::expr_t* start,
            ASR::expr_t* end, ASR::expr_t* step, ASR::expr_t* vector_length,
            Allocator& al, ASR::TranslationUnit_t& unit,
            SymbolTable*& global_scope, Location& loc);

        Vec<ASR::stmt_t*> replace_doloop(Allocator &al, const ASR::DoLoop_t &loop,
                                         int comp=-1);

        static inline bool is_aggregate_type(ASR::expr_t* var) {
            return ASR::is_a<ASR::Struct_t>(*ASRUtils::expr_type(var));
        }

        template <class Struct>
        class PassVisitor: public ASR::BaseWalkVisitor<Struct> {

            private:

                Struct& self() { return static_cast<Struct&>(*this); }

            public:

                bool asr_changed, retain_original_stmt, remove_original_stmt;
                Allocator& al;
                Vec<ASR::stmt_t*> pass_result;
                SymbolTable* current_scope;

                PassVisitor(Allocator& al_, SymbolTable* current_scope_): al{al_},
                current_scope{current_scope_} {
                    pass_result.n = 0;
                }

                void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
                    Vec<ASR::stmt_t*> body;
                    body.reserve(al, n_body);
                    if (pass_result.size() > 0) {
                        asr_changed = true;
                        for (size_t j=0; j < pass_result.size(); j++) {
                            body.push_back(al, pass_result[j]);
                        }
                        pass_result.n = 0;
                    }
                    for (size_t i=0; i<n_body; i++) {
                        // Not necessary after we check it after each visit_stmt in every
                        // visitor method:
                        pass_result.n = 0;
                        retain_original_stmt = false;
                        remove_original_stmt = false;
                        self().visit_stmt(*m_body[i]);
                        if (pass_result.size() > 0) {
                            asr_changed = true;
                            for (size_t j=0; j < pass_result.size(); j++) {
                                body.push_back(al, pass_result[j]);
                            }
                            if( retain_original_stmt ) {
                                body.push_back(al, m_body[i]);
                                retain_original_stmt = false;
                            }
                            pass_result.n = 0;
                        } else if(!remove_original_stmt) {
                            body.push_back(al, m_body[i]);
                        }
                    }
                    m_body = body.p;
                    n_body = body.size();
                }

                void visit_Program(const ASR::Program_t &x) {
                    // FIXME: this is a hack, we need to pass in a non-const `x`,
                    // which requires to generate a TransformVisitor.
                    ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
                    current_scope = xx.m_symtab;
                    transform_stmts(xx.m_body, xx.n_body);

                    // Transform nested functions and subroutines
                    for (auto &item : x.m_symtab->get_scope()) {
                        if (ASR::is_a<ASR::Function_t>(*item.second)) {
                            ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                            self().visit_Function(*s);
                        }
                        if (ASR::is_a<ASR::AssociateBlock_t>(*item.second)) {
                            ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                            self().visit_AssociateBlock(*s);
                        }
                        if (ASR::is_a<ASR::Block_t>(*item.second)) {
                            ASR::Block_t *s = ASR::down_cast<ASR::Block_t>(item.second);
                            self().visit_Block(*s);
                        }
                    }
                }

                void visit_Function(const ASR::Function_t &x) {
                    // FIXME: this is a hack, we need to pass in a non-const `x`,
                    // which requires to generate a TransformVisitor.
                    ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
                    current_scope = xx.m_symtab;
                    transform_stmts(xx.m_body, xx.n_body);

                    for (auto &item : x.m_symtab->get_scope()) {
                        if (ASR::is_a<ASR::Block_t>(*item.second)) {
                            ASR::Block_t *s = ASR::down_cast<ASR::Block_t>(item.second);
                            self().visit_Block(*s);
                        }
                    }
                }

                void visit_AssociateBlock(const ASR::AssociateBlock_t& x) {
                    ASR::AssociateBlock_t &xx = const_cast<ASR::AssociateBlock_t&>(x);
                    current_scope = xx.m_symtab;
                    transform_stmts(xx.m_body, xx.n_body);
                }

                void visit_Block(const ASR::Block_t& x) {
                    ASR::Block_t &xx = const_cast<ASR::Block_t&>(x);
                    current_scope = xx.m_symtab;
                    transform_stmts(xx.m_body, xx.n_body);

                    for (auto &item : x.m_symtab->get_scope()) {
                        self().visit_symbol(*item.second);
                    }
                }

                void visit_DoLoop(const ASR::DoLoop_t& x) {
                    self().visit_do_loop_head(x.m_head);
                    ASR::DoLoop_t& xx = const_cast<ASR::DoLoop_t&>(x);
                    transform_stmts(xx.m_body, xx.n_body);
                }

        };

        template <class Struct>
        class SkipOptimizationFunctionVisitor: public PassVisitor<Struct> {

            public:

                SkipOptimizationFunctionVisitor(Allocator& al_): PassVisitor<Struct>(al_, nullptr) {
                }

                void visit_Function(const ASR::Function_t &x) {
                    if( ASRUtils::is_intrinsic_optimization<ASR::Function_t>(&x) ) {
                        return ;
                    }
                    PassUtils::PassVisitor<Struct>::visit_Function(x);
                }

        };

        class UpdateDependenciesVisitor : public ASR::BaseWalkVisitor<UpdateDependenciesVisitor> {

            private:

                Vec<char*> function_dependencies;
                Vec<char*> module_dependencies;
                Allocator& al;
                bool fill_function_dependencies;
                bool fill_module_dependencies;

            public:

                UpdateDependenciesVisitor(Allocator &al_)
                : al(al_), fill_function_dependencies(false),
                fill_module_dependencies(false)
                {}

                void visit_Function(const ASR::Function_t& x) {
                    ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
                    function_dependencies.n = 0;
                    function_dependencies.reserve(al, 1);
                    bool fill_function_dependencies_copy = fill_function_dependencies;
                    fill_function_dependencies = true;
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_Function(x);
                    xx.m_dependencies = function_dependencies.p;
                    xx.n_dependencies = function_dependencies.size();
                    fill_function_dependencies = fill_function_dependencies_copy;
                }

                void visit_Module(const ASR::Module_t& x) {
                    ASR::Module_t& xx = const_cast<ASR::Module_t&>(x);
                    module_dependencies.n = 0;
                    module_dependencies.from_pointer_n_copy(al, xx.m_dependencies, xx.n_dependencies);
                    bool fill_module_dependencies_copy = fill_module_dependencies;
                    fill_module_dependencies = true;
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_Module(x);
                    xx.n_dependencies = module_dependencies.size();
                    xx.m_dependencies = module_dependencies.p;
                    fill_module_dependencies = fill_module_dependencies_copy;
                }

                void visit_FunctionCall(const ASR::FunctionCall_t& x) {
                    if( fill_function_dependencies ) {
                        function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                    }
                    if( ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) &&
                        fill_module_dependencies ) {
                        ASR::ExternalSymbol_t* x_m_name = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
                        module_dependencies.push_back(al, x_m_name->m_module_name);
                    }
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_FunctionCall(x);
                }

                void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
                    if( fill_function_dependencies ) {
                        function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                    }
                    if( ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) &&
                        fill_module_dependencies ) {
                        ASR::ExternalSymbol_t* x_m_name = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
                        module_dependencies.push_back(al, x_m_name->m_module_name);
                    }
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_SubroutineCall(x);
                }
        };

    }

} // namespace LFortran

#endif // LFORTRAN_PASS_UTILS_H
