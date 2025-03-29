#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_array_op.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>

#include <libasr/asr_builder.h>

#include <vector>

namespace LCompilers {

class ArrayVarAddressReplacer: public ASR::BaseExprReplacer<ArrayVarAddressReplacer> {

    public:

    Allocator& al;
    Vec<ASR::expr_t**>& vars;

    ArrayVarAddressReplacer(Allocator& al_, Vec<ASR::expr_t**>& vars_):
        al(al_), vars(vars_) {
        call_replacer_on_value = false;
    }

    void replace_ArraySize(ASR::ArraySize_t* /*x*/) {

    }

    void replace_ArrayBound(ASR::ArrayBound_t* /*x*/) {

    }

    void replace_Var(ASR::Var_t* x) {
        if( ASRUtils::is_array(ASRUtils::symbol_type(x->m_v)) ) {
            vars.push_back(al, current_expr);
        }
    }

    void replace_StructInstanceMember(ASR::StructInstanceMember_t* x) {
        if( !ASRUtils::is_array(x->m_type) ) {
            return ;
        }
        if( ASRUtils::is_array(ASRUtils::symbol_type(x->m_m)) ) {
            vars.push_back(al, current_expr);
        } else {
            ASR::BaseExprReplacer<ArrayVarAddressReplacer>::replace_StructInstanceMember(x);
        }
    }

    void replace_ArrayItem(ASR::ArrayItem_t* /*x*/) {
    }

    void replace_IntrinsicArrayFunction(ASR::IntrinsicArrayFunction_t* /*x*/) {
    }

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        if( !ASRUtils::is_elemental(x->m_name) ) {
            return ;
        }

        ASR::BaseExprReplacer<ArrayVarAddressReplacer>::replace_FunctionCall(x);
    }

};

class ArrayVarAddressCollector: public ASR::CallReplacerOnExpressionsVisitor<ArrayVarAddressCollector> {

    private:

    ArrayVarAddressReplacer replacer;

    public:

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.replace_expr(*current_expr);
    }

    ArrayVarAddressCollector(Allocator& al_, Vec<ASR::expr_t**>& vars_):
        replacer(al_, vars_) {
        visit_expr_after_replacement = false;
    }

    void visit_Allocate(const ASR::Allocate_t& /*x*/) {
    }

    void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t& /*x*/) {
    }

    void visit_ImplicitDeallocate(const ASR::ImplicitDeallocate_t& /*x*/) {
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        if( !PassUtils::is_elemental(x.m_name) ) {
            return ;
        }
    }

    void visit_Associate(const ASR::Associate_t& /*x*/) {
    }

};

class FixTypeVisitor: public ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor> {
    public:

    FixTypeVisitor(Allocator& al_) {
        (void)al_;      // Explicitly mark the parameter as unused
    }

    void visit_StructType(const ASR::StructType_t& x) {
        std::string derived_type_name = ASRUtils::symbol_name(x.m_derived_type);
        if( x.m_derived_type == current_scope->resolve_symbol(derived_type_name) ) {
            return ;
        }

        ASR::StructType_t& xx = const_cast<ASR::StructType_t&>(x);
        xx.m_derived_type = current_scope->resolve_symbol(derived_type_name);
    }

    void visit_Cast(const ASR::Cast_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_Cast(x);
        ASR::Cast_t& xx = const_cast<ASR::Cast_t&>(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) &&
             ASRUtils::is_array(x.m_type) ) {
            xx.m_type = ASRUtils::type_get_past_array(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_IntrinsicElementalFunction(const ASR::IntrinsicElementalFunction_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_IntrinsicElementalFunction(x);
        ASR::IntrinsicElementalFunction_t& xx = const_cast<ASR::IntrinsicElementalFunction_t&>(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_args[0])) ) {
            xx.m_type = ASRUtils::extract_type(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_FunctionCall(x);
        if( !PassUtils::is_elemental(x.m_name) ) {
            return ;
        }
        ASR::FunctionCall_t& xx = const_cast<ASR::FunctionCall_t&>(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_args[0].m_value)) ) {
            xx.m_type = ASRUtils::extract_type(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    template <typename T>
    void visit_ArrayOp(const T& x) {
        T& xx = const_cast<T&>(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(xx.m_left)) &&
            !ASRUtils::is_array(ASRUtils::expr_type(xx.m_right)) ) {
            xx.m_type = ASRUtils::extract_type(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_RealBinOp(const ASR::RealBinOp_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_RealBinOp(x);
        visit_ArrayOp(x);
    }

    void visit_RealCompare(const ASR::RealCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_RealCompare(x);
        visit_ArrayOp(x);
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_IntegerCompare(x);
        visit_ArrayOp(x);
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_ComplexCompare(x);
        visit_ArrayOp(x);
    }

    void visit_StringCompare(const ASR::StringCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_StringCompare(x);
        visit_ArrayOp(x);
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_LogicalBinOp(x);
        visit_ArrayOp(x);
    }

    void visit_StructInstanceMember(const ASR::StructInstanceMember_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_StructInstanceMember(x);
        if( !ASRUtils::is_array(x.m_type) ) {
            return ;
        }
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_v)) &&
            !ASRUtils::is_array(ASRUtils::symbol_type(x.m_m)) ) {
            ASR::StructInstanceMember_t& xx = const_cast<ASR::StructInstanceMember_t&>(x);
            xx.m_type = ASRUtils::extract_type(x.m_type);
        }
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t& x){
        if( !ASRUtils::is_array(x.m_type) ) {
            return ;
        }
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_RealUnaryMinus(x);
        ASR::RealUnaryMinus_t& xx = const_cast<ASR::RealUnaryMinus_t&>(x);
        xx.m_type = ASRUtils::extract_type(x.m_type);
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t& x){
        if( !ASRUtils::is_array(x.m_type) ) {
            return ;
        }
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_IntegerUnaryMinus(x);
        ASR::IntegerUnaryMinus_t& xx = const_cast<ASR::IntegerUnaryMinus_t&>(x);
        xx.m_type = ASRUtils::extract_type(x.m_type);
    }
};

class ReplaceArrayOp: public ASR::BaseExprReplacer<ReplaceArrayOp> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;

    public:

    SymbolTable* current_scope;
    ASR::expr_t* result_expr;
    bool& remove_original_stmt;

    ReplaceArrayOp(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
                   bool& remove_original_stmt_):
        al(al_), pass_result(pass_result_),
        current_scope(nullptr), result_expr(nullptr),
        remove_original_stmt(remove_original_stmt_) {}

    #define remove_original_stmt_if_size_0(type) if( ASRUtils::get_fixed_size_of_array(type) == 0 ) { \
            remove_original_stmt = true; \
            return ; \
        } \

    void replace_ArrayConstant(ASR::ArrayConstant_t* x) {
        remove_original_stmt_if_size_0(x->m_type)
        pass_result.reserve(al, x->m_n_data);
        const Location& loc = x->base.base.loc;
        LCOMPILERS_ASSERT(result_expr != nullptr);
        ASR::ttype_t* result_type = ASRUtils::expr_type(result_expr);
        ASR::ttype_t* result_element_type = ASRUtils::extract_type(result_type);
        for( int64_t i = 0; i < ASRUtils::get_fixed_size_of_array(x->m_type); i++ ) {
            ASR::expr_t* x_i = ASRUtils::fetch_ArrayConstant_value(al, x, i);
            Vec<ASR::array_index_t> array_index_args;
            array_index_args.reserve(al, 1);
            ASR::array_index_t array_index_arg;
            array_index_arg.loc = loc;
            array_index_arg.m_left = nullptr;
            array_index_arg.m_right = make_ConstantWithKind(
                make_IntegerConstant_t, make_Integer_t, i + 1, 4, loc);
            array_index_arg.m_step = nullptr;
            array_index_args.push_back(al, array_index_arg);
            ASR::expr_t* y_i = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc,
                result_expr, array_index_args.p, array_index_args.size(),
                result_element_type, ASR::arraystorageType::ColMajor, nullptr));
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, loc, y_i, x_i, nullptr)));
        }
    }

    bool are_all_elements_scalars(ASR::expr_t** args, size_t n) {
        for( size_t i = 0; i < n; i++ ) {
            if (ASR::is_a<ASR::ImpliedDoLoop_t>(*args[i])) {
                return false;
            }
            if( ASRUtils::is_array(ASRUtils::expr_type(args[i])) ) {
                return false;
            }
        }
        return true;
    }

    void replace_ArrayConstructor(ASR::ArrayConstructor_t* x) {
        // TODO: Remove this because the ArrayConstructor node should
        // be replaced with its value already (if present) in array_struct_temporary pass.
        if( x->m_value == nullptr ) {
            if( !are_all_elements_scalars(x->m_args, x->n_args) ) {
                PassUtils::ReplacerUtils::replace_ArrayConstructor_(
                    al, x, result_expr, &pass_result, current_scope);
                return ;
            }

            if( !ASRUtils::is_fixed_size_array(x->m_type) ) {
                PassUtils::ReplacerUtils::replace_ArrayConstructor_(
                    al, x, result_expr, &pass_result, current_scope);
                return ;
            }
        }

        ASR::ttype_t* arr_type = nullptr;
        ASR::ArrayConstant_t* arr_value = nullptr;
        if( x->m_value ) {
            arr_value = ASR::down_cast<ASR::ArrayConstant_t>(x->m_value);
            arr_type = arr_value->m_type;
        } else {
            arr_type = x->m_type;
        }

        remove_original_stmt_if_size_0(arr_type)

        pass_result.reserve(al, x->n_args);
        const Location& loc = x->base.base.loc;
        LCOMPILERS_ASSERT(result_expr != nullptr);

        ASR::ttype_t* result_type = ASRUtils::expr_type(result_expr);
        ASRUtils::ExprStmtDuplicator duplicator(al);
        ASR::ttype_t* result_element_type = ASRUtils::extract_type(result_type);
        result_element_type = duplicator.duplicate_ttype(result_element_type);

        FixTypeVisitor fix_type_visitor(al);
        fix_type_visitor.current_scope = current_scope;
        fix_type_visitor.visit_ttype(*result_element_type);

        for( int64_t i = 0; i < ASRUtils::get_fixed_size_of_array(arr_type); i++ ) {
            ASR::expr_t* x_i = nullptr;
            if( x->m_value ) {
                x_i = ASRUtils::fetch_ArrayConstant_value(al, arr_value, i);
            } else {
                x_i = x->m_args[i];
            }
            LCOMPILERS_ASSERT(!ASRUtils::is_array(ASRUtils::expr_type(x_i)));
            Vec<ASR::array_index_t> array_index_args;
            array_index_args.reserve(al, 1);
            ASR::array_index_t array_index_arg;
            array_index_arg.loc = loc;
            array_index_arg.m_left = nullptr;
            array_index_arg.m_right = make_ConstantWithKind(
                make_IntegerConstant_t, make_Integer_t, i + 1, 4, loc);
            array_index_arg.m_step = nullptr;
            array_index_args.push_back(al, array_index_arg);
            ASR::expr_t* y_i = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc,
                result_expr, array_index_args.p, array_index_args.size(),
                result_element_type, ASR::arraystorageType::ColMajor, nullptr));
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, loc, y_i, x_i, nullptr)));
        }
    }

};

ASR::expr_t* at(Vec<ASR::expr_t*>& vec, int64_t index) {
    index = index + vec.size();
    if( index < 0 ) {
        return nullptr;
    }
    return vec[index];
}

class ArrayOpVisitor: public ASR::CallReplacerOnExpressionsVisitor<ArrayOpVisitor> {
    private:

    Allocator& al;
    ReplaceArrayOp replacer;
    Vec<ASR::stmt_t*> pass_result;
    Vec<ASR::stmt_t*>* parent_body;
    bool realloc_lhs;
    bool remove_original_stmt;

    public:

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.current_scope = current_scope;
        replacer.replace_expr(*current_expr);
    }

    ArrayOpVisitor(Allocator& al_, bool realloc_lhs_):
        al(al_), replacer(al, pass_result, remove_original_stmt),
        parent_body(nullptr), realloc_lhs(realloc_lhs_),
        remove_original_stmt(false) {
        pass_result.n = 0;
        pass_result.reserve(al, 0);
    }

    void visit_Variable(const ASR::Variable_t& /*x*/) {
        // Do nothing
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        bool remove_original_stmt_copy = remove_original_stmt;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        if( parent_body ) {
            for (size_t j=0; j < pass_result.size(); j++) {
                parent_body->push_back(al, pass_result[j]);
            }
        }
        for (size_t i = 0; i < n_body; i++) {
            pass_result.n = 0;
            pass_result.reserve(al, 1);
            remove_original_stmt = false;
            Vec<ASR::stmt_t*>* parent_body_copy = parent_body;
            parent_body = &body;
            visit_stmt(*m_body[i]);
            parent_body = parent_body_copy;
            if( pass_result.size() > 0 ) {
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
            } else {
                if( !remove_original_stmt ) {
                    body.push_back(al, m_body[i]);
                    remove_original_stmt = false;
                }
            }
        }
        m_body = body.p;
        n_body = body.size();
        pass_result.n = 0;
        remove_original_stmt = remove_original_stmt_copy;
    }

    bool call_replace_on_expr(ASR::exprType expr_type) {
        switch( expr_type ) {
            case ASR::exprType::ArrayConstant:
            case ASR::exprType::ArrayConstructor: {
                return true;
            }
            default: {
                return false;
            }
        }
    }

    void increment_index_variables(std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
                                   size_t var_with_maxrank, int64_t loop_depth,
                                   Vec<ASR::stmt_t*>& do_loop_body, const Location& loc) {
        ASR::expr_t* step = make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, 4, loc);
        for( size_t i = 0; i < var2indices.size(); i++ ) {
            if( i == var_with_maxrank ) {
                continue;
            }
            ASR::expr_t* index_var = at(var2indices[i], loop_depth);
            if( index_var == nullptr ) {
                continue;
            }
            ASR::expr_t* plus_one = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, index_var,
                ASR::binopType::Add, step, ASRUtils::expr_type(index_var), nullptr));
            ASR::stmt_t* increment = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, index_var, plus_one, nullptr));
            do_loop_body.push_back(al, increment);
        }
    }

    void set_index_variables(std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
                             Vec<ASR::expr_t*>& vars_expr, size_t var_with_maxrank, size_t max_rank,
                             int64_t loop_depth, Vec<ASR::stmt_t*>& dest_vec, const Location& loc) {
        for( size_t i = 0; i < var2indices.size(); i++ ) {
            if( i == var_with_maxrank ) {
                continue;
            }
            ASR::expr_t* index_var = at(var2indices[i], loop_depth);
            if( index_var == nullptr ) {
                continue;
            }
            ASR::expr_t* lbound = PassUtils::get_bound(vars_expr[i],
                loop_depth + max_rank + 1, "lbound", al);
            ASR::stmt_t* set_index_var = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, index_var, lbound, nullptr));
            dest_vec.push_back(al, set_index_var);
        }
    }

    enum IndexType {
        ScalarIndex, ArrayIndex
    };

    void set_index_variables(std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
                             std::unordered_map<ASR::expr_t*, std::pair<ASR::expr_t*, IndexType>>& index2var,
                             size_t var_with_maxrank, size_t max_rank, int64_t loop_depth,
                             Vec<ASR::stmt_t*>& dest_vec, const Location& loc) {
        for( size_t i = 0; i < var2indices.size(); i++ ) {
            if( i == var_with_maxrank ) {
                continue;
            }
            ASR::expr_t* index_var = at(var2indices[i], loop_depth);
            if( index_var == nullptr ) {
                continue;
            }
            size_t bound_dim = loop_depth + max_rank + 1;
            if( index2var[index_var].second == IndexType::ArrayIndex ) {
                bound_dim = 1;
            }
            ASR::expr_t* lbound;
            if( !ASRUtils::is_array(ASRUtils::expr_type(index2var[index_var].first)) ){
                lbound = index2var[index_var].first;
            } else {
                lbound = PassUtils::get_bound(
                    index2var[index_var].first, bound_dim, "lbound", al);
            }
            ASR::stmt_t* set_index_var = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, index_var, lbound, nullptr));
            dest_vec.push_back(al, set_index_var);
        }
    }

    inline void fill_array_indices_in_vars_expr(
        ASR::expr_t* expr, bool is_expr_array,
        Vec<ASR::expr_t*>& vars_expr,
        size_t& offset_for_array_indices) {
        if( is_expr_array ) {
            ASR::array_index_t* m_args = nullptr; size_t n_args = 0;
            ASRUtils::extract_indices(expr, m_args, n_args);
            for( size_t i = 0; i < n_args; i++ ) {
                if( m_args[i].m_left == nullptr &&
                    m_args[i].m_right != nullptr &&
                    m_args[i].m_step == nullptr ) {
                    if( ASRUtils::is_array(ASRUtils::expr_type(
                            m_args[i].m_right)) ) {
                        vars_expr.push_back(al, m_args[i].m_right);
                    }
                }
            }
            offset_for_array_indices++;
        }
    }

    inline void create_array_item_array_indexed_expr(
            ASR::expr_t* expr, ASR::expr_t** expr_address,
            bool is_expr_array, int var2indices_key,
            size_t var_rank, const Location& loc,
            std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
            size_t& j, ASR::ttype_t* int32_type) {
        if( is_expr_array ) {
            ASR::array_index_t* m_args = nullptr; size_t n_args = 0;
            Vec<ASR::array_index_t> array_item_args;
            array_item_args.reserve(al, n_args);
            ASRUtils::extract_indices(expr, m_args, n_args);
            ASRUtils::ExprStmtDuplicator expr_duplicator(al);
            Vec<ASR::expr_t*> new_indices; new_indices.reserve(al, n_args);
            int k = 0;
            for( size_t i = 0; i < (n_args == 0 ? var_rank : n_args); i++ ) {
                if( m_args && m_args[i].m_left == nullptr &&
                    m_args[i].m_right != nullptr &&
                    m_args[i].m_step == nullptr ) {
                    if( ASRUtils::is_array(ASRUtils::expr_type(
                            m_args[i].m_right)) ) {
                        ASR::array_index_t array_index;
                        array_index.loc = loc;
                        array_index.m_left = nullptr;
                        Vec<ASR::array_index_t> indices1; indices1.reserve(al, 1);
                        ASR::array_index_t index1; index1.loc = loc; index1.m_left = nullptr;
                        index1.m_right = var2indices[j][0]; index1.m_step = nullptr;
                        new_indices.push_back(al, index1.m_right);
                        indices1.push_back(al, index1);
                        array_index.m_right = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc,
                            m_args[i].m_right, indices1.p, 1, int32_type,
                            ASR::arraystorageType::ColMajor, nullptr));
                        array_index.m_step = nullptr;
                        array_item_args.push_back(al, array_index);
                        j++;
                        k++;
                    } else {
                        array_item_args.push_back(al, m_args[i]);
                    }
                } else {
                    ASR::array_index_t index1; index1.loc = loc; index1.m_left = nullptr;
                    index1.m_right = var2indices[var2indices_key][k]; index1.m_step = nullptr;
                    array_item_args.push_back(al, index1);
                    new_indices.push_back(al, var2indices[var2indices_key][k]);
                    k++;
                }
            }
            var2indices[var2indices_key] = new_indices;
            ASR::ttype_t* expr_type = ASRUtils::extract_type(
                    ASRUtils::expr_type(expr));
            *expr_address = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(
                al, loc, ASRUtils::extract_array_variable(expr), array_item_args.p,
                array_item_args.size(), expr_type, ASR::arraystorageType::ColMajor, nullptr));
        }
    }

    template <typename T>
    void generate_loop_for_array_indexed_with_array_indices(const T& x,
        ASR::expr_t** target_address, ASR::expr_t** value_address,
        const Location& loc) {
        ASR::expr_t* target = *target_address;
        ASR::expr_t* value = *value_address;
        size_t var_rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(target));
        Vec<ASR::expr_t*> vars_expr; vars_expr.reserve(al, 2);
        bool is_target_array = ASRUtils::is_array(ASRUtils::expr_type(target));
        bool is_value_array = ASRUtils::is_array(ASRUtils::expr_type(value));
        Vec<ASR::array_index_t> array_indices_args; array_indices_args.reserve(al, 1);
        Vec<ASR::array_index_t> rhs_array_indices_args; rhs_array_indices_args.reserve(al, 1);
        int n_array_indices_args = -1;
        int temp_n = -1;
        size_t do_loop_depth = var_rank;
        if( is_target_array ) {
            vars_expr.push_back(al, ASRUtils::extract_array_variable(target));
            ASRUtils::extract_array_indices(target, al, array_indices_args, n_array_indices_args);
        }
        if( is_value_array ) {
            vars_expr.push_back(al, ASRUtils::extract_array_variable(value));
            ASRUtils::extract_array_indices(value, al, rhs_array_indices_args, temp_n);
        }

        size_t offset_for_array_indices = 0;

        fill_array_indices_in_vars_expr(
            target, is_target_array,
            vars_expr, offset_for_array_indices);
        fill_array_indices_in_vars_expr(
            value, is_value_array,
            vars_expr, offset_for_array_indices);

        // Common code for target and value
        std::unordered_map<size_t, Vec<ASR::expr_t*>> var2indices;
        std::unordered_map<ASR::expr_t*, std::pair<ASR::expr_t*, IndexType>> index2var;
        ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        for( size_t i = 0; i < vars_expr.size(); i++ ) {
            Vec<ASR::expr_t*> indices;
            indices.reserve(al, var_rank);
            for( size_t j = 0; j < (i >= offset_for_array_indices ? 1 : var_rank); j++ ) {
                std::string index_var_name = current_scope->get_unique_name(
                    "__libasr_index_" + std::to_string(j) + "_");
                ASR::symbol_t* index = ASR::down_cast<ASR::symbol_t>(ASRUtils::make_Variable_t_util(
                    al, loc, current_scope, s2c(al, index_var_name), nullptr, 0, ASR::intentType::Local,
                    nullptr, nullptr, ASR::storage_typeType::Default, int32_type, nullptr,
                    ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required, false));
                current_scope->add_symbol(index_var_name, index);
                ASR::expr_t* index_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, index));
                if ((i == offset_for_array_indices - 1) && is_value_array &&
                        rhs_array_indices_args[j].m_left != nullptr) {
                    index2var[index_expr] = std::make_pair(rhs_array_indices_args[j].m_left, IndexType::ScalarIndex);
                } else {
                    index2var[index_expr] = std::make_pair(vars_expr[i],
                        i >= offset_for_array_indices ? IndexType::ArrayIndex : IndexType::ScalarIndex);
                }
                indices.push_back(al, index_expr);
            }
            var2indices[i] = indices;
        }

        size_t j = offset_for_array_indices;

        create_array_item_array_indexed_expr(
            target, target_address, is_target_array, 0,
            var_rank, loc, var2indices, j, int32_type);
        create_array_item_array_indexed_expr(
            value, value_address, is_value_array, 1,
            var_rank, loc, var2indices, j, int32_type);

        size_t vars_expr_size = vars_expr.size();
        for( size_t i = offset_for_array_indices; i < vars_expr_size; i++ ) {
            var2indices.erase(i);
        }
        vars_expr.n = offset_for_array_indices;

        size_t var_with_maxrank = 0;

        ASR::do_loop_head_t do_loop_head;
        do_loop_head.loc = loc;
        do_loop_head.m_v = at(var2indices[var_with_maxrank], -1);
        size_t bound_dim = do_loop_depth;
        if( index2var[do_loop_head.m_v].second == IndexType::ArrayIndex ) {
            bound_dim = 1;
        }
        if( n_array_indices_args > -1 && array_indices_args[n_array_indices_args].m_right != nullptr &&
                array_indices_args[n_array_indices_args].m_left != nullptr &&
                array_indices_args[n_array_indices_args].m_step != nullptr) {
            do_loop_head.m_start = array_indices_args[n_array_indices_args].m_left;
            do_loop_head.m_end = array_indices_args[n_array_indices_args].m_right;
            do_loop_head.m_increment = array_indices_args[n_array_indices_args].m_step;
        } else {
            do_loop_head.m_start = PassUtils::get_bound(
                index2var[do_loop_head.m_v].first, bound_dim, "lbound", al);
            do_loop_head.m_end = PassUtils::get_bound(
                index2var[do_loop_head.m_v].first, bound_dim, "ubound", al);
            do_loop_head.m_increment = nullptr;
        }
        Vec<ASR::stmt_t*> parent_do_loop_body; parent_do_loop_body.reserve(al, 1);
        Vec<ASR::stmt_t*> do_loop_body; do_loop_body.reserve(al, 1);
        set_index_variables(var2indices, index2var, var_with_maxrank,
                            var_rank, -1, parent_do_loop_body, loc);
        do_loop_body.push_back(al, const_cast<ASR::stmt_t*>(&(x.base)));
        increment_index_variables(var2indices, var_with_maxrank, -1,
                                  do_loop_body, loc);
        ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
            do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
        do_loop_depth--;
        n_array_indices_args--;
        parent_do_loop_body.push_back(al, do_loop);
        do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
        parent_do_loop_body.reserve(al, 1);

        for( int64_t i = -2; i >= -static_cast<int64_t>(var_rank); i-- ) {
            set_index_variables(var2indices, index2var, var_with_maxrank,
                                var_rank, i, parent_do_loop_body, loc);
            increment_index_variables(var2indices, var_with_maxrank, i,
                                      do_loop_body, loc);
            ASR::do_loop_head_t do_loop_head;
            do_loop_head.loc = loc;
            do_loop_head.m_v = at(var2indices[var_with_maxrank], i);
            bound_dim = do_loop_depth;
            if( index2var[do_loop_head.m_v].second == IndexType::ArrayIndex ) {
                bound_dim = 1;
            }
            if( n_array_indices_args > -1 && array_indices_args[n_array_indices_args].m_right != nullptr &&
                    array_indices_args[n_array_indices_args].m_left != nullptr &&
                    array_indices_args[n_array_indices_args].m_step != nullptr) {
                do_loop_head.m_start = array_indices_args[n_array_indices_args].m_left;
                do_loop_head.m_end = array_indices_args[n_array_indices_args].m_right;
                do_loop_head.m_increment = array_indices_args[n_array_indices_args].m_step;
            } else {
                do_loop_head.m_start = PassUtils::get_bound(
                    index2var[do_loop_head.m_v].first, bound_dim, "lbound", al);
                do_loop_head.m_end = PassUtils::get_bound(
                    index2var[do_loop_head.m_v].first, bound_dim, "ubound", al);
                do_loop_head.m_increment = nullptr;
            }
            ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
                do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
            do_loop_depth--;
            n_array_indices_args--;
            parent_do_loop_body.push_back(al, do_loop);
            do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
            parent_do_loop_body.reserve(al, 1);
        }

        for( size_t i = 0; i < do_loop_body.size(); i++ ) {
            pass_result.push_back(al, do_loop_body[i]);
        }
    }

    template <typename T>
    void generate_loop(const T& x, Vec<ASR::expr_t**>& vars,
                       Vec<ASR::expr_t**>& fix_types_args,
                       const Location& loc) {
        Vec<size_t> var_ranks;
        Vec<ASR::expr_t*> vars_expr;
        var_ranks.reserve(al, vars.size()); vars_expr.reserve(al, vars.size());
        for( size_t i = 0; i < vars.size(); i++ ) {
            ASR::expr_t* expr = *vars[i];
            ASR::ttype_t* type = ASRUtils::expr_type(expr);
            var_ranks.push_back(al, ASRUtils::extract_n_dims_from_ttype(type));
            vars_expr.push_back(al, expr);
        }

        std::unordered_map<size_t, Vec<ASR::expr_t*>> var2indices;
        ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        for( size_t i = 0; i < vars.size(); i++ ) {
            Vec<ASR::expr_t*> indices;
            indices.reserve(al, var_ranks[i]);
            for( size_t j = 0; j < var_ranks[i]; j++ ) {
                std::string index_var_name = current_scope->get_unique_name(
                    "__libasr_index_" + std::to_string(j) + "_");
                ASR::symbol_t* index = ASR::down_cast<ASR::symbol_t>(ASRUtils::make_Variable_t_util(
                    al, loc, current_scope, s2c(al, index_var_name), nullptr, 0, ASR::intentType::Local,
                    nullptr, nullptr, ASR::storage_typeType::Default, int32_type, nullptr,
                    ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required, false));
                current_scope->add_symbol(index_var_name, index);
                ASR::expr_t* index_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, index));
                indices.push_back(al, index_expr);
            }
            var2indices[i] = indices;
        }

        for( size_t i = 0; i < vars.size(); i++ ) {
            Vec<ASR::array_index_t> indices;
            indices.reserve(al, var_ranks[i]);
            for( size_t j = 0; j < var_ranks[i]; j++ ) {
                ASR::array_index_t array_index;
                array_index.loc = loc;
                array_index.m_left = nullptr;
                array_index.m_right = var2indices[i][j];
                array_index.m_step = nullptr;
                indices.push_back(al, array_index);
            }
            ASR::ttype_t* var_i_type = ASRUtils::extract_type(
                ASRUtils::expr_type(*vars[i]));
            *vars[i] = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc, *vars[i], indices.p,
                indices.size(), var_i_type, ASR::arraystorageType::ColMajor, nullptr));
        }

        ASRUtils::RemoveArrayProcessingNodeVisitor array_broadcast_visitor(al);
        for( size_t i = 0; i < fix_types_args.size(); i++ ) {
            array_broadcast_visitor.current_expr = fix_types_args[i];
            array_broadcast_visitor.call_replacer();
        }

        FixTypeVisitor fix_types(al);
        fix_types.current_scope = current_scope;
        for( size_t i = 0; i < fix_types_args.size(); i++ ) {
            fix_types.visit_expr(*(*fix_types_args[i]));
        }

        size_t var_with_maxrank = 0;
        for( size_t i = 0; i < var_ranks.size(); i++ ) {
            if( var_ranks[i] > var_ranks[var_with_maxrank] ) {
                var_with_maxrank = i;
            }
        }

        ASR::do_loop_head_t do_loop_head;
        do_loop_head.loc = loc;
        do_loop_head.m_v = at(var2indices[var_with_maxrank], -1);
        do_loop_head.m_start = PassUtils::get_bound(vars_expr[var_with_maxrank],
            var_ranks[var_with_maxrank], "lbound", al);
        do_loop_head.m_end = PassUtils::get_bound(vars_expr[var_with_maxrank],
            var_ranks[var_with_maxrank], "ubound", al);
        do_loop_head.m_increment = nullptr;
        Vec<ASR::stmt_t*> parent_do_loop_body; parent_do_loop_body.reserve(al, 1);
        Vec<ASR::stmt_t*> do_loop_body; do_loop_body.reserve(al, 1);
        set_index_variables(var2indices, vars_expr, var_with_maxrank,
                            var_ranks[var_with_maxrank], -1, parent_do_loop_body, loc);
        do_loop_body.push_back(al, const_cast<ASR::stmt_t*>(&(x.base)));
        increment_index_variables(var2indices, var_with_maxrank, -1,
                                  do_loop_body, loc);
        ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
            do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
        parent_do_loop_body.push_back(al, do_loop);
        do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
        parent_do_loop_body.reserve(al, 1);

        for( int64_t i = -2; i >= -static_cast<int64_t>(var_ranks[var_with_maxrank]); i-- ) {
            set_index_variables(var2indices, vars_expr, var_with_maxrank,
                                var_ranks[var_with_maxrank], i, parent_do_loop_body, loc);
            increment_index_variables(var2indices, var_with_maxrank, i,
                                      do_loop_body, loc);
            ASR::do_loop_head_t do_loop_head;
            do_loop_head.loc = loc;
            do_loop_head.m_v = at(var2indices[var_with_maxrank], i);
            do_loop_head.m_start = PassUtils::get_bound(vars_expr[var_with_maxrank],
                var_ranks[var_with_maxrank] + i + 1, "lbound", al);
            do_loop_head.m_end = PassUtils::get_bound(vars_expr[var_with_maxrank],
                var_ranks[var_with_maxrank] + i + 1, "ubound", al);
            do_loop_head.m_increment = nullptr;
            ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
                do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
            parent_do_loop_body.push_back(al, do_loop);
            do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
            parent_do_loop_body.reserve(al, 1);
        }

        for( size_t i = 0; i < do_loop_body.size(); i++ ) {
            pass_result.push_back(al, do_loop_body[i]);
        }
    }

    void insert_realloc_for_target(ASR::expr_t* target, ASR::expr_t* value, Vec<ASR::expr_t**>& vars) {
        ASR::ttype_t* target_type = ASRUtils::expr_type(target);
        if( (realloc_lhs == false || !ASRUtils::is_allocatable(target_type) || vars.size() == 1) &&
            !(ASR::is_a<ASR::Var_t>(*value) && ASR::is_a<ASR::Var_t>(*target) &&
              ASRUtils::is_allocatable(target_type)) ) {
            return ;
        }

        // First element in vars is target itself
        ASR::expr_t* realloc_var = nullptr;
        size_t target_rank = ASRUtils::extract_n_dims_from_ttype(target_type);
        for( size_t i = 1; i < vars.size(); i++ ) {
            size_t var_rank = ASRUtils::extract_n_dims_from_ttype(
                ASRUtils::expr_type(*vars[i]));
            if( target_rank == var_rank ) {
                realloc_var = *vars[i];
                break ;
            }
        }

        Location loc; loc.first = 1, loc.last = 1;
        ASRUtils::ASRBuilder builder(al, loc);
        Vec<ASR::dimension_t> realloc_dims;
        realloc_dims.reserve(al, target_rank);
        for( size_t i = 0; i < target_rank; i++ ) {
            ASR::dimension_t realloc_dim;
            realloc_dim.loc = loc;
            realloc_dim.m_start = builder.i32(1);
            realloc_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                al, loc, realloc_var, builder.i32(i + 1), int32, nullptr));
            realloc_dims.push_back(al, realloc_dim);
        }

        Vec<ASR::alloc_arg_t> alloc_args; alloc_args.reserve(al, 1);
        ASR::alloc_arg_t alloc_arg;
        alloc_arg.loc = loc;
        alloc_arg.m_a = target;
        alloc_arg.m_dims = realloc_dims.p;
        alloc_arg.n_dims = realloc_dims.size();
        alloc_arg.m_len_expr = nullptr;
        alloc_arg.m_type = nullptr;
        alloc_args.push_back(al, alloc_arg);

        pass_result.push_back(al, ASRUtils::STMT(ASR::make_ReAlloc_t(
            al, loc, alloc_args.p, alloc_args.size())));
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if (ASRUtils::is_simd_array(x.m_target)) {
            if( !(ASRUtils::is_allocatable(x.m_value) ||
                  ASRUtils::is_pointer(ASRUtils::expr_type(x.m_value))) ) {
                return ;
            }
        }
        ASR::Assignment_t& xx = const_cast<ASR::Assignment_t&>(x);
        const std::vector<ASR::exprType>& skip_exprs = {
            ASR::exprType::IntrinsicArrayFunction,
            ASR::exprType::ArrayReshape,
        };
        if ( ASR::is_a<ASR::IntrinsicArrayFunction_t>(*xx.m_value) ) {
            // We need to do this because, we may have an assignment
            // in which IntrinsicArrayFunction is evaluated already and
            // value is an ArrayConstant, thus we need to unroll it.
            ASR::IntrinsicArrayFunction_t* iaf = ASR::down_cast<ASR::IntrinsicArrayFunction_t>(xx.m_value);
            if ( iaf->m_value != nullptr ) {
                xx.m_value = iaf->m_value;
            }
        }
        if( !ASRUtils::is_array(ASRUtils::expr_type(xx.m_target)) ||
            std::find(skip_exprs.begin(), skip_exprs.end(), xx.m_value->type) != skip_exprs.end() ||
            (ASRUtils::is_simd_array(xx.m_target) && ASRUtils::is_simd_array(xx.m_value)) ) {
            return ;
        }
        xx.m_value = ASRUtils::get_past_array_broadcast(xx.m_value);
        const Location loc = x.base.base.loc;

        #define is_array_indexed_with_array_indices_check(expr) \
            ASR::is_a<ASR::ArraySection_t>(*expr) || ( \
            ASR::is_a<ASR::ArrayItem_t>(*expr) && \
            ASRUtils::is_array_indexed_with_array_indices( \
                ASR::down_cast<ASR::ArrayItem_t>(expr)))
        if( ( is_array_indexed_with_array_indices_check(xx.m_value) ) ||
            ( is_array_indexed_with_array_indices_check(xx.m_target) ) ) {
            generate_loop_for_array_indexed_with_array_indices(
                x, &(xx.m_target), &(xx.m_value), loc);
            return ;
        }

        if( call_replace_on_expr(xx.m_value->type) ) {
            replacer.result_expr = xx.m_target;
            ASR::expr_t** current_expr_copy = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&xx.m_value);
            this->call_replacer();
            current_expr = current_expr_copy;
            replacer.result_expr = nullptr;
            return ;
        }

        Vec<ASR::expr_t**> vars;
        vars.reserve(al, 1);
        ArrayVarAddressCollector var_collector_target(al, vars);
        var_collector_target.current_expr = const_cast<ASR::expr_t**>(&(xx.m_target));
        var_collector_target.call_replacer();
        ArrayVarAddressCollector var_collector_value(al, vars);
        var_collector_value.current_expr = const_cast<ASR::expr_t**>(&(xx.m_value));
        var_collector_value.call_replacer();

        if (vars.size() == 1 &&
            ASRUtils::is_array(ASRUtils::expr_type(ASRUtils::get_past_array_broadcast(xx.m_value)))
        ) {
            return ;
        }

        if (ASRUtils::is_array(ASRUtils::expr_type(xx.m_value))) {
            insert_realloc_for_target(xx.m_target, xx.m_value, vars);
        }

        Vec<ASR::expr_t**> fix_type_args;
        fix_type_args.reserve(al, 2);
        fix_type_args.push_back(al, const_cast<ASR::expr_t**>(&(xx.m_target)));
        fix_type_args.push_back(al, const_cast<ASR::expr_t**>(&(xx.m_value)));

        generate_loop(x, vars, fix_type_args, loc);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        if( !PassUtils::is_elemental(x.m_name) ) {
            return ;
        }
        const Location loc = x.base.base.loc;

        Vec<ASR::expr_t**> vars;
        vars.reserve(al, 1);
        for( size_t i = 0; i < x.n_args; i++ ) {
            if( x.m_args[i].m_value != nullptr &&
                ASRUtils::is_array(ASRUtils::expr_type(x.m_args[i].m_value)) ) {
                vars.push_back(al, &(x.m_args[i].m_value));
            }
        }

        if( vars.size() == 0 ) {
            return ;
        }

        Vec<ASR::expr_t**> fix_type_args;
        fix_type_args.reserve(al, 1);

        generate_loop(x, vars, fix_type_args, loc);
    }

    void visit_If(const ASR::If_t& x) {
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_test)) ) {
            ASR::CallReplacerOnExpressionsVisitor<ArrayOpVisitor>::visit_If(x);
            return ;
        }

        const Location loc = x.base.base.loc;

        Vec<ASR::expr_t**> vars;
        vars.reserve(al, 1);
        ArrayVarAddressCollector array_var_adress_collector_target(al, vars);
        array_var_adress_collector_target.visit_If(x);

        if( vars.size() == 0 ) {
            return ;
        }

        Vec<ASR::expr_t**> fix_type_args;
        fix_type_args.reserve(al, 1);

        generate_loop(x, vars, fix_type_args, loc);

        ASRUtils::RemoveArrayProcessingNodeVisitor remove_array_processing_node_visitor(al);
        remove_array_processing_node_visitor.visit_If(x);

        FixTypeVisitor fix_type_visitor(al);
        fix_type_visitor.current_scope = current_scope;
        fix_type_visitor.visit_If(x);
    }

};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit,
                           const LCompilers::PassOptions& pass_options) {
    ArrayOpVisitor v(al, pass_options.realloc_lhs);
    v.call_replacer_on_value = false;
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
