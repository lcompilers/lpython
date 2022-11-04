#include <fstream>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_decoder.h>
#include <libasr/codegen/wasm_to_wat.h>

namespace LFortran {

namespace wasm {

class WATGenerator : public WASMDecoder<WATGenerator> {
   public:
    WATGenerator(Allocator &al, diag::Diagnostics &diagonostics)
        : WASMDecoder(al, diagonostics) {}

    std::string get_wat() {
        std::string result = "(module";
        std::string indent = "\n    ";
        for (uint32_t i = 0U; i < func_types.size(); i++) {
            result +=
                indent + "(type (;" + std::to_string(i) + ";) (func (param";
            for (uint32_t j = 0; j < func_types[i].param_types.size(); j++) {
                result +=
                    " " + var_type_to_string[func_types[i].param_types.p[j]];
            }
            result += ") (result";
            for (uint32_t j = 0; j < func_types[i].result_types.size(); j++) {
                result +=
                    " " + var_type_to_string[func_types[i].result_types.p[j]];
            }
            result += ")))";
        }

        for (uint32_t i = 0; i < imports.size(); i++) {
            result += indent + "(import \"" + imports[i].mod_name + "\" \"" +
                      imports[i].name + "\" ";
            if (imports[i].kind == 0x00) {
                result += "(func (;" + std::to_string(imports[i].type_idx) +
                          ";) (type " + std::to_string(imports[i].type_idx) +
                          ")))";
            } else if (imports[i].kind == 0x02) {
                result +=
                    "(memory (;0;) " +
                    std::to_string(imports[i].mem_page_size_limits.first) +
                    " " +
                    std::to_string(imports[i].mem_page_size_limits.second) +
                    "))";
            }
        }

        for (uint32_t i = 0; i < type_indices.size(); i++) {
            uint32_t func_index = type_indices.p[i];
            result += indent + "(func $" + std::to_string(func_index);
            result += " (type " + std::to_string(func_index) + ") (param";
            for (uint32_t j = 0; j < func_types[func_index].param_types.size();
                 j++) {
                result +=
                    " " +
                    var_type_to_string[func_types[func_index].param_types.p[j]];
            }
            result += ") (result";
            for (uint32_t j = 0; j < func_types[func_index].result_types.size();
                 j++) {
                result += " " + var_type_to_string[func_types[func_index]
                                                       .result_types.p[j]];
            }
            result += ")";
            result += indent + "    (local";
            for (uint32_t j = 0; j < codes.p[i].locals.size(); j++) {
                for (uint32_t k = 0; k < codes.p[i].locals.p[j].count; k++) {
                    result +=
                        " " + var_type_to_string[codes.p[i].locals.p[j].type];
                }
            }
            result += ")";

            {
                WASM_INSTS_VISITOR::WATVisitor v =
                    WASM_INSTS_VISITOR::WATVisitor(wasm_bytes,
                                                   codes.p[i].insts_start_index,
                                                   "", indent + "    ");
                v.decode_instructions();
                result += v.src;
            }

            result += indent + ")";
        }

        for (uint32_t i = 0; i < exports.size(); i++) {
            result += indent + "(export \"" + exports.p[i].name + "\" (" +
                      kind_to_string[exports.p[i].kind] + " $" +
                      std::to_string(exports.p[i].index) + "))";
        }

        for (uint32_t i = 0; i < data_segments.size(); i++) {
            std::string date_segment_insts;
            {
                WASM_INSTS_VISITOR::WATVisitor v =
                    WASM_INSTS_VISITOR::WATVisitor(
                        wasm_bytes, data_segments.p[i].insts_start_index, "",
                        "");
                v.decode_instructions();
                date_segment_insts = v.src;
            }
            result += indent + "(data (;" + std::to_string(i) + ";) (" +
                      date_segment_insts + ") \"" + data_segments[i].text +
                      "\")";
        }

        result += "\n)\n";

        return result;
    }
};

}  // namespace wasm

Result<std::string> wasm_to_wat(Vec<uint8_t> &wasm_bytes, Allocator &al,
                                diag::Diagnostics &diagnostics) {
    wasm::WATGenerator wasm_generator(al, diagnostics);
    wasm_generator.wasm_bytes.from_pointer_n(wasm_bytes.data(),
                                             wasm_bytes.size());

    std::string wat;

    try {
        wasm_generator.decode_wasm();
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    }

    wat = wasm_generator.get_wat();

    return wat;
}

}  // namespace LFortran
