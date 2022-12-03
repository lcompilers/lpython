#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_compare.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LFortran {

using ASR::down_cast;

/*
 * This ASR pass handles TupleCompare.
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
            s2c(al, x), ASR::intentType::In, nullptr, nullptr, \
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
            default: {
                LFORTRAN_ASSERT(false);
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
            s2c(al, "result"), ASR::intentType::Local, nullptr, nullptr,
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

        ASR::asr_t *fn = ASR::make_Function_t(
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
            false);
        ASR::symbol_t *fn_sym = ASR::down_cast<ASR::symbol_t>(fn);
        global_scope->add_symbol(fn_name, fn_sym);
        compare_func_map[tuple_type_name] = fn_sym;
    }

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


} // namespace LFortran
