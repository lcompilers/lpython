#include <fstream>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_to_wat.h>

// #define WAT_DEBUG

#ifdef WAT_DEBUG
#define DEBUG(s) std::cout << s << std::endl
#else
#define DEBUG(s)
#endif

namespace LCompilers {

namespace wasm {

void WASMDecoder::load_file(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    wasm_bytes.reserve(al, size);
    file.read((char*)wasm_bytes.data(), size);
    file.close();
}

void WASMDecoder::decode_type_section(uint32_t offset) {
    // read type section contents
    uint32_t no_of_func_types = read_unsigned_num(wasm_bytes, offset);
    DEBUG("no_of_func_types: " + std::to_string(no_of_func_types));
    func_types.resize(al, no_of_func_types);

    for (uint32_t i = 0; i < no_of_func_types; i++) {
        if (wasm_bytes[offset] != 0x60) {
            throw LCompilers::LCompilersException("Invalid type section");
        }
        offset++;

        // read result type 1
        uint32_t no_of_params = read_unsigned_num(wasm_bytes, offset);
        func_types.p[i].param_types.resize(al, no_of_params);

        for (uint32_t j = 0; j < no_of_params; j++) {
            func_types.p[i].param_types.p[j] = wasm_bytes[offset++];
        }

        uint32_t no_of_results = read_unsigned_num(wasm_bytes, offset);
        func_types.p[i].result_types.resize(al, no_of_results);

        for (uint32_t j = 0; j < no_of_results; j++) {
            func_types.p[i].result_types.p[j] = wasm_bytes[offset++];
        }
    }
}

void WASMDecoder::decode_function_section(uint32_t offset) {
    // read function section contents
    uint32_t no_of_indices = read_unsigned_num(wasm_bytes, offset);
    DEBUG("no_of_indices: " + std::to_string(no_of_indices));
    type_indices.resize(al, no_of_indices);

    for (uint32_t i = 0; i < no_of_indices; i++) {
        type_indices.p[i] = read_unsigned_num(wasm_bytes, offset);
    }
}

void WASMDecoder::decode_export_section(uint32_t offset) {
    // read export section contents
    uint32_t no_of_exports = read_unsigned_num(wasm_bytes, offset);
    DEBUG("no_of_exports: " + std::to_string(no_of_exports));
    exports.resize(al, no_of_exports);

    for (uint32_t i = 0; i < no_of_exports; i++) {
        uint32_t name_size = read_unsigned_num(wasm_bytes, offset);
        exports.p[i].name.resize(name_size); // do not pass al to this resize as it is std::string.resize()
        for (uint32_t j = 0; j < name_size; j++) {
            exports.p[i].name[j] = wasm_bytes[offset++];
        }
        DEBUG("export name: " + exports.p[i].name);
        exports.p[i].kind = wasm_bytes[offset++];
        DEBUG("export kind: " + std::to_string(exports.p[i].kind));
        exports.p[i].index = read_unsigned_num(wasm_bytes, offset);
        DEBUG("export index: " + std::to_string(exports.p[i].index));
    }
}

void WASMDecoder::decode_code_section(uint32_t offset) {
    // read code section contents
    uint32_t no_of_codes = read_unsigned_num(wasm_bytes, offset);
    DEBUG("no_of_codes: " + std::to_string(no_of_codes));
    codes.resize(al, no_of_codes);

    for (uint32_t i = 0; i < no_of_codes; i++) {
        codes.p[i].size = read_unsigned_num(wasm_bytes, offset);
        uint32_t code_start_offset = offset;
        uint32_t no_of_locals = read_unsigned_num(wasm_bytes, offset);
        DEBUG("no_of_locals: " + std::to_string(no_of_locals));
        codes.p[i].locals.resize(al, no_of_locals);

        DEBUG("Entering loop");
        for (uint32_t j = 0U; j < no_of_locals; j++) {
            codes.p[i].locals.p[j].count = read_unsigned_num(wasm_bytes, offset);
            DEBUG("count: " + std::to_string(codes.p[i].locals.p[j].count));
            codes.p[i].locals.p[j].type = wasm_bytes[offset++];
            DEBUG("type: " + std::to_string(codes.p[i].locals.p[j].type));
        }
        DEBUG("Exiting loop");

        codes.p[i].insts_start_index = offset;

        // skip offset to directly the end of instructions
        offset = code_start_offset + codes.p[i].size;
    }
}

void WASMDecoder::decode_wasm() {
    // first 8 bytes are magic number and wasm version number
    // currently, in this first version, we are skipping them
    uint32_t index = 8U;

    while (index < wasm_bytes.size()) {
        uint32_t section_id = read_unsigned_num(wasm_bytes, index);
        uint32_t section_size = read_unsigned_num(wasm_bytes, index);
        switch (section_id) {
            case 1U:
                decode_type_section(index);
                // exit(0);
                break;
            case 3U:
                decode_function_section(index);
                // exit(0);
                break;
            case 7U:
                decode_export_section(index);
                // exit(0);
                break;
            case 10U:
                decode_code_section(index);
                // exit(0)
                break;
            default:
                std::cout << "Unknown section id: " << section_id << std::endl;
                break;
        }
        index += section_size;
    }

    LCOMPILERS_ASSERT(index == wasm_bytes.size());
    LCOMPILERS_ASSERT(type_indices.size() == codes.size());
}

std::string WASMDecoder::get_wat() {
    std::string result = "(module";
    for (uint32_t i = 0; i < type_indices.size(); i++) {
        result += "\n    (func $" + std::to_string(i);
        result += "\n        (param";
        uint32_t func_index = type_indices.p[i];
        for (uint32_t j = 0; j < func_types[func_index].param_types.size(); j++) {
            result += " " + var_type_to_string[func_types[func_index].param_types.p[j]];
        }
        result += ") (result";
        for (uint32_t j = 0; j < func_types[func_index].result_types.size(); j++) {
            result += " " + var_type_to_string[func_types[func_index].result_types.p[j]];
        }
        result += ")";
        result += "\n        (local";
        for (uint32_t j = 0; j < codes.p[i].locals.size(); j++) {
            for (uint32_t k = 0; k < codes.p[i].locals.p[j].count; k++) {
                result += " " + var_type_to_string[codes.p[i].locals.p[j].type];
            }
        }
        result += ")";

        {
            WASM_INSTS_VISITOR::WATVisitor v = WASM_INSTS_VISITOR::WATVisitor();
            v.indent = "\n        ";
            v.decode_instructions(wasm_bytes, codes.p[i].insts_start_index);
            result += v.src;
        }

        result += "\n    )";
    }

    for (uint32_t i = 0; i < exports.size(); i++) {
        result += "\n    (export \"" + exports.p[i].name + "\" (" + kind_to_string[exports.p[i].kind] + " $" + std::to_string(exports.p[i].index) + "))";
    }
    result += "\n)\n";

    return result;
}

}  // namespace wasm

}  // namespace LCompilers
