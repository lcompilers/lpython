#ifndef LIBASR_PASS_WHILE_ELSE_H
#define LIBASR_PASS_WHILE_ELSE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

void pass_while_else(Allocator &al, ASR::TranslationUnit_t &unit,
                     const PassOptions &pass_options);
} // namespace LCompilers

#endif // LIBASR_PASS_WHILE_ELSE_H
