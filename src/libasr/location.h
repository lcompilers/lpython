#ifndef LFORTRAN_PARSER_LOCATION_H
#define LFORTRAN_PARSER_LOCATION_H

#include <cstdint>
#include <vector>

namespace LFortran
{

struct Location
{
  uint32_t first;
  uint32_t last;
};

static inline uint32_t bisection(const std::vector<uint32_t> &vec, uint32_t i) {
    if (vec.size() == 0) return 0;
    if (i < vec[0]) return 0;
    if (i >= vec[vec.size()-1]) return vec.size();
    uint32_t i1 = 0, i2 = vec.size()-1;
    while (i1 < i2-1) {
        uint32_t imid = (i1+i2)/2;
        if (i < vec[imid]) {
            i2 = imid;
        } else {
            i1 = imid;
        }
    }
    return i1+1;
}

struct LocationManager {
    // The index into these vectors is the interval ID, starting from 0
    //
    // The output code is partitioned into consecutive (non-overlapping)
    // interval by a single array `out_start` of the starting points: interval N
    // is (out_start[N], out_start[N+1]). Each interval has an ID (starting from
    // 0), and each interval corresponds to some interval in the original code.
    //
    // There are two types of intervals, stored in `interval_type`:
    //
    // 0 ... 1:1 interval. One to one intervals have the same lengths in both in
    //       and out codes and they correspond to just copying text from in to
    //       out. The length of the interval N is out_start[N+1]-out_start[N],
    //       and it starts at in_start[N] in input code and out_start[N] in
    //       output code. Every character in output code can be mapped to a
    //       character in input code and vice versa.
    // 1 ... Many to many interval: This type happens for example in a macro
    //       expansion, e.g.:
    //           #define X(a,b) -> sin(a*x)+cos(b*y)
    //           X(1.5, 3.5)
    //       gets expanded to:
    //           sin(1.5*x)+cos(3.5*y)
    //       So the single input code interval `X(1.5, 3.5)` corresponds to an interval
    //       `sin(1.5*x)+cos(3.5*y)`, but we do not specify any more detailed
    //       correspondence on a character basis. If for example `x` is defined
    //       but `y` is undefined, then the compiler error message can for
    //       example look like this:
    //           Semantic error: `y` is undefined:
    //           sin(1.5*x)+cos(3.5*y)
    //                              ^
    //           Note: In a macro expansion:
    //           X(1.5, 3.5)
    //           ^~~~~~~~~~~
    //           Note: the macro was defined at:
    //           #define X(a,b) -> sin(a*x)+cos(b*y)
    //                   ^~~~~~
    //       The last part can be inferred from a Preprocessor symbol table: The
    //       error message of undefined `y` leads to line with the macro `X`, so
    //       we can lookup `X` in a macro symbol table and find its definition.
    //
    // # Handling of include files
    //
    // Example of input file `A.cpp`:
    //     #include "B.h"
    //     void f(const MyType &x);
    // Expands to the output file:
    //     struct MyType {
    //         int i;
    //     }
    //     void f(const MyType &x);
    // Here `f` from the output file maps using interval type 0 to `f` in A.cpp.
    // The `struct MyType` maps using interval type 0 to `struct MyType` in B.h.
    // In addition, for this interval we store a list of files+positions
    // of all recursive include lines.
    //
    // Note: even for macro expansion we can use intervals type 0 to point to
    // the copied text, such as `cos(` can point directly into the macro
    // definition, `3.5` can point to `b` using interval 1 (but can also point
    // to `3.5` in `X(1.5, 3.5)` using interval 0), `*y)` can again use interval
    // 0.
    // Macros are recursive, so the same idea: one interval of type 0, and a
    // list of files+positions.
    //
    //
    //
    std::vector<uint32_t> out_start; // consecutive intervals in the output code
    std::vector<uint32_t> in_start; // start + size in the original code
    std::vector<uint32_t> in_newlines; // position of all \n in the original code

    // For preprocessor (if preprocessor==true).
    // TODO: design a common structure, that also works with #include, that
    // has these mappings for each file
    bool preprocessor = false;
    std::string in_filename;
    uint32_t current_line=0;
    std::vector<uint32_t> out_start0; // consecutive intervals in the output code
    std::vector<uint32_t> in_start0; // start + size in the original code
    std::vector<uint32_t> in_size0; // Size of the `in` interval
    std::vector<uint32_t> interval_type0; // 0 .... 1:1; 1 ... many to many;
    std::vector<uint32_t> in_newlines0; // position of all \n in the original code
//    std::vector<uint32_t> filename_id; // file name for each interval, ID
//    std::vector<std::string> filenames; // filenames lookup for an ID

    // Converts a position in the output code to a position in the original code
    // Every character in the output code has a corresponding location in the
    // original code, so this function always succeeds
    uint32_t output_to_input_pos(uint32_t out_pos, bool show_last) const {
        if (out_start.size() == 0) return 0;
        uint32_t interval = bisection(out_start, out_pos)-1;
        uint32_t rel_pos = out_pos - out_start[interval];
        uint32_t in_pos = in_start[interval] + rel_pos;
        if (preprocessor) {
            // If preprocessor was used, do one more remapping
            uint32_t interval0 = bisection(out_start0, in_pos)-1;
            if (interval_type0[interval0] == 0) {
                // 1:1 interval
                uint32_t rel_pos0 = in_pos - out_start0[interval0];
                uint32_t in_pos0 = in_start0[interval0] + rel_pos0;
                return in_pos0;
            } else {
                // many to many interval
                uint32_t in_pos0;
                if (in_pos == out_start0[interval0+1]-1 || show_last) {
                    // The end of the interval in "out" code
                    // Return the end of the interval in "in" code
                    in_pos0 = in_start0[interval0]+in_size0[interval0]-1;
                } else {
                    // Otherwise return the beginning of the interval in "in"
                    in_pos0 = in_start0[interval0];
                }
                return in_pos0;
            }
        } else {
            return in_pos;
        }
    }

    // Converts a linear position `position` to a (line, col) tuple
    // `position` starts from 0
    // `line` and `col` starts from 1
    // `in_newlines` starts from 0
    void pos_to_linecol(uint32_t position, uint32_t &line, uint32_t &col) const {
        const std::vector<uint32_t> *newlines;
        if (preprocessor) {
            newlines = &in_newlines0;
        } else {
            newlines = &in_newlines;
        }
        int32_t interval = bisection(*newlines, position);
        if (interval >= 1 && position == (*newlines)[interval-1]) {
            // position is exactly the \n character, make sure `line` is
            // the line with \n, and `col` points to the position of \n
            interval -= 1;
        }
        line = interval+1;
        if (line == 1) {
            col = position+1;
        } else {
            col = position-(*newlines)[interval-1];
        }
    }

    void get_newlines(const std::string &s, std::vector<uint32_t> &newlines) {
        for (uint32_t pos=0; pos < s.size(); pos++) {
            if (s[pos] == '\n') newlines.push_back(pos);
        }
    }

    void init_simple(const std::string &input) {
        uint32_t n = input.size();
        out_start = {0, n};
        in_start = {0, n};
        get_newlines(input, in_newlines);
    }

};


} // namespace LFortran


#endif
