#ifndef LFORTRAN_WASM_DECODER_H
#define LFORTRAN_WASM_DECODER_H

#include <fstream>

#include <libasr/assert.h>
#include <libasr/alloc.h>
#include <libasr/containers.h>
#include <libasr/codegen/wasm_utils.h>

// #define WAT_DEBUG

#ifdef WAT_DEBUG
#define DEBUG(s) std::cout << s << std::endl
#else
#define DEBUG(s)
#endif

namespace LFortran {

namespace {

// This exception is used to abort the visitor pattern when an error occurs.
class CodeGenAbort {};

// Local exception that is only used in this file to exit the visitor
// pattern and caught later (not propagated outside)
class CodeGenError {
   public:
    diag::Diagnostic d;

   public:
    CodeGenError(const std::string &msg)
        : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen)} {}

    CodeGenError(const std::string &msg, const Location &loc)
        : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen,
                             {diag::Label("", {loc})})} {}
};

}  // namespace

namespace wasm {

template <class Struct>
class WASMDecoder {
   private:
    Struct &self() { return static_cast<Struct &>(*this); }

   public:
    std::unordered_map<uint8_t, std::string> var_type_to_string;
    std::unordered_map<uint8_t, std::string> kind_to_string;

    Allocator &al;
    diag::Diagnostics &diag;
    Vec<uint8_t> wasm_bytes;
    size_t PREAMBLE_SIZE;

    Vec<wasm::FuncType> func_types;
    Vec<wasm::Import> imports;
    Vec<uint32_t> type_indices;
    Vec<wasm::Export> exports;
    Vec<wasm::Code> codes;
    Vec<wasm::Data> data_segments;

    WASMDecoder(Allocator &al, diag::Diagnostics &diagonostics)
        : al(al), diag(diagonostics) {
        var_type_to_string = {
            {0x7F, "i32"}, {0x7E, "i64"}, {0x7D, "f32"}, {0x7C, "f64"}};
        kind_to_string = {
            {0x00, "func"}, {0x01, "table"}, {0x02, "mem"}, {0x03, "global"}};

        PREAMBLE_SIZE = 8 /* BYTES */;
        // wasm_bytes.reserve(al, 1024 * 128);
        // func_types.reserve(al, 1024 * 128);
        // type_indices.reserve(al, 1024 * 128);
        // exports.reserve(al, 1024 * 128);
        // codes.reserve(al, 1024 * 128);
    }

    void load_file(std::string filename) {
        std::ifstream file(filename, std::ios::binary);
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        wasm_bytes.reserve(al, size);
        file.read((char *)wasm_bytes.data(), size);
        file.close();
    }

    bool is_preamble_ok(uint32_t offset) {
        uint8_t expected_preamble[] = {0x00, 0x61, 0x73, 0x6D,
                                       0x01, 0x00, 0x00, 0x00};
        for (size_t i = 0; i < PREAMBLE_SIZE; i++) {
            uint8_t cur_byte = read_b8(wasm_bytes, offset);
            if (cur_byte != expected_preamble[i]) {
                return false;
            }
        }
        return true;
    }

    void decode_type_section(uint32_t offset) {
        // read type section contents
        uint32_t no_of_func_types = read_u32(wasm_bytes, offset);
        DEBUG("no_of_func_types: " + std::to_string(no_of_func_types));
        func_types.resize(al, no_of_func_types);

        for (uint32_t i = 0; i < no_of_func_types; i++) {
            if (wasm_bytes[offset] != 0x60) {
                throw CodeGenError("Invalid type section");
            }
            offset++;

            // read result type 1
            uint32_t no_of_params = read_u32(wasm_bytes, offset);
            func_types.p[i].param_types.resize(al, no_of_params);

            for (uint32_t j = 0; j < no_of_params; j++) {
                func_types.p[i].param_types.p[j] = read_b8(wasm_bytes, offset);
            }

            uint32_t no_of_results = read_u32(wasm_bytes, offset);
            func_types.p[i].result_types.resize(al, no_of_results);

            for (uint32_t j = 0; j < no_of_results; j++) {
                func_types.p[i].result_types.p[j] = read_b8(wasm_bytes, offset);
            }
        }
    }

    void decode_imports_section(uint32_t offset) {
        // read imports section contents
        uint32_t no_of_imports = read_u32(wasm_bytes, offset);
        DEBUG("no_of_imports: " + std::to_string(no_of_imports));
        imports.resize(al, no_of_imports);

        for (uint32_t i = 0; i < no_of_imports; i++) {
            uint32_t mod_name_size = read_u32(wasm_bytes, offset);
            imports.p[i].mod_name.resize(
                mod_name_size);  // do not pass al to this resize as it is
                                 // std::string.resize()
            for (uint32_t j = 0; j < mod_name_size; j++) {
                imports.p[i].mod_name[j] = read_b8(wasm_bytes, offset);
            }

            uint32_t name_size = read_u32(wasm_bytes, offset);
            imports.p[i].name.resize(
                name_size);  // do not pass al to this resize as it is
                             // std::string.resize()
            for (uint32_t j = 0; j < name_size; j++) {
                imports.p[i].name[j] = read_b8(wasm_bytes, offset);
            }

            imports.p[i].kind = read_b8(wasm_bytes, offset);

            switch (imports.p[i].kind) {
                case 0x00: {
                    imports.p[i].type_idx = read_u32(wasm_bytes, offset);
                    break;
                }
                case 0x02: {
                    uint8_t byte = read_b8(wasm_bytes, offset);
                    if (byte == 0x00) {
                        imports.p[i].mem_page_size_limits.first =
                            read_u32(wasm_bytes, offset);
                        imports.p[i].mem_page_size_limits.second =
                            imports.p[i].mem_page_size_limits.first;
                    } else {
                        LFORTRAN_ASSERT(byte == 0x01);
                        imports.p[i].mem_page_size_limits.first =
                            read_u32(wasm_bytes, offset);
                        imports.p[i].mem_page_size_limits.second =
                            read_u32(wasm_bytes, offset);
                    }
                    break;
                }

                default: {
                    throw CodeGenError(
                        "Only importing functions and memory are currently "
                        "supported");
                }
            }
        }
    }

    void decode_function_section(uint32_t offset) {
        // read function section contents
        uint32_t no_of_indices = read_u32(wasm_bytes, offset);
        DEBUG("no_of_indices: " + std::to_string(no_of_indices));
        type_indices.resize(al, no_of_indices);

        for (uint32_t i = 0; i < no_of_indices; i++) {
            type_indices.p[i] = read_u32(wasm_bytes, offset);
        }
    }

    void decode_export_section(uint32_t offset) {
        // read export section contents
        uint32_t no_of_exports = read_u32(wasm_bytes, offset);
        DEBUG("no_of_exports: " + std::to_string(no_of_exports));
        exports.resize(al, no_of_exports);

        for (uint32_t i = 0; i < no_of_exports; i++) {
            uint32_t name_size = read_u32(wasm_bytes, offset);
            exports.p[i].name.resize(
                name_size);  // do not pass al to this resize as it is
                             // std::string.resize()
            for (uint32_t j = 0; j < name_size; j++) {
                exports.p[i].name[j] = read_b8(wasm_bytes, offset);
            }
            DEBUG("export name: " + exports.p[i].name);
            exports.p[i].kind = read_b8(wasm_bytes, offset);
            DEBUG("export kind: " + std::to_string(exports.p[i].kind));
            exports.p[i].index = read_u32(wasm_bytes, offset);
            DEBUG("export index: " + std::to_string(exports.p[i].index));
        }
    }

    void decode_code_section(uint32_t offset) {
        // read code section contents
        uint32_t no_of_codes = read_u32(wasm_bytes, offset);
        DEBUG("no_of_codes: " + std::to_string(no_of_codes));
        codes.resize(al, no_of_codes);

        for (uint32_t i = 0; i < no_of_codes; i++) {
            codes.p[i].size = read_u32(wasm_bytes, offset);
            uint32_t code_start_offset = offset;
            uint32_t no_of_locals = read_u32(wasm_bytes, offset);
            DEBUG("no_of_locals: " + std::to_string(no_of_locals));
            codes.p[i].locals.resize(al, no_of_locals);

            DEBUG("Entering loop");
            for (uint32_t j = 0U; j < no_of_locals; j++) {
                codes.p[i].locals.p[j].count = read_u32(wasm_bytes, offset);
                DEBUG("count: " + std::to_string(codes.p[i].locals.p[j].count));
                codes.p[i].locals.p[j].type = read_b8(wasm_bytes, offset);
                DEBUG("type: " + std::to_string(codes.p[i].locals.p[j].type));
            }
            DEBUG("Exiting loop");

            codes.p[i].insts_start_index = offset;

            // skip offset to directly the end of instructions
            offset = code_start_offset + codes.p[i].size;
        }
    }

    void decode_data_section(uint32_t offset) {
        // read code section contents
        uint32_t no_of_data_segments = read_u32(wasm_bytes, offset);
        DEBUG("no_of_data_segments: " + std::to_string(no_of_data_segments));
        data_segments.resize(al, no_of_data_segments);

        for (uint32_t i = 0; i < no_of_data_segments; i++) {
            uint32_t num = read_u32(wasm_bytes, offset);
            if (num != 0) {
                throw CodeGenError(
                    "Only active default memory (index = 0) is currently "
                    "supported");
            }

            data_segments.p[i].insts_start_index = offset;
            while (read_b8(wasm_bytes, offset) != 0x0B)
                ;

            uint32_t text_size = read_u32(wasm_bytes, offset);
            data_segments.p[i].text.resize(
                text_size);  // do not pass al to this resize as it is
                             // std::string.resize()
            for (uint32_t j = 0; j < text_size; j++) {
                data_segments.p[i].text[j] = read_b8(wasm_bytes, offset);
            }
        }
    }
    void decode_wasm() {
        // first 8 bytes are magic number and wasm version number
        uint32_t index = 0;
        if (!is_preamble_ok(index)) {
            std::cerr << "Unexpected Preamble: ";
            for (size_t i = 0; i < PREAMBLE_SIZE; i++) {
                fprintf(stderr, "0x%.02X, ", wasm_bytes[i]);
            }
            throw CodeGenError(
                "Expected: 0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00");
        }
        index += PREAMBLE_SIZE;
        while (index < wasm_bytes.size()) {
            uint32_t section_id = read_u32(wasm_bytes, index);
            uint32_t section_size = read_u32(wasm_bytes, index);
            switch (section_id) {
                case 1U:
                    decode_type_section(index);
                    // exit(0);
                    break;
                case 2U:
                    decode_imports_section(index);
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
                case 11U:
                    decode_data_section(index);
                    // exit(0)
                    break;
                default:
                    std::cout << "Unknown section id: " << section_id
                              << std::endl;
                    break;
            }
            index += section_size;
        }

        LFORTRAN_ASSERT(index == wasm_bytes.size());
        LFORTRAN_ASSERT(type_indices.size() == codes.size());
    }
};

}  // namespace wasm
}  // namespace LFortran

#endif  // LFORTRAN_WASM_DECODER_H
