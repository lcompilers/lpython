#ifndef LCOMPILERS_PASS_MANAGER_H
#define LCOMPILERS_PASS_MANAGER_H

#include <libasr/asr.h>
#include <libasr/string_utils.h>
#include <libasr/alloc.h>

// TODO: Remove lpython/lfortran includes, make it compiler agnostic
#if __has_include(<lfortran/utils.h>)
    #include <lfortran/utils.h>
#endif

#if __has_include(<lpython/utils.h>)
    #include <lpython/utils.h>
#endif

#include <libasr/pass/do_loops.h>
#include <libasr/pass/for_all.h>
#include <libasr/pass/implied_do_loops.h>
#include <libasr/pass/array_op.h>
#include <libasr/pass/select_case.h>
#include <libasr/pass/global_stmts.h>
#include <libasr/pass/param_to_const.h>
#include <libasr/pass/print_arr.h>
#include <libasr/pass/arr_slice.h>
#include <libasr/pass/flip_sign.h>
#include <libasr/pass/div_to_mul.h>
#include <libasr/pass/fma.h>
#include <libasr/pass/loop_unroll.h>
#include <libasr/pass/sign_from_value.h>
#include <libasr/pass/class_constructor.h>
#include <libasr/pass/unused_functions.h>
#include <libasr/pass/inline_function_calls.h>
#include <libasr/pass/dead_code_removal.h>
#include <libasr/pass/for_all.h>
#include <libasr/pass/select_case.h>
#include <libasr/pass/loop_vectorise.h>
#include <libasr/pass/update_array_dim_intrinsic_calls.h>
#include <libasr/pass/pass_array_by_data.h>

#include <map>
#include <vector>

namespace LCompilers {

    enum ASRPass {
        do_loops, global_stmts, implied_do_loops, array_op,
        arr_slice, print_arr, class_constructor, unused_functions,
        flip_sign, div_to_mul, fma, sign_from_value,
        inline_function_calls, loop_unroll, dead_code_removal,
        forall, select_case, loop_vectorise,
        array_dim_intrinsics_update, pass_array_by_data
    };

    static inline void set_runtime_library_dir(PassOptions& pass_options) {
        pass_options.runtime_library_dir = LFortran::get_runtime_library_dir();
    }

    class PassManager {
        private:

        std::vector<ASRPass> _passes;
        std::vector<ASRPass> _with_optimization_passes;
        std::vector<ASRPass> _user_defined_passes;
        std::map<std::string, ASRPass> _passes_db = {
            {"do_loops", ASRPass::do_loops},
            {"global_stmts", ASRPass::global_stmts},
            {"implied_do_loops", ASRPass::implied_do_loops},
            {"array_op", ASRPass::array_op},
            {"arr_slice", ASRPass::arr_slice},
            {"print_arr", ASRPass::print_arr},
            {"class_constructor", ASRPass::class_constructor},
            {"unused_functions", ASRPass::unused_functions},
            {"flip_sign", ASRPass::flip_sign},
            {"div_to_mul", ASRPass::div_to_mul},
            {"fma", ASRPass::fma},
            {"sign_from_value", ASRPass::sign_from_value},
            {"inline_function_calls", ASRPass::inline_function_calls},
            {"loop_unroll", ASRPass::loop_unroll},
            {"dead_code_removal", ASRPass::dead_code_removal},
            {"forall", ASRPass::forall},
            {"select_case", ASRPass::select_case},
            {"loop_vectorise", ASRPass::loop_vectorise},
            {"array_dim_intrinsics_update", ASRPass::array_dim_intrinsics_update},
            {"pass_array_by_data", ASRPass::pass_array_by_data}
        };

        bool is_fast;
        bool apply_default_passes;

        void _apply_passes(Allocator& al, LFortran::ASR::TranslationUnit_t* asr,
                           std::vector<ASRPass>& passes, PassOptions pass_options) {
            set_runtime_library_dir(pass_options);
            for (size_t i = 0; i < passes.size(); i++) {
                switch (passes[i]) {
                    case (ASRPass::do_loops) : {
                        LFortran::pass_replace_do_loops(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::global_stmts) : {
                        LFortran::pass_wrap_global_stmts_into_function(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::implied_do_loops) : {
                        LFortran::pass_replace_implied_do_loops(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::array_op) : {
                        LFortran::pass_replace_array_op(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::flip_sign) : {
                        LFortran::pass_replace_flip_sign(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::fma) : {
                        LFortran::pass_replace_fma(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::loop_unroll) : {
                        LFortran::pass_loop_unroll(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::inline_function_calls) : {
                        LFortran::pass_inline_function_calls(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::dead_code_removal) : {
                        LFortran::pass_dead_code_removal(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::sign_from_value) : {
                        LFortran::pass_replace_sign_from_value(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::div_to_mul) : {
                        LFortran::pass_replace_div_to_mul(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::class_constructor) : {
                        LFortran::pass_replace_class_constructor(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::arr_slice) : {
                        LFortran::pass_replace_arr_slice(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::print_arr) : {
                        LFortran::pass_replace_print_arr(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::unused_functions) : {
                        LFortran::pass_unused_functions(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::forall) : {
                        LFortran::pass_replace_forall(al, *asr, pass_options);
                        break ;
                    }
                    case (ASRPass::select_case) : {
                        LFortran::pass_replace_select_case(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::loop_vectorise) : {
                        LFortran::pass_loop_vectorise(al, *asr, pass_options);
                        break;
                    }
                    case (ASRPass::array_dim_intrinsics_update): {
                        LFortran::pass_update_array_dim_intrinsic_calls(al, *asr, pass_options);
                        break ;
                    }
                    case (ASRPass::pass_array_by_data): {
                        LFortran::pass_array_by_data(al, *asr, pass_options);
                        break ;
                    }
                }
            }
        }

        public:

        PassManager(): is_fast{false}, apply_default_passes{false} {
            _passes = {
                ASRPass::global_stmts,
                ASRPass::class_constructor,
                ASRPass::implied_do_loops,
                ASRPass::pass_array_by_data,
                ASRPass::arr_slice,
                ASRPass::array_op,
                ASRPass::print_arr,
                ASRPass::array_dim_intrinsics_update,
                ASRPass::do_loops,
                ASRPass::forall,
                ASRPass::select_case,
                ASRPass::unused_functions
            };

            _with_optimization_passes = {
                ASRPass::global_stmts,
                ASRPass::class_constructor,
                ASRPass::implied_do_loops,
                ASRPass::pass_array_by_data,
                ASRPass::arr_slice,
                ASRPass::array_op,
                ASRPass::print_arr,
                ASRPass::loop_vectorise,
                ASRPass::loop_unroll,
                ASRPass::array_dim_intrinsics_update,
                ASRPass::do_loops,
                ASRPass::forall,
                ASRPass::dead_code_removal,
                ASRPass::select_case,
                ASRPass::unused_functions,
                ASRPass::flip_sign,
                ASRPass::sign_from_value,
                ASRPass::div_to_mul,
                ASRPass::fma,
                ASRPass::inline_function_calls
            };

            _user_defined_passes.clear();
        }

        void parse_pass_arg(std::string& arg_pass) {
            _user_defined_passes.clear();
            if (arg_pass == "") {
                return ;
            }

            std::string current_pass = "";
            for( size_t i = 0; i < arg_pass.size(); i++ ) {
                char ch = arg_pass[i];
                if (ch != ' ' && ch != ',') {
                    current_pass.push_back(ch);
                }
                if (ch == ',' || i == arg_pass.size() - 1) {
                    current_pass = LFortran::to_lower(current_pass);
                    if( _passes_db.find(current_pass) == _passes_db.end() ) {
                        std::cerr << current_pass << " isn't supported yet.";
                        std::cerr << " Only the following passes are supported:- "<<std::endl;
                        for( auto it: _passes_db ) {
                            std::cerr << it.first << std::endl;
                        }
                        exit(1);
                    }
                    _user_defined_passes.push_back(_passes_db[current_pass]);
                    current_pass.clear();
                }
            }
        }

        void apply_passes(Allocator& al, LFortran::ASR::TranslationUnit_t* asr,
                          PassOptions& pass_options) {
            if( !_user_defined_passes.empty() ) {
                _apply_passes(al, asr, _user_defined_passes, pass_options);
            } else if( apply_default_passes ) {
                if( is_fast ) {
                    _apply_passes(al, asr, _with_optimization_passes, pass_options);
                } else {
                    _apply_passes(al, asr, _passes, pass_options);
                }
            }
        }

        void use_optimization_passes() {
            is_fast = true;
        }

        void do_not_use_optimization_passes() {
            is_fast = false;
        }

        void use_default_passes() {
            apply_default_passes = true;
        }

        void do_not_use_default_passes() {
            apply_default_passes = false;
        }

    };

}
#endif
