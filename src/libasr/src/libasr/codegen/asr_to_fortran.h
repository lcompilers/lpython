#ifndef LFORTRAN_ASR_TO_FORTRAN_H
#define LFORTRAN_ASR_TO_FORTRAN_H

#include <libasr/asr.h>
#include <libasr/asr_utils.h>

namespace LCompilers {

    // Converts ASR to Fortran source code
    Result<std::string> asr_to_fortran(ASR::TranslationUnit_t &asr,
            diag::Diagnostics &diagnostics, bool color, int indent);

} // namespace LCompilers

#endif // LFORTRAN_ASR_TO_FORTRAN_H
