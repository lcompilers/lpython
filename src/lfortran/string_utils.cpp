#include <lfortran/string_utils.h>

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

std::vector<std::string> split(const std::string &s)
{
    std::vector<std::string> result;
    std::string split_chars = " \n";
    size_t old_pos = 0;
    size_t new_pos;
    while ((new_pos = s.find_first_of(split_chars, old_pos)) != std::string::npos) {
        result.push_back(s.substr(old_pos, new_pos-old_pos));
        old_pos = new_pos+1;
    }
    result.push_back(s.substr(old_pos));
    return result;
}

} // namespace LFortran
