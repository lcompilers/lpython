#ifndef LFORTRAN_ASR_VERIFY_H
#define LFORTRAN_ASR_VERIFY_H

#include <libasr/asr.h>

namespace LFortran {

    // Verifies that ASR is correctly constructed and contains valid Fortran
    // code and passes all our requirements on ASR, such as:
    //
    //   * All types and kinds are correctly inferred and implicit casting
    //     nodes are correctly inserted in expressions
    //   * Types match for function / subroutine calls
    //   * All symbols in the Symbol Table correctly link back to it or the
    //     parent table.
    //   * All Fortran rules will be checked eventually, such as:
    //     * Initializer expression only uses intrinsic functions
    //     * Any function used in array dimension declaration is pure
    //     * Pure function only calls pure functions
    //     * ...
    //
    // This should not replace correct semantic checking in ast2asr. This is
    // only meant as a tool for LFortran developers to check there are no bugs
    // in LFortran code that constructs ASR and that some requirement was not
    // accidentally broken.
    //   This should not be called in Release mode for performance reasons, but
    // it should be called in our tests to ensure ast2asr, deserialization, all
    // the ASR passes and any other code that constructs ASR does not have
    // bugs.
    //   Any code that takes ASR as an argument can assume that it is verified.
    // Such as the LLVM, C++ backend, or any ASR pass, or pickle.

    // The function will raise an exception if there is an error. Otherwise
    // it will return true. It can be used in Debug mode only as:
    //
    //   LFORTRAN_ASSERT(asr_verify(*asr));
    //
    bool asr_verify(const ASR::TranslationUnit_t &unit,
        bool check_external, diag::Diagnostics &diagnostics);

} // namespace LFortran

#endif // LFORTRAN_ASR_VERIFY_H
