#ifndef LIBASR_PASS_PYTHON_BIND_H
#define LIBASR_PASS_PYTHON_BIND_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_python_bind(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_PYTHON_BIND_H
