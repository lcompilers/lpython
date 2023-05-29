#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/pass_array_by_data.h>

#include <vector>
#include <utility>
#include <deque>

/*
This ASR to ASR pass can be called whenever you want to avoid
using descriptors for passing arrays to functions/subroutines
in the backend. By default, it is currently being used by all
the backends (via pass manager). Backends like WASM which
do not support array descriptors always need this pass
to be called.

The possibility to avoid descriptors and pass array by data
to functions/subroutines is determined by ASRUtils::is_pass_array_by_data_possible
defined in asr_utils.h.

Advantages and dis-advantages of this pass are as follows,

Advantages:

* Avoiding array descriptors and just using simple data points leads to
easier handling in the backend.

* A lot of indirection to access dimensional information is also avoided
because it is already provided via extra variable arguments.

Dis-advantages:

* Requires access to all the code, as function interfaces have to be modified. Hence, not always possible.

* The arrays become contiguous, which most of the time is fine, but sometimes you might lose performance.

*/


namespace LCompilers {

/*
The following visitor converts function/subroutines (a.k.a procedures)
with array arguments having empty dimensions to arrays having dimensional
information available from function arguments. See example below,

    subroutine f(array1, array2)
        integer, intent(in) :: array1(:)
        integer, intent(out) :: array2(:)
    end subroutine

gets converted to,

    subroutine f_array1_array2(array1, m1, n1, array2, m2, n2)
        integer, intent(in) :: m1, m2, n1, n2
        integer, intent(in) :: array1(m1:n1)
        integer, intent(out) :: array2(m2:n2)
    end subroutine
 */
class PassArrayByDataProcedureVisitor : public PassUtils::PassVisitor<PassArrayByDataProcedureVisitor>
{
    private:

        ASRUtils::ExprStmtDuplicator node_duplicator;

    public:

        std::map< ASR::symbol_t*, std::pair<ASR::symbol_t*, std::vector<size_t>> > proc2newproc;
        std::set<ASR::symbol_t*> newprocs;

        PassArrayByDataProcedureVisitor(Allocator& al_) : PassVisitor(al_, nullptr),
        node_duplicator(al_)
        {}

        ASR::symbol_t* insert_new_procedure(ASR::Function_t* x, std::vector<size_t>& indices) {
            Vec<ASR::stmt_t*> new_body;
            new_body.reserve(al, x->n_body);
            node_duplicator.allow_procedure_calls = true;
            node_duplicator.allow_reshape = false;
            for( size_t i = 0; i < x->n_body; i++ ) {
                node_duplicator.success = true;
                ASR::stmt_t* new_stmt = node_duplicator.duplicate_stmt(x->m_body[i]);
                if( !node_duplicator.success ) {
                    return nullptr;
                }
                new_body.push_back(al, new_stmt);
            }

            node_duplicator.allow_procedure_calls = true;
            SymbolTable* new_symtab = al.make_new<SymbolTable>(current_scope);
            ASRUtils::SymbolDuplicator symbol_duplicator(al);
            for( auto& item: x->m_symtab->get_scope() ) {
                symbol_duplicator.duplicate_symbol(item.second, new_symtab);
            }
            Vec<ASR::expr_t*> new_args;
            std::string suffix = "";
            new_args.reserve(al, x->n_args);
            ASR::expr_t* return_var = nullptr;
            for( size_t i = 0; i < x->n_args + 1; i++ ) {
                ASR::Variable_t* arg = nullptr;
                ASR::Function_t* arg_func = nullptr;
                if( i < x->n_args ) {
                    if (ASR::is_a<ASR::Var_t>(*(x->m_args[i]))) {
                        ASR::Var_t* x_arg = ASR::down_cast<ASR::Var_t>(x->m_args[i]);
                        if (ASR::is_a<ASR::Function_t>(*(x_arg->m_v))) {
                            arg_func = ASR::down_cast<ASR::Function_t>(x_arg->m_v);
                        } else {
                            arg = ASRUtils::EXPR2VAR(x->m_args[i]);
                        }
                    }
                } else if( x->m_return_var ) {
                    arg = ASRUtils::EXPR2VAR(x->m_return_var);
                } else {
                    break ;
                }
                if( std::find(indices.begin(), indices.end(), i) !=
                    indices.end() ) {
                    if( arg_func ) {
                        suffix += "_" + std::string(arg_func->m_name);
                    } else {
                        suffix += "_" + std::string(arg->m_name);
                    }
                }
                ASR::expr_t* new_arg;
                if (arg_func) {
                    new_arg = ASRUtils::EXPR(ASR::make_Var_t(al,
                                        arg_func->base.base.loc, new_symtab->get_symbol(
                                                        std::string(arg_func->m_name))));
                } else {
                    new_arg = ASRUtils::EXPR(ASR::make_Var_t(al,
                                        arg->base.base.loc, new_symtab->get_symbol(
                                                        std::string(arg->m_name))));
                }
                if( i < x->n_args ) {
                    new_args.push_back(al, new_arg);
                } else {
                    return_var = new_arg;
                }
            }
            ASR::symbol_t* new_symbol = nullptr;
            std::string new_name = std::string(x->m_name) + suffix;
            if( ASR::is_a<ASR::Function_t>( *((ASR::symbol_t*) x) ) ) {
                ASR::FunctionType_t* x_func_type = ASRUtils::get_FunctionType(x);
                std::string new_bindc_name = "";
                if( x_func_type->m_bindc_name ) {
                    new_bindc_name = std::string(x_func_type->m_bindc_name) + suffix;
                }
                ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, x->base.base.loc,
                                            new_symtab, s2c(al, new_name), x->m_dependencies, x->n_dependencies,
                                            new_args.p, new_args.size(),  new_body.p, new_body.size(),
                                            return_var, x_func_type->m_abi, x->m_access, x_func_type->m_deftype,
                                            s2c(al, new_bindc_name), x_func_type->m_elemental,
                                            x_func_type->m_pure, x_func_type->m_module, x_func_type->m_inline,
                                            x_func_type->m_static, x_func_type->m_type_params, x_func_type->n_type_params, nullptr, 0, false, false, false);
                new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
            }
            current_scope->add_symbol(new_name, new_symbol);
            proc2newproc[(ASR::symbol_t*) x] = std::make_pair(new_symbol, indices);
            newprocs.insert(new_symbol);
            return new_symbol;
        }

        void edit_new_procedure_args(ASR::Function_t* x, std::vector<size_t>& indices) {
            Vec<ASR::expr_t*> new_args;
            new_args.reserve(al, x->n_args);
            for( size_t i = 0; i < x->n_args; i++ ) {
                new_args.push_back(al, x->m_args[i]);
                if( std::find(indices.begin(), indices.end(), i) !=
                    indices.end() ) {
                    ASR::Variable_t* arg = ASRUtils::EXPR2VAR(x->m_args[i]);
                    ASR::dimension_t* dims = nullptr;
                    int n_dims = ASRUtils::extract_dimensions_from_ttype(arg->m_type, dims);
                    Vec<ASR::expr_t*> dim_variables;
                    std::string arg_name = std::string(arg->m_name);
                    PassUtils::create_vars(dim_variables, 2 * n_dims, arg->base.base.loc, al,
                                            x->m_symtab, arg_name, ASR::intentType::In, arg->m_presence);
                    Vec<ASR::dimension_t> new_dims;
                    new_dims.reserve(al, n_dims);
                    for( int j = 0, k = 0; j < n_dims; j++ ) {
                        ASR::dimension_t new_dim;
                        new_dim.loc = dims[j].loc;
                        new_dim.m_start = dim_variables[k];
                        new_dim.m_length = dim_variables[k + 1];
                        new_dims.push_back(al, new_dim);
                        k += 2;
                    }
                    ASR::ttype_t* new_type = ASRUtils::duplicate_type(al, arg->m_type, &new_dims);
                    arg->m_type = new_type;
                    for( int k = 0; k < 2 * n_dims; k++ ) {
                        new_args.push_back(al, dim_variables[k]);
                    }
                }
            }

            x->m_args = new_args.p;
            x->n_args = new_args.size();
        }

        void visit_TranslationUnit(const ASR::TranslationUnit_t& x) {
            // Visit Module first so that all functions in it are updated
            for (auto &a : x.m_global_scope->get_scope()) {
                if( ASR::is_a<ASR::Module_t>(*a.second) ) {
                    this->visit_symbol(*a.second);
                }
            }

            // Visit all other symbols
            for (auto &a : x.m_global_scope->get_scope()) {
                if( !ASR::is_a<ASR::Module_t>(*a.second) ) {
                    this->visit_symbol(*a.second);
                }
            }
        }

        template <typename T>
        bool visit_SymbolContainingFunctions(const T& x,
            std::deque<ASR::Function_t*>& pass_array_by_data_functions) {
            T& xx = const_cast<T&>(x);
            current_scope = xx.m_symtab;
            for( auto& item: xx.m_symtab->get_scope() ) {
                if( ASR::is_a<ASR::Function_t>(*item.second) ) {
                    ASR::Function_t* subrout = ASR::down_cast<ASR::Function_t>(item.second);
                    std::vector<size_t> arg_indices;
                    if( ASRUtils::is_pass_array_by_data_possible(subrout, arg_indices) ) {
                        ASR::symbol_t* sym = insert_new_procedure(subrout, arg_indices);
                        if( sym != nullptr ) {
                            ASR::Function_t* new_subrout = ASR::down_cast<ASR::Function_t>(sym);
                            edit_new_procedure_args(new_subrout, arg_indices);
                            pass_array_by_data_functions.push_back(new_subrout);
                        }
                    }
                }
            }
            return pass_array_by_data_functions.size() > 0;
        }

        #define bfs_visit_SymbolContainingFunctions() std::deque<ASR::Function_t*> pass_array_by_data_functions;    \
            visit_SymbolContainingFunctions(x, pass_array_by_data_functions);    \
            while( pass_array_by_data_functions.size() > 0 ) {    \
                ASR::Function_t* function = pass_array_by_data_functions.front();    \
                pass_array_by_data_functions.pop_front();    \
                visit_SymbolContainingFunctions(*function, pass_array_by_data_functions);    \
            }    \

        void visit_Program(const ASR::Program_t& x) {
            bfs_visit_SymbolContainingFunctions()
        }

        void visit_Module(const ASR::Module_t& x) {
            // Do not visit intrinsic modules
            if( x.m_intrinsic ) {
                return ;
            }

            bfs_visit_SymbolContainingFunctions()
        }
};

class EditProcedureVisitor: public ASR::ASRPassBaseWalkVisitor<EditProcedureVisitor> {

    public:

    PassArrayByDataProcedureVisitor& v;

    EditProcedureVisitor(PassArrayByDataProcedureVisitor& v_):
    v(v_) {}

    void visit_Function(const ASR::Function_t &x) {
        ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }

        visit_ttype(*x.m_function_signature);

        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
        }
        transform_stmts(xx.m_body, xx.n_body);
        if (x.m_return_var)
            visit_expr(*x.m_return_var);
        current_scope = current_scope_copy;
    }

    #define edit_symbol(attr) ASR::symbol_t* x_sym = xx.m_##attr;    \
        SymbolTable* x_sym_symtab = ASRUtils::symbol_parent_symtab(x_sym);    \
        if( x_sym_symtab->get_counter() != current_scope->get_counter() &&    \
            !ASRUtils::is_parent(x_sym_symtab, current_scope) ) {    \
            std::string x_sym_name = std::string(ASRUtils::symbol_name(x_sym));    \
            xx.m_##attr = current_scope->resolve_symbol(x_sym_name);    \
            LCOMPILERS_ASSERT(xx.m_##attr != nullptr);    \
        }    \

    void visit_Var(const ASR::Var_t& x) {
        ASR::Var_t& xx = const_cast<ASR::Var_t&>(x);
        ASR::symbol_t* x_sym_ = xx.m_v;
        if ( v.proc2newproc.find(x_sym_) != v.proc2newproc.end() ) {
            xx.m_v = v.proc2newproc[x_sym_].first;
            return ;
        }

        edit_symbol(v)
    }

    void visit_BlockCall(const ASR::BlockCall_t& x) {
        ASR::BlockCall_t& xx = const_cast<ASR::BlockCall_t&>(x);
        edit_symbol(m)
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        ASR::FunctionCall_t& xx = const_cast<ASR::FunctionCall_t&>(x);
        edit_symbol(name)
        ASR::ASRPassBaseWalkVisitor<EditProcedureVisitor>::visit_FunctionCall(x);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        ASR::SubroutineCall_t& xx = const_cast<ASR::SubroutineCall_t&>(x);
        edit_symbol(name)
        ASR::ASRPassBaseWalkVisitor<EditProcedureVisitor>::visit_SubroutineCall(x);
    }

};

/*
    The following visitor replaces procedure calls with arrays as arguments
    to procedure calls having dimensional information passed as arguments. See example below,

        call f1(array1, array2)
        sum = f(array) + g(array)

    gets converted to,

        call f1_array1_array2(array1, m1, n1, array2, m2, n2)
        sum = f_array(array, m, n) + g_array(array, m, n)

    As can be seen dimensional information, m1, n1 is passed along
    with array1 and similarly m2, n2 is passed along with array2.
 */
class EditProcedureCallsVisitor : public ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>
{
    private:

        Allocator& al;
        PassArrayByDataProcedureVisitor& v;
        std::set<ASR::symbol_t*>& not_to_be_erased;

    public:

        EditProcedureCallsVisitor(Allocator& al_,
            PassArrayByDataProcedureVisitor& v_,
            std::set<ASR::symbol_t*>& not_to_be_erased_):
        al(al_), v(v_), not_to_be_erased(not_to_be_erased_) {}

        template <typename T>
        void update_args_for_pass_arr_by_data_funcs_passed_as_callback(const T& x) {
            bool args_updated = false;
            Vec<ASR::call_arg_t> new_args;
            new_args.reserve(al, x.n_args);
            for ( size_t i = 0; i < x.n_args; i++ ) {
                ASR::call_arg_t arg = x.m_args[i];
                ASR::expr_t* expr = arg.m_value;
                if (expr) {
                    if (ASR::is_a<ASR::Var_t>(*expr)) {
                        ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(expr);
                        ASR::symbol_t* sym = var->m_v;
                        if ( v.proc2newproc.find(sym) != v.proc2newproc.end() ) {
                            ASR::symbol_t* new_var_sym = v.proc2newproc[sym].first;
                            ASR::expr_t* new_var = ASRUtils::EXPR(ASR::make_Var_t(al, var->base.base.loc, new_var_sym));
                            {
                                // update exisiting arg
                                arg.m_value = new_var;
                                arg.loc = arg.loc;
                            }
                            args_updated = true;
                        }
                    }
                }
                new_args.push_back(al, arg);
            }
            if (args_updated) {
                T&xx = const_cast<T&>(x);
                xx.m_args = new_args.p;
                xx.n_args = new_args.size();
            }
        }

        Vec<ASR::call_arg_t> construct_new_args(size_t n_args, ASR::call_arg_t* orig_args, std::vector<size_t>& indices) {
            Vec<ASR::call_arg_t> new_args;
            new_args.reserve(al, n_args);
            for( size_t i = 0; i < n_args; i++ ) {
                new_args.push_back(al, orig_args[i]);
                if (orig_args[i].m_value == nullptr ||
                    std::find(indices.begin(), indices.end(), i) == indices.end()) {
                    continue;
                }

                Vec<ASR::expr_t*> dim_vars;
                dim_vars.reserve(al, 2);
                ASRUtils::get_dimensions(orig_args[i].m_value, dim_vars, al);
                for( size_t j = 0; j < dim_vars.size(); j++ ) {
                    ASR::call_arg_t dim_var;
                    dim_var.loc = dim_vars[j]->base.loc;
                    dim_var.m_value = dim_vars[j];
                    new_args.push_back(al, dim_var);
                }
            }
            return new_args;
        }

        bool can_edit_call(ASR::call_arg_t* args, size_t n_args) {
            for ( size_t i = 0; i < n_args; i++ ) {
                if( args[i].m_value &&
                    ASRUtils::expr_type(args[i].m_value) &&
                    ASR::is_a<ASR::Pointer_t>(*
                        ASRUtils::expr_type(args[i].m_value)) ) {
                    return false;
                }
            }
            return true;
        }

        template <typename T>
        void visit_Call(const T& x) {
            ASR::symbol_t* subrout_sym = x.m_name;
            bool is_external = ASR::is_a<ASR::ExternalSymbol_t>(*subrout_sym);
            subrout_sym = ASRUtils::symbol_get_past_external(subrout_sym);

            if( !can_edit_call(x.m_args, x.n_args) ) {
                not_to_be_erased.insert(subrout_sym);
                return ;
            }

            if( v.proc2newproc.find(subrout_sym) == v.proc2newproc.end() ) {
                update_args_for_pass_arr_by_data_funcs_passed_as_callback(x);
                return;
            }

            ASR::symbol_t* new_func_sym = v.proc2newproc[subrout_sym].first;
            std::vector<size_t>& indices = v.proc2newproc[subrout_sym].second;

            Vec<ASR::call_arg_t> new_args = construct_new_args(x.n_args, x.m_args, indices);

            {
                ASR::Function_t* new_func_ = ASR::down_cast<ASR::Function_t>(new_func_sym);
                size_t min_args = 0, max_args = 0;
                for( size_t i = 0; i < new_func_->n_args; i++ ) {
                    ASR::Var_t* arg = ASR::down_cast<ASR::Var_t>(new_func_->m_args[i]);
                    if( ASR::is_a<ASR::Variable_t>(*arg->m_v) &&
                        ASR::down_cast<ASR::Variable_t>(arg->m_v)->m_presence
                            == ASR::presenceType::Optional ) {
                        max_args += 1;
                    } else {
                        min_args += 1;
                        max_args += 1;
                    }
                }
                if( !(min_args <= new_args.size() &&
                    new_args.size() <= max_args) ) {
                    throw LCompilersException("Number of arguments in the new "
                                            "function call doesn't satisfy "
                                            "min_args <= new_args.size() <= max_args, " +
                                            std::to_string(min_args) + " <= " +
                                            std::to_string(new_args.size()) + " <= " +
                                            std::to_string(max_args));
                }
            }
            ASR::symbol_t* new_func_sym_ = new_func_sym;
            if( is_external ) {
                ASR::ExternalSymbol_t* func_ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
                // TODO: Use SymbolTable::get_unique_name to avoid potential
                // clashes with user defined functions
                char* new_func_sym_name = ASRUtils::symbol_name(new_func_sym);
                if( current_scope->resolve_symbol(new_func_sym_name) == nullptr ) {
                    new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                        ASR::make_ExternalSymbol_t(al, x.m_name->base.loc, func_ext_sym->m_parent_symtab,
                            new_func_sym_name, new_func_sym, func_ext_sym->m_module_name,
                            func_ext_sym->m_scope_names, func_ext_sym->n_scope_names, new_func_sym_name,
                            func_ext_sym->m_access));
                    func_ext_sym->m_parent_symtab->add_symbol(new_func_sym_name, new_func_sym_);
                } else {
                    new_func_sym_ = current_scope->resolve_symbol(new_func_sym_name);
                }
            }
            T& xx = const_cast<T&>(x);
            xx.m_name = new_func_sym_;
            xx.m_original_name = new_func_sym_;
            xx.m_args = new_args.p;
            xx.n_args = new_args.size();
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            visit_Call(x);
            ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>::visit_SubroutineCall(x);
        }

        void visit_FunctionCall(const ASR::FunctionCall_t& x) {
            visit_Call(x);
            ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>::visit_FunctionCall(x);
        }
};

/*
Since the above visitors have replaced procedure and calls to those procedures
with arrays as arguments, we don't need the original ones anymore. So we remove
them from the ASR. The reason to do this is that some backends like WASM don't
have support for accepting and returning arrays via array descriptors. Therefore,
they cannot generate code for such functions. To avoid backends like WASM from failing
we remove those functions by implementing and calling the following visitor.
*/
class RemoveArrayByDescriptorProceduresVisitor : public PassUtils::PassVisitor<RemoveArrayByDescriptorProceduresVisitor>
{
    private:

        PassArrayByDataProcedureVisitor& v;
        std::set<ASR::symbol_t*>& not_to_be_erased;

    public:

        RemoveArrayByDescriptorProceduresVisitor(Allocator& al_, PassArrayByDataProcedureVisitor& v_,
            std::set<ASR::symbol_t*>& not_to_be_erased_):
            PassVisitor(al_, nullptr), v(v_), not_to_be_erased(not_to_be_erased_) {}

        // Shouldn't be done because allocatable arrays when
        // assigned to array constants work fine in gfortran
        // and hence we will need calling the original function
        // and not the new one. WASM backend should be supporting
        // such cases for the following to be removed.
        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            current_scope = xx.m_symtab;

            std::vector<std::string> to_be_erased;

            for( auto& item: current_scope->get_scope() ) {
                if( v.proc2newproc.find(item.second) != v.proc2newproc.end() &&
                    not_to_be_erased.find(item.second) == not_to_be_erased.end() ) {
                    LCOMPILERS_ASSERT(item.first == ASRUtils::symbol_name(item.second))
                    to_be_erased.push_back(item.first);
                }
            }

            for (auto &item: to_be_erased) {
                current_scope->erase_symbol(item);
            }
        }

        void visit_Function(const ASR::Function_t& x) {
            ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
            current_scope = xx.m_symtab;

            std::vector<std::string> to_be_erased;

            for( auto& item: current_scope->get_scope() ) {
                if( v.proc2newproc.find(item.second) != v.proc2newproc.end() &&
                    not_to_be_erased.find(item.second) == not_to_be_erased.end() ) {
                    LCOMPILERS_ASSERT(item.first == ASRUtils::symbol_name(item.second))
                    to_be_erased.push_back(item.first);
                }
            }

            for (auto &item: to_be_erased) {
                current_scope->erase_symbol(item);
            }
        }

};

void pass_array_by_data(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/) {
    PassArrayByDataProcedureVisitor v(al);
    v.visit_TranslationUnit(unit);
    EditProcedureVisitor e(v);
    e.visit_TranslationUnit(unit);
    std::set<ASR::symbol_t*> not_to_be_erased;
    EditProcedureCallsVisitor u(al, v, not_to_be_erased);
    u.visit_TranslationUnit(unit);
    RemoveArrayByDescriptorProceduresVisitor x(al, v, not_to_be_erased);
    x.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor y(al);
    y.visit_TranslationUnit(unit);
}

} // namespace LCompilers
