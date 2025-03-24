#ifndef LFORTRAN_STRING_UTILS_H
#define LFORTRAN_STRING_UTILS_H

#include <string>
#include <vector>
#include <cctype>

#include <libasr/alloc.h>
#include <libasr/containers.h>

namespace LCompilers {


bool startswith(const std::string &s, const std::string &e);
bool endswith(const std::string &s, const std::string &e);
std::string to_lower(const std::string &s);
std::vector<std::string> string_split(const std::string &s,
    const std::string &split_string, bool strs_to_lower=true);
std::vector<std::string> string_split_avoid_parentheses(const std::string &s,
    bool strs_to_lower=true);
std::vector<std::string> split(const std::string &s);
std::string join(const std::string j, const std::vector<std::string> &v);
std::vector<std::string> slice(const std::vector<std::string> &v,
        int start=0, int end=-1);
char *s2c(Allocator &al, const std::string &s);

// Replaces all occurrences of `regex` (a regular expression, must escape
// special characters) with `replace`
std::string replace(const std::string &s,
    const std::string &regex, const std::string &replace);

std::string read_file(const std::string &filename);

// Returns the parent path to the given path
std::string parent_path(const std::string &path);
// Returns true if the path is relative
bool is_relative_path(const std::string &path);
// Joins paths (paths can be empty)
std::string join_paths(const std::vector<std::string> &paths);

// Escapes special characters from the given string
// using C style escaping
std::string str_escape_c(const std::string &s);
char* str_unescape_c(Allocator &al, LCompilers::Str &s);

// Escapes double quote characters from the given string
// given string must be enclosed in double quotes
std::string str_escape_fortran_double_quote(const std::string &s);
char* str_unescape_fortran(Allocator &al, LCompilers::Str &s, char ch);

bool str_compare(const unsigned char *pos, std::string s);
void rtrim(std::string& str);

} // namespace LCompilers

#endif // LFORTRAN_STRING_UTILS_H
