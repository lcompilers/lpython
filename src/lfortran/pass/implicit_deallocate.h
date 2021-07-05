#ifndef LFORTRAN_PASS_IMPLICIT_DEALLOCATE_H
#define LFORTRAN_PASS_IMPLICIT_DEALLOCATE_H

#include <lfortran/asr.h>

namespace LFortran {

    void pass_insert_implicit_deallocate(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_IMPLICIT_DEALLOCATE_H
