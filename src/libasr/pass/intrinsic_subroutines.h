#ifndef LIBASR_PASS_INTRINSIC_SUBROUTINES_H
#define LIBASR_PASS_INTRINSIC_SUBROUTINES_H


#include <libasr/asr_builder.h>
#include <libasr/casting_utils.h>

namespace LCompilers::ASRUtils {

/*
To add a new subroutine implementation,

1. Create a new namespace like, `Sin`, `LogGamma` in this file.
2. In the above created namespace add `eval_*`, `instantiate_*`, and `create_*`.
3. Then register in the maps present in `IntrinsicImpureSubroutineRegistry`.

You can use helper macros and define your own helper macros to reduce
the code size.
*/

enum class IntrinsicImpureSubroutines : int64_t {
    RandomNumber,
    RandomInit,
    RandomSeed,
    GetCommand,
    GetEnvironmentVariable,
    ExecuteCommandLine,
    GetCommandArgument,
    CpuTime,
    Srand,
    SystemClock,
    DateAndTime,
    // ...
};

typedef ASR::stmt_t* (*impl_subroutine)(
    Allocator&, const Location &,
    SymbolTable*, Vec<ASR::ttype_t*>&,
    Vec<ASR::call_arg_t>&, int64_t);

typedef ASR::asr_t* (*create_intrinsic_subroutine)(
    Allocator&, const Location&,
    Vec<ASR::expr_t*>&,
    diag::Diagnostics&);

typedef void (*verify_subroutine)(
    const ASR::IntrinsicImpureSubroutine_t&,
    diag::Diagnostics&);

typedef ASR::expr_t* (*get_initial_value_sub)(Allocator&, ASR::ttype_t*);

namespace RandomInit {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2) {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for random_init expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASRUtils::require_impl(ASRUtils::is_logical(*ASRUtils::expr_type(x.m_args[0])), "First argument must be of logical type", x.base.base.loc, diagnostics);
            ASRUtils::require_impl(ASRUtils::is_logical(*ASRUtils::expr_type(x.m_args[1])), "Second argument must be of logical type", x.base.base.loc, diagnostics);
        } else {
            ASRUtils::require_impl(false, "Unexpected number of args, random_init takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_RandomInit(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::RandomInit), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_RandomInit(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        
        std::string c_func_name = "_lfortran_random_init";
        std::string new_name = "_lcompilers_random_init_";

        declare_basic_variables(new_name);
        fill_func_arg_sub("repeatable", arg_types[0], InOut);
        fill_func_arg_sub("image_distinct", arg_types[1], InOut);
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1; args_1.reserve(al, 0);
        ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
           ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[0])),
           ASRUtils::intent_return_var, ASR::abiType::BindC, false);
        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));
        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 0);
        body.push_back(al, b.Assignment(args[0], b.Call(s, call_args, arg_types[0])));
        body.push_back(al, b.Assignment(args[1], b.Call(s, call_args, arg_types[1])));
        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }
} // namespace RandomInit

namespace RandomSeed {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args <= 3, "random_seed can have maximum 3 args", x.base.base.loc, diagnostics);
        if (x.n_args == 1) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[0])), "Arguments to random_seed must be of integer type", x.base.base.loc, diagnostics);
        } else if (x.n_args == 2) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[0])) && ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[0])), "Arguments to random_seed must be of integer type", x.base.base.loc, diagnostics);
        } else if (x.n_args == 3) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[0])) && ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[1])) && ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[2])), "Arguments to random_seed must be of integer type", x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_RandomSeed(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 0);
        ASRBuilder b(al, loc);
        for (int i = 0; i < int(args.size()); i++) {
            if(args[i]) {
                m_args.push_back(al, args[i]);
            } else {
                m_args.push_back(al, b.f32(1));
            }
        }
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::RandomSeed), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_RandomSeed(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {

        std::string c_func_name_1 = "_lfortran_random_seed";
        std::string c_func_name_2 = "_lfortran_dp_rand_num";
        std::string new_name = "_lcompilers_random_seed_";
        declare_basic_variables(new_name);
        int flag = 0;
        if (!is_real(*arg_types[0])) {
            fill_func_arg_sub("size", arg_types[0], InOut);
            ASR::symbol_t *s_1 = b.create_c_func_subroutines(c_func_name_1, fn_symtab, 1, arg_types[0]);
            fn_symtab->add_symbol(c_func_name_1, s_1);
            dep.push_back(al, s2c(al, c_func_name_1));
            Vec<ASR::expr_t*> call_args; call_args.reserve(al, 1);
            call_args.push_back(al, b.i32(8));
            body.push_back(al, b.Assignment(args[0], b.Call(s_1, call_args, arg_types[0])));
        } else {
            fill_func_arg_sub("size", real32, InOut);
            body.push_back(al, b.Assignment(args[0], b.f32(0)));
        }
        if (!is_real(*arg_types[1])) {
            flag = 1;
            fill_func_arg_sub("put", arg_types[1], InOut);
            body.push_back(al, b.Assignment(args[1], args[1]));
        } else {
            fill_func_arg_sub("put", real32, InOut);
            body.push_back(al, b.Assignment(args[1], b.f32(0)));
        }
        if (!is_real(*arg_types[2])) {
            fill_func_arg_sub("get", arg_types[2], InOut);
            if (flag == 1) {
                body.push_back(al, b.Assignment(args[2], args[1]));
            } else {
                std::vector<LCompilers::ASR::expr_t *> vals;
                std::string c_func = c_func_name_2;
                int kind = ASRUtils::extract_kind_from_ttype_t(arg_types[2]);
                if ( is_real(*arg_types[2]) ) {
                    if (kind == 4) {
                        c_func = "_lfortran_sp_rand_num";
                    } else {
                        c_func = "_lfortran_dp_rand_num";
                    }
                } else if ( is_integer(*arg_types[2]) ) {
                    if (kind == 4) {
                        c_func = "_lfortran_int32_rand_num";
                    } else {
                        c_func = "_lfortran_int64_rand_num";
                    }
                }
                ASR::symbol_t *s_2 = b.create_c_func_subroutines(c_func, fn_symtab, 0, arg_types[2]);
                fn_symtab->add_symbol(c_func, s_2);
                dep.push_back(al, s2c(al, c_func));
                Vec<ASR::expr_t*> call_args2; call_args2.reserve(al, 0);
                ASR::ttype_t* elem_type = extract_type(arg_types[2]);
                for (int i = 0; i < 8; i++) {
                    auto xx = declare("i_" + std::to_string(i), elem_type, Local);
                    body.push_back(al, b.Assignment(xx, b.Call(s_2, call_args2, int32)));
                    vals.push_back(xx);
                }
                body.push_back(al, b.Assignment(args[2], b.ArrayConstant(vals, extract_type(arg_types[2]), false)));
            }
        } else {
            fill_func_arg_sub("get", real32, InOut);
            body.push_back(al, b.Assignment(args[2], b.f32(0)));
        }
       
        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }

} // namespace RandomSeed

namespace Srand {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1, "srand takes 1 argument only", x.base.base.loc, diagnostics);
        if (x.n_args == 1) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[0])), "Arguments to srand must be of integer type", x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Srand(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        diag.semantic_warning_label(
                "`srand` is an LFortran extension", { loc }, "Use `random_init` instead");
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1); m_args.push_back(al, args[0]);
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::Srand), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_Srand(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {

        std::string c_func_name = "_lfortran_init_random_seed";
        std::string new_name = "_lcompilers_srand_";
        declare_basic_variables(new_name);
        fill_func_arg_sub("r", arg_types[0], InOut);
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1; args_1.reserve(al, 1);
        ASR::expr_t *arg = b.Variable(fn_symtab_1, "n", arg_types[0],
            ASR::intentType::In, ASR::abiType::BindC, true);
        args_1.push_back(al, arg);

        ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
           ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[0])),
           ASRUtils::intent_return_var, ASR::abiType::BindC, false);
           
        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));

        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 1);
        call_args.push_back(al, args[0]);
        body.push_back(al, b.Assignment(args[0], b.Call(s, call_args, arg_types[0])));
        
        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }

} // namespace Srand

namespace RandomNumber {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for random_number expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, random_number takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_RandomNumber(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1); m_args.push_back(al, args[0]);
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::RandomNumber), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_RandomNumber(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string c_func_name;
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_types[0]);
        if ( kind == 4 ) {
            c_func_name = "_lfortran_sp_rand_num";
        } else {
            c_func_name = "_lfortran_dp_rand_num";
        }
        std::string new_name = "_lcompilers_random_number_";

        declare_basic_variables(new_name);
        fill_func_arg_sub("r", ASRUtils::duplicate_type_with_empty_dims(al, arg_types[0]), InOut);
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1; args_1.reserve(al, 0);
        ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
           ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[0])),
           ASRUtils::intent_return_var, ASR::abiType::BindC, false);
        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));

        if (ASRUtils::is_array(ASRUtils::expr_type(args[0]))) {
            /*
                real :: b(3)
                call random_number(b)
                    To
                real :: b(3)
                do i=lbound(b,1),ubound(b,1)
                    call random_number(b(i))
                end do
            */
            ASR::dimension_t* array_dims = nullptr;
            int array_rank = extract_dimensions_from_ttype(arg_types[0], array_dims);
            std::vector<ASR::expr_t*> do_loop_variables;
            for (int i = 0; i < array_rank; i++) {
                do_loop_variables.push_back(declare("i_" + std::to_string(i), int32, Local));
            }
            ASR::stmt_t* func_call = b.CallIntrinsicSubroutine(scope, {ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[0]))},
                                    {b.ArrayItem_01(args[0], do_loop_variables)}, 0, RandomNumber::instantiate_RandomNumber);
            fn_name = scope->get_unique_name(fn_name, false);
            body.push_back(al, PassUtils::create_do_loop_helper_random_number(al, loc, do_loop_variables, s, args[0],
                    ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[0])),
                    b.ArrayItem_01(args[0], do_loop_variables), func_call, 1));
        } else {
            Vec<ASR::expr_t*> call_args; call_args.reserve(al, 0);
            body.push_back(al, b.Assignment(args[0], b.Call(s, call_args, arg_types[0])));
        }
        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }

} // namespace RandomNumber

namespace GetCommand {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for get_command expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
        if( x.n_args > 0 ) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[0])), 
                            "First argument must be of character type", x.base.base.loc, diagnostics);
        }
        if( x.n_args > 1 ) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[1])), 
                            "Second argument must be of integer type", x.base.base.loc, diagnostics);
        }
        if( x.n_args > 2 ) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[2])), 
                            "Third argument must be of integer type", x.base.base.loc, diagnostics);
        }
        if( x.n_args > 3 ) {
            ASRUtils::require_impl(false, "Unexpected number of args, get_command takes 3 arguments, found " + 
                            std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_GetCommand(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 3);
        if(args[0]) m_args.push_back(al, args[0]);
        if(args[1]) m_args.push_back(al, args[1]);
        if(args[2]) m_args.push_back(al, args[2]);
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::GetCommand), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_GetCommand(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        
        std::string c_func_name_1 = "_lfortran_get_command_command";
        std::string c_func_name_2 = "_lfortran_get_command_length";
        std::string c_func_name_3 = "_lfortran_get_command_status";

        std::string new_name = "_lcompilers_get_command_";
        declare_basic_variables(new_name);
        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 0);

        if(arg_types.size() > 0){
            fill_func_arg_sub("command", arg_types[0], InOut);
            ASR::symbol_t *s_1 = b.create_c_func_subroutines(c_func_name_1, fn_symtab, 0, arg_types[0]);
            fn_symtab->add_symbol(c_func_name_1, s_1);
            dep.push_back(al, s2c(al, c_func_name_1));
            body.push_back(al, b.Assignment(args[0], b.Call(s_1, call_args, arg_types[0])));
        }
        if(arg_types.size() > 1){
            fill_func_arg_sub("length", arg_types[1], InOut);
            ASR::symbol_t *s_2 = b.create_c_func_subroutines(c_func_name_2, fn_symtab, 0, arg_types[1]);
            fn_symtab->add_symbol(c_func_name_2, s_2);
            dep.push_back(al, s2c(al, c_func_name_2));
            body.push_back(al, b.Assignment(args[1], b.Call(s_2, call_args, arg_types[1])));
        }
        if(arg_types.size() > 2){
            fill_func_arg_sub("status", arg_types[2], InOut);
            ASR::symbol_t *s_3 = b.create_c_func_subroutines(c_func_name_3, fn_symtab, 0, arg_types[2]);
            fn_symtab->add_symbol(c_func_name_3, s_3);
            dep.push_back(al, s2c(al, c_func_name_3));
            body.push_back(al, b.Assignment(args[2], b.Call(s_3, call_args, arg_types[2])));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }
    
} // namespace GetCommand

namespace GetCommandArgument {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args > 0) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[0])), "First argument must be of integer type", x.base.base.loc, diagnostics);
        } 
        if(x.n_args > 1) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[1])), "Second argument must be of character type", x.base.base.loc, diagnostics);
        }
        if (x.n_args > 2) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[2])), "Third argument must be of integer type", x.base.base.loc, diagnostics);
        } 
        if (x.n_args == 4) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[3])), "Fourth argument must be of integer type", x.base.base.loc, diagnostics);
        } else {
            ASRUtils::require_impl(false, "Unexpected number of args, get_command_argument takes atmost 4 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_GetCommandArgument(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, args.size());
        m_args.push_back(al, args[0]);
        for (int i = 1; i < int(args.size()); i++) {
            if(args[i]) m_args.push_back(al, args[i]);
        }
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::GetCommandArgument), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_GetCommandArgument(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        
        std::string c_func_name_1 = "_lfortran_get_command_argument_value";
        std::string c_func_name_2 = "_lfortran_get_command_argument_length";
        std::string c_func_name_3 = "_lfortran_get_command_argument_status";

        std::string new_name = "_lcompilers_get_command_argument_";
        declare_basic_variables(new_name);
        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 0);
        fill_func_arg_sub("number", arg_types[0], In);
        if (arg_types.size() > 1) {
            fill_func_arg_sub("value", arg_types[1], InOut);
            Vec<ASR::expr_t*> args_1; args_1.reserve(al, 0);
            SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
            ASR::expr_t *arg = b.Variable(fn_symtab_1, "n", arg_types[0],
                ASR::intentType::In, ASR::abiType::BindC, true);
            args_1.push_back(al, arg);
            SetChar dep_1; dep_1.reserve(al, 1);
            Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
            ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name_1,
            ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[1])),
            ASRUtils::intent_return_var, ASR::abiType::BindC, false);
            ASR::symbol_t *s_1 = make_ASR_Function_t(c_func_name_1, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name_1));
            
            fn_symtab->add_symbol(c_func_name_1, s_1);
            dep.push_back(al, s2c(al, c_func_name_1));
            Vec<ASR::expr_t*> call_args1; call_args1.reserve(al, 1);
            call_args1.push_back(al, args[0]);
            body.push_back(al, b.Assignment(args[1], b.Call(s_1, call_args1, arg_types[1])));
        }
        if (arg_types.size() > 2) {
            fill_func_arg_sub("length", arg_types[2], InOut);
            
            ASR::symbol_t *s_2 = b.create_c_func_subroutines(c_func_name_2, fn_symtab, 1, arg_types[2]);
            fn_symtab->add_symbol(c_func_name_2, s_2);
            dep.push_back(al, s2c(al, c_func_name_2));
            Vec<ASR::expr_t*> call_args2; call_args2.reserve(al, 1);
            call_args2.push_back(al, args[0]);
            body.push_back(al, b.Assignment(args[2], b.Call(s_2, call_args2, arg_types[2])));
        }
        if (arg_types.size() == 4) {
            fill_func_arg_sub("status", arg_types[3], InOut);
            ASR::symbol_t *s_3 = b.create_c_func_subroutines(c_func_name_3, fn_symtab, 0, arg_types[3]);
            fn_symtab->add_symbol(c_func_name_3, s_3);
            dep.push_back(al, s2c(al, c_func_name_3));
            body.push_back(al, b.Assignment(args[3], b.Call(s_3, call_args, arg_types[3])));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }
    
} // namespace GetCommandArgument

namespace SystemClock {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args > 0) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[0])), "`count` argument must be of integer type", x.base.base.loc, diagnostics);
        }
        if (x.n_args > 1) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[1])) || ASRUtils::is_real(*ASRUtils::expr_type(x.m_args[1])), "`count_rate` argument must be of integer or real type", x.base.base.loc, diagnostics);
        }
        if (x.n_args > 2) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[2])), "`count_max` argument must be of integer type", x.base.base.loc, diagnostics);
        } 
        if (x.n_args > 3) {
            ASRUtils::require_impl(false, "Unexpected number of args, system_clock takes atmost 3 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_SystemClock(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        int64_t count_id = 0, count_rate_id = 1, count_max_id = 2, count_count_rate_id = 3, count_count_max_id = 4, count_rate_count_max_id = 5, count_count_rate_count_max_id = 6;
        int64_t overload_id = -1;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, args.size());
        ASRBuilder b(al, loc);
        if(args[0]) overload_id = count_id;
        if(args[1]) overload_id = count_rate_id;
        if(args[2]) overload_id = count_max_id;
        if(args[0] && args[1]) overload_id = count_count_rate_id;
        if(args[0] && args[2]) overload_id = count_count_max_id;
        if(args[1] && args[2]) overload_id = count_rate_count_max_id;
        if(args[0] && args[1] && args[2]) overload_id = count_count_rate_count_max_id;
        for (int i = 0; i < int(args.size()); i++) {
            if(args[i]) m_args.push_back(al, args[i]);
            else m_args.push_back(al, b.i32(1));
        }
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::SystemClock), m_args.p, m_args.n, overload_id);
    }

    static inline ASR::stmt_t* instantiate_SystemClock(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t overload_id) {

        std::string c_func_name_1 = "_lfortran_i32sys_clock_count";
        std::string c_func_name_2 = "_lfortran_i32sys_clock_count_rate";
        std::string c_func_name_3 = "_lfortran_i32sys_clock_count_max";
        std::string new_name = "_lcompilers_system_clock_";
        declare_basic_variables(new_name);
        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 0);
        if (overload_id == 0 || overload_id == 3 || overload_id == 4 || overload_id == 6) {
            if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) == 8) {
                c_func_name_1 = "_lfortran_i64sys_clock_count";
            }
            fill_func_arg_sub("count", arg_types[0], InOut);
            ASR::symbol_t *s_1 = b.create_c_func_subroutines(c_func_name_1, fn_symtab, 0, arg_types[0]);
            fn_symtab->add_symbol(c_func_name_1, s_1);
            dep.push_back(al, s2c(al, c_func_name_1));
            body.push_back(al, b.Assignment(args[0], b.Call(s_1, call_args, arg_types[0])));
        } else {
            fill_func_arg_sub("count", int32, InOut);
            body.push_back(al, b.Assignment(args[0], b.i32(0)));
        }
        if (overload_id == 1 || overload_id == 3 || overload_id == 5 || overload_id == 6) {
            if (ASRUtils::extract_kind_from_ttype_t(arg_types[1]) == 8) {
                if (is_real(*arg_types[1])) {
                    c_func_name_2 = "_lfortran_i64r64sys_clock_count_rate";
                } else {
                    c_func_name_2 = "_lfortran_i64sys_clock_count_rate";
                }
            } else if (is_real(*arg_types[1])) {
                c_func_name_2 = "_lfortran_i32r32sys_clock_count_rate";
            }
            fill_func_arg_sub("count_rate", arg_types[1], InOut);
            ASR::symbol_t *s_2 = b.create_c_func_subroutines(c_func_name_2, fn_symtab, 0, arg_types[1]);
            fn_symtab->add_symbol(c_func_name_2, s_2);
            dep.push_back(al, s2c(al, c_func_name_2));
            body.push_back(al, b.Assignment(args[1], b.Call(s_2, call_args, arg_types[1])));
        } else {
            fill_func_arg_sub("count_rate", int32, InOut);
            body.push_back(al, b.Assignment(args[1], b.i32(0)));
        }
        if (overload_id == 2 || overload_id == 4 || overload_id == 5 || overload_id == 6) {
            if (ASRUtils::extract_kind_from_ttype_t(arg_types[2]) == 8) {
                c_func_name_3 = "_lfortran_i64sys_clock_count_max";
            }
            fill_func_arg_sub("count_max", arg_types[2], InOut);
            ASR::symbol_t *s_3 = b.create_c_func_subroutines(c_func_name_3, fn_symtab, 0, arg_types[2]);
            fn_symtab->add_symbol(c_func_name_3, s_3);
            dep.push_back(al, s2c(al, c_func_name_3));
            body.push_back(al, b.Assignment(args[2], b.Call(s_3, call_args, arg_types[2])));
        } else {
            fill_func_arg_sub("count_max", int32, InOut);
            body.push_back(al, b.Assignment(args[2], b.i32(0)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }
    
} // namespace SystemClock

namespace DateAndTime {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args > 0) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[0])), "`date` argument must be of character type", x.base.base.loc, diagnostics);
        }
        if (x.n_args > 1) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[1])), "`time` argument must be of character or real type", x.base.base.loc, diagnostics);
        }
        if (x.n_args > 2) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[2])), "`zone` argument must be of character type", x.base.base.loc, diagnostics);
        } 
        if (x.n_args > 3) {
            ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(x.m_args[3])), "`values` argument must be of integer array type", x.base.base.loc, diagnostics);
        } 
        if (x.n_args > 4) {
            ASRUtils::require_impl(false, "Unexpected number of args, date_and_time takes atmost 4 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_DateAndTime(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, args.size());
        ASRBuilder b(al, loc);
        for (int i = 0; i < int(args.size()); i++) {
            if(args[i]) {
                m_args.push_back(al, args[i]);
            } else {
                m_args.push_back(al, b.f32(1));
            }
        }
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::DateAndTime), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_DateAndTime(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {

        std::string c_func_name_1 = "_lfortran_date";
        std::string c_func_name_2 = "_lfortran_time";
        std::string c_func_name_3 = "_lfortran_zone";
        std::string c_func_name_4 = "_lfortran_values";
        std::string new_name = "_lcompilers_date_and_time_";
        declare_basic_variables(new_name);
        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 0);

        if (!is_real(*arg_types[0])) {
            fill_func_arg_sub("date", arg_types[0], InOut);
            ASR::symbol_t *s_1 = b.create_c_func_subroutines(c_func_name_1, fn_symtab, 0, arg_types[0]);
            fn_symtab->add_symbol(c_func_name_1, s_1);
            dep.push_back(al, s2c(al, c_func_name_1));
            body.push_back(al, b.Assignment(args[0], b.Call(s_1, call_args, arg_types[0])));
        } else {
            fill_func_arg_sub("date", real32, InOut);
            body.push_back(al, b.Assignment(args[0], b.f32(0)));
        }
        if (!is_real(*arg_types[1])) {
            fill_func_arg_sub("time", arg_types[1], InOut);
            ASR::symbol_t *s_2 = b.create_c_func_subroutines(c_func_name_2, fn_symtab, 0, arg_types[1]);
            fn_symtab->add_symbol(c_func_name_2, s_2);
            dep.push_back(al, s2c(al, c_func_name_2));
            body.push_back(al, b.Assignment(args[1], b.Call(s_2, call_args, arg_types[1])));
        }  else {
            fill_func_arg_sub("time", real32, InOut);
            body.push_back(al, b.Assignment(args[1], b.f32(0)));
        }
        if (!is_real(*arg_types[2])) {
            fill_func_arg_sub("zone", arg_types[2], InOut);
            ASR::symbol_t *s_3 = b.create_c_func_subroutines(c_func_name_3, fn_symtab, 0, arg_types[2]);
            fn_symtab->add_symbol(c_func_name_3, s_3);
            dep.push_back(al, s2c(al, c_func_name_3));
            body.push_back(al, b.Assignment(args[2], b.Call(s_3, call_args, arg_types[2])));
        } else {
            fill_func_arg_sub("zone", real32, InOut);
            body.push_back(al, b.Assignment(args[2], b.f32(0)));
        }
        if (!is_real(*arg_types[3])) {
            fill_func_arg_sub("values", arg_types[3], InOut);
            std::vector<LCompilers::ASR::expr_t *> vals;
            ASR::symbol_t *s_4 = b.create_c_func_subroutines(c_func_name_4, fn_symtab, 1, int32);
            fn_symtab->add_symbol(c_func_name_4, s_4);
            dep.push_back(al, s2c(al, c_func_name_4));
            for (int i = 0; i < 8; i++) {
                Vec<ASR::expr_t*> call_args2; call_args2.reserve(al, 1);
                call_args2.push_back(al, b.i32(i+1));
                auto xx = declare("i_" + std::to_string(i), int32, Local);
                body.push_back(al, b.Assignment(xx, b.Call(s_4, call_args2, int32)));
                vals.push_back(xx);
            }
            body.push_back(al, b.Assignment(args[3], b.ArrayConstant(vals, extract_type(arg_types[3]), false)));
        } else {
            fill_func_arg_sub("values", real32, InOut);
            body.push_back(al, b.Assignment(args[3], b.f32(0)));
        }
        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }
    
} // namespace DateAndTime

namespace GetEnvironmentVariable {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[0])), "First argument must be of character type", x.base.base.loc, diagnostics);
        } else if (x.n_args == 2) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[0])) && ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[1])), "First two arguments of `get_environment_variable` must be of character type", x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_GetEnvironmentVariable(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, args.size());
        m_args.push_back(al, args[0]);
        for (int i = 1; i < int(args.size()); i++) {
            if(args[i]) m_args.push_back(al, args[i]);
        }
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::GetEnvironmentVariable), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_GetEnvironmentVariable(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
                
        std::string c_func_name = "_lfortran_get_environment_variable";
        std::string new_name = "_lcompilers_get_environment_variable_";
        declare_basic_variables(new_name);
        fill_func_arg_sub("name", arg_types[0], InOut);
        fill_func_arg_sub("value", arg_types[1], InOut);
        if (arg_types.size() == 3) {
            fill_func_arg_sub("length", arg_types[2], InOut);
        }
        if (arg_types.size() == 4) {
            fill_func_arg_sub("status", arg_types[3], InOut);
        }
        if (arg_types.size() == 5) {
            fill_func_arg_sub("trim_name", arg_types[4], InOut);
        }
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);

        Vec<ASR::expr_t*> args_1; args_1.reserve(al, 1);
        ASR::expr_t *arg = b.Variable(fn_symtab_1, "n", arg_types[0],
            ASR::intentType::InOut, ASR::abiType::BindC, true);
        args_1.push_back(al, arg);

        ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
           ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[1])),
           ASRUtils::intent_return_var, ASR::abiType::BindC, false);
           
        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));

        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 1);
        call_args.push_back(al, args[0]);
        body.push_back(al, b.Assignment(args[1], b.Call(s, call_args, arg_types[1])));
        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }

} // namespace GetEnvironmentVariable

namespace ExecuteCommandLine {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1) {
            ASRUtils::require_impl(ASRUtils::is_character(*ASRUtils::expr_type(x.m_args[0])), "First argument must be of character type", x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_ExecuteCommandLine(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, args.size());
        m_args.push_back(al, args[0]);
        for (int i = 1; i < int(args.size()); i++) {
            if(args[i]) m_args.push_back(al, args[i]);
        }
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::ExecuteCommandLine), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_ExecuteCommandLine(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {

        std::string c_func_name = "_lfortran_exec_command";
        std::string new_name = "_lcompilers_execute_command_line_";
        ASR::symbol_t* s = scope->get_symbol(new_name);
        if (s) {
            ASRBuilder b(al, loc);
            return b.SubroutineCall(s, new_args);
        } else {
            declare_basic_variables(new_name);
            fill_func_arg_sub("command", arg_types[0], InOut);
            if (arg_types.size() == 2) {
                fill_func_arg_sub("wait", arg_types[1], InOut);
            } else if (arg_types.size() == 3) {
                fill_func_arg_sub("exitstat", arg_types[2], InOut);
            } else if (arg_types.size() == 4) {
                fill_func_arg_sub("cmdstat", arg_types[3], InOut);
            } else if (arg_types.size() == 5) {
                fill_func_arg_sub("cmdmsg", arg_types[4], InOut);
            }

            SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
            Vec<ASR::expr_t*> args_1; args_1.reserve(al, 1);
            ASR::expr_t *arg = b.Variable(fn_symtab_1, "n", arg_types[0],
                ASR::intentType::InOut, ASR::abiType::BindC, true);
            args_1.push_back(al, arg);

            ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
            ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types[0])),
            ASRUtils::intent_return_var, ASR::abiType::BindC, false);

            SetChar dep_1; dep_1.reserve(al, 1);
            Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
            ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
                body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));

            Vec<ASR::expr_t*> call_args; call_args.reserve(al, 1);
            call_args.push_back(al, args[0]);
            body.push_back(al, b.Assignment(args[0], b.Call(s, call_args, arg_types[0])));
            ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, nullptr, ASR::abiType::Intrinsic, ASR::deftypeType::Implementation, nullptr);
            scope->add_symbol(fn_name, new_symbol);
            return b.SubroutineCall(new_symbol, new_args);
        }
    }

} // namespace ExecuteCommandLine

namespace CpuTime {

    static inline void verify_args(const ASR::IntrinsicImpureSubroutine_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1) {
            ASRUtils::require_impl(ASRUtils::is_real(*ASRUtils::expr_type(x.m_args[0])), "First argument must be of real type", x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_CpuTime(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1); m_args.push_back(al, args[0]);
        return ASR::make_IntrinsicImpureSubroutine_t(al, loc, static_cast<int64_t>(IntrinsicImpureSubroutines::CpuTime), m_args.p, m_args.n, 0);
    }

    static inline ASR::stmt_t* instantiate_CpuTime(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
                
        std::string c_func_name;
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) == 4) {
            c_func_name = "_lfortran_s_cpu_time";
        } else {
            c_func_name = "_lfortran_d_cpu_time";
        }
        std::string new_name = "_lcompilers_cpu_time_" + type_to_str_python(arg_types[0]);
        declare_basic_variables(new_name);
        fill_func_arg_sub("time", arg_types[0], InOut);

        ASR::symbol_t *s = b.create_c_func_subroutines(c_func_name, fn_symtab, 0, arg_types[0]);
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));

        Vec<ASR::expr_t*> call_args; call_args.reserve(al, 0);
        body.push_back(al, b.Assignment(args[0], b.Call(s, call_args, arg_types[0])));
        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.SubroutineCall(new_symbol, new_args);
    }

} // namespace CpuTime

} // namespace LCompilers::ASRUtils

#endif // LIBASR_PASS_INTRINSIC_SUBROUTINES_H
