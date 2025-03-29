#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/array_by_data.h>

#include <vector>
#include <utility>
#include <deque>
#include <set>

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
            // first duplicate the external symbols
            // so they can be referenced by derived_type
            for( auto& item: x->m_symtab->get_scope() ) {
                if (ASR::is_a<ASR::ExternalSymbol_t>(*item.second)) {
                    symbol_duplicator.duplicate_symbol(item.second, new_symtab);
                }
            }
            for( auto& item: x->m_symtab->get_scope() ) {
                if (!ASR::is_a<ASR::ExternalSymbol_t>(*item.second)) {
                    symbol_duplicator.duplicate_symbol(item.second, new_symtab);
                }
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
                if( std::find(indices.begin(), indices.end(), i) != indices.end() ) {
                    if( arg_func ) {
                        suffix += "_" + ASRUtils::type_to_str_fortran(arg_func->m_function_signature);
                    } else {
                        suffix += "_" + ASRUtils::type_to_str_fortran(arg->m_type);
                    }
                    suffix += "_" + std::to_string(i);
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
            suffix = to_lower(suffix);
            for( size_t suffixi = 0; suffixi < suffix.size(); suffixi++ ) {
                if( !((suffix[suffixi] >= 'a' && suffix[suffixi] <= 'z') ||
                    (suffix[suffixi] >= '0' && suffix[suffixi] <= '9')) ) {
                    suffix[suffixi] = '_';
                }
            }
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
                    x_func_type->m_static,
                    x_func_type->m_restrictions, x_func_type->n_restrictions, false, false, false);
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

            ASR::FunctionType_t* func_type = ASRUtils::get_FunctionType(*x);
            x->m_function_signature = ASRUtils::TYPE(ASRUtils::make_FunctionType_t_util(
                al, func_type->base.base.loc, new_args.p, new_args.size(), x->m_return_var, func_type, current_scope));
            x->m_args = new_args.p;
            x->n_args = new_args.size();
        }


        template <typename T>
        bool visit_SymbolContainingFunctions(const T& x,
            std::deque<ASR::Function_t*>& pass_array_by_data_functions) {
            T& xx = const_cast<T&>(x);
            current_scope = xx.m_symtab;
            for( auto& item: xx.m_symtab->get_scope() ) {
                if( ASR::is_a<ASR::Function_t>(*item.second) ) {
                    ASR::Function_t* subrout = ASR::down_cast<ASR::Function_t>(item.second);
                    pass_array_by_data_functions.push_back(subrout);
                    std::vector<size_t> arg_indices;
                    if( ASRUtils::is_pass_array_by_data_possible(subrout, arg_indices) ) {
                        ASR::symbol_t* sym = insert_new_procedure(subrout, arg_indices);
                        if( sym != nullptr ) {
                            ASR::Function_t* new_subrout = ASR::down_cast<ASR::Function_t>(sym);
                            edit_new_procedure_args(new_subrout, arg_indices);
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

        void visit_TranslationUnit(const ASR::TranslationUnit_t& x) {
            // Visit functions in global scope first
            bfs_visit_SymbolContainingFunctions();

            // Visit Module so that all functions in it are updated
            for (auto &a : x.m_symtab->get_scope()) {
                if( ASR::is_a<ASR::Module_t>(*a.second) ) {
                    this->visit_symbol(*a.second);
                }
            }

            // Visit the program
            for (auto &a : x.m_symtab->get_scope()) {
                if( ASR::is_a<ASR::Program_t>(*a.second) ) {
                    this->visit_symbol(*a.second);
                }
            }
        }

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

#define edit_symbol_reference(attr) ASR::symbol_t* x_sym = xx.m_##attr;    \
    SymbolTable* x_sym_symtab = ASRUtils::symbol_parent_symtab(x_sym);    \
    if( x_sym_symtab->get_counter() != current_scope->get_counter() &&    \
        !ASRUtils::is_parent(x_sym_symtab, current_scope) ) {    \
        std::string x_sym_name = std::string(ASRUtils::symbol_name(x_sym));    \
        xx.m_##attr = current_scope->resolve_symbol(x_sym_name);    \
        LCOMPILERS_ASSERT(xx.m_##attr != nullptr);    \
    }    \

#define edit_symbol_pointer(attr) ASR::symbol_t* x_sym = x->m_##attr;    \
    SymbolTable* x_sym_symtab = ASRUtils::symbol_parent_symtab(x_sym);    \
    if( x_sym_symtab->get_counter() != current_scope->get_counter() &&    \
        !ASRUtils::is_parent(x_sym_symtab, current_scope) ) {    \
        std::string x_sym_name = std::string(ASRUtils::symbol_name(x_sym));    \
        x->m_##attr = current_scope->resolve_symbol(x_sym_name);    \
        LCOMPILERS_ASSERT(x->m_##attr != nullptr);    \
    }    \

class EditProcedureReplacer: public ASR::BaseExprReplacer<EditProcedureReplacer> {

    public:

    PassArrayByDataProcedureVisitor& v;
    SymbolTable* current_scope;

    EditProcedureReplacer(PassArrayByDataProcedureVisitor& v_):
     v(v_), current_scope(nullptr) {}

    /*
        We need this for the cases where pass array by data is possible for parent function and also
        for the child function. Let's take the following example:
        ```fortran
        subroutine cobyla(x)
            implicit none

            real, intent(inout) :: x(:)
            call evaluate(calcfc_internal)
            contains

            subroutine calcfc_internal(x_internal)
                implicit none
                real, intent(in) :: x_internal(:)
            end subroutine calcfc_internal

            subroutine evaluate(calcfc)
                use, non_intrinsic :: pintrf_mod, only : OBJCON
                implicit none
                procedure(OBJCON) :: calcfc
            end subroutine evaluate

        end subroutine
        ```

        Here, cobyla is parent and calcfc_internal is child. What happens is by `bfs` we first
        create `cobyla_real____0` and add it as a symbol in global scope. Then we visit `cobyla`,
        create `calcfc_internal_real____0` as replacement of `calcfc_internal` and add it as a symbol
        in scope of `cobyla`. Now, we visit `cobyla_real____0` and replace `calcfc_internal` with
        `calcfc_internal_real____0`
        
        This means both symbols `cobyla` and `cobyla_real____0` have `calcfc_internal_real____0` as a symbol
        but eventually `cobyla` is removed from scope.

        In `proc2newproc` we map `cobyla(1) -> cobyla_real____0(2)` and `calcfc_internal(1.1) -> calcfc_internal_real____0(1.2)`
        and also have `calcfc_internal(2.1) -> calcfc_internal_real____0(2.2)`. Where `(x)` represents symtab counter.

        When it comes to replace `call evaluate(calcfc_internal(1.1))`, it gets replaced with `call evaluate(calcfc_internal_real____0(1.2))`
        which eventually gets deleted. We need to actually replace it with `call evaluate(calcfc_internal_real____0(2.2))`, so what we do is
        we resolve the symbol to the latest symbol in the scope.

        From `calcfc_internal(1.1)`, we get `calcfc_internal_real____0(1.2)` and then we get the parent of `calcfc_internal_real____0(1.2)`,
        i.e. `cobyla(1)`, resolve it to `cobyla_real____0(2)` and then get the symbol `calcfc_internal_real____0(2.2)` from it.
    */
    ASR::symbol_t* resolve_new_proc(ASR::symbol_t* old_sym) {
        ASR::symbol_t* ext_sym = ASRUtils::symbol_get_past_external(old_sym);
        if( v.proc2newproc.find(ext_sym) != v.proc2newproc.end() ) {
            ASR::symbol_t* new_sym = v.proc2newproc[ext_sym].first;
            ASR::asr_t* new_sym_parent = ASRUtils::symbol_parent_symtab(new_sym)->asr_owner;
            if ( ASR::is_a<ASR::symbol_t>(*new_sym_parent) && 
                 current_scope->get_counter() != ASRUtils::symbol_parent_symtab(new_sym)->get_counter() ) {
                ASR::symbol_t* resolved_parent_sym = resolve_new_proc(ASR::down_cast<ASR::symbol_t>(new_sym_parent));
                if ( resolved_parent_sym != nullptr ) {
                    ASR::symbol_t* sym_to_return = ASRUtils::symbol_symtab(resolved_parent_sym)->get_symbol(ASRUtils::symbol_name(new_sym));
                    return sym_to_return ? sym_to_return : new_sym;
                }
            }
            return new_sym;
        }
        return nullptr;
    }

    void replace_Var(ASR::Var_t* x) {
        ASR::symbol_t* x_sym_ = x->m_v;
        bool is_external = ASR::is_a<ASR::ExternalSymbol_t>(*x_sym_);
        if ( v.proc2newproc.find(ASRUtils::symbol_get_past_external(x_sym_)) != v.proc2newproc.end() ) {
            x->m_v = resolve_new_proc(x_sym_);
            if( is_external ) {
                ASR::ExternalSymbol_t* x_sym_ext = ASR::down_cast<ASR::ExternalSymbol_t>(x_sym_);
                std::string new_func_sym_name = current_scope->get_unique_name(ASRUtils::symbol_name(
                    ASRUtils::symbol_get_past_external(x_sym_)));
                 ASR::symbol_t* new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                    ASR::make_ExternalSymbol_t(v.al, x_sym_->base.loc, current_scope,
                        s2c(v.al, new_func_sym_name), x->m_v, x_sym_ext->m_module_name,
                        x_sym_ext->m_scope_names, x_sym_ext->n_scope_names, ASRUtils::symbol_name(x->m_v),
                        x_sym_ext->m_access));
                v.proc2newproc[x_sym_] = {new_func_sym_, {}};
                current_scope->add_symbol(new_func_sym_name, new_func_sym_);
                x->m_v = new_func_sym_;
            }
            return ;
        }
        
        edit_symbol_pointer(v)
    }

    void replace_ArrayPhysicalCast(ASR::ArrayPhysicalCast_t* x) {
        ASR::BaseExprReplacer<EditProcedureReplacer>::replace_ArrayPhysicalCast(x);
        // TODO: Allow for DescriptorArray to DescriptorArray physical cast for allocatables
        // later on
        x->m_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg));
        if( (x->m_old == x->m_new &&
             x->m_old != ASR::array_physical_typeType::DescriptorArray) ||
            (x->m_old == x->m_new && x->m_old == ASR::array_physical_typeType::DescriptorArray &&
            (ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(x->m_arg)) ||
             ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x->m_arg))))) {
            *current_expr = x->m_arg;
        } else {
            ASR::Array_t* arr = ASR::down_cast<ASR::Array_t>(ASRUtils::type_get_past_pointer(
                  ASRUtils::type_get_past_allocatable(x->m_type)));
            if (ASRUtils::is_dimension_empty(arr->m_dims, arr->n_dims)) {
              arr->m_dims = ASR::down_cast<ASR::Array_t>(
                  ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(
                      ASRUtils::expr_type(x->m_arg))))->m_dims;
            }
        }
    }

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        edit_symbol_pointer(name)
        ASR::BaseExprReplacer<EditProcedureReplacer>::replace_FunctionCall(x);
    }

};

class EditProcedureVisitor: public ASR::CallReplacerOnExpressionsVisitor<EditProcedureVisitor> {

    public:

    PassArrayByDataProcedureVisitor& v;
    EditProcedureReplacer replacer;

    EditProcedureVisitor(PassArrayByDataProcedureVisitor& v_):
    v(v_), replacer(v) {}

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.current_scope = current_scope;
        replacer.replace_expr(*current_expr);
    }

    void visit_BlockCall(const ASR::BlockCall_t& x) {
        ASR::BlockCall_t& xx = const_cast<ASR::BlockCall_t&>(x);
        edit_symbol_reference(m)
        ASR::CallReplacerOnExpressionsVisitor<EditProcedureVisitor>::visit_BlockCall(x);
    }

    void visit_AssociateBlockCall(const ASR::AssociateBlockCall_t& x) {
        ASR::AssociateBlockCall_t& xx = const_cast<ASR::AssociateBlockCall_t&>(x);
        edit_symbol_reference(m)
        ASR::CallReplacerOnExpressionsVisitor<EditProcedureVisitor>::visit_AssociateBlockCall(x);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        ASR::SubroutineCall_t& xx = const_cast<ASR::SubroutineCall_t&>(x);
        edit_symbol_reference(name)
        ASR::CallReplacerOnExpressionsVisitor<EditProcedureVisitor>::visit_SubroutineCall(x);
    }

    void visit_Module(const ASR::Module_t& x) {
        for (auto it: x.m_symtab->get_scope()) {
            if ( ASR::is_a<ASR::Variable_t>(*it.second) &&
                ASR::down_cast<ASR::Variable_t>(it.second)->m_type_declaration ) {
                ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(it.second);
                ASR::symbol_t* resolved_type_dec = nullptr;
                if ( v.proc2newproc.find(var->m_type_declaration) != v.proc2newproc.end() ) {
                    resolved_type_dec = v.proc2newproc[var->m_type_declaration].first;
                } else if ( v.proc2newproc.find(ASRUtils::symbol_get_past_external(var->m_type_declaration)) != v.proc2newproc.end() ) {
                    resolved_type_dec = v.proc2newproc[ASRUtils::symbol_get_past_external(var->m_type_declaration)].first;
                }
                if ( resolved_type_dec ) {
                    ASR::Function_t* fn = ASR::down_cast<ASR::Function_t>(resolved_type_dec);
                    var->m_type_declaration = resolved_type_dec;
                    var->m_type = fn->m_function_signature;
                }
            }
        }
        ASR::CallReplacerOnExpressionsVisitor<EditProcedureVisitor>::visit_Module(x);
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

        // this is exactly the same as the one in EditProcedureReplacer
        ASR::symbol_t* resolve_new_proc(ASR::symbol_t* old_sym) {
            ASR::symbol_t* ext_sym = ASRUtils::symbol_get_past_external(old_sym);
            if( v.proc2newproc.find(ext_sym) != v.proc2newproc.end() ) {
                ASR::symbol_t* new_sym = v.proc2newproc[ext_sym].first;
                ASR::asr_t* new_sym_parent = ASRUtils::symbol_parent_symtab(new_sym)->asr_owner;
                if ( ASR::is_a<ASR::symbol_t>(*new_sym_parent) && 
                        current_scope->get_counter() != ASRUtils::symbol_parent_symtab(new_sym)->get_counter() ) {
                    ASR::symbol_t* resolved_parent_sym = resolve_new_proc(ASR::down_cast<ASR::symbol_t>(new_sym_parent));
                    if ( resolved_parent_sym != nullptr ) {
                        ASR::symbol_t* sym_to_return = ASRUtils::symbol_symtab(resolved_parent_sym)->get_symbol(ASRUtils::symbol_name(new_sym));
                        return sym_to_return ? sym_to_return : new_sym;
                    }
                }
                return new_sym;
            }
            return nullptr;
        }

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
                            ASR::symbol_t* new_var_sym = resolve_new_proc(sym);
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

        static inline void get_dimensions(ASR::expr_t* array, Vec<ASR::expr_t*>& dims,
                                        Allocator& al) {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::dimension_t* compile_time_dims = nullptr;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(array_type, compile_time_dims);
            for( int i = 0; i < n_dims; i++ ) {
                ASR::expr_t* start = compile_time_dims[i].m_start;
                if( start == nullptr ) {
                    start = PassUtils::get_bound(array, i + 1, "lbound", al);
                }
                ASR::expr_t* length = compile_time_dims[i].m_length;
                if( length == nullptr ) {
                    if (ASRUtils::is_allocatable(array_type)) {
                        length = ASRUtils::get_size(array, i + 1, al);
                    } else {
                        length = PassUtils::get_bound(array, i + 1, "ubound", al);
                    }
                }
                dims.push_back(al, start);
                dims.push_back(al, length);
            }
        }

        Vec<ASR::call_arg_t> construct_new_args(size_t n_args, ASR::call_arg_t* orig_args, std::vector<size_t>& indices) {
            Vec<ASR::call_arg_t> new_args;
            new_args.reserve(al, n_args);
            for( size_t i = 0; i < n_args; i++ ) {
                if (orig_args[i].m_value == nullptr) {
                    new_args.push_back(al, orig_args[i]);
                    continue;
                } else if (std::find(indices.begin(), indices.end(), i) == indices.end()) {
                    ASR::expr_t* expr = orig_args[i].m_value;    
                    if (ASR::is_a<ASR::Var_t>(*expr)) {
                        ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(expr);
                        ASR::symbol_t* sym = var->m_v;
                        if ( v.proc2newproc.find(sym) != v.proc2newproc.end() ) {
                            ASR::symbol_t* new_var_sym = v.proc2newproc[sym].first;
                            ASR::expr_t* new_var = ASRUtils::EXPR(ASR::make_Var_t(al, var->base.base.loc, new_var_sym));
                            orig_args[i].m_value = new_var;
                        }
                    }
                    new_args.push_back(al, orig_args[i]);
                    continue;
                }

                ASR::expr_t* orig_arg_i = orig_args[i].m_value;
                ASR::ttype_t* orig_arg_type = ASRUtils::expr_type(orig_arg_i);
                if( ASRUtils::is_array(orig_arg_type) ) {
                    ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(
                        ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(orig_arg_type)));
                    if( array_t->m_physical_type != ASR::array_physical_typeType::PointerToDataArray ) {
                        ASR::expr_t* physical_cast = ASRUtils::EXPR(ASRUtils::make_ArrayPhysicalCast_t_util(
                            al, orig_arg_i->base.loc, orig_arg_i, array_t->m_physical_type,
                            ASR::array_physical_typeType::PointerToDataArray, ASRUtils::duplicate_type(al, orig_arg_type,
                            nullptr, ASR::array_physical_typeType::PointerToDataArray, true), nullptr));
                        ASR::call_arg_t physical_cast_arg;
                        physical_cast_arg.loc = orig_arg_i->base.loc;
                        physical_cast_arg.m_value = physical_cast;
                        new_args.push_back(al, physical_cast_arg);
                    } else {
                        new_args.push_back(al, orig_args[i]);
                    }
                } else {
                    new_args.push_back(al, orig_args[i]);
                }

                Vec<ASR::expr_t*> dim_vars;
                dim_vars.reserve(al, 2);
                get_dimensions(orig_arg_i, dim_vars, al);
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
            T& xx = const_cast<T&>(x);
            ASR::symbol_t* subrout_sym = x.m_name;
            bool is_external = ASR::is_a<ASR::ExternalSymbol_t>(*subrout_sym);
            subrout_sym = ASRUtils::symbol_get_past_external(subrout_sym);

            if(ASR::is_a<ASR::Variable_t>(*ASRUtils::symbol_get_past_external(x.m_name))){
                // Case: procedure(cb) :: call_back (Here call_back is variable of type cb which is a function)
                ASR::Variable_t* variable = ASR::down_cast<ASR::Variable_t>(ASRUtils::symbol_get_past_external(x.m_name));
                ASR::symbol_t* type_dec = variable->m_type_declaration;
                subrout_sym = ASRUtils::symbol_get_past_external(type_dec);

                // check if subrout_sym is present as value in proc2newproc
                bool is_present = false;
                std::vector<size_t> present_indices;
                for ( auto it: v.proc2newproc ) {
                    if (it.second.first == subrout_sym) {
                        is_present = true;
                        present_indices = it.second.second;
                        break;
                    }
                }

                if ( v.proc2newproc.find(subrout_sym) != v.proc2newproc.end()) {
                    ASR::symbol_t* new_x_name = nullptr;
                    if (is_external && (v.proc2newproc.find(x.m_name) != v.proc2newproc.end())) { 
                        new_x_name = v.proc2newproc[x.m_name].first;
                    } else {
                        new_x_name = resolve_new_proc(x.m_name);
                    }
                    if ( new_x_name != nullptr ) {
                        ASR::Function_t* new_func = ASR::down_cast<ASR::Function_t>(resolve_new_proc(subrout_sym));
                        ASR::down_cast<ASR::Variable_t>(ASRUtils::symbol_get_past_external(x.m_name))->m_type = new_func->m_function_signature;
                        xx.m_name = new_x_name;
                        xx.m_original_name = new_x_name;
                        std::vector<size_t>& indices = v.proc2newproc[subrout_sym].second;
                        Vec<ASR::call_arg_t> new_args = construct_new_args(x.n_args, x.m_args, indices);
                        xx.m_args = new_args.p;
                        xx.n_args = new_args.size();
                        return;
                    }
                } else if ( is_present ) {
                    Vec<ASR::call_arg_t> new_args = construct_new_args(x.n_args, x.m_args, present_indices);
                    xx.m_args = new_args.p;
                    xx.n_args = new_args.size();
                    return;
                }
            }
            if( !can_edit_call(x.m_args, x.n_args) ) {
                not_to_be_erased.insert(subrout_sym);
                return ;
            }

            if( v.proc2newproc.find(subrout_sym) == v.proc2newproc.end() ) {
                update_args_for_pass_arr_by_data_funcs_passed_as_callback(x);
                return;
            }

            ASR::symbol_t* new_func_sym = resolve_new_proc(subrout_sym);
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
                    throw LCompilersException("Number of arguments in the new function call " +
                        std::string(ASRUtils::symbol_name(subrout_sym)) + " doesn't satisfy "
                        "min_args <= new_args.size() <= max_args, " + std::to_string(min_args) + " <= " +
                        std::to_string(new_args.size()) + " <= " + std::to_string(max_args));
                }
            }
            ASR::symbol_t* new_func_sym_ = new_func_sym;
            if( is_external ) {
                ASR::ExternalSymbol_t* func_ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
                // TODO: Use SymbolTable::get_unique_name to avoid potential
                // clashes with user defined functions
                char* new_func_sym_name = ASRUtils::symbol_name(new_func_sym);
                if( current_scope->resolve_symbol(new_func_sym_name) == nullptr ) {
                    if ( ASR::is_a<ASR::Variable_t>(*ASRUtils::symbol_get_past_external(func_ext_sym->m_external)) &&
                         ASR::down_cast<ASR::Variable_t>(ASRUtils::symbol_get_past_external(func_ext_sym->m_external))->m_type_declaration ) {
                        ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(ASRUtils::symbol_get_past_external(func_ext_sym->m_external));
                        ASR::symbol_t* type_dec_sym = current_scope->resolve_symbol(ASRUtils::symbol_name(var->m_type_declaration));
                        std::string module_name = func_ext_sym->m_module_name;

                        if ( type_dec_sym && ASR::is_a<ASR::ExternalSymbol_t>(*type_dec_sym) ) {
                            module_name = ASR::down_cast<ASR::ExternalSymbol_t>(type_dec_sym)->m_module_name;
                        }
                        new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                            ASR::make_ExternalSymbol_t(al, x.m_name->base.loc, func_ext_sym->m_parent_symtab,
                                new_func_sym_name, new_func_sym, s2c(al, module_name),
                                func_ext_sym->m_scope_names, func_ext_sym->n_scope_names, new_func_sym_name,
                                func_ext_sym->m_access));
                    } else {
                        new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                            ASR::make_ExternalSymbol_t(al, x.m_name->base.loc, func_ext_sym->m_parent_symtab,
                                new_func_sym_name, new_func_sym, func_ext_sym->m_module_name,
                                func_ext_sym->m_scope_names, func_ext_sym->n_scope_names, new_func_sym_name,
                                func_ext_sym->m_access));
                    }

                    func_ext_sym->m_parent_symtab->add_symbol(new_func_sym_name, new_func_sym_);
                } else {
                    new_func_sym_ = current_scope->resolve_symbol(new_func_sym_name);
                }
            }
            if(!ASR::is_a<ASR::Variable_t>(*x.m_name)){
                xx.m_name = new_func_sym_;
                xx.m_original_name = new_func_sym_;
            } else if(v.proc2newproc.find(x.m_name) != v.proc2newproc.end()){
                xx.m_name = resolve_new_proc(x.m_name);
                xx.m_original_name = resolve_new_proc(x.m_name);
            }
            xx.m_args = new_args.p;
            xx.n_args = new_args.size();
        }

        void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_symtab;
            for (auto &a : x.m_symtab->get_scope()) {
                if (ASR::is_a<ASR::Module_t>(*a.second)) {
                    this->visit_symbol(*a.second);
                }
            }
            for (auto &a : x.m_symtab->get_scope()) {
                if (!ASR::is_a<ASR::Module_t>(*a.second)) {
                    this->visit_symbol(*a.second);
                }
            }
            current_scope = current_scope_copy;
        }
        
        void visit_Program(const ASR::Program_t &x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_symtab;
            for (auto &a : x.m_symtab->get_scope()) {
                if(!ASR::is_a<ASR::Function_t>(*a.second)){
                    this->visit_symbol(*a.second);
                }
            }
            for (auto &a : x.m_symtab->get_scope()) {
                if(ASR::is_a<ASR::Function_t>(*a.second)){
                    this->visit_symbol(*a.second);
                }
            }
            this->transform_stmts(xx.m_body, xx.n_body);
            current_scope = current_scope_copy;
        }

        void visit_Module(const ASR::Module_t &x) {
            SymbolTable* current_scope_copy = current_scope;
            current_scope = x.m_symtab;
            for (auto &a : x.m_symtab->get_scope()) {
                if(!ASR::is_a<ASR::Function_t>(*a.second)){
                    this->visit_symbol(*a.second);
                }
            }
            for (auto &a : x.m_symtab->get_scope()) {
                if(ASR::is_a<ASR::Function_t>(*a.second)){
                    this->visit_symbol(*a.second);
                }
            }
            current_scope = current_scope_copy;
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>::visit_SubroutineCall(x);
            visit_Call(x);
        }

        void visit_FunctionCall(const ASR::FunctionCall_t& x) {
            ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>::visit_FunctionCall(x);
            visit_Call(x);
        }

        void visit_Variable(const ASR::Variable_t &x) {
            if (v.proc2newproc.find((ASR::symbol_t*) &x) != v.proc2newproc.end()){
                return;
            }
            // Case: procedure(cb) :: call_back (Here call_back is variable of type cb which is a function)
            ASR::symbol_t* type_dec = x.m_type_declaration;
            if (v.proc2newproc.find(ASRUtils::symbol_get_past_external(type_dec)) != v.proc2newproc.end() && 
                    v.proc2newproc.find(type_dec) == v.proc2newproc.end()){
                ASR::symbol_t* new_func = resolve_new_proc(ASRUtils::symbol_get_past_external(type_dec));
                ASR::ExternalSymbol_t* x_sym_ext = ASR::down_cast<ASR::ExternalSymbol_t>(type_dec);
                std::string new_func_sym_name = x_sym_ext->m_parent_symtab->get_unique_name(ASRUtils::symbol_name(
                    ASRUtils::symbol_get_past_external(type_dec)));
                ASR::symbol_t* new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                    ASR::make_ExternalSymbol_t(v.al, type_dec->base.loc, x_sym_ext->m_parent_symtab,
                        s2c(v.al, new_func_sym_name), new_func, x_sym_ext->m_module_name,
                        x_sym_ext->m_scope_names, x_sym_ext->n_scope_names, ASRUtils::symbol_name(new_func),
                        x_sym_ext->m_access));
                v.proc2newproc[type_dec] = {new_func_sym_, {}};
                x_sym_ext->m_parent_symtab->add_symbol(new_func_sym_name, new_func_sym_);
            }
            if(type_dec && v.proc2newproc.find(type_dec) != v.proc2newproc.end() && 
                ASR::is_a<ASR::Function_t>(*ASRUtils::symbol_get_past_external(type_dec))){
                ASR::symbol_t* new_sym = resolve_new_proc(type_dec);
                if ( new_sym == nullptr ) {
                    return;
                }
                ASR::expr_t* sym_val = x.m_symbolic_value;
                ASR::expr_t* m_val = x.m_value;
                ASR::Function_t * subrout = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(new_sym));
                if (x.m_symbolic_value && ASR::is_a<ASR::PointerNullConstant_t>(*x.m_symbolic_value)) {
                    ASR::PointerNullConstant_t* pnc = ASR::down_cast<ASR::PointerNullConstant_t>(x.m_symbolic_value);
                    pnc->m_type = subrout->m_function_signature;
                    sym_val = (ASR::expr_t*) pnc;
                }
                if (x.m_value && ASR::is_a<ASR::PointerNullConstant_t>(*x.m_value)) {
                    ASR::PointerNullConstant_t* pnc = ASR::down_cast<ASR::PointerNullConstant_t>(x.m_value);
                    pnc->m_type = subrout->m_function_signature;
                    m_val = (ASR::expr_t*) pnc;
                }
                std::string new_sym_name = x.m_parent_symtab->get_unique_name(x.m_name);
                ASR::symbol_t* new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                    ASRUtils::make_Variable_t_util(v.al, x.base.base.loc, x.m_parent_symtab, s2c(v.al, new_sym_name), 
                        x.m_dependencies, x.n_dependencies, x.m_intent, 
                        sym_val, m_val, x.m_storage, subrout->m_function_signature, 
                        new_sym, x.m_abi, x.m_access, x.m_presence, x.m_value_attr));
                v.proc2newproc[(ASR::symbol_t *) &x] = {new_func_sym_, {}};
                x.m_parent_symtab->add_symbol(new_sym_name, new_func_sym_); 
                not_to_be_erased.insert(new_func_sym_);
            }
            ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>::visit_Variable(x);
        }

        void visit_StructInstanceMember(const ASR::StructInstanceMember_t &x) {
            //Case: prob % calfun => temp_calfun (where calfun is procedure variable)
            ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>::visit_StructInstanceMember(x);
            if (v.proc2newproc.find(ASRUtils::symbol_get_past_external(x.m_m)) != v.proc2newproc.end()) {
                ASR::symbol_t* new_func_sym_ = x.m_m;
                if (ASR::is_a<ASR::ExternalSymbol_t>(*x.m_m)) {
                    ASR::symbol_t* new_func = v.proc2newproc[ASRUtils::symbol_get_past_external(x.m_m)].first;
                    ASR::ExternalSymbol_t* x_sym_ext = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_m);
                    std::string new_func_sym_name = x_sym_ext->m_parent_symtab->get_unique_name(ASRUtils::symbol_name(x.m_m));
                    new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                        ASR::make_ExternalSymbol_t(v.al, x.m_m->base.loc, x_sym_ext->m_parent_symtab,
                            s2c(v.al, new_func_sym_name), new_func, x_sym_ext->m_module_name,
                            x_sym_ext->m_scope_names, x_sym_ext->n_scope_names, ASRUtils::symbol_name(new_func),
                            x_sym_ext->m_access));
                    v.proc2newproc[x.m_m] = {new_func_sym_, {}};
                    x_sym_ext->m_parent_symtab->add_symbol(new_func_sym_name, new_func_sym_);
                } else if (ASR::is_a<ASR::Variable_t>(*x.m_m)) {
                    new_func_sym_ = v.proc2newproc[x.m_m].first;
                }
                ASR::StructInstanceMember_t& sim = const_cast<ASR::StructInstanceMember_t&>(x);
                sim.m_m = new_func_sym_;
                sim.m_type = ASRUtils::symbol_type(new_func_sym_);
            }
        }
 
        void visit_Struct(const ASR::Struct_t &x) { 
            //just to update names of changed symbols of Struct
            ASR::ASRPassBaseWalkVisitor<EditProcedureCallsVisitor>::visit_Struct(x);
            ASR::Struct_t& ss = const_cast<ASR::Struct_t&>(x);
            for (size_t i = 0; i < x.n_members; i++) {
                ASR::symbol_t* old_sym = x.m_symtab->get_symbol(x.m_members[i]);
                if (v.proc2newproc.find(old_sym) != v.proc2newproc.end()) {
                    ss.m_members[i] = ASRUtils::symbol_name(v.proc2newproc[old_sym].first);
                }
            }
        }
      
        void visit_Var(const ASR::Var_t &x) {
            if (v.proc2newproc.find(x.m_v) != v.proc2newproc.end()){
                ASR::symbol_t* new_sym = v.proc2newproc[x.m_v].first;
                ASR::Var_t& vv = const_cast<ASR::Var_t&>(x);
                vv.m_v = new_sym;
                return;
            }
            if (ASR::is_a<ASR::ExternalSymbol_t>(*x.m_v) &&
                v.proc2newproc.find(ASRUtils::symbol_get_past_external(x.m_v)) != v.proc2newproc.end()) {
                ASR::symbol_t* new_func_sym_ = x.m_v;
                ASR::symbol_t* new_func = v.proc2newproc[ASRUtils::symbol_get_past_external(x.m_v)].first;
                ASR::ExternalSymbol_t* x_sym_ext = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_v);
                std::string new_func_sym_name = x_sym_ext->m_parent_symtab->get_unique_name(ASRUtils::symbol_name(x.m_v));
                new_func_sym_ = ASR::down_cast<ASR::symbol_t>(
                    ASR::make_ExternalSymbol_t(v.al, x.m_v->base.loc, x_sym_ext->m_parent_symtab,
                        s2c(v.al, new_func_sym_name), new_func, x_sym_ext->m_module_name,
                        x_sym_ext->m_scope_names, x_sym_ext->n_scope_names, ASRUtils::symbol_name(new_func),
                        x_sym_ext->m_access));
                v.proc2newproc[x.m_v] = {new_func_sym_, {}};
                x_sym_ext->m_parent_symtab->add_symbol(new_func_sym_name, new_func_sym_);
                ASR::Var_t& vv = const_cast<ASR::Var_t&>(x);
                vv.m_v = new_func_sym_;
            }
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
        template <typename T>
        void visit_Unit(const T& x) {
            T& xx = const_cast<T&>(x);
            current_scope = xx.m_symtab;

            std::vector<std::string> to_be_erased;

            for( auto& item: current_scope->get_scope() ) {
                if (ASR::is_a<ASR::Function_t>(*item.second)) {
                    ASR::FunctionType_t* func_type = ASRUtils::get_FunctionType(item.second);
                    SymbolTable* current_scope_copy = current_scope;
                    visit_symbol(*item.second);
                    current_scope = current_scope_copy;
                    if (func_type->n_arg_types >= 1 && ASR::is_a<ASR::ClassType_t>(*func_type->m_arg_types[0])) {
                        continue;
                    }
                }
                ASR::symbol_t* sym = item.second;
                if (ASR::is_a<ASR::ExternalSymbol_t>(*item.second)) {
                    sym = ASR::down_cast<ASR::ExternalSymbol_t>(item.second)->m_external;
                }
                if( v.proc2newproc.find(sym) != v.proc2newproc.end() &&
                    not_to_be_erased.find(sym) == not_to_be_erased.end() ) {
                    LCOMPILERS_ASSERT(item.first == ASRUtils::symbol_name(item.second))
                    to_be_erased.push_back(item.first);
                }
                if ( ASR::is_a<ASR::Module_t>(*item.second) ||
                    ASR::is_a<ASR::Program_t>(*item.second) ||
                    ASR::is_a<ASR::Function_t>(*item.second) ||
                    ASR::is_a<ASR::Struct_t>(*item.second) ||
                    ASR::is_a<ASR::Block_t>(*item.second)) {
                    SymbolTable* current_scope_copy = current_scope;
                    visit_symbol(*item.second);
                    current_scope = current_scope_copy;
                }
            }

            for (auto &item: to_be_erased) {
                current_scope->erase_symbol(item);
            }
        }

        void visit_TranslationUnit(const ASR::TranslationUnit_t& x) {
            visit_Unit(x);
        }

        void visit_Program(const ASR::Program_t& x) {
            visit_Unit(x);
        }

        void visit_Function(const ASR::Function_t& x) {
            visit_Unit(x);
        }

        void visit_Module(const ASR::Module_t& x) {
            if (x.m_intrinsic) {
                return ;
            }

            visit_Unit(x);
        }

        void visit_Struct(const ASR::Struct_t& x) {
            visit_Unit(x);
        }

        void visit_Block(const ASR::Block_t& x) {
            visit_Unit(x);
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
