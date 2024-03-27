#ifndef LIBASR_PASS_REPLACE_FUNCTION_CALL_IN_DECLARATION_H
#define LIBASR_PASS_REPLACE_FUNCTION_CALL_IN_DECLARATION_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_function_call_in_declaration(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_FUNCTION_CALL_IN_DECLARATION_H
