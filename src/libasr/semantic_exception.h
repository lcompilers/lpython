#ifndef LFORTRAN_SEMANTICS_SEMANTIC_EXCEPTION_H
#define LFORTRAN_SEMANTICS_SEMANTIC_EXCEPTION_H

#include <libasr/exception.h>

namespace LCompilers {

// This exception is only used internally in the lfortran/semantics/ directory
// and in lfortran/asr_utils.h/cpp. Nowhere else.

class SemanticAbort
{
};

} // namespace LCompilers


#endif
