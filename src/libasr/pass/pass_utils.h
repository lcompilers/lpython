#ifndef LFORTRAN_PASS_UTILS_H
#define LFORTRAN_PASS_UTILS_H

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_pass_walk_visitor.h>

#include <deque>

namespace LCompilers {

    namespace PassUtils {

        bool is_array(ASR::expr_t* x);

        void get_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims);

        ASR::ttype_t* set_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims,
                                    bool create_new=false, Allocator* al=nullptr);

        int get_rank(ASR::expr_t* x);

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars,
            Allocator& al, SymbolTable* current_scope=nullptr, bool perform_cast=false,
            ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr);

        ASR::expr_t* create_array_ref(ASR::symbol_t* arr, Vec<ASR::expr_t*>& idx_vars, Allocator& al,
            const Location& loc, ASR::ttype_t* _type, SymbolTable* current_scope=nullptr,
            bool perform_cast=false, ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr);

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, ASR::expr_t* idx_var, Allocator& al,
            SymbolTable* current_scope=nullptr, bool perform_cast=false,
            ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr);

        static inline bool is_elemental(ASR::symbol_t* x) {
            x = ASRUtils::symbol_get_past_external(x);
            if( !ASR::is_a<ASR::Function_t>(*x) ) {
                return false;
            }
            return ASRUtils::get_FunctionType(
                ASR::down_cast<ASR::Function_t>(x))->m_elemental;
        }

        bool is_args_contains_allocatable(ASR::expr_t* x);
        void fix_dimension(ASR::Cast_t* x, ASR::expr_t* arg_expr);

        ASR::ttype_t* get_matching_type(ASR::expr_t* sibling, Allocator& al);

        ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                                ASR::ttype_t* var_type, Allocator& al, SymbolTable*& current_scope);

        ASR::expr_t* create_var(int counter, std::string suffix, const Location& loc,
                                ASR::expr_t* sibling, Allocator& al, SymbolTable*& current_scope);

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

        ASR::expr_t* get_flipsign(ASR::expr_t* arg0, ASR::expr_t* arg1,
                             Allocator& al, ASR::TranslationUnit_t& unit, const Location& loc,
                             PassOptions& pass_options);

        ASR::expr_t* to_int32(ASR::expr_t* x, ASR::ttype_t* int32type, Allocator& al);

        ASR::expr_t* create_auxiliary_variable_for_expr(ASR::expr_t* expr, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::stmt_t*& assign_stmt);

        ASR::expr_t* create_auxiliary_variable(const Location& loc, std::string& name,
            Allocator& al, SymbolTable*& current_scope, ASR::ttype_t* var_type,
            ASR::intentType var_intent=ASR::intentType::Local, ASR::symbol_t* var_decl=nullptr);

        ASR::expr_t* get_fma(ASR::expr_t* arg0, ASR::expr_t* arg1, ASR::expr_t* arg2,
                             Allocator& al, ASR::TranslationUnit_t& unit, Location& loc,
                             PassOptions& pass_options);

        ASR::expr_t* get_sign_from_value(ASR::expr_t* arg0, ASR::expr_t* arg1,
                                         Allocator& al, ASR::TranslationUnit_t& unit,
                                         Location& loc, PassOptions& pass_options);

        ASR::stmt_t* get_vector_copy(ASR::expr_t* array0, ASR::expr_t* array1, ASR::expr_t* start,
            ASR::expr_t* end, ASR::expr_t* step, ASR::expr_t* vector_length,
            Allocator& al, ASR::TranslationUnit_t& unit,
            SymbolTable*& global_scope, Location& loc);

        Vec<ASR::stmt_t*> replace_doloop(Allocator &al, const ASR::DoLoop_t &loop,
                                         int comp=-1, bool use_loop_variable_after_loop=false);

        ASR::stmt_t* create_do_loop_helper_pack(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, ASR::expr_t* array, ASR::expr_t* mask,
            ASR::expr_t* res, ASR::expr_t* idx, int curr_idx);

        ASR::stmt_t* create_do_loop_helper_unpack(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, ASR::expr_t* vector, ASR::expr_t* mask,
            ASR::expr_t* res, ASR::expr_t* idx, int curr_idx);

        ASR::stmt_t* create_do_loop_helper_count(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, ASR::expr_t* mask, ASR::expr_t* res,
            int curr_idx);

        ASR::stmt_t* create_do_loop_helper_count_dim(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, std::vector<ASR::expr_t*> res_idx,
            ASR::stmt_t* inner_most_do_loop, ASR::expr_t* c, ASR::expr_t* mask, ASR::expr_t* res,
            int curr_idx, int dim);

        ASR::stmt_t* create_do_loop_helper_norm2(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, ASR::expr_t* array, ASR::expr_t* res,
            int curr_idx);

        ASR::stmt_t* create_do_loop_helper_norm2_dim(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, std::vector<ASR::expr_t*> res_idx,
            ASR::stmt_t* inner_most_do_loop, ASR::expr_t* c, ASR::expr_t* array, ASR::expr_t* res,
            int curr_idx, int dim);

        ASR::stmt_t* create_do_loop_helper_parity(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, ASR::expr_t* array, ASR::expr_t* res,
            int curr_idx);

        ASR::stmt_t* create_do_loop_helper_parity_dim(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, std::vector<ASR::expr_t*> res_idx,
            ASR::stmt_t* inner_most_do_loop, ASR::expr_t* c, ASR::expr_t* array, ASR::expr_t* res,
            int curr_idx, int dim);

        ASR::stmt_t* create_do_loop_helper_random_number(Allocator &al, const Location &loc,
            std::vector<ASR::expr_t*> do_loop_variables, ASR::symbol_t* s, ASR::expr_t* arr,
            ASR::ttype_t* return_type, ASR::expr_t* arr_item, ASR::stmt_t* stmt, int curr_idx);

        static inline bool is_aggregate_type(ASR::expr_t* var) {
            return ASR::is_a<ASR::StructType_t>(*ASRUtils::expr_type(var));
        }

        /*  Checks for any non-primitive-function-return type 
            like fixed strings or allocatables.
            allocatable string, allocatable integer, etc.. */
        static inline bool is_non_primitive_return_type(ASR::ttype_t* x){
            // TODO : Handle other allocatable types and fixed strings.
            return ASRUtils::is_descriptorString(x);
        }

        static inline bool is_aggregate_or_array_type(ASR::expr_t* var) {
            return (ASR::is_a<ASR::StructType_t>(*ASRUtils::expr_type(var)) ||
                    ASRUtils::is_array(ASRUtils::expr_type(var)) ||
                    ASR::is_a<ASR::SymbolicExpression_t>(*ASRUtils::expr_type(var)));
        }
        
        static inline bool is_aggregate_or_array_or_nonPrimitive_type(ASR::expr_t* var) {
            return  is_aggregate_or_array_type(var) || 
                    is_non_primitive_return_type(ASRUtils::expr_type(var));
        }


        static inline bool is_symbolic_list_type(ASR::expr_t* var) {
            if (ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(var))) {
                ASR::List_t *list = ASR::down_cast<ASR::List_t>(ASRUtils::expr_type(var));
                return (list->m_type->type == ASR::ttypeType::SymbolicExpression);
            }
            return false;
        }

        static inline void allocate_res_var(Allocator& al, ASR::FunctionCall_t* x, Vec<ASR::call_arg_t> &new_args,
                            ASR::expr_t* result_var_, Vec<ASR::stmt_t*>& pass_result, std::vector<int> map) {
            ASR::expr_t* func_call_merge = nullptr;
            ASR::Function_t* sum = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(x->m_name));
            ASR::symbol_t* res = sum->m_symtab->resolve_symbol("result");
            if (res) {
                ASR::ttype_t* type = ASRUtils::duplicate_type(al, x->m_type);
                ASR::Array_t* res_arr = ASR::down_cast<ASR::Array_t>(type);
                for (size_t i = 0; i < res_arr->n_dims; i++) {
                    if (ASR::is_a<ASR::FunctionCall_t>(*res_arr->m_dims[i].m_length)) {
                        func_call_merge = res_arr->m_dims[i].m_length;
                        ASR::FunctionCall_t* func_call = ASR::down_cast<ASR::FunctionCall_t>(func_call_merge);
                        if (ASR::is_a<ASR::ArraySize_t>(*func_call->m_args[0].m_value)) {
                            ASR::ArraySize_t *array_size = ASR::down_cast<ASR::ArraySize_t>(func_call->m_args[0].m_value);
                            if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*new_args[map[0]].m_value)) {
                                array_size->m_v = ASR::down_cast<ASR::ArrayPhysicalCast_t>(new_args[map[0]].m_value)->m_arg;
                            } else {
                                array_size->m_v = new_args[map[0]].m_value;
                            }
                            func_call->m_args[0].m_value = ASRUtils::EXPR((ASR::asr_t*) array_size);
                        }
                        if (ASR::is_a<ASR::ArraySize_t>(*func_call->m_args[1].m_value)) {
                            ASR::ArraySize_t *array_size = ASR::down_cast<ASR::ArraySize_t>(func_call->m_args[1].m_value);
                            if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*new_args[map[1]].m_value))
                                array_size->m_v = ASR::down_cast<ASR::ArrayPhysicalCast_t>(new_args[map[1]].m_value)->m_arg;
                            else {
                                array_size->m_v = new_args[map[1]].m_value;
                            }
                            func_call->m_args[1].m_value = ASRUtils::EXPR((ASR::asr_t*) array_size);
                        }
                        if (ASR::is_a<ASR::IntegerCompare_t>(*func_call->m_args[2].m_value)) {
                            ASR::IntegerCompare_t *integer_compare = ASR::down_cast<ASR::IntegerCompare_t>(func_call->m_args[2].m_value);
                            integer_compare->m_right = new_args[map[2]].m_value;
                            func_call->m_args[2].m_value = ASRUtils::EXPR((ASR::asr_t*) integer_compare);
                        }
                        res_arr->m_dims[i].m_length = func_call_merge;
                    }
                }
                if (func_call_merge) {
                    // allocate result array
                    Vec<ASR::alloc_arg_t> alloc_args; alloc_args.reserve(al, 1);
                    ASR::alloc_arg_t alloc_arg; alloc_arg.loc = x->base.base.loc;
                    alloc_arg.m_a = result_var_; alloc_arg.m_len_expr = nullptr;
                    alloc_arg.m_type = nullptr; alloc_arg.m_dims = res_arr->m_dims;
                    alloc_arg.n_dims = res_arr->n_dims;
                    alloc_args.push_back(al, alloc_arg);

                    ASR::stmt_t* allocate_stmt = ASRUtils::STMT(ASR::make_Allocate_t(al,
                                            x->base.base.loc, alloc_args.p, alloc_args.n, nullptr, nullptr, nullptr));
                    pass_result.push_back(al, allocate_stmt);
                }
            }
        }

        static inline ASR::expr_t* get_actual_arg_(ASR::expr_t* arg, Vec<ASR::call_arg_t>& func_call_args, Vec<ASR::expr_t*> actual_args) {
            if (!ASR::is_a<ASR::Var_t>(*arg)) return arg;
            std::string arg_name = ASRUtils::symbol_name(ASR::down_cast<ASR::Var_t>(arg)->m_v);
            for (size_t i = 0; i < func_call_args.size(); i++) {
                ASR::expr_t* func_arg = func_call_args[i].m_value;
                if (func_arg == nullptr) {
                    continue;
                }
                if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*func_arg)) {
                    func_arg = ASR::down_cast<ASR::ArrayPhysicalCast_t>(func_arg)->m_arg;
                }
                if (ASR::is_a<ASR::Var_t>(*func_arg)) {
                    std::string func_arg_name = ASRUtils::symbol_name(ASR::down_cast<ASR::Var_t>(func_arg)->m_v);
                    if (arg_name == func_arg_name) {
                        return actual_args[i];
                    }
                }
            }
            return arg;
        }

        template <class StructType>
        class PassVisitor: public ASR::ASRPassBaseWalkVisitor<StructType> {

            private:

                StructType& self() { return static_cast<StructType&>(*this); }

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

        template <class StructType>
        class SkipOptimizationFunctionVisitor: public PassVisitor<StructType> {

            public:

                SkipOptimizationFunctionVisitor(Allocator& al_): PassVisitor<StructType>(al_, nullptr) {
                }

                void visit_Function(const ASR::Function_t &x) {
                    if( ASRUtils::is_intrinsic_optimization<ASR::Function_t>(&x) ) {
                        return ;
                    }
                    PassUtils::PassVisitor<StructType>::visit_Function(x);
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
                bool _return_var_or_intent_out = false;
                SymbolTable* current_scope;

            public:

                UpdateDependenciesVisitor(Allocator &al_)
                : al(al_), fill_function_dependencies(false),
                fill_module_dependencies(false),
                fill_variable_dependencies(false)
                {
                    function_dependencies.n = 0;
                    module_dependencies.n = 0;
                    variable_dependencies.n = 0;
                    current_scope = nullptr;
                }

                void visit_Function(const ASR::Function_t& x) {
                    ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
                    SymbolTable* current_scope_copy = current_scope;
                    current_scope = xx.m_symtab;
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
                    current_scope = current_scope_copy;
                }

                void visit_Module(const ASR::Module_t& x) {
                    SymbolTable *parent_symtab = current_scope;
                    current_scope = x.m_symtab;
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
                    current_scope = parent_symtab;
                }

                void visit_Variable(const ASR::Variable_t& x) {
                    ASR::Variable_t& xx = const_cast<ASR::Variable_t&>(x);
                    variable_dependencies.n = 0;
                    variable_dependencies.reserve(al, 1);
                    bool fill_variable_dependencies_copy = fill_variable_dependencies;
                    fill_variable_dependencies = true;
                    _return_var_or_intent_out = (x.m_intent == ASR::intentType::Out ||
                                                x.m_intent == ASR::intentType::ReturnVar ||
                                                x.m_intent == ASR::intentType::InOut);
                    BaseWalkVisitor<UpdateDependenciesVisitor>::visit_Variable(x);
                    _return_var_or_intent_out = false;
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
                    if (fill_function_dependencies) {
                        ASR::symbol_t* asr_owner_sym = nullptr;
                        if (current_scope->asr_owner && ASR::is_a<ASR::symbol_t>(*current_scope->asr_owner)) {
                            asr_owner_sym = ASR::down_cast<ASR::symbol_t>(current_scope->asr_owner);
                        }

                        SymbolTable* temp_scope = current_scope;

                        if (asr_owner_sym && temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(x.m_name)->get_counter() &&
                            !ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) && !ASR::is_a<ASR::Variable_t>(*x.m_name)) {
                            if (ASR::is_a<ASR::AssociateBlock_t>(*asr_owner_sym) || ASR::is_a<ASR::Block_t>(*asr_owner_sym)) {
                                temp_scope = temp_scope->parent;
                                if (temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(x.m_name)->get_counter()) {
                                    function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                                }
                            } else {
                                function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                            }
                        }

                        if (_return_var_or_intent_out && temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(x.m_name)->get_counter() &&
                            !ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name)) {
                            function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                        }
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
                    if (fill_function_dependencies) {
                        ASR::symbol_t* asr_owner_sym = nullptr;
                        if (current_scope->asr_owner && ASR::is_a<ASR::symbol_t>(*current_scope->asr_owner)) {
                            asr_owner_sym = ASR::down_cast<ASR::symbol_t>(current_scope->asr_owner);
                        }

                        SymbolTable* temp_scope = current_scope;

                        if (asr_owner_sym && temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(x.m_name)->get_counter() &&
                            !ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) && !ASR::is_a<ASR::Variable_t>(*x.m_name)) {
                            if (ASR::is_a<ASR::AssociateBlock_t>(*asr_owner_sym) || ASR::is_a<ASR::Block_t>(*asr_owner_sym)) {
                                temp_scope = temp_scope->parent;
                                if (temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(x.m_name)->get_counter()) {
                                    function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                                }
                            } else {
                                function_dependencies.push_back(al, ASRUtils::symbol_name(x.m_name));
                            }
                        }
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
                    SymbolTable *parent_symtab = current_scope;
                    ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(x.m_m);
                    current_scope = block->m_symtab;
                    for (size_t i=0; i<block->n_body; i++) {
                        visit_stmt(*(block->m_body[i]));
                    }
                    current_scope = parent_symtab;
                }

                void visit_AssociateBlock(const ASR::AssociateBlock_t& x) {
                    SymbolTable *parent_symtab = current_scope;
                    current_scope = x.m_symtab;
                    for (auto &a : x.m_symtab->get_scope()) {
                        this->visit_symbol(*a.second);
                    }
                    for (size_t i=0; i<x.n_body; i++) {
                        visit_stmt(*x.m_body[i]);
                    }
                    current_scope = parent_symtab;
                }

                void visit_Block(const ASR::Block_t& x) {
                    SymbolTable *parent_symtab = current_scope;
                    current_scope = x.m_symtab;
                    for (auto &a : x.m_symtab->get_scope()) {
                        this->visit_symbol(*a.second);
                    }
                    for (size_t i=0; i<x.n_body; i++) {
                        visit_stmt(*x.m_body[i]);
                    }
                    current_scope = parent_symtab;
                }
            // TODO: Uncomment the following in LFortran
            /*
                template <typename T>
                void visit_UserDefinedType(const T& x) {
                    T& xx = const_cast<T&>(x);
                    SetChar vec; vec.reserve(al, 1);
                    for( auto itr: x.m_symtab->get_scope() ) {
                        ASR::ttype_t* type = ASRUtils::extract_type(
                            ASRUtils::symbol_type(itr.second));
                        if( ASR::is_a<ASR::StructType_t>(*type) ) {
                            ASR::StructType_t* struct_t = ASR::down_cast<ASR::StructType_t>(type);
                            vec.push_back(al, ASRUtils::symbol_name(struct_t->m_derived_type));
                        } else if( ASR::is_a<ASR::EnumType_t>(*type) ) {
                            ASR::EnumType_t* enum_t = ASR::down_cast<ASR::EnumType_t>(type);
                            vec.push_back(al, ASRUtils::symbol_name(enum_t->m_enum_type));
                        }
                    }
                    xx.m_dependencies = vec.p;
                    xx.n_dependencies = vec.size();
                }

                void visit_Struct(const ASR::Struct_t& x) {
                    visit_UserDefinedType(x);
                }

                void visit_Union(const ASR::Union_t& x) {
                    visit_UserDefinedType(x);
                }
            */
        };

    namespace ReplacerUtils {
        template <typename T>
        void replace_StructConstructor(ASR::StructConstructor_t* x,
            T* replacer, bool inside_symtab, bool& remove_original_statement,
            Vec<ASR::stmt_t*>* result_vec,
            bool perform_cast=false,
            ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr) {
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
            ASR::StructType_t* dt_der = ASR::down_cast<ASR::StructType_t>(x->m_type);
            ASR::Struct_t* dt_dertype = ASR::down_cast<ASR::Struct_t>(
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
                    LCOMPILERS_ASSERT(ASR::is_a<ASR::Struct_t>(*dt_der_sym));
                    dt_dertype = ASR::down_cast<ASR::Struct_t>(dt_der_sym);
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
                if( ASR::is_a<ASR::StructConstructor_t>(*x->m_args[i].m_value) ) {
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
                    ASR::expr_t* x_m_args_i = x->m_args[i].m_value;
                    if( perform_cast ) {
                        LCOMPILERS_ASSERT(casted_type != nullptr);
                        x_m_args_i = ASRUtils::EXPR(ASR::make_Cast_t(replacer->al, x->base.base.loc,
                            x_m_args_i, cast_kind, casted_type, nullptr));
                    }
                    ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al,
                                                x->base.base.loc, derived_ref,
                                                x_m_args_i, nullptr));
                    result_vec->push_back(replacer->al, assign);
                }
            }
            replacer->result_var = nullptr;
        }

        static inline void create_do_loop(Allocator& al, ASR::ImpliedDoLoop_t* idoloop,
        ASR::expr_t* arr_var, Vec<ASR::stmt_t*>* result_vec,
        ASR::expr_t* arr_idx=nullptr, bool perform_cast=false,
        ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
        ASR::ttype_t* casted_type=nullptr) {
            ASR::do_loop_head_t head;
            head.m_v = idoloop->m_var;
            head.m_start = idoloop->m_start;
            head.m_end = idoloop->m_end;
            head.m_increment = idoloop->m_increment;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            ASR::ttype_t *_type = ASRUtils::expr_type(idoloop->m_start);
            ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_var->base.loc, 1, _type));
            ASR::expr_t *const_n, *offset, *num_grps, *grp_start;
            const_n = offset = num_grps = grp_start = nullptr;
            if( arr_idx == nullptr ) {
                const_n = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_var->base.loc, idoloop->n_values, _type));
                offset = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.loc, idoloop->m_var,
                            ASR::binopType::Sub, idoloop->m_start, _type, nullptr));
                num_grps = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.loc, offset,
                                ASR::binopType::Mul, const_n, _type, nullptr));
                grp_start = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.loc, num_grps,
                                ASR::binopType::Add, const_1, _type, nullptr));
            }
            for( size_t i = 0; i < idoloop->n_values; i++ ) {
                Vec<ASR::array_index_t> args;
                ASR::array_index_t ai;
                ai.loc = arr_var->base.loc;
                ai.m_left = nullptr;
                if( arr_idx == nullptr ) {
                    ASR::expr_t* const_i = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_var->base.loc, i, _type));
                    ASR::expr_t* idx = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.loc,
                                            grp_start, ASR::binopType::Add, const_i, _type, nullptr));
                    ai.m_right = idx;
                } else {
                    ai.m_right = arr_idx;
                }
                ai.m_step = nullptr;
                args.reserve(al, 1);
                args.push_back(al, ai);
                ASR::ttype_t* array_ref_type = ASRUtils::expr_type(arr_var);
                Vec<ASR::dimension_t> empty_dims;
                empty_dims.reserve(al, 1);
                array_ref_type = ASRUtils::duplicate_type(al, array_ref_type, &empty_dims);
                ASR::expr_t* array_ref = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, arr_var->base.loc,
                                            arr_var, args.p, args.size(),
                                            ASRUtils::type_get_past_pointer(
                                                ASRUtils::type_get_past_allocatable(array_ref_type)),
                                            ASR::arraystorageType::RowMajor, nullptr));
                if( ASR::is_a<ASR::ImpliedDoLoop_t>(*idoloop->m_values[i]) ) {
                    create_do_loop(al, ASR::down_cast<ASR::ImpliedDoLoop_t>(idoloop->m_values[i]),
                                   arr_var, &doloop_body, arr_idx, perform_cast, cast_kind, casted_type);
                } else {
                    ASR::expr_t* idoloop_m_values_i = idoloop->m_values[i];
                    if( perform_cast ) {
                        LCOMPILERS_ASSERT(casted_type != nullptr);
                        idoloop_m_values_i = ASRUtils::EXPR(ASR::make_Cast_t(al, array_ref->base.loc,
                            idoloop_m_values_i, cast_kind, casted_type, nullptr));
                    }
                    ASR::stmt_t* doloop_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.loc,
                                                    array_ref, idoloop_m_values_i, nullptr));
                    doloop_body.push_back(al, doloop_stmt);
                    if( arr_idx != nullptr ) {
                        ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_var->base.loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, arr_var->base.loc, 4))));
                        ASR::expr_t* increment = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, arr_var->base.loc,
                                                    arr_idx, ASR::binopType::Add, one, ASRUtils::expr_type(arr_idx), nullptr));
                        ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, arr_var->base.loc,
                                                    arr_idx, increment, nullptr));
                        doloop_body.push_back(al, assign_stmt);
                    }
                }
            }
            ASR::stmt_t* doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, arr_var->base.loc,
                nullptr, head, doloop_body.p, doloop_body.size(), nullptr, 0));
            result_vec->push_back(al, doloop);
        }

        template <typename LOOP_BODY>
        static inline void create_do_loop(Allocator& al, const Location& loc,
            int value_rank, ASR::expr_t* value_array, Vec<ASR::expr_t*>& idx_vars,
            Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body, SymbolTable* current_scope,
            Vec<ASR::stmt_t*>* result_vec) {
            PassUtils::create_idx_vars(idx_vars, value_rank, loc, al, current_scope, "_t");
            LCOMPILERS_ASSERT(value_rank == (int) idx_vars.size())

            ASR::stmt_t* doloop = nullptr;
            for( int i = 0; i < value_rank; i++ ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                head.m_start = PassUtils::get_bound(value_array, i + 1, "lbound", al);
                head.m_end = PassUtils::get_bound(value_array, i + 1, "ubound", al);
                head.m_increment = nullptr;
                head.loc = head.m_v->base.loc;

                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    loop_body();
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head,
                    doloop_body.p, doloop_body.size(), nullptr, 0));
            }
            result_vec->push_back(al, doloop);
        }

        template <typename LOOP_BODY>
        static inline void create_do_loop(Allocator& al, const Location& loc,
            ASR::ArrayItem_t* array_item, Vec<ASR::expr_t*>& idx_vars, Vec<ASR::expr_t*>& temp_idx_vars, 
            Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body, SymbolTable* current_scope,
            Vec<ASR::stmt_t*>* result_vec) {
            int value_rank = array_item->n_args;
            PassUtils::create_idx_vars(idx_vars, value_rank, loc, al, current_scope, "_t");
            LCOMPILERS_ASSERT(value_rank == (int) idx_vars.size())
            temp_idx_vars.reserve(al, array_item->n_args);
            for( int i = 0; i < value_rank; i++ ) {
                if(ASRUtils::is_array(ASRUtils::expr_type(array_item->m_args[i].m_right))){
                    ASR::expr_t* ref = PassUtils::create_array_ref(array_item->m_args[i].m_right, idx_vars[i], al, current_scope);
                    temp_idx_vars.push_back(al, ref);
                } else { 
                    temp_idx_vars.push_back(al, array_item->m_args[i].m_right);
                }
            }
            ASR::stmt_t* doloop = nullptr;
            for( int i = 0; i < value_rank; i++ ) {
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                if(ASRUtils::is_array(ASRUtils::expr_type(array_item->m_args[i].m_right))) {
                    head.m_start = PassUtils::get_bound(array_item->m_args[i].m_right, 1, "lbound", al);
                    head.m_end = PassUtils::get_bound(array_item->m_args[i].m_right, 1, "ubound", al);
                    head.m_increment = nullptr;
                } else {
                    continue;
                } 
                head.loc = head.m_v->base.loc;

                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    loop_body();
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head,
                    doloop_body.p, doloop_body.size(), nullptr, 0));
            }
            if(doloop != nullptr) result_vec->push_back(al, doloop);
        }

        template <typename LOOP_BODY>
        static inline void create_do_loop(Allocator& al, const Location& loc,
            ASR::ArraySection_t* array_section, Vec<ASR::expr_t*>& idx_vars, Vec<ASR::expr_t*>& temp_idx_vars,
            Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body, SymbolTable* current_scope,
            Vec<ASR::stmt_t*>* result_vec) {
            PassUtils::create_idx_vars(idx_vars, array_section->n_args, loc, al, current_scope, "_t");
            LCOMPILERS_ASSERT(array_section->n_args == idx_vars.size())
            temp_idx_vars.reserve(al, array_section->n_args);
            for( size_t i = 0; i < array_section->n_args; i++ ) {
                if ( ASRUtils::is_array(ASRUtils::expr_type(array_section->m_args[i].m_right)) ) {
                    ASR::expr_t* ref = PassUtils::create_array_ref(array_section->m_args[i].m_right, idx_vars[i], al, current_scope);
                    temp_idx_vars.push_back(al, ref);
                } else if ( array_section->m_args[i].m_step != nullptr ) { 
                    temp_idx_vars.push_back(al, idx_vars[i]);
                } else {
                    temp_idx_vars.push_back(al, array_section->m_args[i].m_right);
                }
            }

            ASR::stmt_t* doloop = nullptr;
            for( size_t i = 0; i < array_section->n_args; i++ ) {
                // TODO: Add an If debug node to check if the lower and upper bounds of both the arrays are same.
                ASR::do_loop_head_t head;
                head.m_v = idx_vars[i];
                if( ASRUtils::is_array(ASRUtils::expr_type(array_section->m_args[i].m_right)) ) {
                    head.m_start = PassUtils::get_bound(array_section->m_args[i].m_right, 1, "lbound", al);
                    head.m_end = PassUtils::get_bound(array_section->m_args[i].m_right, 1, "ubound", al);
                    head.m_increment = nullptr;
                } else if ( array_section->m_args[i].m_step == nullptr ) {
                    continue ;
                } else {
                    head.m_start = array_section->m_args[i].m_left;
                    head.m_end = array_section->m_args[i].m_right;
                    head.m_increment = array_section->m_args[i].m_step;
                }
                head.loc = head.m_v->base.loc;

                doloop_body.reserve(al, 1);
                if( doloop == nullptr ) {
                    loop_body();
                } else {
                    doloop_body.push_back(al, doloop);
                }
                doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head,
                            doloop_body.p, doloop_body.size(), nullptr, 0));
            }
            result_vec->push_back(al, doloop);
        }

        template <typename LOOP_BODY>
        static inline void create_do_loop_assign(Allocator& al, const Location& loc,
            ASR::expr_t* lhs, ASR::expr_t* rhs, Vec<ASR::expr_t*>& idx_vars, Vec<ASR::expr_t*>& temp_idx_vars_lhs, 
            Vec<ASR::expr_t*>& temp_idx_vars_rhs, Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body, 
            SymbolTable* current_scope, Vec<ASR::stmt_t*>* result_vec) {
            if (ASR::is_a<ASR::ArrayItem_t>(*lhs) && ASR::is_a<ASR::ArrayItem_t>(*rhs)) {
                // Case : A([1,2,3]) = B([1,2,3])
                ASR::ArrayItem_t* lhs_array = ASR::down_cast<ASR::ArrayItem_t>(lhs);
                ASR::ArrayItem_t* rhs_array = ASR::down_cast<ASR::ArrayItem_t>(rhs);
                int n_array_indices = 0;
                for( size_t i = 0; i < rhs_array->n_args; i++ ) {
                    if (ASRUtils::is_array(ASRUtils::expr_type(rhs_array->m_args[i].m_right))) {
                        n_array_indices++;
                    }
                }
                if(n_array_indices == 0) return;
                PassUtils::create_idx_vars(idx_vars, n_array_indices, loc, al, current_scope, "_t");
                temp_idx_vars_rhs.reserve(al, rhs_array->n_args);
                temp_idx_vars_lhs.reserve(al, lhs_array->n_args);
                for( size_t i = 0, j = 0; i < rhs_array->n_args; i++ ) {
                    if (ASRUtils::is_array(ASRUtils::expr_type(rhs_array->m_args[i].m_right))) {
                        ASR::expr_t* ref = PassUtils::create_array_ref(rhs_array->m_args[i].m_right, idx_vars[j], al, current_scope);
                        temp_idx_vars_rhs.push_back(al, ref);
                        j++;
                    } else { 
                        temp_idx_vars_rhs.push_back(al, rhs_array->m_args[i].m_right);
                    }
                }
                for( size_t i = 0, j = 0; i < lhs_array->n_args; i++ ) {
                    if (ASRUtils::is_array(ASRUtils::expr_type(lhs_array->m_args[i].m_right))) {
                        ASR::expr_t* ref = PassUtils::create_array_ref(lhs_array->m_args[i].m_right, idx_vars[j], al, current_scope);
                        temp_idx_vars_lhs.push_back(al, ref);
                        j++;
                    } else { 
                        temp_idx_vars_lhs.push_back(al, lhs_array->m_args[i].m_right);
                    }
                }

                ASR::stmt_t* doloop = nullptr;
                for( size_t i = 0, j = 0; i < rhs_array->n_args; i++ ) {
                    ASR::do_loop_head_t head;
                    head.m_v = idx_vars[j];
                    if (ASRUtils::is_array(ASRUtils::expr_type(rhs_array->m_args[i].m_right))) {
                        head.m_start = PassUtils::get_bound(rhs_array->m_args[i].m_right, 1, "lbound", al);
                        head.m_end = PassUtils::get_bound(rhs_array->m_args[i].m_right, 1, "ubound", al);
                        head.m_increment = nullptr;
                        j++;
                    } else {
                        continue;
                    } 
                    head.loc = head.m_v->base.loc;

                    doloop_body.reserve(al, 1);
                    if( doloop == nullptr ) {
                        loop_body();
                    } else {
                        doloop_body.push_back(al, doloop);
                    }
                    doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head,
                        doloop_body.p, doloop_body.size(), nullptr, 0));
                }
                result_vec->push_back(al, doloop);
            } else if (ASR::is_a<ASR::ArrayItem_t>(*lhs) && ASR::is_a<ASR::Var_t>(*rhs)) {
                // Case : A([1,2,3]) = A2
                ASR::ArrayItem_t* lhs_array = ASR::down_cast<ASR::ArrayItem_t>(lhs);
                int n_array_indices = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(rhs));
                if(n_array_indices == 0) return;
                PassUtils::create_idx_vars(idx_vars, n_array_indices, loc, al, current_scope, "_t");
                temp_idx_vars_rhs.reserve(al, n_array_indices);
                temp_idx_vars_lhs.reserve(al, lhs_array->n_args);
                for( int i = 0; i < n_array_indices; i++ ) {
                    temp_idx_vars_rhs.push_back(al, idx_vars[i]);
                }
                for( size_t i = 0, j = 0; i < lhs_array->n_args; i++ ) {
                    if (ASRUtils::is_array(ASRUtils::expr_type(lhs_array->m_args[i].m_right))) {
                        ASR::expr_t* ref = PassUtils::create_array_ref(lhs_array->m_args[i].m_right, idx_vars[j], al, current_scope);
                        temp_idx_vars_lhs.push_back(al, ref);
                        j++;
                    } else { 
                        temp_idx_vars_lhs.push_back(al, lhs_array->m_args[i].m_right);
                    }
                }

                ASR::stmt_t* doloop = nullptr;
                for( size_t i = 0, j = 0; i < lhs_array->n_args; i++ ) {
                    ASR::do_loop_head_t head;
                    head.m_v = idx_vars[j];
                    if (ASRUtils::is_array(ASRUtils::expr_type(lhs_array->m_args[i].m_right))) {
                        head.m_start = PassUtils::get_bound(lhs_array->m_args[i].m_right, 1, "lbound", al);
                        head.m_end = PassUtils::get_bound(lhs_array->m_args[i].m_right, 1, "ubound", al);
                        head.m_increment = nullptr;
                        j++;
                    } else {
                        continue;
                    } 
                    head.loc = head.m_v->base.loc;

                    doloop_body.reserve(al, 1);
                    if( doloop == nullptr ) {
                        loop_body();
                    } else {
                        doloop_body.push_back(al, doloop);
                    }
                    doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr, head,
                        doloop_body.p, doloop_body.size(), nullptr, 0));
                }
                result_vec->push_back(al, doloop);
            }
        }

        void visit_ArrayConstant(ASR::ArrayConstant_t* x, Allocator& al,
            ASR::expr_t* arr_var, Vec<ASR::stmt_t*>* result_vec,
            ASR::expr_t* idx_var, SymbolTable* current_scope,
            bool perform_cast=false, ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr);

        void visit_ArrayConstructor(ASR::ArrayConstructor_t* x, Allocator& al,
            ASR::expr_t* arr_var, Vec<ASR::stmt_t*>* result_vec,
            ASR::expr_t* idx_var, SymbolTable* current_scope,
            bool perform_cast=false, ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr);

        template <typename T>
        static inline void replace_ArrayConstant(ASR::ArrayConstant_t* x, T* replacer,
            bool& remove_original_statement, Vec<ASR::stmt_t*>* result_vec,
            bool perform_cast=false,
            ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr) {
            LCOMPILERS_ASSERT(replacer->result_var != nullptr);
            if( ASRUtils::get_fixed_size_of_array(x->m_type) == 0 ) {
                remove_original_statement = true;
                return ;
            }

            const Location& loc = x->base.base.loc;
            if( ASR::is_a<ASR::Var_t>(*replacer->result_var) ) {
                [[maybe_unused]] ASR::ttype_t* result_var_type = ASRUtils::expr_type(replacer->result_var);
                LCOMPILERS_ASSERT_MSG(ASRUtils::extract_n_dims_from_ttype(result_var_type) == 1,
                                    "Initialisation using ArrayConstant is "
                                    "supported only for single dimensional arrays, found: " +
                                    std::to_string(ASRUtils::extract_n_dims_from_ttype(result_var_type)))
                Vec<ASR::expr_t*> idx_vars;
                PassUtils::create_idx_vars(idx_vars, 1, loc, replacer->al, replacer->current_scope);
                ASR::expr_t* idx_var = idx_vars[0];
                ASR::expr_t* lb = PassUtils::get_bound(replacer->result_var, 1, "lbound", replacer->al);
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al,
                                                loc, idx_var, lb, nullptr));
                result_vec->push_back(replacer->al, assign_stmt);
                visit_ArrayConstant(x, replacer->al, replacer->result_var, result_vec,
                                    idx_var, replacer->current_scope,
                                    perform_cast, cast_kind, casted_type);
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
                for( size_t k = 0; k < (size_t) ASRUtils::get_fixed_size_of_array(x->m_type); k++ ) {
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

                    ASR::expr_t* array_ref = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(replacer->al,
                                                    target_section->base.base.loc,
                                                    target_section->m_v,
                                                    args.p, args.size(),
                                                    ASRUtils::type_get_past_pointer(
                                                        ASRUtils::type_get_past_allocatable(array_ref_type)),
                                                    ASR::arraystorageType::RowMajor, nullptr));
                    ASR::expr_t* x_m_args_k = ASRUtils::fetch_ArrayConstant_value(replacer->al, x, k);
                    if( perform_cast ) {
                        LCOMPILERS_ASSERT(casted_type != nullptr);
                        x_m_args_k = ASRUtils::EXPR(ASR::make_Cast_t(replacer->al, array_ref->base.loc,
                            x_m_args_k, cast_kind, casted_type, nullptr));
                    }
                    ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, target_section->base.base.loc,
                                                    array_ref, x_m_args_k, nullptr));
                    result_vec->push_back(replacer->al, assign_stmt);
                    ASR::expr_t* increment = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, target_section->base.base.loc,
                                                idx_var, ASR::binopType::Add, const_1, ASRUtils::expr_type(idx_var), nullptr));
                    assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, target_section->base.base.loc, idx_var, increment, nullptr));
                    result_vec->push_back(replacer->al, assign_stmt);
                }
            }
        }

        static inline void replace_ArrayConstructor_(Allocator& al, ASR::ArrayConstructor_t* x,
            ASR::expr_t* result_var, Vec<ASR::stmt_t*>* result_vec, SymbolTable* current_scope,
            bool perform_cast=false, ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr) {
            LCOMPILERS_ASSERT(result_var != nullptr);
            const Location& loc = x->base.base.loc;
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Var_t>(*result_var));
            [[maybe_unused]] ASR::ttype_t* result_var_type = ASRUtils::expr_type(result_var);
            LCOMPILERS_ASSERT_MSG(ASRUtils::extract_n_dims_from_ttype(result_var_type) == 1,
                                "Initialisation using ArrayConstructor is "
                                "supported only for single dimensional arrays, found: " +
                                std::to_string(ASRUtils::extract_n_dims_from_ttype(result_var_type)))
            Vec<ASR::expr_t*> idx_vars;
            PassUtils::create_idx_vars(idx_vars, 1, loc, al, current_scope, "__libasr_index_");
            ASR::expr_t* idx_var = idx_vars[0];
            ASR::expr_t* lb = PassUtils::get_bound(result_var, 1, "lbound", al);
            ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al,
                                            loc, idx_var, lb, nullptr));
            result_vec->push_back(al, assign_stmt);
            visit_ArrayConstructor(x, al, result_var, result_vec,
                idx_var, current_scope, perform_cast, cast_kind, casted_type);
        }

        template <typename T>
        static inline void replace_ArrayConstructor(ASR::ArrayConstructor_t* x, T* replacer,
            bool& remove_original_statement, Vec<ASR::stmt_t*>* result_vec,
            bool perform_cast=false,
            ASR::cast_kindType cast_kind=ASR::cast_kindType::IntegerToInteger,
            ASR::ttype_t* casted_type=nullptr) {
            LCOMPILERS_ASSERT(replacer->result_var != nullptr);
            if( x->n_args == 0 ) {
                remove_original_statement = true;
                return ;
            }

            const Location& loc = x->base.base.loc;
            if( ASR::is_a<ASR::Var_t>(*replacer->result_var) ) {
                [[maybe_unused]] ASR::ttype_t* result_var_type = ASRUtils::expr_type(replacer->result_var);
                LCOMPILERS_ASSERT_MSG(ASRUtils::extract_n_dims_from_ttype(result_var_type) == 1,
                                    "Initialisation using ArrayConstructor is "
                                    "supported only for single dimensional arrays, found: " +
                                    std::to_string(ASRUtils::extract_n_dims_from_ttype(result_var_type)))
                Vec<ASR::expr_t*> idx_vars;
                PassUtils::create_idx_vars(idx_vars, 1, loc, replacer->al, replacer->current_scope);
                ASR::expr_t* idx_var = idx_vars[0];
                ASR::expr_t* lb = PassUtils::get_bound(replacer->result_var, 1, "lbound", replacer->al);
                ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al,
                                                loc, idx_var, lb, nullptr));
                result_vec->push_back(replacer->al, assign_stmt);
                visit_ArrayConstructor(x, replacer->al, replacer->result_var, result_vec,
                                    idx_var, replacer->current_scope,
                                    perform_cast, cast_kind, casted_type);
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

                        ASR::expr_t* array_ref = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(replacer->al,
                                                        target_section->base.base.loc,
                                                        target_section->m_v,
                                                        args.p, args.size(),
                                                        ASRUtils::type_get_past_pointer(
                                                            ASRUtils::type_get_past_allocatable(array_ref_type)),
                                                        ASR::arraystorageType::RowMajor, nullptr));
                        ASR::expr_t* x_m_args_k = x->m_args[k];
                        if( perform_cast ) {
                            LCOMPILERS_ASSERT(casted_type != nullptr);
                            x_m_args_k = ASRUtils::EXPR(ASR::make_Cast_t(replacer->al, array_ref->base.loc,
                                x_m_args_k, cast_kind, casted_type, nullptr));
                        }
                        ASR::stmt_t* assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, target_section->base.base.loc,
                                                        array_ref, x_m_args_k, nullptr));
                        result_vec->push_back(replacer->al, assign_stmt);
                        ASR::expr_t* increment = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(replacer->al, target_section->base.base.loc,
                                                    idx_var, ASR::binopType::Add, const_1, ASRUtils::expr_type(idx_var), nullptr));
                        assign_stmt = ASRUtils::STMT(ASR::make_Assignment_t(replacer->al, target_section->base.base.loc, idx_var, increment, nullptr));
                        result_vec->push_back(replacer->al, assign_stmt);
                    }
                }
            }
        }
    }

    static inline void handle_fn_return_var(Allocator &al, ASR::Function_t *x,
            bool (*is_array_or_struct_or_symbolic)(ASR::expr_t*)) {
        if (ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::BindPython) {
            return;
        }
        if (x->m_return_var) {
            /*
            * The `return_var` of the function, which is either an array or
            * struct, is set to `null`, and the destination variable will be
            * passed as the last argument to the existing function. This helps
            * in avoiding deep copies and the destination memory directly gets
            * filled inside the function.
            */
            if( is_array_or_struct_or_symbolic(x->m_return_var) || is_symbolic_list_type(x->m_return_var)) {
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
                                                ASRUtils::expr_type(e), x->m_symtab));
                }
                s_func_type->m_arg_types = arg_types.p;
                s_func_type->n_arg_types = arg_types.n;
                s_func_type->m_return_var_type = nullptr;
            }
        }
        for (auto &item : x->m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                handle_fn_return_var(al, ASR::down_cast<ASR::Function_t>(
                    item.second), is_array_or_struct_or_symbolic);
            }
        }
    }


    } // namespace PassUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_UTILS_H
