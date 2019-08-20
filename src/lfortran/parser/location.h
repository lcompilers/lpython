#ifndef LFORTRAN_PARSER_LOCATION_H
#define LFORTRAN_PARSER_LOCATION_H

#include <vector>
#include <lfortran/ast.h>

namespace LFortran
{

struct Location
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

} // namespace LFortran


#endif
