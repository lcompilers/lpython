#pragma once

#include <string>

#include <libasr/asr.h>
#include <libasr/diagnostics.h>

namespace LCompilers {

  struct error_highlight {
    std::string message;
    uint32_t first_line;
    uint32_t first_column;
    uint32_t last_line;
    uint32_t last_column;
    std::string filename;
    diag::Level severity;
  };

  struct document_symbols {
    std::string symbol_name;
    uint32_t first_line;
    uint32_t first_column;
    uint32_t last_line;
    uint32_t last_column;
    std::string filename;
    ASR::symbolType symbol_type;
    int parent_index;  //<- position instead of pointer to avoid std::vector reallocation issues
  };

} // namespace LCompilers
