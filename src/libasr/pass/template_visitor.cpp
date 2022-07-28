#include <cmath>

#include <libasr/asr_utils.h>
#include <libasr/pass/pass_utils.h>
#include <lpython/semantics/semantic_exception.h>

namespace LFortran {

class TemplateVisitor : public PassUtils::PassVisitor<TemplateVisitor>
{
public:
    ASR::asr_t *tmp;
    std::map<std::string, ASR::ttype_t*> subs;
    ASR::asr_t *new_function;
    int new_function_num;

    TemplateVisitor(Allocator &al, std::map<std::string, ASR::ttype_t*> subs,
            SymbolTable *current_scope, int new_function_num):
        PassVisitor(al, current_scope),
        subs{subs},
        new_function_num{new_function_num}
        {}

    void visit_TemplateFunction(const ASR::TemplateFunction_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);

        std::string func_name = x.m_name;
        func_name = "__lpython_generic_" + func_name + "_" + std::to_string(new_function_num);

        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *param_var = ASR::down_cast<ASR::Variable_t>(
                (ASR::down_cast<ASR::Var_t>(x.m_args[i]))->m_v);
            ASR::ttype_t *param_type = ASRUtils::expr_type(x.m_args[i]);
            // TODO: Should handle nested types
            /*
            ASR::ttype_t *arg_type = ASR::is_a<ASR::TypeParameter_t>(*param_type) ?
                subs[ASR::down_cast<ASR::TypeParameter_t>(param_type)->m_param] : param_type;
            */
            ASR::ttype_t *arg_type = substitute_type(param_type);

            Location loc = param_var->base.base.loc;
            std::string var_name = param_var->m_name;
            ASR::intentType s_intent = param_var->m_intent;
            ASR::expr_t *init_expr = nullptr;
            ASR::expr_t *value = nullptr;
            ASR::storage_typeType storage_type = param_var->m_storage;
            ASR::abiType abi_type = param_var->m_abi;
            ASR::accessType s_access = param_var->m_access;
            ASR::presenceType s_presence = param_var->m_presence;
            bool value_attr = param_var->m_value_attr;

            // TODO: Copying variable can be abstracted into a function
            ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), s_intent, init_expr, value, storage_type, arg_type,
                abi_type, s_access, s_presence, value_attr);
            current_scope->add_symbol(var_name, ASR::down_cast<ASR::symbol_t>(v));

            ASR::symbol_t *var = current_scope->get_symbol(var_name);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, var)));
        }

        ASR::Variable_t *return_var = ASR::down_cast<ASR::Variable_t>(
            (ASR::down_cast<ASR::Var_t>(x.m_return_var))->m_v);
        std::string return_var_name = return_var->m_name;
        ASR::ttype_t *return_param_type = ASRUtils::expr_type(x.m_return_var);
        ASR::ttype_t *return_type = ASR::is_a<ASR::TypeParameter_t>(*return_param_type) ?
            subs[ASR::down_cast<ASR::TypeParameter_t>(return_param_type)->m_param] : return_param_type;
        ASR::asr_t *new_return_var = ASR::make_Variable_t(al, return_var->base.base.loc,
            current_scope, s2c(al, return_var_name), return_var->m_intent, nullptr, nullptr,
            return_var->m_storage, return_type, return_var->m_abi, return_var->m_access,
            return_var->m_presence, return_var->m_value_attr);
        current_scope->add_symbol(return_var_name, ASR::down_cast<ASR::symbol_t>(new_return_var));
        ASR::asr_t *new_return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
            current_scope->get_symbol(return_var_name));

        // Rebuild the symbol table
        for (auto const &sym_pair: x.m_symtab->get_scope()) {
            if (current_scope->resolve_symbol(sym_pair.first) == nullptr) {
                ASR::symbol_t *sym = sym_pair.second;
                ASR::ttype_t *new_sym_type = substitute_type(ASRUtils::symbol_type(sym));
                if (ASR::is_a<ASR::Variable_t>(*sym)) {
                    ASR::Variable_t *var_sym = ASR::down_cast<ASR::Variable_t>(sym);
                    std::string var_sym_name = var_sym->m_name;
                    ASR::asr_t *new_var = ASR::make_Variable_t(al, var_sym->base.base.loc,
                        current_scope, s2c(al, var_sym_name), var_sym->m_intent, nullptr, nullptr,
                        var_sym->m_storage, new_sym_type, var_sym->m_abi, var_sym->m_access,
                        var_sym->m_presence, var_sym->m_value_attr);
                    current_scope->add_symbol(var_sym_name, ASR::down_cast<ASR::symbol_t>(new_var));
                }
            }
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                ASR::stmt_t *tmp_stmt = ASRUtils::STMT(tmp);
                body.push_back(al, tmp_stmt);
            }
            tmp = nullptr;
        }

        ASR::abiType func_abi = x.m_abi;
        ASR::accessType func_access = x.m_access;
        ASR::deftypeType func_deftype = x.m_deftype;
        char *bindc_name = x.m_bindc_name;

        new_function = ASR::make_Function_t(
            al, x.base.base.loc,
            current_scope, s2c(al, func_name),
            args.p, args.size(),
            body.p, body.size(),
            ASRUtils::EXPR(new_return_var_ref),
            func_abi, func_access, func_deftype, bindc_name);

        ASR::symbol_t *t = ASR::down_cast<ASR::symbol_t>(new_function);
        parent_scope->add_symbol(func_name, t);
        current_scope = parent_scope;
    }

    // stmt_t
    void visit_Assignment(const ASR::Assignment_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = ASRUtils::EXPR(tmp);

        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value, nullptr);
    }

    void visit_Return(const ASR::Return_t &x) {
        tmp = ASR::make_Return_t(al, x.base.base.loc);
    }

    // expr_t
    void visit_Var(const ASR::Var_t &x) {
        std::string sym_name = ASRUtils::symbol_name(x.m_v);
        ASR::symbol_t *sym = current_scope->get_symbol(sym_name);
        tmp = ASR::make_Var_t(al, x.base.base.loc, sym);
    }

    void visit_TemplateBinOp(const ASR::TemplateBinOp_t &x) {
        ASR::expr_t *left;
        ASR::expr_t *right;
        
        this->visit_expr(*x.m_left);  
        left = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        right = ASRUtils::EXPR(tmp);
        
        ASR::binopType op;
        op = x.m_op;

        // No type check for now
        make_BinOp_helper(left, right, op, x.base.base.loc);
    }

    ASR::ttype_t* substitute_type(ASR::ttype_t *param_type) {
        if (ASR::is_a<ASR::List_t>(*param_type)) {
            ASR::List_t *list_type = ASR::down_cast<ASR::List_t>(param_type);
            ASR::ttype_t *elem_type = substitute_type(list_type->m_type);
            return ASRUtils::TYPE(ASR::make_List_t(al, param_type->base.loc, elem_type));
        }
        if (ASR::is_a<ASR::TypeParameter_t>(*param_type)) {
            ASR::TypeParameter_t *param = ASR::down_cast<ASR::TypeParameter_t>(param_type);
            std::string param_name = param->m_param;
            return subs[param_name];
        }
        return param_type;
    }    
    
    void make_BinOp_helper(ASR::expr_t *left, ASR::expr_t *right,
            ASR::binopType op, const Location &loc) {
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;
    
        // bool right_is_int = ASRUtils::is_character(*left_type) && ASRUtils::is_integer(*right_type);
        // bool left_is_int = ASRUtils::is_integer(*left_type) && ASRUtils::is_character(*right_type);   

        if ((ASRUtils::is_integer(*left_type) || ASRUtils::is_real(*left_type) ||
                ASRUtils::is_complex(*left_type) || ASRUtils::is_logical(*left_type)) &&
                (ASRUtils::is_integer(*right_type) || ASRUtils::is_real(*right_type) ||
                ASRUtils::is_complex(*right_type) || ASRUtils::is_logical(*right_type))) {
            left = cast_helper(ASRUtils::expr_type(right), left);
            right = cast_helper(ASRUtils::expr_type(left), right);
            dest_type = ASRUtils::expr_type(left);
        } else if (ASRUtils::is_character(*left_type) && ASRUtils::is_character(*right_type)
                && op == ASR::binopType::Add) {
            // string concat
            ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(left_type);
            ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(right_type);
            LFORTRAN_ASSERT(left_type2->n_dims == 0);
            LFORTRAN_ASSERT(right_type2->n_dims == 0);
            dest_type = ASR::down_cast<ASR::ttype_t>(
                    ASR::make_Character_t(al, loc, left_type2->m_kind,
                    left_type2->m_len + right_type2->m_len, nullptr, nullptr, 0));
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                char* left_value = ASR::down_cast<ASR::StringConstant_t>(ASRUtils::expr_value(left))->m_s;
                char* right_value = ASR::down_cast<ASR::StringConstant_t>(ASRUtils::expr_value(right))->m_s;
                char* result;
                std::string result_s = std::string(left_value) + std::string(right_value);
                result = s2c(al, result_s);
                LFORTRAN_ASSERT((int64_t)strlen(result) == ASR::down_cast<ASR::Character_t>(dest_type)->m_len)
                value = ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al, loc, result, dest_type));
            }
            tmp = ASR::make_StringConcat_t(al, loc, left, right, dest_type, value);
            return;
        }

        if (ASRUtils::is_integer(*dest_type)) {
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(left))->m_n;
                int64_t right_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(right))->m_n;
                int64_t result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Sub): { result = left_value - right_value; break; }
                    case (ASR::binopType::Mul): { result = left_value * right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    case (ASR::binopType::Pow): { result = std::pow(left_value, right_value); break; }
                    case (ASR::binopType::BitAnd): { result = left_value & right_value; break; }
                    case (ASR::binopType::BitOr): { result = left_value | right_value; break; }
                    case (ASR::binopType::BitXor): { result = left_value ^ right_value; break; }
                    case (ASR::binopType::BitLShift): {
                        if (right_value < 0) {
                            throw SemanticError("Negative shift count not allowed.", loc);
                        }
                        result = left_value << right_value;
                        break;
                    }
                    case (ASR::binopType::BitRShift): {
                        if (right_value < 0) {
                            throw SemanticError("Negative shift count not allowed.", loc);
                        }
                        result = left_value >> right_value;
                        break;
                    }
                    default: { LFORTRAN_ASSERT(false); } // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, loc, result, dest_type));
            }
            tmp = ASR::make_IntegerBinOp_t(al, loc, left, op, right, dest_type, value);
        } else if (ASRUtils::is_real(*dest_type)) {
            if (op == ASR::binopType::BitAnd || op == ASR::binopType::BitOr || op == ASR::binopType::BitXor ||
                op == ASR::binopType::BitLShift || op == ASR::binopType::BitRShift) {
                throw SemanticError("Unsupported binary operation on floats: '" + ASRUtils::binop_to_str_python(op) + "'", loc);
            }
            right = cast_helper(left_type, right);
            dest_type = ASRUtils::expr_type(right);
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                double left_value = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(left))->m_r;
                double right_value = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(right))->m_r;
                double result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Sub): { result = left_value - right_value; break; }
                    case (ASR::binopType::Mul): { result = left_value * right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    case (ASR::binopType::Pow): { result = std::pow(left_value, right_value); break; }
                    default: { LFORTRAN_ASSERT(false); }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(al, loc, result, dest_type));
            }
            tmp = ASR::make_RealBinOp_t(al, loc, left, op, right, dest_type, value);
        }        
    }

    ASR::expr_t *cast_helper(ASR::ttype_t *left_type, ASR::expr_t *right,
            bool is_assign=false) {
        ASR::ttype_t *right_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(right));
        if (ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type)) {
            int lkind = ASR::down_cast<ASR::Integer_t>(left_type)->m_kind;
            int rkind = ASR::down_cast<ASR::Integer_t>(right_type)->m_kind;
            if ((is_assign && (lkind != rkind)) || (lkind > rkind)) {
                return ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, right->base.loc, right, ASR::cast_kindType::IntegerToInteger,
                    left_type));
            }
        }
        return right;
    }

};

ASR::symbol_t* pass_instantiate_generic_function(Allocator &al, std::map<std::string, ASR::ttype_t*> subs,
        SymbolTable *current_scope, int new_function_num, ASR::TemplateFunction_t &func) {
    TemplateVisitor tv(al, subs, current_scope, new_function_num);
    tv.visit_TemplateFunction(func);
    return ASR::down_cast<ASR::symbol_t>(tv.new_function);
}

}