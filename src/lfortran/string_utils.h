#ifndef LFORTRAN_STRING_UTILS_H
#define LFORTRAN_STRING_UTILS_H

#include <string>
#include <vector>

namespace LFortran
{


bool startswith(const std::string &s, const std::string &e);
bool endswith(const std::string &s, const std::string &e);
std::vector<std::string> split(const std::string &s);


} // namespace LFortran




#endif // LFORTRAN_STRING_UTILS_H
