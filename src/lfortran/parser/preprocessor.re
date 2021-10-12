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
        std::map<std::string, std::string> &macro_definitions) const {
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
                macro_definitions[macro_name] = macro_subs;
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
                    std::string expansion = macro_definitions[t];
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

} // namespace LFortran
