#ifndef LFORTRAN_PASS_UTILS_H
#define LFORTRAN_PASS_UTILS_H

#include <libasr/asr.h>
#include <libasr/containers.h>

#include <queue>

namespace LCompilers {

    namespace PassUtils {

        bool is_array(ASR::expr_t* x);

        void get_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims);

        ASR::ttype_t* set_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims,
                                    bool create_new=false, Allocator* al=nullptr);

        int get_rank(ASR::expr_t* x);

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars, Allocator& al);

        ASR::expr_t* create_array_ref(ASR::symbol_t* arr, Vec<ASR::expr_t*>& idx_vars, Allocator& al,
                                      const Location& loc, ASR::ttype_t* _type);

        static inline bool is_elemental(ASR::symbol_t* x) {
            x = ASRUtils::symbol_get_past_external(x);
            if( !ASR::is_a<ASR::Function_t>(*x) ) {
                return false;
            }
            return ASRUtils::get_FunctionType(
                ASR::down_cast<ASR::Function_t>(x))->m_elemental;
        }

        void fix_dimension(ASR::Cast_t* x, ASR::expr_t* arg_expr);

        ASR::ttype_t* get_matching_type(ASR::expr_t* sibling, Allocator& al);

        ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                                ASR::ttype_t* var_type, Allocator& al, SymbolTable*& current_scope,
                                ASR::storage_typeType storage_=ASR::storage_typeType::Default);

        ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                                ASR::expr_t* sibling, Allocator& al, SymbolTable*& current_scope,
                                ASR::storage_typeType storage_=ASR::storage_typeType::Default);

        void create_vars(Vec<ASR::expr_t*>& vars, int n_vars, const Location& loc,
                         Allocator& al, SymbolTable*& current_scope, std::string suffix="_k",
                         ASR::intentType intent=ASR::intentType::Local,
                         ASR::presenceType presence=ASR::presenceType::Required);

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc,
                             Allocator& al, SymbolTable*& current_scope, std::string suffix="_k");

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, ASR::array_index_t* m_args, int n_dims,
                             std::vector<int>& value_indices, const Location& loc, Allocator& al,
                             SymbolTable*& current_scope, std::string suffix="_k");

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, Vec<ASR::expr_t*>& loop_vars,
                             std::vector<int>& loop_var_indices,
                             Vec<ASR::expr_t*>& vars, Vec<ASR::expr_t*>& incs,
                             const Location& loc, Allocator& al,
                             SymbolTable*& current_scope, std::string suffix="_k");

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

        ASR::expr_t* create_auxiliary_variable(const Location& loc, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::ttype_t* var_type,
            ASR::intentType var_intent=ASR::intentType::Local);

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
        class PassVisitor: public ASR::ASRPassBaseWalkVisitor<Struct> {

            private:

                Struct& self() { return static_cast<Struct&>(*this); }

            public:

                bool asr_changed, retain_original_stmt, remove_original_stmt;
                Allocator& al;
                Vec<ASR::stmt_t*> pass_result;

                PassVisitor(Allocator& al_, SymbolTable* current_scope_): al{al_}
                {
                    this->current_scope = current_scope_;
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
                    SymbolTable* current_scope_copy = this->current_scope;
                    this->current_scope = xx.m_symtab;
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
                    this->current_scope = current_scope_copy;
                }

                void visit_Function(const ASR::Function_t &x) {
                    // FIXME: this is a hack, we need to pass in a non-const `x`,
                    // which requires to generate a TransformVisitor.
                    ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
                    SymbolTable* current_scope_copy = this->current_scope;
                    this->current_scope = xx.m_symtab;
                    self().visit_ttype(*x.m_function_signature);
                    for (size_t i=0; i<x.n_args; i++) {
                        self().visit_expr(*x.m_args[i]);
                    }
                    transform_stmts(xx.m_body, xx.n_body);

                    if (x.m_return_var) {
                        self().visit_expr(*x.m_return_var);
                    }

                    for (auto &item : x.m_symtab->get_scope()) {
                        if (ASR::is_a<ASR::Function_t>(*item.second)) {
                            ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                            self().visit_Function(*s);
                        }
                        if (ASR::is_a<ASR::Block_t>(*item.second)) {
                            ASR::Block_t *s = ASR::down_cast<ASR::Block_t>(item.second);
                            self().visit_Block(*s);
                        }
                        if (ASR::is_a<ASR::AssociateBlock_t>(*item.second)) {
                            ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                            self().visit_AssociateBlock(*s);
                        }
                    }
                    this->current_scope = current_scope_copy;
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

                SetChar function_dependencies;
                SetChar module_dependencies;
                SetChar variable_dependencies;
                Allocator& al;
                bool fill_function_dependencies;
                bool fill_module_dependencies;
                bool fill_variable_dependencies;

            public:

                UpdateDependenciesVisitor(Allocator &al_)
                : al(al_), fill_function_dependencies(false),
                fill_module_dependencies(false),
                fill_variable_dependencies(false)
                {
                    function_dependencies.n = 0;
                    module_dependencies.n = 0;
                    variable_dependencies.n = 0;
                }

                void visit_Function(const ASR::Function_t& x) {
                    ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
                    SetChar function_dependencies_copy;
                    function_dependencies_copy.from_pointer_n_copy(al, function_dependencies.p, function_dependencies.size());
                    function_dependencies.n = 0;
                    function_dependencies.reserve(al, 1);
                    bool fill_function_dependencies_copy = fill_function_dependencies;
                    fill_function_dependencies = true;
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_Function(x);
                    xx.m_dependencies = function_dependencies.p;
                    xx.n_dependencies = function_dependencies.size();
                    fill_function_dependencies = fill_function_dependencies_copy;
                    function_dependencies.from_pointer_n_copy(al,
                        function_dependencies_copy.p,
                        function_dependencies_copy.size()
                    );
                }

                void visit_Module(const ASR::Module_t& x) {
                    ASR::Module_t& xx = const_cast<ASR::Module_t&>(x);
                    module_dependencies.n = 0;
                    module_dependencies.reserve(al, 1);
                    bool fill_module_dependencies_copy = fill_module_dependencies;
                    fill_module_dependencies = true;
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_Module(x);
                    for( size_t i = 0; i < xx.n_dependencies; i++ ) {
                        module_dependencies.push_back(al, xx.m_dependencies[i]);
                    }
                    xx.n_dependencies = module_dependencies.size();
                    xx.m_dependencies = module_dependencies.p;
                    fill_module_dependencies = fill_module_dependencies_copy;
                }

                void visit_Variable(const ASR::Variable_t& x) {
                    ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
                    variable_dependencies.n = 0;
                    variable_dependencies.reserve(al, 1);
                    bool fill_variable_dependencies_copy = fill_variable_dependencies;
                    fill_variable_dependencies = true;
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_Variable(x);
                    xx.n_dependencies = variable_dependencies.size();
                    xx.m_dependencies = variable_dependencies.p;
                    fill_variable_dependencies = fill_variable_dependencies_copy;
                }

                void visit_Var(const ASR::Var_t& x) {
                    if( fill_variable_dependencies ) {
                        variable_dependencies.push_back(al, ASRUtils::symbol_name(x.m_v));
                    }
                }

                void visit_FunctionCall(const ASR::FunctionCall_t& x) {
                    if( fill_function_dependencies ) {
                        function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                    }
                    if( ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) &&
                        fill_module_dependencies ) {
                        ASR::ExternalSymbol_t* x_m_name = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
                        if( ASR::is_a<ASR::Module_t>(*ASRUtils::get_asr_owner(x_m_name->m_external)) ) {
                            module_dependencies.push_back(al, x_m_name->m_module_name);
                        }
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
                        if( ASR::is_a<ASR::Module_t>(*ASRUtils::get_asr_owner(x_m_name->m_external)) ) {
                            module_dependencies.push_back(al, x_m_name->m_module_name);
                        }
                    }
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_SubroutineCall(x);
                }

                void visit_BlockCall(const ASR::BlockCall_t& x) {
                    ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(x.m_m);
                    for (size_t i=0; i<block->n_body; i++) {
                        visit_stmt(*(block->m_body[i]));
                    }
                }
        };

    namespace ReplacerUtils {
        template <typename T>
        void replace_StructTypeConstructor(ASR::StructTypeConstructor_t* x,
            T* replacer, bool inside_symtab, bool& remove_original_statement,
            Vec<ASR::stmt_t*>* result_vec) {
            if( x->n_args == 0 ) {
                if( !inside_symtab ) {
                    remove_original_statement = true;
                }
                return ;
            }
            if( replacer->result_var == nullptr ) {
                std::string result_var_name = replacer->current_scope->get_unique_name("temp_struct_var__");
                replacer->result_var = PassUtils::create_auxiliary_variable(x->base.base.loc,
                                    result_var_name, replacer->al, replacer->current_scope, x->m_type);
                *replacer->current_expr = replacer->result_var;
            } else {
                if( inside_symtab ) {
                    *replacer->current_expr = nullptr;
                } else {
                    remove_original_statement = true;
                }
            }

            std::deque<ASR::symbol_t*> constructor_arg_syms;
            ASR::Struct_t* dt_der = ASR::down_cast<ASR::Struct_t>(x->m_type);
            ASR::StructType_t* dt_dertype = ASR::down_cast<ASR::StructType_t>(
                                            ASRUtils::symbol_get_past_external(dt_der->m_derived_type));
            while( dt_dertype ) {
                for( int i = (int) dt_dertype->n_members - 1; i >= 0; i-- ) {
                    constructor_arg_syms.push_front(
                        dt_dertype->m_symtab->get_symbol(
                            dt_dertype->m_members[i]));
                }
                if( dt_dertype->m_parent != nullptr ) {
                    ASR::symbol_t* dt_der_sym = ASRUtils::symbol_get_past_external(
                                            dt_dertype->m_parent);
                    LCOMPILERS_ASSERT(ASR::is_a<ASR::StructType_t>(*dt_der_sym));
                    dt_dertype = ASR::down_cast<ASR::StructType_t>(dt_der_sym);
                } else {
                    dt_dertype = nullptr;
                }
            }
            LCOMPILERS_ASSERT(constructor_arg_syms.size() == x->n_args);

            for( size_t i = 0; i < x->n_args; i++ ) {
                if( x->m_args[i].m_value == nullptr ) {
                    continue ;
                }
                ASR::symbol_t* member = constructor_arg_syms[i];
                if( ASR::is_a<ASR::StructTypeConstructor_t>(*x->m_args[i].m_value) ) {
                    ASR::expr_t* result_var_copy = replacer->result_var;
                    ASR::symbol_t *v = nullptr;
                    if (ASR::is_a<ASR::Var_t>(*result_var_copy)) {
                        v = ASR::down_cast<ASR::Var_t>(result_var_copy)->m_v;
                    }
                    replacer->result_var = ASRUtils::EXPR(ASRUtils::getStructInstanceMember_t(replacer->al,
                                                x->base.base.loc, (ASR::asr_t*) result_var_copy, v,
                                                member, replacer->current_scope));
                    ASR::expr_t** current_expr_copy = replacer->current_expr;
                    replacer->current_expr = &(x->m_args[i].m_value);
                    replacer->replace_expr(x->m_args[i].m_value);
                    replacer->current_expr = current_expr_copy;
                    replacer->result_var = result_var_copy;
                } else {
                    ASR::symbol_t *v = nullptr;
                    if (ASR::is_a<ASR::Var_t>(*replacer->result_var)) {
                        v = ASR::down_cast<ASR::Var_t>(replacer->result_var)->m_v;
                    }
                    ASR::expr_t* derived_ref = ASRUtils::EXPR(ASRUtils::getStructInstanceMember_t(replacer->al,
                                                    x->base.base.loc, (ASR::asr_t*) replacer->result_var, v,
                                                    member, replacer->current_scope));
                    ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al,
                                                x->base.base.loc, derived_ref,
                                                x->m_args[i].m_value, nullptr));
                    result_vec->push_back(replacer->al, assign);
                }
            }
        }

        template <typename T>
        void create_do_loop(T* replacer, ASR::ImpliedDoLoop_t* idoloop,
        ASR::Var_t* arr_var, Vec<ASR::stmt_t*>* result_vec,
        ASR::expr_t* arr_idx=nullptr) {
            ASR::do_loop_head_t head;
            head.m_v = idoloop->m_var;
            head.m_start = idoloop->m_start;
            head.m_end = idoloop->m_end;
            head.m_increment = idoloop->m_increment;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(replacer->al, 1);
            ASR::ttype_t *_type = ASRUtils::expr_type(idoloop->m_start);
            ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(replacer->al, arr_var->base.base.loc, 1, _type));
            ASR::expr_t *const_n, *offset, *num_grps, *grp_start;
            const_n = offset = num_grps = grp_start = nullptr;
            if( arr_idx == nullptr ) {
                const_n = ASRUtils::EXPR(ASR::make_IntegerConstant_t(replacer->al, arr_var->base.base.loc, idoloop->n_values, _type));
                offset = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, arr_var->base.base.loc, idoloop->m_var,
                            ASR::binopType::Sub, idoloop->m_start, _type, nullptr));
                num_grps = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, arr_var->base.base.loc, offset,
                                ASR::binopType::Mul, const_n, _type, nullptr));
                grp_start = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, arr_var->base.base.loc, num_grps,
                                ASR::binopType::Add, const_1, _type, nullptr));
            }
            for( size_t i = 0; i < idoloop->n_values; i++ ) {
                Vec<ASR::array_index_t> args;
                ASR::array_index_t ai;
                ai.loc = arr_var->base.base.loc;
                ai.m_left = nullptr;
                if( arr_idx == nullptr ) {
                    ASR::expr_t* const_i = ASRUtils::EXPR(ASR::make_IntegerConstant_t(replacer->al, arr_var->base.base.loc, i, _type));
                    ASR::expr_t* idx = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, arr_var->base.base.loc,
                                            grp_start, ASR::binopType::Add, const_i, _type, nullptr));
                    ai.m_right = idx;
                } else {
                    ai.m_right = arr_idx;
                }
                ai.m_step = nullptr;
                args.reserve(replacer->al, 1);
                args.push_back(replacer->al, ai);
                ASR::ttype_t* array_ref_type = ASRUtils::expr_type(ASRUtils::EXPR((ASR::asr_t*)arr_var));
                Vec<ASR::dimension_t> empty_dims;
                empty_dims.reserve(replacer->al, 1);
                array_ref_type = ASRUtils::duplicate_type(replacer->al, array_ref_type, &empty_dims);
                ASR::expr_t* array_ref = ASRUtils::EXPR(ASR::make_ArrayItem_t(replacer->al, arr_var->base.base.loc,
                                            ASRUtils::EXPR((ASR::asr_t*)arr_var),
                                            args.p, args.size(),
                                            array_ref_type, ASR::arraystorageType::RowMajor,
                                            nullptr));
                if( ASR::is_a<ASR::ImpliedDoLoop_t>(*idoloop->m_values[i]) ) {
                    throw LCompilersException("Pass for nested ImpliedDoLoop nodes isn't implemented yet."); // idoloop->m_values[i]->base.loc
                }
                ASR::stmt_t* doloop_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, arr_var->base.base.loc,
                                                array_ref, idoloop->m_values[i], nullptr));
                doloop_body.push_back(replacer->al, doloop_stmt);
                if( arr_idx != nullptr ) {
                    ASR::expr_t* increment = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, arr_var->base.base.loc,
                                                arr_idx, ASR::binopType::Add, const_1, ASRUtils::expr_type(arr_idx), nullptr));
                    ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, arr_var->base.base.loc,
                                                arr_idx, increment, nullptr));
                    doloop_body.push_back(replacer->al, assign_stmt);
                }
            }
            ASR::stmt_t* doloop = ASRUtils::STMT(ASR::make_DoLoop_t(replacer->al, arr_var->base.base.loc,
                                                                    nullptr, head, doloop_body.p, doloop_body.size()));
            result_vec->push_back(replacer->al, doloop);
        }

        template <typename T>
        void replace_ArrayConstant(ASR::ArrayConstant_t* x, T* replacer,
            bool& remove_original_statement, Vec<ASR::stmt_t*>* result_vec) {
            Allocator& al = replacer->al;
            if( !replacer->result_var ) {
                return ;
            }
            if( x->n_args == 0 ) {
                remove_original_statement = true;
                return ;
            }

            const Location& loc = x->base.base.loc;
            if( ASR::is_a<ASR::Var_t>(*replacer->result_var) ) {
                LCOMPILERS_ASSERT_MSG(PassUtils::get_rank(replacer->result_var) == 1,
                                    "Initialisation using ArrayConstant is "
                                    "supported only for single dimensional arrays.")
                ASR::Var_t* arr_var = ASR::down_cast<ASR::Var_t>(replacer->result_var);
                Vec<ASR::expr_t*> idx_vars;
                PassUtils::create_idx_vars(idx_vars, 1, loc, replacer->al, replacer->current_scope);
                ASR::expr_t* idx_var = idx_vars[0];
                ASR::expr_t* lb = PassUtils::get_bound(replacer->result_var, 1, "lbound", replacer->al);
                ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(replacer->al, loc, 1,
                                            ASRUtils::expr_type(idx_var)));
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al,
                                                arr_var->base.base.loc, idx_var, lb, nullptr));
                result_vec->push_back(replacer->al, assign_stmt);
                for( size_t k = 0; k < x->n_args; k++ ) {
                    ASR::expr_t* curr_init = x->m_args[k];
                    if( ASR::is_a<ASR::ImpliedDoLoop_t>(*curr_init) ) {
                        ASR::ImpliedDoLoop_t* idoloop = ASR::down_cast<ASR::ImpliedDoLoop_t>(curr_init);
                        create_do_loop(replacer, idoloop, arr_var, result_vec, idx_var);
                    } else {
                        Vec<ASR::array_index_t> args;
                        ASR::array_index_t ai;
                        ai.loc = arr_var->base.base.loc;
                        ai.m_left = nullptr;
                        ai.m_right = idx_var;
                        ai.m_step = nullptr;
                        args.reserve(replacer->al, 1);
                        args.push_back(replacer->al, ai);
                        ASR::ttype_t* array_ref_type = ASRUtils::expr_type(ASRUtils::EXPR((ASR::asr_t*)arr_var));
                        if( ASR::is_a<ASR::Struct_t>(*ASRUtils::type_get_past_pointer(array_ref_type)) ) {
                            ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(
                                ASRUtils::type_get_past_pointer(array_ref_type));
                            if( replacer->current_scope->get_counter() != ASRUtils::symbol_parent_symtab(
                                    struct_t->m_derived_type)->get_counter() ) {
                                ASR::symbol_t* m_derived_type = replacer->current_scope->resolve_symbol(
                                    ASRUtils::symbol_name(struct_t->m_derived_type));
                                ASR::ttype_t* struct_type = ASRUtils::TYPE(ASR::make_Struct_t(al,
                                    struct_t->base.base.loc, m_derived_type, struct_t->m_dims, struct_t->n_dims));
                                if( ASR::is_a<ASR::Pointer_t>(*array_ref_type) ) {
                                    struct_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, array_ref_type->base.loc, struct_type));
                                }
                                array_ref_type = struct_type;
                            }
                        }
                        Vec<ASR::dimension_t> empty_dims;
                        empty_dims.reserve(replacer->al, 1);
                        array_ref_type = ASRUtils::duplicate_type(replacer->al, array_ref_type, &empty_dims);
                        ASR::expr_t* array_ref = ASRUtils::EXPR(ASR::make_ArrayItem_t(replacer->al, arr_var->base.base.loc,
                                                        ASRUtils::EXPR((ASR::asr_t*)arr_var),
                                                        args.p, args.size(),
                                                        array_ref_type, ASR::arraystorageType::ColMajor,
                                                        nullptr));
                        ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, arr_var->base.base.loc,
                                                        array_ref, x->m_args[k], nullptr));
                        result_vec->push_back(replacer->al, assign_stmt);
                        ASR::expr_t* increment = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, arr_var->base.base.loc, idx_var,
                                                    ASR::binopType::Add, const_1, ASRUtils::expr_type(idx_var), nullptr));
                        assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, arr_var->base.base.loc, idx_var, increment, nullptr));
                        result_vec->push_back(replacer->al, assign_stmt);
                    }
                }
            } else if( ASR::is_a<ASR::ArraySection_t>(*replacer->result_var) ) {
                ASR::ArraySection_t* target_section = ASR::down_cast<ASR::ArraySection_t>(replacer->result_var);
                int sliced_dims_count = 0;
                size_t sliced_dim_index = 0;
                for( size_t i = 0; i < target_section->n_args; i++ ) {
                    if( !(target_section->m_args[i].m_left == nullptr &&
                            target_section->m_args[i].m_right != nullptr &&
                            target_section->m_args[i].m_step == nullptr) ) {
                        sliced_dims_count += 1;
                        sliced_dim_index = i + 1;
                    }
                }
                if( sliced_dims_count != 1 ) {
                    throw LCompilersException("Target expressions only having one "
                                            "dimension sliced are supported for now.");
                }

                Vec<ASR::expr_t*> idx_vars;
                PassUtils::create_idx_vars(idx_vars, 1, loc, replacer->al, replacer->current_scope);
                ASR::expr_t* idx_var = idx_vars[0];
                ASR::expr_t* lb = PassUtils::get_bound(target_section->m_v, sliced_dim_index, "lbound", replacer->al);
                ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(replacer->al, loc, 1,
                                            ASRUtils::expr_type(idx_var)));
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al,
                                                target_section->base.base.loc, idx_var, lb, nullptr));
                result_vec->push_back(replacer->al, assign_stmt);
                for( size_t k = 0; k < x->n_args; k++ ) {
                    ASR::expr_t* curr_init = x->m_args[k];
                    if( ASR::is_a<ASR::ImpliedDoLoop_t>(*curr_init) ) {
                        throw LCompilersException("Do loops in array initialiser expressions not supported yet.");
                    } else {
                        Vec<ASR::array_index_t> args;
                        args.reserve(replacer->al, target_section->n_args);
                        for( size_t i = 0; i < target_section->n_args; i++ ) {
                            if( i + 1 == sliced_dim_index ) {
                                ASR::array_index_t ai;
                                ai.loc = target_section->base.base.loc;
                                ai.m_left = nullptr;
                                ai.m_step = nullptr;
                                ai.m_right = idx_var;
                                args.push_back(replacer->al, ai);
                            } else {
                                args.push_back(replacer->al, target_section->m_args[i]);
                            }
                        }

                        ASR::ttype_t* array_ref_type = ASRUtils::expr_type(replacer->result_var);
                        Vec<ASR::dimension_t> empty_dims;
                        empty_dims.reserve(replacer->al, 1);
                        array_ref_type = ASRUtils::duplicate_type(replacer->al, array_ref_type, &empty_dims);

                        ASR::expr_t* array_ref = ASRUtils::EXPR(ASR::make_ArrayItem_t(replacer->al,
                                                        target_section->base.base.loc,
                                                        target_section->m_v,
                                                        args.p, args.size(),
                                                        array_ref_type, ASR::arraystorageType::RowMajor,
                                                        nullptr));
                        ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, target_section->base.base.loc,
                                                        array_ref, x->m_args[k], nullptr));
                        result_vec->push_back(replacer->al, assign_stmt);
                        ASR::expr_t* increment = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, target_section->base.base.loc,
                                                    idx_var, ASR::binopType::Add, const_1, ASRUtils::expr_type(idx_var), nullptr));
                        assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, target_section->base.base.loc, idx_var, increment, nullptr));
                        result_vec->push_back(replacer->al, assign_stmt);
                    }
                }
            }
            remove_original_statement = true;
        }
    }

    static inline void handle_fn_return_var(Allocator &al, ASR::Function_t *x,
            bool (*is_array_or_struct)(ASR::expr_t*)) {
        if (x->m_return_var) {
            /*
            * The `return_var` of the function, which is either an array or
            * struct, is set to `null`, and the destination variable will be
            * passed as the last argument to the existing function. This helps
            * in avoiding deep copies and the destination memory directly gets
            * filled inside the function.
            */
            if( is_array_or_struct(x->m_return_var)) {
                for( auto& s_item: x->m_symtab->get_scope() ) {
                    ASR::symbol_t* curr_sym = s_item.second;
                    if( curr_sym->type == ASR::symbolType::Variable ) {
                        ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(curr_sym);
                        if( var->m_intent == ASR::intentType::Unspecified ) {
                            var->m_intent = ASR::intentType::In;
                        } else if( var->m_intent == ASR::intentType::ReturnVar ) {
                            var->m_intent = ASR::intentType::Out;
                        }
                    }
                }
                Vec<ASR::expr_t*> a_args;
                a_args.reserve(al, x->n_args + 1);
                for( size_t i = 0; i < x->n_args; i++ ) {
                    a_args.push_back(al, x->m_args[i]);
                }
                LCOMPILERS_ASSERT(x->m_return_var)
                a_args.push_back(al, x->m_return_var);
                x->m_args = a_args.p;
                x->n_args = a_args.n;
                x->m_return_var = nullptr;
                ASR::FunctionType_t* s_func_type = ASR::down_cast<ASR::FunctionType_t>(
                    x->m_function_signature);
                Vec<ASR::ttype_t*> arg_types;
                arg_types.reserve(al, a_args.n);
                for(auto &e: a_args) {
                    ASRUtils::ReplaceWithFunctionParamVisitor replacer(al, x->m_args, x->n_args);
                    arg_types.push_back(al, replacer.replace_args_with_FunctionParam(
                                                ASRUtils::expr_type(e)));
                }
                s_func_type->m_arg_types = arg_types.p;
                s_func_type->n_arg_types = arg_types.n;
                s_func_type->m_return_var_type = nullptr;
            }
        }
        for (auto &item : x->m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                handle_fn_return_var(al, ASR::down_cast<ASR::Function_t>(
                    item.second), is_array_or_struct);
            }
        }
    }


    } // namespace PassUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_UTILS_H
