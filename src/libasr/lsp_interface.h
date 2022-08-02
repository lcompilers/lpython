#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>

namespace LFortran::LPython {

       struct error_highlight {
              std::string message;
              uint32_t first_line;
              uint32_t first_column;
              uint32_t last_line;
              uint32_t last_column;
              uint32_t severity;
       };
       struct document_symbols {
              std::string symbol_name;
              uint32_t first_line;
              uint32_t first_column;
              uint32_t last_line;
              uint32_t last_column;
       };

} // namespace LFortran::Python

#endif
