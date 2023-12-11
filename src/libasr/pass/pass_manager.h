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

#include <libasr/pass/replace_do_loops.h>
#include <libasr/pass/replace_for_all.h>
#include <libasr/pass/replace_init_expr.h>
#include <libasr/pass/replace_implied_do_loops.h>
#include <libasr/pass/replace_array_op.h>
#include <libasr/pass/replace_select_case.h>
#include <libasr/pass/wrap_global_stmts.h>
#include <libasr/pass/replace_param_to_const.h>
#include <libasr/pass/replace_print_arr.h>
#include <libasr/pass/replace_where.h>
#include <libasr/pass/replace_print_list_tuple.h>
#include <libasr/pass/replace_arr_slice.h>
#include <libasr/pass/replace_flip_sign.h>
#include <libasr/pass/replace_div_to_mul.h>
#include <libasr/pass/replace_symbolic.h>
#include <libasr/pass/replace_intrinsic_function.h>
#include <libasr/pass/replace_fma.h>
#include <libasr/pass/loop_unroll.h>
#include <libasr/pass/replace_sign_from_value.h>
#include <libasr/pass/replace_class_constructor.h>
#include <libasr/pass/unused_functions.h>
#include <libasr/pass/inline_function_calls.h>
#include <libasr/pass/dead_code_removal.h>
#include <libasr/pass/replace_for_all.h>
#include <libasr/pass/replace_init_expr.h>
#include <libasr/pass/replace_select_case.h>
#include <libasr/pass/loop_vectorise.h>
#include <libasr/pass/update_array_dim_intrinsic_calls.h>
#include <libasr/pass/array_by_data.h>
#include <libasr/pass/list_expr.h>
#include <libasr/pass/create_subroutine_from_function.h>
#include <libasr/pass/transform_optional_argument_functions.h>
#include <libasr/pass/nested_vars.h>
#include <libasr/pass/unique_symbols.h>
#include <libasr/pass/insert_deallocate.h>
#include <libasr/pass/replace_print_struct_type.h>
#include <libasr/codegen/asr_to_fortran.h>
#include <libasr/asr_verify.h>
#include <libasr/pickle.h>

#include <map>
#include <vector>
#include <algorithm>
#include <fstream>

namespace LCompilers {

    typedef void (*pass_function)(Allocator&, ASR::TranslationUnit_t&,
                                  const LCompilers::PassOptions&);

    class PassManager {
        private:

        std::vector<std::string> _passes;
        std::vector<std::string> _with_optimization_passes;
        std::vector<std::string> _user_defined_passes;
        std::vector<std::string> _skip_passes, _c_skip_passes;
        std::map<std::string, pass_function> _passes_db = {
            {"do_loops", &pass_replace_do_loops},
            {"global_stmts", &pass_wrap_global_stmts},
            {"implied_do_loops", &pass_replace_implied_do_loops},
            {"array_op", &pass_replace_array_op},
            {"symbolic", &pass_replace_symbolic},
            {"intrinsic_function", &pass_replace_intrinsic_function},
            {"arr_slice", &pass_replace_arr_slice},
            {"print_arr", &pass_replace_print_arr},
            {"print_list_tuple", &pass_replace_print_list_tuple},
            {"class_constructor", &pass_replace_class_constructor},
            {"unused_functions", &pass_unused_functions},
            {"flip_sign", &pass_replace_flip_sign},
            {"div_to_mul", &pass_replace_div_to_mul},
            {"fma", &pass_replace_fma},
            {"sign_from_value", &pass_replace_sign_from_value},
            {"inline_function_calls", &pass_inline_function_calls},
            {"loop_unroll", &pass_loop_unroll},
            {"dead_code_removal", &pass_dead_code_removal},
            {"forall", &pass_replace_for_all},
            {"select_case", &pass_replace_select_case},
            {"loop_vectorise", &pass_loop_vectorise},
            {"array_dim_intrinsics_update", &pass_update_array_dim_intrinsic_calls},
            {"pass_list_expr", &pass_list_expr},
            {"pass_array_by_data", &pass_array_by_data},
            {"subroutine_from_function", &pass_create_subroutine_from_function},
            {"transform_optional_argument_functions", &pass_transform_optional_argument_functions},
            {"init_expr", &pass_replace_init_expr},
            {"nested_vars", &pass_nested_vars},
            {"where", &pass_replace_where},
            {"print_struct_type", &pass_replace_print_struct_type},
            {"unique_symbols", &pass_unique_symbols},
            {"insert_deallocate", &pass_insert_deallocate}
        };

        bool is_fast;
        bool apply_default_passes;
        bool c_skip_pass; // This will contain the passes that are to be skipped in C

        public:
        bool rtlib=false;

        void apply_passes(Allocator& al, ASR::TranslationUnit_t* asr,
                           std::vector<std::string>& passes, PassOptions &pass_options,
                           [[maybe_unused]] diag::Diagnostics &diagnostics) {
            if (pass_options.pass_cumulative) {
                int _pass_max_idx = -1, _opt_max_idx = -1;
                for (std::string &current_pass: passes) {
                    auto it1 = std::find(_passes.begin(), _passes.end(), current_pass);
                    if (it1 != _passes.end()) {
                        _pass_max_idx = std::max(_pass_max_idx,
                                            (int)(it1 - _passes.begin()));
                    }
                    auto it2 = std::find(_with_optimization_passes.begin(),
                                    _with_optimization_passes.end(), current_pass);
                    if (it2 != _with_optimization_passes.end()) {
                        _opt_max_idx = std::max(_opt_max_idx,
                                            (int)(it2 - _with_optimization_passes.begin()));
                    }
                }
                passes.clear();
                if (_pass_max_idx != -1) {
                    for (int i=0; i<=_pass_max_idx; i++)
                        passes.push_back(_passes[i]);
                }
                if (_opt_max_idx != -1) {
                    for (int i=0; i<=_opt_max_idx; i++)
                        passes.push_back(_with_optimization_passes[i]);
                }
            }
            for (size_t i = 0; i < passes.size(); i++) {
                // TODO: rework the whole pass manager: construct the passes
                // ahead of time (not at the last minute), and remove this much
                // earlier
                // Note: this is not enough for rtlib, we also need to include
                // it

                if (rtlib && passes[i] == "unused_functions") continue;
                if( std::find(_skip_passes.begin(), _skip_passes.end(), passes[i]) != _skip_passes.end())
                    continue;
                if (c_skip_pass && std::find(_c_skip_passes.begin(),
                        _c_skip_passes.end(), passes[i]) != _c_skip_passes.end())
                    continue;
                if (pass_options.verbose) {
                    std::cerr << "ASR Pass starts: '" << passes[i] << "'\n";
                }
                _passes_db[passes[i]](al, *asr, pass_options);
#if defined(WITH_LFORTRAN_ASSERT)
                if (!asr_verify(*asr, true, diagnostics)) {
                    std::cerr << diagnostics.render2();
                    throw LCompilersException("Verify failed in the pass: "
                        + passes[i]);
                };
#endif
                if (pass_options.verbose) {
                    std::cerr << "ASR Pass ends: '" << passes[i] << "'\n";
                }
            }
        }

        void _parse_pass_arg(std::string& arg, std::vector<std::string>& passes) {
            if (arg == "") return;

            std::string current_pass = "";
            for( size_t i = 0; i < arg.size(); i++ ) {
                char ch = arg[i];
                if (ch != ' ' && ch != ',') {
                    current_pass.push_back(ch);
                }
                if (ch == ',' || i == arg.size() - 1) {
                    current_pass = to_lower(current_pass);
                    if( _passes_db.find(current_pass) == _passes_db.end() ) {
                        std::cerr << current_pass << " isn't supported yet.";
                        std::cerr << " Only the following passes are supported:- "<<std::endl;
                        for( auto it: _passes_db ) {
                            std::cerr << it.first << std::endl;
                        }
                        exit(1);
                    }
                    passes.push_back(current_pass);
                    current_pass.clear();
                }
            }
        }

        PassManager(): is_fast{false}, apply_default_passes{false},
            c_skip_pass{false} {
            _passes = {
                "nested_vars",
                "global_stmts",
                "init_expr",
                "implied_do_loops",
                "class_constructor",
                "pass_list_expr",
                "where",
                "subroutine_from_function",
                "array_op",
                "symbolic",
                "intrinsic_function",
                "subroutine_from_function",
                "array_op",
                "pass_array_by_data",
                "print_struct_type",
                "print_arr",
                "print_list_tuple",
                "print_struct_type",
                "array_dim_intrinsics_update",
                "do_loops",
                "forall",
                "select_case",
                "inline_function_calls",
                "unused_functions",
                "transform_optional_argument_functions",
                "unique_symbols",
                "insert_deallocate"
            };

            _with_optimization_passes = {
                "nested_vars",
                "global_stmts",
                "init_expr",
                "implied_do_loops",
                "class_constructor",
                "pass_list_expr",
                "where",
                "subroutine_from_function",
                "array_op",
                "symbolic",
                "intrinsic_function",
                "subroutine_from_function",
                "array_op",
                "pass_array_by_data",
                "print_struct_type",
                "print_arr",
                "print_list_tuple",
                "print_struct_type",
                "loop_vectorise",
                "array_dim_intrinsics_update",
                "do_loops",
                "forall",
                "dead_code_removal",
                "select_case",
                "unused_functions",
                "flip_sign",
                "sign_from_value",
                "div_to_mul",
                "fma",
                "transform_optional_argument_functions",
                "inline_function_calls",
                "unique_symbols",
                "insert_deallocate"
            };

            // These are re-write passes which are already handled
            // appropriately in C backend.
            _c_skip_passes = {
                "pass_list_expr",
                "print_list_tuple",
                "do_loops",
                "select_case",
                "inline_function_calls"
            };
            _user_defined_passes.clear();
        }

        void parse_pass_arg(std::string& arg_pass, std::string& skip_pass) {
            _user_defined_passes.clear();
            _skip_passes.clear();
            _parse_pass_arg(arg_pass, _user_defined_passes);
            _parse_pass_arg(skip_pass, _skip_passes);
        }

        void apply_passes(Allocator& al, ASR::TranslationUnit_t* asr,
                          PassOptions& pass_options,
                          diag::Diagnostics &diagnostics) {
            if( !_user_defined_passes.empty() ) {
                pass_options.fast = true;
                apply_passes(al, asr, _user_defined_passes, pass_options,
                    diagnostics);
            } else if( apply_default_passes ) {
                pass_options.fast = is_fast;
                if( is_fast ) {
                    apply_passes(al, asr, _with_optimization_passes, pass_options,
                        diagnostics);
                } else {
                    apply_passes(al, asr, _passes, pass_options, diagnostics);
                }
            }
        }

        void dump_all_passes(Allocator& al, ASR::TranslationUnit_t* asr,
                           PassOptions &pass_options,
                           [[maybe_unused]] diag::Diagnostics &diagnostics, LocationManager &lm) {
            std::vector<std::string> passes;
            if (pass_options.fast) {
                passes = _with_optimization_passes;
            } else {
                passes = _passes;
            }
            for (size_t i = 0; i < passes.size(); i++) {
                // TODO: rework the whole pass manager: construct the passes
                // ahead of time (not at the last minute), and remove this much
                // earlier
                // Note: this is not enough for rtlib, we also need to include
                // it
                if (pass_options.verbose) {
                    std::cerr << "ASR Pass starts: '" << passes[i] << "'\n";
                }
                _passes_db[passes[i]](al, *asr, pass_options);
                if (pass_options.dump_all_passes) {
                    std::string str_i = std::to_string(i+1);
                    if ( i < 9 )  str_i = "0" + str_i;
                    if (pass_options.json) {
                        std::ofstream outfile ("pass_json_" + str_i + "_" + passes[i] + ".json");
                        outfile << pickle_json(*asr, lm, pass_options.no_loc, pass_options.with_intrinsic_mods) << "\n";
                        outfile.close();
                    }
                    if (pass_options.tree) {
                        std::ofstream outfile ("pass_tree_" + str_i + "_" + passes[i] + ".txt");
                        outfile << pickle_tree(*asr, false, pass_options.with_intrinsic_mods) << "\n";
                        outfile.close();
                    }
                    if (pass_options.visualize) {
                        std::string json = pickle_json(*asr, lm, pass_options.no_loc, pass_options.with_intrinsic_mods);
                        std::ofstream outfile ("pass_viz_" + str_i + "_" + passes[i] + ".html");
                        outfile << generate_visualize_html(json) << "\n";
                        outfile.close();
                    }
                    std::ofstream outfile ("pass_" + str_i + "_" + passes[i] + ".clj");
                    outfile << ";; ASR after applying the pass: " << passes[i]
                        << "\n" << pickle(*asr, false, true, pass_options.with_intrinsic_mods) << "\n";
                    outfile.close();
                }
                if (pass_options.dump_fortran) {
                    LCompilers::Result<std::string> fortran_code = asr_to_fortran(*asr, diagnostics, false, 4);
                    if (!fortran_code.ok) {
                        LCOMPILERS_ASSERT(diagnostics.has_error());
                        throw LCompilersException("Fortran code could not be generated after pass: "
                        + passes[i]);
                    }
                    std::string str_i = std::to_string(i+1);
                    if ( i < 9 )  str_i = "0" + str_i;
                    std::ofstream outfile ("pass_fortran_" + str_i + "_" + passes[i] + ".f90");
                    outfile << "! Fortran code after applying the pass: " << passes[i]
                        << "\n" << fortran_code.result << "\n";
                    outfile.close();
                }
#if defined(WITH_LFORTRAN_ASSERT)
                if (!asr_verify(*asr, true, diagnostics)) {
                    std::cerr << diagnostics.render2();
                    throw LCompilersException("Verify failed in the pass: "
                        + passes[i]);
                };
#endif
                if (pass_options.verbose) {
                    std::cerr << "ASR Pass ends: '" << passes[i] << "'\n";
                }
            }
        }

        void use_optimization_passes() {
            is_fast = true;
        }

        void do_not_use_optimization_passes() {
            is_fast = false;
        }

        void use_default_passes(bool _c_skip_pass=false) {
            apply_default_passes = true;
            c_skip_pass = _c_skip_pass;
        }

        void skip_c_passes() {
            c_skip_pass = true;
        }

        void do_not_use_default_passes() {
            apply_default_passes = false;
        }

        void use_fortran_passes() {
            _user_defined_passes.push_back("unique_symbols");
        }
    };

}
#endif
