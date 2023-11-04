#ifndef LIBASR_ASR_TO_PY_H
#define LIBASR_ASR_TO_PY_H

#include <libasr/asr.h>
#include <tuple>

namespace LCompilers {

    std::tuple<std::string, std::string, std::string> asr_to_py(ASR::TranslationUnit_t &asr, bool c_order, std::string chdr_filename);

} // namespace LCompilers

#endif // LIBASR_ASR_TO_PY_H
