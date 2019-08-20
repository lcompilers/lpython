#ifndef LFORTRAN_PARSER_LOCATION_H
#define LFORTRAN_PARSER_LOCATION_H

#include <cstdint>
#include <vector>
#include <lfortran/ast.h>

namespace LFortran
{

struct Location
{
  uint16_t first_line;
  uint16_t first_column;
  uint16_t last_line;
  uint16_t last_column;
};

} // namespace LFortran


#endif
