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

} // namespace LFortran


#endif
