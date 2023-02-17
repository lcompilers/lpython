#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_compare.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LCompilers {

using ASR::down_cast;

/*
 * This ASR pass handles TupleCompare and ListCompare Nodes.
 * Converts:
 *      a: tuple[T1, T2]
 *      b: tuple[T1, T2]
 *      assert a == b
 *
 *  to:
 *      a: tuple[T1, T2]
 *      b: tuple[T1, T2]
 *      # Replaces with as FunctionCall
 *      assert _lcompilers_tuple_compare_tuple[T1,T2](a, b)
 *
 *
 * Similarly,
 *
 * Converts:
 *       a: list[T]
 *       b: list[T]
 *       assert a == b
 *
 *   to:
 *       a: list[T]
 *       b: list[T]
 *       # Replaces with as FunctionCall
 *       assert _lcompilers_list_compare_list[T](a, b)
 */


class CompareExprReplacer : public ASR::BaseExprReplacer<CompareExprReplacer>
{
private:

    Allocator& al;
    ASR::TranslationUnit_t &unit;
    std::map<std::string, ASR::symbol_t*> &compare_func_map;

public:
    CompareExprReplacer(Allocator &al_, ASR::TranslationUnit_t &unit_,
                      std::map<std::string, ASR::symbol_t*> &compare_func_map_) :
        al(al_), unit(unit_),
        compare_func_map(compare_func_map_)
        { }


    #define create_args(x, type, symtab, vec_exprs) { \
        ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>( \
            ASR::make_Variable_t(al, loc, symtab, \
            s2c(al, x), nullptr, 0, ASR::intentType::In, nullptr, nullptr, \
            ASR::storage_typeType::Default, type, \
            ASR::abiType::Source, ASR::accessType::Public, \
            ASR::presenceType::Required, false)); \
        ASR::expr_t* arg_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg)); \
        vec_exprs.push_back(al, arg_expr); \
        symtab->add_symbol(x, arg); \
    }

    ASR::symbol_t* get_tuple_compare_func(Location& loc,
                SymbolTable*& scope, ASR::ttype_t *t) {
        std::string type_name = ASRUtils::type_to_str_python(t);
        if (compare_func_map.find(type_name) == compare_func_map.end()) {
            create_tuple_compare(loc, scope, t);
        }
        return compare_func_map[type_name];
    }

    ASR::symbol_t* get_list_compare_func(Location& loc,
                SymbolTable*& scope, ASR::ttype_t *t) {
        std::string type_name = ASRUtils::type_to_str_python(t);
        if (compare_func_map.find(type_name) == compare_func_map.end()) {
            create_list_compare(loc, scope, t);
        }
        return compare_func_map[type_name];
    }

    ASR::expr_t* compare_helper(Location& loc,
                    SymbolTable*& global_scope, ASR::expr_t *left, ASR::expr_t *rig,
                    ASR::ttype_t *type) {
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return ASRUtils::EXPR(ASR::make_IntegerCompare_t(al,
                    loc, left, ASR::cmpopType::Eq, rig, bool_type, nullptr));
            }
            case ASR::ttypeType::Real: {
                return ASRUtils::EXPR(ASR::make_RealCompare_t(al,
                    loc, left, ASR::cmpopType::Eq, rig, bool_type, nullptr));
            }
            case ASR::ttypeType::Character: {
                return ASRUtils::EXPR(ASR::make_StringCompare_t(al,
                    loc, left, ASR::cmpopType::Eq, rig, bool_type, nullptr));
            }
            case ASR::ttypeType::Complex: {
                return ASRUtils::EXPR(ASR::make_ComplexCompare_t(al,
                    loc, left, ASR::cmpopType::Eq, rig, bool_type, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return ASRUtils::EXPR(ASR::make_LogicalCompare_t(al,
                    loc, left, ASR::cmpopType::Eq, rig, bool_type, nullptr));
            }
            case ASR::ttypeType::Tuple: {
                ASR::symbol_t *fn = get_tuple_compare_func(loc, global_scope, type);
                Vec<ASR::call_arg_t> args;
                args.reserve(al, 2);
                ASR::call_arg_t call_arg;
                call_arg.loc = left->base.loc;
                call_arg.m_value = left;
                args.push_back(al, call_arg);
                call_arg.loc = rig->base.loc;
                call_arg.m_value = rig;
                args.push_back(al, call_arg);

                return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
                    fn, nullptr, args.p, args.n,
                    bool_type, nullptr, nullptr));
            }
            case ASR::ttypeType::List: {
                ASR::symbol_t *fn = get_list_compare_func(loc, global_scope, type);
                Vec<ASR::call_arg_t> args;
                args.reserve(al, 2);
                ASR::call_arg_t call_arg;
                call_arg.loc = left->base.loc;
                call_arg.m_value = left;
                args.push_back(al, call_arg);
                call_arg.loc = rig->base.loc;
                call_arg.m_value = rig;
                args.push_back(al, call_arg);

                return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
                    fn, nullptr, args.p, args.n,
                    bool_type, nullptr, nullptr));
            }
            default: {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    void create_tuple_compare(Location& loc,
                SymbolTable*& global_scope, ASR::ttype_t* type) {
        /*
            def _lcompilers_tuple_compare(a: tuple[], b: tuple[]) -> bool:
                res: bool = True
                for i in range(len(a)):
                    res &= a[i]==b[i]
                return res
        */
        SymbolTable* tup_compare_symtab = al.make_new<SymbolTable>(global_scope);
        std::string tuple_type_name = ASRUtils::type_to_str_python(type);
        ASR::Tuple_t *tuple_type = ASR::down_cast<ASR::Tuple_t>(type);

        std::string fn_name = global_scope->get_unique_name("_lcompilers_tuple_compare_" + tuple_type_name);
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));

        Vec<ASR::expr_t*> arg_exprs;
        arg_exprs.reserve(al, 2);

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 2 + tuple_type->n_type);

        // Declare arguments
        create_args("a", type, tup_compare_symtab, arg_exprs)
        create_args("b", type, tup_compare_symtab, arg_exprs)

        // Declare `result`
        ASR::symbol_t* arg = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, tup_compare_symtab,
            s2c(al, "result"), nullptr, 0, ASR::intentType::ReturnVar, nullptr, nullptr,
            ASR::storage_typeType::Default, bool_type,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false));
        ASR::expr_t* result = ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg));
        tup_compare_symtab->add_symbol("result", arg);


        ASR::expr_t* value = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al,
                        loc, true, bool_type));
        // Initialize `result` with `True`
        ASR::stmt_t* init_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                        result, value, nullptr));
        body.push_back(al, init_stmt);

        for (size_t i=0; i<tuple_type->n_type; i++) {
            ASR::expr_t *pos_i = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, i, int_type));
            ASR::expr_t *a_i = ASRUtils::EXPR(ASR::make_TupleItem_t(al, loc, arg_exprs[0], pos_i,
                tuple_type->m_type[i], nullptr));
            ASR::expr_t *b_i = ASRUtils::EXPR(ASR::make_TupleItem_t(al, loc, arg_exprs[1], pos_i,
                tuple_type->m_type[i], nullptr));
            ASR::expr_t *cmp_i = compare_helper(loc, global_scope, a_i, b_i, tuple_type->m_type[i]);
            ASR::expr_t *cmp_and = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
                    result, ASR::logicalbinopType::And, cmp_i, bool_type, nullptr));
            ASR::stmt_t *t = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                        result, cmp_and, nullptr));
            body.push_back(al, t);
        }

        // Return
        body.push_back(al, ASRUtils::STMT(ASR::make_Return_t(al, loc)));

        ASR::asr_t *fn = ASRUtils::make_Function_t_util(
            al, loc,
            /* a_symtab */ tup_compare_symtab,
            /* a_name */ s2c(al, fn_name),
            nullptr, 0,
            /* a_args */ arg_exprs.p,
            /* n_args */ arg_exprs.n,
            /* a_body */ body.p,
            /* n_body */ body.n,
            /* a_return_var */ result,
            ASR::abiType::Source,
            ASR::accessType::Public, ASR::deftypeType::Implementation,
            nullptr,
            false, false, false, false, false,
            nullptr, 0,
            nullptr, 0,
            false, false, false);
        ASR::symbol_t *fn_sym = ASR::down_cast<ASR::symbol_t>(fn);
        global_scope->add_symbol(fn_name, fn_sym);
        compare_func_map[tuple_type_name] = fn_sym;
    }

    /*
    This function replaces `TupleCompare` with a `FunctionCall` which returns
    the True if both the Tuples are equal or else returns False.
    Converts:
        a: tuple[i32, i32]
        b: tuple[i32, i32]
        assert a == b

    to:
        a: tuple[i32, i32]
        b: tuple[i32, i32]
        assert _lcompilers_tuple_compare(a, b)
    */
    void replace_TupleCompare(const ASR::TupleCompare_t* x) {
        Location loc = x->base.base.loc;
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));
        Vec<ASR::call_arg_t> args;
        args.reserve(al, 2);
        ASR::call_arg_t call_arg;
        call_arg.loc = x->m_left->base.loc;
        call_arg.m_value = x->m_left;
        args.push_back(al, call_arg);
        call_arg.loc = x->m_right->base.loc;
        call_arg.m_value = x->m_right;
        args.push_back(al, call_arg);
        ASR::symbol_t *fn_sym = get_tuple_compare_func(unit.base.base.loc,
                unit.m_global_scope, ASRUtils::expr_type(x->m_left));
        *current_expr = ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
            fn_sym, nullptr, args.p, args.n, bool_type, nullptr, nullptr));
        if (x->m_op == ASR::cmpopType::NotEq) {
            *current_expr = ASRUtils::EXPR(ASR::make_LogicalNot_t(al, loc,
                        *current_expr, bool_type, nullptr));
        }
    }


    void _create_list_compare_while_loop(Location &loc, SymbolTable* symtab,
                           ASR::expr_t* a, ASR::expr_t *b, ASR::expr_t *len,
                           ASR::expr_t* result,
                           Vec<ASR::stmt_t*>& body, ASR::ttype_t* item_type) {
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, 1, loc, al, symtab);
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(
            al, loc, 4, nullptr, 0));
        ASR::expr_t *const_zero = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                        al, loc, 0, int_type));
        ASR::expr_t *const_one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                        al, loc, 1, int_type));
        ASR::stmt_t* _tmp = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], const_zero, nullptr));
        body.push_back(al, _tmp);


        ASR::expr_t* loop_test = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
            al, loc, len, ASR::cmpopType::Gt, idx_vars[0], bool_type, nullptr));

        Vec<ASR::stmt_t*> loop_body;
        loop_body.reserve(al, 2);

        ASR::expr_t *a_i = ASRUtils::EXPR(ASR::make_ListItem_t(al, loc, a, idx_vars[0],
            item_type, nullptr));
        ASR::expr_t *b_i = ASRUtils::EXPR(ASR::make_ListItem_t(al, loc, b, idx_vars[0],
            item_type, nullptr));
        ASR::expr_t *cmp_i = compare_helper(loc, symtab, a_i, b_i, item_type);
        ASR::expr_t *cmp_and = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
                result, ASR::logicalbinopType::And, cmp_i, bool_type, nullptr));
        _tmp = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                    result, cmp_and, nullptr));
        loop_body.push_back(al, _tmp);

        _tmp = ASRUtils::STMT(ASR::make_Assignment_t(
            al, loc, idx_vars[0], ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                al, loc, idx_vars[0], ASR::binopType::Add, const_one,
                int_type, nullptr)), nullptr));
        loop_body.push_back(al, _tmp);

        _tmp = ASRUtils::STMT(ASR::make_WhileLoop_t(
            al, loc, loop_test, loop_body.p, loop_body.n));
        body.push_back(al, _tmp);
    }

    void create_list_compare(Location& loc,
                SymbolTable*& global_scope, ASR::ttype_t* type) {
        /*
            def _lcompilers_list_compare(a: list[], b: list[]) -> bool:
                res: bool = True
                a_len = len(a)
                b_len = len(b)
                if a_len != b_len:
                    return False
                i: i32
                for i in range(len(a)):
                    res &= a[i]==b[i]
                return res
        */
        SymbolTable* list_compare_symtab = al.make_new<SymbolTable>(global_scope);
        std::string list_type_name = ASRUtils::type_to_str_python(type);
        ASR::List_t *list_type = ASR::down_cast<ASR::List_t>(type);

        std::string fn_name = global_scope->get_unique_name("_lcompilers_list_compare_" + list_type_name);
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(
            al, loc, 4, nullptr, 0));
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));

        Vec<ASR::expr_t*> arg_exprs;
        arg_exprs.reserve(al, 2);

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 5);

        // Declare arguments
        create_args("a", type, list_compare_symtab, arg_exprs)
        create_args("b", type, list_compare_symtab, arg_exprs)

        ASR::expr_t *a_len = ASRUtils::EXPR(ASR::make_ListLen_t(al, loc,
                    arg_exprs[0], int_type, nullptr));
        ASR::expr_t *b_len = ASRUtils::EXPR(ASR::make_ListLen_t(al, loc,
                    arg_exprs[1], int_type, nullptr));

        // Declare `result`
        ASR::symbol_t* res_arg = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, list_compare_symtab,
            s2c(al, "result"), nullptr, 0, ASR::intentType::ReturnVar, nullptr, nullptr,
            ASR::storage_typeType::Default, bool_type,
            ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false));
        ASR::expr_t* result = ASRUtils::EXPR(ASR::make_Var_t(al, loc, res_arg));
        list_compare_symtab->add_symbol("result", res_arg);

        ASR::expr_t* value = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al,
                        loc, true, bool_type));

        // Initialize `result` with `True`
        ASR::stmt_t* _tmp = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                        result, value, nullptr));
        body.push_back(al, _tmp);

        {
            ASR::expr_t* a_test = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                al, loc, a_len, ASR::cmpopType::NotEq, b_len,
                bool_type, nullptr));

            Vec<ASR::stmt_t*> if_body;
            if_body.reserve(al, 2);
            value = ASRUtils::EXPR(ASR::make_LogicalConstant_t(al,
                        loc, false, bool_type));
            ASR::stmt_t* if_body_stmt = ASRUtils::STMT(ASR::make_Assignment_t(
                al, loc, result, value, nullptr));
            if_body.push_back(al, if_body_stmt);

            // Return
            if_body.push_back(al, ASRUtils::STMT(ASR::make_Return_t(al, loc)));

            _tmp = ASRUtils::STMT(ASR::make_If_t(al, loc, a_test,
                if_body.p, if_body.n, nullptr, 0));
            body.push_back(al, _tmp);
        }

        _create_list_compare_while_loop(loc, global_scope,
                    arg_exprs[0], arg_exprs[1], a_len, result,
                    body, list_type->m_type);

        // Return
        body.push_back(al, ASRUtils::STMT(ASR::make_Return_t(al, loc)));

        ASR::asr_t *fn = ASRUtils::make_Function_t_util(
            al, loc,
            /* a_symtab */ list_compare_symtab,
            /* a_name */ s2c(al, fn_name),
            nullptr, 0,
            /* a_args */ arg_exprs.p,
            /* n_args */ arg_exprs.n,
            /* a_body */ body.p,
            /* n_body */ body.n,
            /* a_return_var */ result,
            ASR::abiType::Source,
            ASR::accessType::Public, ASR::deftypeType::Implementation,
            nullptr,
            false, false, false, false, false,
            nullptr, 0,
            nullptr, 0,
            false, false, false);
        ASR::symbol_t *fn_sym = ASR::down_cast<ASR::symbol_t>(fn);
        global_scope->add_symbol(fn_name, fn_sym);
        compare_func_map[list_type_name] = fn_sym;
    }

    /*
    This function replaces `ListCompare` with a `FunctionCall` which returns
    the True if both the lists are equal or else returns False.
    Converts:
        a: list[i32]
        b: list[i32]
        assert a == b

    to:
        a: list[i32]
        b: list[i32]
        assert _lcompilers_list_compare(a, b)
    */
    void replace_ListCompare(const ASR::ListCompare_t* x) {
        Location loc = x->base.base.loc;
        ASR::ttype_t* bool_type = ASRUtils::TYPE(ASR::make_Logical_t(
            al, loc, 4, nullptr, 0));
        Vec<ASR::call_arg_t> args;
        args.reserve(al, 2);
        ASR::call_arg_t call_arg;
        call_arg.loc = x->m_left->base.loc;
        call_arg.m_value = x->m_left;
        args.push_back(al, call_arg);
        call_arg.loc = x->m_right->base.loc;
        call_arg.m_value = x->m_right;
        args.push_back(al, call_arg);
        ASR::symbol_t *fn_sym = get_list_compare_func(unit.base.base.loc,
                unit.m_global_scope, ASRUtils::expr_type(x->m_left));
        *current_expr = ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
            fn_sym, nullptr, args.p, args.n, bool_type, nullptr, nullptr));
        if (x->m_op == ASR::cmpopType::NotEq) {
            *current_expr = ASRUtils::EXPR(ASR::make_LogicalNot_t(al, loc,
                        *current_expr, bool_type, nullptr));
        }
    }

};

class CompareExprVisitor : public ASR::CallReplacerOnExpressionsVisitor<CompareExprVisitor>
{
private:

    CompareExprReplacer replacer;
    std::map<std::string, ASR::symbol_t *> compare_func_map;

public:

    CompareExprVisitor(Allocator& al_, ASR::TranslationUnit_t &unit_) :
        replacer(al_, unit_, compare_func_map)
        { }

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.replace_expr(*current_expr);
    }

};

void pass_compare(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/) {
    CompareExprVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
