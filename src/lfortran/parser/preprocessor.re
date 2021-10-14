#include <iostream>
#include <map>

#include <lfortran/parser/preprocessor.h>
#include <lfortran/assert.h>
#include <lfortran/utils.h>
#include <lfortran/string_utils.h>

namespace LFortran
{

std::string CPreprocessor::token(unsigned char *tok, unsigned char* cur) const
{
    return std::string((char *)tok, cur - tok);
}

void parse_macro_definition(const std::string &line,
    std::string &name, std::string &subs)
{
    size_t i = 0;
    i += std::string("#define").size();
    while (line[i] == ' ') i++;
    size_t s1 = i;
    while (line[i] != ' ') i++;
    name = std::string(&line[s1], i-s1);
    while (line[i] == ' ') i++;
    subs = line.substr(i, line.size()-i-1);
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
    while (*cur != ')') {
        args.push_back(parse_argument(cur));
        if (*cur == ',') cur++;
    }
    return args;
}

void parse_macro_definition2(const std::string &line,
    std::string &name, std::vector<std::string> &args, std::string &subs)
{
    size_t i = 0;
    i += std::string("#define").size();
    while (line[i] == ' ') i++;
    size_t s1 = i;
    while (line[i] != '(') i++;
    name = std::string(&line[s1], i-s1);
    i++;
    const char *line_i = &line[i];
    unsigned char *cur0 = (unsigned char*) line_i;
    unsigned char *cur = cur0;
    args = parse_arguments(cur);
    i += cur-cur0+1;
    while (line[i] == ' ') i++;
    subs = line.substr(i, line.size()-i-1);
}

void parse_include_line(const std::string &line, std::string &filename)
{
    size_t i = 0;
    i += std::string("#include").size();
    while (line[i] != '"') i++;
    i++;
    size_t s1 = i;
    while (line[i] != '"') i++;
    filename = std::string(&line[s1], i-s1);
}

void get_newlines(const std::string &s, std::vector<uint32_t> &newlines) {
    for (uint32_t pos=0; pos < s.size(); pos++) {
        if (s[pos] == '\n') newlines.push_back(pos);
    }
}

std::string CPreprocessor::run(const std::string &input, LocationManager &lm,
        cpp_symtab &macro_definitions) const {
    LFORTRAN_ASSERT(input[input.size()] == '\0');
    unsigned char *string_start=(unsigned char*)(&input[0]);
    unsigned char *cur = string_start;
    Location loc;
    std::string output;
    lm.preprocessor = true;
    get_newlines(input, lm.in_newlines0);
    lm.out_start0.push_back(0);
    lm.in_start0.push_back(0);
    for (;;) {
        unsigned char *tok = cur;
        unsigned char *mar;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            end = "\x00";
            newline = "\n";
            whitespace = [ \t\v\r]+;
            digit = [0-9];
            char =  [a-zA-Z_];
            name = char (char | digit)*;

            int = digit+;

            * {
                token_loc(loc, tok, cur, string_start);
                output.append(token(tok, cur));
                continue;
            }
            end {
                break;
            }
            "#define" whitespace name whitespace [^\n\x00]* newline  {
                std::string macro_name, macro_subs;
                parse_macro_definition(token(tok, cur),
                    macro_name, macro_subs);
                CPPMacro fn;
                fn.expansion = macro_subs;
                macro_definitions[macro_name] = fn;
                lm.out_start0.push_back(output.size());
                lm.in_start0.push_back(cur-string_start);
                // The just created interval ID:
                size_t N = lm.out_start0.size()-2;
                lm.in_size0.push_back(lm.out_start0[N+1]-lm.out_start0[N]);
                lm.interval_type0.push_back(0);
                continue;
            }
            "#define" whitespace name '(' whitespace? name whitespace? (',' whitespace? name whitespace?)* ')' whitespace [^\n\x00]* newline  {
                std::string macro_name, macro_subs;
                std::vector<std::string> args;
                parse_macro_definition2(token(tok, cur),
                    macro_name, args, macro_subs);
                CPPMacro fn;
                fn.function_like = true;
                fn.args = args;
                fn.expansion = macro_subs;
                macro_definitions[macro_name] = fn;
                lm.out_start0.push_back(output.size());
                lm.in_start0.push_back(cur-string_start);
                // The just created interval ID:
                size_t N = lm.out_start0.size()-2;
                lm.in_size0.push_back(lm.out_start0[N+1]-lm.out_start0[N]);
                lm.interval_type0.push_back(0);
                continue;
            }
            "#include" whitespace '"' [^"\x00]* '"' [^\n\x00]* newline {
                std::string line = token(tok, cur);
                std::string include;
                std::string filename;
                parse_include_line(line, filename);
                // Construct a filename relative to the current file
                // TODO: make this multiplatform
                std::string base_dir = lm.in_filename;
                std::string::size_type n = base_dir.rfind("/");
                if (n != std::string::npos) {
                    base_dir = base_dir.substr(0, n);
                    filename = base_dir + "/" + filename;
                }
                if (!read_file(filename, include)) {
                    throw LFortranException("C preprocessor: include file '" + filename + "' cannot be opened");
                }

                LocationManager lm_tmp = lm; // Make a copy
                include = run(include, lm_tmp, macro_definitions);

                // Prepare the start of the interval
                lm.out_start0.push_back(output.size());
                lm.in_start0.push_back(tok-string_start);
                // The just created interval ID:
                size_t N = lm.out_start0.size()-2;
                lm.in_size0.push_back(lm.out_start0[N+1]-lm.out_start0[N]);
                lm.interval_type0.push_back(0);

                // Include
                output.append(include);

                // Prepare the end of the interval
                lm.out_start0.push_back(output.size());
                lm.in_start0.push_back(cur-string_start);
                // The just created interval ID:
                N = lm.out_start0.size()-2;
                lm.in_size0.push_back(token(tok, cur).size()-1);
                lm.interval_type0.push_back(1);
                continue;
            }
            name {
                std::string t = token(tok, cur);
                if (macro_definitions.find(t) != macro_definitions.end()) {
                    // Prepare the start of the interval
                    lm.out_start0.push_back(output.size());
                    lm.in_start0.push_back(tok-string_start);
                    // The just created interval ID:
                    size_t N = lm.out_start0.size()-2;
                    lm.in_size0.push_back(lm.out_start0[N+1]-lm.out_start0[N]);
                    lm.interval_type0.push_back(0);

                    // Expand the macro recursively
                    if (macro_definitions[t].function_like) {
                        if (*cur != '(') {
                            throw LFortranException("C preprocessor: function-like macro invocation must have argument list");
                        }
                        cur++;
                        std::vector<std::string> args;
                        args = parse_arguments(cur);
                        if (*cur != ')') {
                            throw LFortranException("C preprocessor: expected )");
                        }
                        cur++;
                        std::string expansion = function_like_macro_expansion(
                            macro_definitions[t].args,
                            macro_definitions[t].expansion,
                            args);
                        // TODO: recursive expansion
                        output.append(expansion);
                    } else {
                        std::string expansion = macro_definitions[t].expansion;
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
                        output.append(expansion);
                    }

                    // Prepare the end of the interval
                    lm.out_start0.push_back(output.size());
                    lm.in_start0.push_back(cur-string_start);
                    // The just created interval ID:
                    N = lm.out_start0.size()-2;
                    lm.in_size0.push_back(t.size());
                    lm.interval_type0.push_back(1);
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
