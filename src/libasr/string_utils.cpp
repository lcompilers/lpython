#include <cctype>
#include <regex>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>

#include <libasr/string_utils.h>
#include <libasr/containers.h>

namespace LFortran
{


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



} // namespace LFortran
