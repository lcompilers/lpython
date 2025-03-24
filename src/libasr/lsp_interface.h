#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>

#include <libasr/asr.h>
#include <libasr/diagnostics.h>

namespace LCompilers {
        namespace LSP {
                // These are the latest definitions per the LSP 3.17 spec.

                enum DiagnosticSeverity {
                        Error = 1,
                        Warning = 2,
                        Information = 3,
                        Hint = 4
                };

                enum SymbolKind {
                        File = 1,
                        Module = 2,
                        Namespace = 3,
                        Package = 4,
                        Class = 5,
                        Method = 6,
                        Property = 7,
                        Field = 8,
                        Constructor = 9,
                        Enum = 10,
                        Interface = 11,
                        Function = 12,
                        Variable = 13,
                        Constant = 14,
                        String = 15,
                        Number = 16,
                        Boolean = 17,
                        Array = 18,
                        Object = 19,
                        Key = 20,
                        Null = 21,
                        EnumMember = 22,
                        Struct = 23,
                        Event = 24,
                        Operator = 25,
                        TypeParameter = 26
                };
        } // namespace LSP

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
       };

} // namespace LCompilers

#endif
