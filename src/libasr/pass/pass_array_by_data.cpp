#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/pass_array_by_data.h>

#include <vector>
#include <utility>


namespace LFortran {

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
        SymbolTable* current_proc_scope;
        bool is_editing_procedure;

    public:

        std::map< ASR::symbol_t*, std::pair<ASR::symbol_t*, std::vector<size_t>> > proc2newproc;

        PassArrayByDataProcedureVisitor(Allocator& al_) : PassVisitor(al_, nullptr),
        node_duplicator(al_), current_proc_scope(nullptr), is_editing_procedure(false)
        {}

        void visit_Var(const ASR::Var_t& x) {
            if( !is_editing_procedure ) {
                return ;
            }
            ASR::Var_t& xx = const_cast<ASR::Var_t&>(x);
            ASR::symbol_t* x_sym = xx.m_v;
            std::string x_sym_name = std::string(ASRUtils::symbol_name(x_sym));
            if( current_proc_scope->get_symbol(x_sym_name) != x_sym ) {
                xx.m_v = current_proc_scope->get_symbol(x_sym_name);
            }
        }

        void visit_alloc_arg(const ASR::alloc_arg_t& x) {
            PassVisitor::visit_alloc_arg(x);
            if( !is_editing_procedure ) {
                return ;
            }
            ASR::alloc_arg_t& xx = const_cast<ASR::alloc_arg_t&>(x);
            ASR::symbol_t* x_sym = xx.m_a;
            std::string x_sym_name = std::string(ASRUtils::symbol_name(x_sym));
            if( current_proc_scope->get_symbol(x_sym_name) != x_sym ) {
                xx.m_a = current_proc_scope->get_symbol(x_sym_name);
            }
        }

        void visit_FunctionCall(const ASR::FunctionCall_t& x) {
            PassVisitor::visit_FunctionCall(x);
            if( !is_editing_procedure ) {
                return ;
            }
            ASR::FunctionCall_t& xx = const_cast<ASR::FunctionCall_t&>(x);
            ASR::symbol_t* x_sym = xx.m_name;
            std::string x_sym_name = std::string(ASRUtils::symbol_name(x_sym));
            if( current_proc_scope->get_symbol(x_sym_name) != x_sym ) {
                xx.m_name = current_proc_scope->get_symbol(x_sym_name);
            }
        }

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
            SymbolTable* new_symtab = al.make_new<SymbolTable>(current_scope);
            for( auto& item: x->m_symtab->get_scope() ) {
                ASR::symbol_t* new_arg = nullptr;
                if( ASR::is_a<ASR::Variable_t>(*item.second) ) {
                    ASR::Variable_t* arg = ASR::down_cast<ASR::Variable_t>(item.second);
                    new_arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(al,
                                                arg->base.base.loc, new_symtab, s2c(al, item.first),
                                                arg->m_intent, arg->m_symbolic_value, arg->m_value,
                                                arg->m_storage, arg->m_type, arg->m_abi, arg->m_access,
                                                arg->m_presence, arg->m_value_attr));
                } else if( ASR::is_a<ASR::ExternalSymbol_t>(*item.second) ) {
                    ASR::ExternalSymbol_t* arg = ASR::down_cast<ASR::ExternalSymbol_t>(item.second);
                    new_arg = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al, arg->base.base.loc,
                                    new_symtab, s2c(al, item.first), arg->m_external, arg->m_module_name,
                                    arg->m_scope_names, arg->n_scope_names, arg->m_original_name, arg->m_access));
                }
                new_symtab->add_symbol(item.first, new_arg);
            }
            Vec<ASR::expr_t*> new_args;
            std::string suffix = "";
            new_args.reserve(al, x->n_args);
            ASR::expr_t* return_var = nullptr;
            for( size_t i = 0; i < x->n_args + 1; i++ ) {
                ASR::Variable_t* arg = nullptr;
                if( i < x->n_args ) {
                    arg = ASRUtils::EXPR2VAR(x->m_args[i]);
                } else if( x->m_return_var ) {
                    arg = ASRUtils::EXPR2VAR(x->m_return_var);
                } else {
                    break ;
                }
                if( std::find(indices.begin(), indices.end(), i) !=
                    indices.end() ) {
                    suffix += "_" + std::string(arg->m_name);
                }
                ASR::expr_t* new_arg = ASRUtils::EXPR(ASR::make_Var_t(al,
                                        arg->base.base.loc, new_symtab->get_symbol(
                                                        std::string(arg->m_name))));
                if( i < x->n_args ) {
                    new_args.push_back(al, new_arg);
                } else {
                    return_var = new_arg;
                }
            }
            ASR::symbol_t* new_symbol = nullptr;
            std::string new_name = std::string(x->m_name) + suffix;
            if( ASR::is_a<ASR::Function_t>( *((ASR::symbol_t*) x) ) ) {
                std::string new_bindc_name = "";
                if( x->m_bindc_name ) {
                    new_bindc_name = std::string(x->m_bindc_name) + suffix;
                }
                ASR::asr_t* new_subrout = ASR::make_Function_t(al, x->base.base.loc,
                                            new_symtab, s2c(al, new_name), new_args.p,
                                            new_args.size(), nullptr, 0, new_body.p, new_body.size(),
                                            return_var, x->m_abi, x->m_access, x->m_deftype,
                                            s2c(al, new_bindc_name), x->m_elemental,
                                            x->m_pure, x->m_module, x->m_inline);
                new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
            }
            current_scope->add_symbol(new_name, new_symbol);
            proc2newproc[(ASR::symbol_t*) x] = std::make_pair(new_symbol, indices);
            return new_symbol;
        }

        void edit_new_procedure(ASR::Function_t* x, std::vector<size_t>& indices) {
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
                                           x->m_symtab, arg_name, ASR::intentType::In);
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

            is_editing_procedure = true;
            current_proc_scope = x->m_symtab;
            for( size_t i = 0; i < x->n_body; i++ ) {
                visit_stmt(*x->m_body[i]);
            }
            is_editing_procedure = false;
            current_proc_scope = nullptr;
        }

        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            current_scope = xx.m_symtab;
            for( auto& item: xx.m_symtab->get_scope() ) {
                if( ASR::is_a<ASR::Function_t>(*item.second) ) {
                    ASR::Function_t* subrout = ASR::down_cast<ASR::Function_t>(item.second);
                    std::vector<size_t> arg_indices;
                    if( ASRUtils::is_pass_array_by_data_possible(subrout, arg_indices) ) {
                        ASR::symbol_t* sym = insert_new_procedure(subrout, arg_indices);
                        if( sym != nullptr ) {
                            ASR::Function_t* new_subrout = ASR::down_cast<ASR::Function_t>(sym);
                            edit_new_procedure(new_subrout, arg_indices);
                        }
                    }
                }
            }
        }
};

/*
    The following visitor replaces subroutine calls with arrays as arguments
    to subroutine calls having dimensional information passed as arguments. See example below,

        call f(array1, array2)

    gets converted to,

        call f_array1_array2(array1, m1, n1, array2, m2, n2)

    As can be seen dimensional information, m1, n1 is passed along
    with array1 and similarly m2, n2 is passed along with array2.
 */
class ReplaceSubroutineCallsVisitor : public PassUtils::PassVisitor<ReplaceSubroutineCallsVisitor>
{
    private:

        PassArrayByDataProcedureVisitor& v;

    public:

        ReplaceSubroutineCallsVisitor(Allocator& al_, PassArrayByDataProcedureVisitor& v_): PassVisitor(al_, nullptr),
        v(v_) {
            pass_result.reserve(al, 1);
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            ASR::symbol_t* subrout_sym = x.m_name;
            if( v.proc2newproc.find(subrout_sym) == v.proc2newproc.end() ) {
                return ;
            }

            ASR::symbol_t* new_subrout_sym = v.proc2newproc[subrout_sym].first;
            std::vector<size_t>& indices = v.proc2newproc[subrout_sym].second;

            Vec<ASR::call_arg_t> new_args;
            new_args.reserve(al, x.n_args);
            for( size_t i = 0; i < x.n_args; i++ ) {
                new_args.push_back(al, x.m_args[i]);
                if( std::find(indices.begin(), indices.end(), i) == indices.end() ) {
                    continue ;
                }

                Vec<ASR::expr_t*> dim_vars;
                dim_vars.reserve(al, 2);
                ASRUtils::get_dimensions(x.m_args[i].m_value, dim_vars, al);
                for( size_t j = 0; j < dim_vars.size(); j++ ) {
                    ASR::call_arg_t dim_var;
                    dim_var.loc = dim_vars[j]->base.loc;
                    dim_var.m_value = dim_vars[j];
                    new_args.push_back(al, dim_var);
                }
            }

            ASR::stmt_t* new_call = ASRUtils::STMT(ASR::make_SubroutineCall_t(al,
                                        x.base.base.loc, new_subrout_sym, new_subrout_sym,
                                        new_args.p, new_args.size(), x.m_dt));
            pass_result.push_back(al, new_call);
        }
};


/*

The following replacer replaces all the function call expressions with arrays
as arguments to function call expressions having dimensional information of
array arguments passed along. See example below,

    sum = f(array) + g(array)

gets converted to,

    sum = f_array(array, m, n) + g_array(array, m, n)

*/
class ReplaceFunctionCalls: public ASR::BaseExprReplacer<ReplaceFunctionCalls> {

    private:

    Allocator& al;
    PassArrayByDataProcedureVisitor& v;

    public:

    ReplaceFunctionCalls(Allocator& al_, PassArrayByDataProcedureVisitor& v_) : al(al_), v(v_)
    {}

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        ASR::symbol_t* subrout_sym = x->m_name;
        if( v.proc2newproc.find(subrout_sym) == v.proc2newproc.end() ) {
            return ;
        }

        ASR::symbol_t* new_func_sym = v.proc2newproc[subrout_sym].first;
        std::vector<size_t>& indices = v.proc2newproc[subrout_sym].second;

        Vec<ASR::call_arg_t> new_args;
        new_args.reserve(al, x->n_args);
        for( size_t i = 0; i < x->n_args; i++ ) {
            new_args.push_back(al, x->m_args[i]);
            if( std::find(indices.begin(), indices.end(), i) == indices.end() ) {
                continue ;
            }

            Vec<ASR::expr_t*> dim_vars;
            dim_vars.reserve(al, 2);
            ASRUtils::get_dimensions(x->m_args[i].m_value, dim_vars, al);
            for( size_t j = 0; j < dim_vars.size(); j++ ) {
                ASR::call_arg_t dim_var;
                dim_var.loc = dim_vars[j]->base.loc;
                dim_var.m_value = dim_vars[j];
                new_args.push_back(al, dim_var);
            }
        }

        ASR::expr_t* new_call = ASRUtils::EXPR(ASR::make_FunctionCall_t(al,
                                    x->base.base.loc, new_func_sym, new_func_sym,
                                    new_args.p, new_args.size(), x->m_type, nullptr,
                                    x->m_dt));
        *current_expr = new_call;
    }

};

/*
The following visitor calls the above replacer i.e., ReplaceFunctionCalls
on expressions present in ASR so that FunctionCall get replaced everywhere
and we don't end up with false positives.
*/
class ReplaceFunctionCallsVisitor : public ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallsVisitor>
{
    private:

        ReplaceFunctionCalls replacer;

    public:

        ReplaceFunctionCallsVisitor(Allocator& al_,
            PassArrayByDataProcedureVisitor& v_) : replacer(al_, v_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
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

    public:

        RemoveArrayByDescriptorProceduresVisitor(Allocator& al_, PassArrayByDataProcedureVisitor& v_):
            PassVisitor(al_, nullptr), v(v_) {}

        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            current_scope = xx.m_symtab;

            std::vector<std::string> to_be_erased;

            for( auto& item: current_scope->get_scope() ) {
                if( v.proc2newproc.find(item.second) != v.proc2newproc.end() ) {
                    LFORTRAN_ASSERT(item.first == ASRUtils::symbol_name(item.second))
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
    ReplaceSubroutineCallsVisitor u(al, v);
    u.visit_TranslationUnit(unit);
    ReplaceFunctionCallsVisitor w(al, v);
    w.visit_TranslationUnit(unit);
    RemoveArrayByDescriptorProceduresVisitor x(al,v);
    x.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran
