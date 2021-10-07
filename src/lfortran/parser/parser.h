#ifndef LFORTRAN_PARSER_PARSER_H
#define LFORTRAN_PARSER_PARSER_H

#include <fstream>
#include <algorithm>
#include <memory>

#include <lfortran/containers.h>
#include <lfortran/parser/tokenizer.h>

namespace LFortran
{

class Parser
{
public:
    std::string inp;

public:
    Allocator &m_a;
    Tokenizer m_tokenizer;
    Vec<AST::ast_t*> result;

    Parser(Allocator &al) : m_a{al} {
        result.reserve(al, 32);
    }

    void parse(const std::string &input);
    int parse();


private:
};

static inline uint32_t bisection(std::vector<uint32_t> vec, uint32_t i) {
    LFORTRAN_ASSERT(vec.size() >= 2);
    if (i < vec[0]) throw LFortranException("Index out of bounds");
    if (i >= vec[vec.size()-1]) return vec.size()-1;
    uint32_t i1 = 0, i2 = vec.size()-1;
    while (i1 < i2-1) {
        uint32_t imid = (i1+i2)/2;
        if (i < vec[imid]) {
            i2 = imid;
        } else {
            i1 = imid;
        }
    }
    return i1;
}

struct LocationManager {
    // The index into these vectors is the interval ID, starting from 0
    std::vector<uint32_t> out_start; // consecutive intervals in the output code
    std::vector<uint32_t> in_start; // start + size in the original code
    std::vector<uint32_t> in_newlines; // position of all \n in the original code
// `in_size` is current commented out, because it is equivalent to
// out_start[n+1]-out_start[n], due to each interval being 1:1 for now. Once we
// implement multiple interval types, we will need it:
//    std::vector<uint32_t> in_size;
//    std::vector<uint8_t> interval_type; // 0 .... 1:1; 1 ... any to one;
//    std::vector<uint32_t> filename_id; // file name for each interval, ID
//    std::vector<std::string> filenames; // filenames lookup for an ID

    // Converts a position in the output code to a position in the original code
    // Every character in the output code has a corresponding location in the
    // original code, so this function always succeeds
    uint32_t output_to_input_pos(uint32_t out_pos) const {
        uint32_t interval = bisection(out_start, out_pos);
        uint32_t rel_pos = out_pos - out_start[interval];
        uint32_t in_pos = in_start[interval] + rel_pos;
        return in_pos;
    }

    // Converts a linear position `position` to a (line, col) tuple
    // `position` starts from 0
    // `line` and `col` starts from 1
    // `in_newlines` starts from 0
    void pos_to_linecol(uint32_t position, uint32_t &line, uint32_t &col) const {
        int32_t interval = bisection(in_newlines, position);
        if (position == in_newlines[interval]) {
            // position is exactly the \n character, make sure `line` is
            // the line with \n, and `col` points to the position of \n
            line = interval;
            LFORTRAN_ASSERT(interval >= 1)
            col = position-in_newlines[interval-1];
        } else {
            line = interval+1;
            col = position-in_newlines[interval];
            if (line == 1) col++;
        }
    }

};


// Parses Fortran code to AST
AST::TranslationUnit_t* parse(Allocator &al, const std::string &s);

// Just like `parse`, but prints a nice error message to std::cout if a
// syntax error happens:
AST::TranslationUnit_t* parse2(Allocator &al, const std::string &s,
        bool use_colors=true, bool fixed_form=false);

// Returns a nice error message as a string
std::string format_syntax_error(const std::string &filename,
        const std::string &input, const Location &loc, const int token,
        const std::string *tstr, bool use_colors,
        const LocationManager &lm);

std::string format_semantic_error(const std::string &filename,
        const std::string &input, const Location &loc,
        const std::string msg, bool use_colors,
        const LocationManager &lm);

// Tokenizes the `input` and return a list of tokens
std::vector<int> tokens(Allocator &al, const std::string &input,
        std::vector<YYSTYPE> *stypes=nullptr,
        std::vector<Location> *locations=nullptr);

// Converts token number to text
std::string token2text(const int token);

std::string fix_continuation(const std::string &s, LocationManager &lm,
        bool fixed_form);

} // namespace LFortran

#endif
