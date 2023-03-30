#ifndef FORELSE_H
#define FORELSE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_forelse(Allocator &al, ASR::TranslationUnit_t &unit,
                              const LCompilers::PassOptions& pass_options);

} // namespace LCompilers


#endif
