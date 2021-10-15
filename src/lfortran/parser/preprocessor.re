#include <iostream>
#include <map>

#include <lfortran/parser/preprocessor.h>
#include <lfortran/assert.h>
#include <lfortran/utils.h>
#include <lfortran/string_utils.h>

namespace LFortran
{

CPreprocessor::CPreprocessor(CompilerOptions &compiler_options)
    : compiler_options{compiler_options} {
    CPPMacro md;
    md.expansion = "1";
    macro_definitions["__LFORTRAN__"] = md;
    md.expansion = "\"" + std::string(LFORTRAN_VERSION) + "\"";
    macro_definitions["__VERSION__"] = md;
    if (compiler_options.platform == Platform::Windows) {
        md.expansion = "1";
        macro_definitions["_WIN32"] = md;
    }
    md.expansion = "\"\"";
    macro_definitions["__FILE__"] = md;
    md.expansion = "0";
    macro_definitions["__LINE__"] = md;
}
std::string CPreprocessor::token(unsigned char *tok, unsigned char* cur) const
{
    return std::string((char *)tok, cur - tok);
}

std::string parse_continuation_lines(unsigned char *&cur) {
    std::string output;
    while (*cur != '\n') {
        output += *cur;
        cur++;
    }
    cur++;
    if (output.size() > 0 && output[output.size()-1] == '\\') {
        output = output.substr(0, output.size()-1);
        output += parse_continuation_lines(cur);
    }
    return output;
}

void handle_continuation_lines(std::string &s, unsigned char *&cur) {
    if (s.size() > 0 && s[s.size()-1] == '\\') {
        s = s.substr(0, s.size()-1);
        s += parse_continuation_lines(cur);
    }
}

std::string parse_argument(unsigned char *&cur) {
    std::string arg;
    while (*cur == ' ') cur++;
    while (*cur != ')' && *cur != ',' && *cur != ' ') {
        arg += *cur;
        cur++;
    }
    while (*cur == ' ') cur++;
    return arg;
}

std::vector<std::string> parse_arguments(unsigned char *&cur) {
    std::vector<std::string> args;
    LFORTRAN_ASSERT(*cur == '(');
    cur++;
    while (*cur != ')') {
        args.push_back(parse_argument(cur));
        if (*cur == ',') cur++;
    }
    return args;
}

void get_newlines(const std::string &s, std::vector<uint32_t> &newlines) {
    for (uint32_t pos=0; pos < s.size(); pos++) {
        if (s[pos] == '\n') newlines.push_back(pos);
    }
}

void interval_end(LocationManager &lm, size_t output_len,
                size_t input_len, size_t input_interval_len,
                uint32_t interval_type) {
    lm.out_start0.push_back(output_len);
    lm.in_start0.push_back(input_len);
    lm.in_size0.push_back(input_interval_len);
    lm.interval_type0.push_back(interval_type);
}

void interval_end_type_0(LocationManager &lm, size_t output_len,
                size_t input_len) {
    size_t input_interval_len = output_len-lm.out_start0[lm.out_start0.size()-1];
    interval_end(lm, output_len, input_len, input_interval_len, 0);
}

struct IfDef {
    bool active=true;
    bool branch_enabled=true;
};

std::string CPreprocessor::run(const std::string &input, LocationManager &lm,
        cpp_symtab &macro_definitions) const {
    LFORTRAN_ASSERT(input[input.size()] == '\0');
    unsigned char *string_start=(unsigned char*)(&input[0]);
    unsigned char *cur = string_start;
    std::string output;
    lm.preprocessor = true;
    get_newlines(input, lm.in_newlines0);
    lm.out_start0.push_back(0);
    lm.in_start0.push_back(0);
    std::vector<IfDef> ifdef_stack;
    bool branch_enabled = true;
    macro_definitions["__FILE__"].expansion = "\"" + lm.in_filename + "\"";
    for (;;) {
        unsigned char *tok = cur;
        unsigned char *mar;
        unsigned char *t1, *t2, *t3, *t4;
        /*!stags:re2c format = 'unsigned char *@@;\n'; */
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:yyfill:enable = 0;
            re2c:flags:tags = 1;
            re2c:define:YYCTYPE = "unsigned char";

            end = "\x00";
            newline = "\n";
            whitespace = [ \t\v\r]+;
            digit = [0-9];
            char =  [a-zA-Z_];
            name = char (char | digit)*;

            int = digit+;

            * {
                if (!branch_enabled) continue;
                output.append(token(tok, cur));
                continue;
            }
            end {
                break;
            }
            "#" whitespace? "define" whitespace @t1 name @t2 (whitespace? | whitespace @t3 [^\n\x00]* @t4 ) newline  {
                if (!branch_enabled) continue;
                std::string macro_name = token(t1, t2), macro_subs;
                if (t3 != nullptr) {
                    LFORTRAN_ASSERT(t4 != nullptr);
                    macro_subs = token(t3, t4);
                    handle_continuation_lines(macro_subs, cur);
                }
                CPPMacro fn;
                fn.expansion = macro_subs;
                macro_definitions[macro_name] = fn;

                interval_end_type_0(lm, output.size(), cur-string_start);
                continue;
            }
            "#" whitespace? "define" whitespace @t1 name @t2 '(' whitespace? name whitespace? (',' whitespace? name whitespace?)* ')' whitespace @t3 [^\n\x00]* @t4 newline  {
                if (!branch_enabled) continue;
                std::string macro_name = token(t1, t2),
                        macro_subs = token(t3, t4);
                handle_continuation_lines(macro_subs, cur);
                std::vector<std::string> args = parse_arguments(t2);
                CPPMacro fn;
                fn.function_like = true;
                fn.args = args;
                fn.expansion = macro_subs;
                macro_definitions[macro_name] = fn;

                interval_end_type_0(lm, output.size(), cur-string_start);
                continue;
            }
            "#" whitespace? "undef" whitespace @t1 name @t2 whitespace? newline  {
                if (!branch_enabled) continue;
                std::string macro_name = token(t1, t2);
                auto search = macro_definitions.find(macro_name);
                if (search != macro_definitions.end()) {
                    macro_definitions.erase(search);
                }

                interval_end_type_0(lm, output.size(), cur-string_start);
                continue;
            }
            "#" whitespace? "ifdef" whitespace @t1 name @t2 whitespace? newline  {
                IfDef ifdef;
                ifdef.active = branch_enabled;
                if (ifdef.active) {
                    std::string macro_name = token(t1, t2);
                    if (macro_definitions.find(macro_name) != macro_definitions.end()) {
                        ifdef.branch_enabled = true;
                    } else {
                        ifdef.branch_enabled = false;
                    }
                    branch_enabled = ifdef.branch_enabled;
                } else {
                    ifdef.branch_enabled = false;
                }
                ifdef_stack.push_back(ifdef);
                if (!ifdef.active) continue;

                interval_end_type_0(lm, output.size(), cur-string_start);
                continue;
            }
            "#" whitespace? "ifndef" whitespace @t1 name @t2 whitespace? newline  {
                IfDef ifdef;
                ifdef.active = branch_enabled;
                if (ifdef.active) {
                    std::string macro_name = token(t1, t2);
                    if (macro_definitions.find(macro_name) != macro_definitions.end()) {
                        ifdef.branch_enabled = false;
                    } else {
                        ifdef.branch_enabled = true;
                    }
                    branch_enabled = ifdef.branch_enabled;
                } else {
                    ifdef.branch_enabled = false;
                }
                ifdef_stack.push_back(ifdef);
                if (!ifdef.active) continue;

                interval_end_type_0(lm, output.size(), cur-string_start);
                continue;
            }
            "#" whitespace? "else" whitespace? newline  {
                if (ifdef_stack.size() == 0) {
                    throw LFortranException("C preprocessor: #else encountered outside of #ifdef or #ifndef");
                }
                IfDef ifdef = ifdef_stack[ifdef_stack.size()-1];
                if (ifdef.active) {
                    ifdef.branch_enabled = !ifdef.branch_enabled;
                    branch_enabled = ifdef.branch_enabled;
                } else {
                    continue;
                }

                interval_end_type_0(lm, output.size(), cur-string_start);
                continue;
            }
            "#" whitespace? "endif" whitespace? newline  {
                if (ifdef_stack.size() == 0) {
                    throw LFortranException("C preprocessor: #endif encountered outside of #ifdef or #ifndef");
                }
                IfDef ifdef = ifdef_stack[ifdef_stack.size()-1];
                ifdef_stack.pop_back();
                if (ifdef.active) {
                    branch_enabled = true;
                } else {
                    continue;
                }

                interval_end_type_0(lm, output.size(), cur-string_start);
                continue;
            }
            "#" whitespace? "include" whitespace '"' @t1 [^"\x00]* @t2 '"' [^\n\x00]* newline {
                if (!branch_enabled) continue;
                std::string filename = token(t1, t2);
                // Construct a filename relative to the current file
                // TODO: make this multiplatform
                std::string base_dir = lm.in_filename;
                std::string::size_type n = base_dir.rfind("/");
                if (n != std::string::npos) {
                    base_dir = base_dir.substr(0, n);
                    filename = base_dir + "/" + filename;
                }
                std::string include;
                if (!read_file(filename, include)) {
                    throw LFortranException("C preprocessor: include file '" + filename + "' cannot be opened");
                }

                LocationManager lm_tmp = lm; // Make a copy
                include = run(include, lm_tmp, macro_definitions);

                // Prepare the start of the interval
                interval_end_type_0(lm, output.size(), tok-string_start);

                // Include
                output.append(include);

                // Prepare the end of the interval
                interval_end(lm, output.size(), cur-string_start,
                    token(tok, cur).size()-1, 1);
                continue;
            }
            name {
                if (!branch_enabled) continue;
                std::string t = token(tok, cur);
                if (macro_definitions.find(t) != macro_definitions.end()) {
                    // Prepare the start of the interval
                    interval_end_type_0(lm, output.size(), tok-string_start);

                    // Expand the macro once
                    std::string expansion;
                    if (macro_definitions[t].function_like) {
                        if (*cur != '(') {
                            throw LFortranException("C preprocessor: function-like macro invocation must have argument list");
                        }
                        std::vector<std::string> args;
                        args = parse_arguments(cur);
                        if (*cur != ')') {
                            throw LFortranException("C preprocessor: expected )");
                        }
                        cur++;
                        expansion = function_like_macro_expansion(
                            macro_definitions[t].args,
                            macro_definitions[t].expansion,
                            args);
                    } else {
                        if (t == "__LINE__") {
                            uint32_t pos = cur-string_start;
                            uint32_t line, col;
                            lm.pos_to_linecol(pos, line, col);
                            expansion = std::to_string(line);
                        } else {
                            expansion = macro_definitions[t].expansion;
                        }
                    }

                    // Recursively expand the expansion
                    std::string expansion2;
                    int i = 0;
                    while (expansion2 != expansion) {
                        expansion2 = expansion;
                        LocationManager lm_tmp = lm; // Make a copy
                        expansion = run(expansion2, lm_tmp, macro_definitions);
                        i++;
                        if (i == 40) {
                            throw LFortranException("C preprocessor: maximum recursion limit reached");
                        }
                    }

                    // Output the final recursively expanded macro
                    output.append(expansion);

                    // Prepare the end of the interval
                    interval_end(lm, output.size(), cur-string_start,
                        t.size(), 1);
                } else {
                    output.append(t);
                }
                continue;
            }
            '"' ('""'|[^"\x00])* '"' {
                if (!branch_enabled) continue;
                output.append(token(tok, cur));
                continue;
            }
            "'" ("''"|[^'\x00])* "'" {
                if (!branch_enabled) continue;
                output.append(token(tok, cur));
                continue;
            }
        */
    }
    lm.out_start0.push_back(output.size());
    lm.in_start0.push_back(input.size());
    // The just created interval ID:
    size_t N = lm.out_start0.size()-2;
    lm.in_size0.push_back(lm.out_start0[N+1]-lm.out_start0[N]);
    lm.interval_type0.push_back(0);

    // Uncomment for debugging
    /*
    std::cout << "in_start0: ";
    for (auto A : lm.in_start0) { std::cout << A << " "; }
    std::cout << std::endl;
    std::cout << "in_size0: ";
    for (auto A : lm.in_size0) { std::cout << A << " "; }
    std::cout << std::endl;
    std::cout << "interval_type0: ";
    for (auto A : lm.interval_type0) { std::cout << A << " "; }
    std::cout << std::endl;
    std::cout << "out_start0: ";
    for (auto A : lm.out_start0) { std::cout << A << " "; }
    std::cout << std::endl;
    */

    return output;
}

std::string CPreprocessor::function_like_macro_expansion(
            std::vector<std::string> &def_args,
            std::string &expansion,
            std::vector<std::string> &call_args) const {
    LFORTRAN_ASSERT(expansion[expansion.size()] == '\0');
    unsigned char *string_start=(unsigned char*)(&expansion[0]);
    unsigned char *cur = string_start;
    std::string output;
    for (;;) {
        unsigned char *tok = cur;
        unsigned char *mar;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            * {
                output.append(token(tok, cur));
                continue;
            }
            end {
                break;
            }
            name {
                std::string t = token(tok, cur);
                auto search = std::find(def_args.begin(), def_args.end(), t);
                if (search != def_args.end()) {
                    size_t i = std::distance(def_args.begin(), search);
                    output.append(call_args[i]);
                } else {
                    output.append(t);
                }
                continue;
            }
            '"' ('""'|[^"\x00])* '"' {
                output.append(token(tok, cur));
                continue;
            }
            "'" ("''"|[^'\x00])* "'" {
                output.append(token(tok, cur));
                continue;
            }
        */
    }
    return output;
}


} // namespace LFortran
