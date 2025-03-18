#include <cctype>
#include <regex>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <libasr/string_utils.h>
#include <libasr/containers.h>

namespace LCompilers {


bool startswith(const std::string &s, const std::string &e)
{
    if (s.size() < e.size()) return false;
    return s.substr(0, e.size()) == e;
}

bool endswith(const std::string &s, const std::string &e)
{
    if (s.size() < e.size()) return false;
    return s.substr(s.size()-e.size()) == e;
}

std::string to_lower(const std::string &s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return res;
}

char *s2c(Allocator &al, const std::string &s) {
    Str x; x.from_str_view(s);
    return x.c_str(al);
}

// Splits the string `s` using the separator `split_string`
std::vector<std::string> string_split(const std::string &s,
    const std::string &split_string, bool strs_to_lower)
{
    std::vector<std::string> result;
    size_t old_pos = 0;
    size_t new_pos;
    std::string substr;
    while ((new_pos = s.find(split_string, old_pos)) != std::string::npos) {
        substr = s.substr(old_pos, new_pos-old_pos);
        if (substr.size() > 0)
            result.push_back(strs_to_lower ? to_lower(substr) : substr);
        old_pos = new_pos+split_string.size();
    }
    substr = s.substr(old_pos);
    result.push_back(strs_to_lower ? to_lower(substr) : substr);
    return result;
}

std::vector<std::string> string_split_avoid_parentheses(const std::string &str, bool strs_to_lower) {
    std::vector<std::string> result;
    std::string word;
    bool in_brackets = false;
    for (char ch : str) {
        if (ch == ' ' && !in_brackets) {
            if (!word.empty()) {
                result.push_back(strs_to_lower ? LCompilers::to_lower(word) : word);
                word.clear();
            }
        } else {
            if (ch == '(') in_brackets = true;
            if (ch == ')') in_brackets = false;
            word += ch;
        }
    }
    if (!word.empty()) result.push_back(strs_to_lower ? LCompilers::to_lower(word) : word);
    return result;
}

// Splits the string `s` using any space or newline
std::vector<std::string> split(const std::string &s)
{
    std::vector<std::string> result;
    std::string split_chars = " \n";
    size_t old_pos = 0;
    size_t new_pos;
    while ((new_pos = s.find_first_of(split_chars, old_pos)) != std::string::npos) {
        std::string substr = s.substr(old_pos, new_pos-old_pos);
        if (substr.size() > 0) result.push_back(substr);
        old_pos = new_pos+1;
    }
    result.push_back(s.substr(old_pos));
    return result;
}

std::string join(const std::string j, const std::vector<std::string> &l)
{
    std::string result;
    for (size_t i=0; i<l.size(); i++) {
        result += l[i];
        if (i < l.size()-1) result += j;
    }
    return result;
}

std::vector<std::string> slice(const std::vector<std::string>& v, int start, int end)
{
    int oldlen = v.size();
    int newlen;

    if ((end == -1) || (end >= oldlen)) {
        newlen = oldlen-start;
    } else {
        newlen = end-start;
    }

    std::vector<std::string> nv(newlen);

    for (int i=0; i<newlen; i++) {
        nv[i] = v[start+i];
    }
    return nv;
}

std::string replace(const std::string &s,
    const std::string &regex, const std::string &replace)
{
    return std::regex_replace(s, std::regex(regex), replace);
}

std::string read_file(const std::string &filename)
{
    std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary
            | std::ios::ate);

    std::ifstream::pos_type filesize = ifs.tellg();
    if (filesize < 0) return std::string();

    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(filesize);
    ifs.read(&bytes[0], filesize);

    return std::string(&bytes[0], filesize);
}

std::string parent_path(const std::string &path) {
    int pos = path.size()-1;
    while (pos >= 0 && path[pos] != '/') pos--;
    if (pos == -1) {
        return "";
    } else {
        return path.substr(0, pos);
    }
}

bool is_relative_path(const std::string &path) {
    return !startswith(path, "/");
}

std::string join_paths(const std::vector<std::string> &paths) {
    std::string p;
    std::string delim = "/";
    for (auto &path : paths) {
        if (path.size() > 0) {
            if (p.size() > 0 && !endswith(p, delim)) {
                p.append(delim);
            }
            p.append(path);
        }
    }
    return p;
}

std::string str_escape_c(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            case '\v': o << "\\v"; break;
            default:
                if ('\x00' <= *c && *c <= '\x1f') {
                    o << "\\u"
                    << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
                } else {
                    o << *c;
                }
        }
    }
    return o.str();
}

char* str_unescape_c(Allocator &al, LCompilers::Str &s) {
    std::string x = "";
    size_t idx = 0;
    for (; idx + 1 < s.size(); idx++) {
        if (s[idx] == '\\' && s[idx+1] == '\n') { // continuation character
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == 'n') {
            x += "\n";
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == 't') {
            x += "\t";
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == 'r') {
            x += "\r";
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == 'b') {
            x += "\b";
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == 'v') {
            x += "\v";
            idx++;
        } else if (s[idx] == '\\' && s[idx + 1] == 'f') {
            x += "\f";
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == '\\') {
            x += "\\";
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == '"') {
            x += '"';
            idx++;
        } else if (s[idx] == '\\' && s[idx+1] == '\'') {
            x += '\'';
            idx++;
        } else {
            x += s[idx];
        }
    }
    if (idx < s.size()) {
        x += s[idx];
    }
    return LCompilers::s2c(al, x);
}

std::string str_escape_fortran_double_quote(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
            case '"': o << "\"\""; break;
        }
    }
    return o.str();
}

char* str_unescape_fortran(Allocator &al, LCompilers::Str &s, char ch) {
    std::string x = "";
    size_t idx = 0;
    for (; idx + 1 < s.size(); idx++) {
        if (s[idx] == ch && s[idx + 1] == ch) {
            x += s[idx];
            idx++;
        } else {
            x += s[idx];
        }
    }
    if (idx < s.size()) {
        x += s[idx];
    }
    return LCompilers::s2c(al, x);
}

bool str_compare(const unsigned char *pos, std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (pos[i] == '\0') {
            return false;
        }

        if (pos[i] != s[i]) {
            return false;
        }
    }
    return true;
}

// trim trailing whitespace from a string in-place
void rtrim(std::string& str) {
    str.erase(std::find_if_not(str.rbegin(), str.rend(), ::isspace).base(), str.end());
}

} // namespace LCompilers
