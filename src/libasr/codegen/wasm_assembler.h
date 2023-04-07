#include <libasr/assert.h>
#include <libasr/codegen/wasm_utils.h>

namespace LCompilers {

namespace wasm {

enum type { i32 = 0x7F, i64 = 0x7E, f32 = 0x7D, f64 = 0x7C };

enum mem_align { b8 = 0, b16 = 1, b32 = 2, b64 = 3 };

void emit_expr_end(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x0B);
}

// function to emit header of Wasm Binary Format
void emit_header(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x00);
    code.push_back(al, 0x61);
    code.push_back(al, 0x73);
    code.push_back(al, 0x6D);
    code.push_back(al, 0x01);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
}

// function to emit string
void emit_str(Vec<uint8_t> &code, Allocator &al, std::string text) {
    std::vector<uint8_t> text_bytes(text.size());
    std::memcpy(text_bytes.data(), text.data(), text.size());
    emit_u32(code, al, text_bytes.size());
    for (auto &byte : text_bytes) emit_b8(code, al, byte);
}

void emit_str_const(Vec<uint8_t> &code, Allocator &al, uint32_t mem_idx,
                    const std::string &text) {
    emit_u32(code, al, 0U);  // for active mode of memory with default mem_idx of 0
    emit_b8(code, al, 0x41); // i32.const
    emit_i32(code, al, (int32_t)mem_idx); // specifying memory location
    emit_expr_end(code, al); // end instructions
    emit_str(code, al, text);
}

void emit_u32_b32_idx(Vec<uint8_t> &code, Allocator &al, uint32_t idx,
                      uint32_t section_size) {
    /*
    Encodes the integer `i` using LEB128 and adds trailing zeros to always
    occupy 4 bytes. Stores the int `i` at the index `idx` in `code`.
    */
    Vec<uint8_t> num;
    num.reserve(al, 4);
    encode_leb128_u32(num, al, section_size);
    std::vector<uint8_t> num_4b = {0x80, 0x80, 0x80, 0x00};
    LCOMPILERS_ASSERT(num.size() <= 4);
    for (uint32_t i = 0; i < num.size(); i++) {
        num_4b[i] |= num[i];
    }
    for (uint32_t i = 0; i < 4u; i++) {
        code.p[idx + i] = num_4b[i];
    }
}

// function to fixup length at the given length index
void fixup_len(Vec<uint8_t> &code, Allocator &al, uint32_t len_idx) {
    uint32_t section_len = code.size() - len_idx - 4u;
    emit_u32_b32_idx(code, al, len_idx, section_len);
}

// function to emit length placeholder
uint32_t emit_len_placeholder(Vec<uint8_t> &code, Allocator &al) {
    uint32_t len_idx = code.size();
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    return len_idx;
}

void emit_export_fn(Vec<uint8_t> &code, Allocator &al, const std::string &name,
                    uint32_t idx) {
    emit_str(code, al, name);
    emit_b8(code, al, 0x00);  // for exporting function
    emit_u32(code, al, idx);
}

void emit_import_fn(Vec<uint8_t> &code, Allocator &al,
                    const std::string &mod_name, const std::string &fn_name,
                    uint32_t type_idx) {
    emit_str(code, al, mod_name);
    emit_str(code, al, fn_name);
    emit_b8(code, al, 0x00);  // for importing function
    emit_u32(code, al, type_idx);
}

void emit_declare_mem(Vec<uint8_t> &code, Allocator &al,
                     uint32_t min_no_pages, uint32_t max_no_pages = 0) {
    if (max_no_pages > 0) {
        emit_b8(code, al,
                0x01);  // for specifying min and max page limits of memory
        emit_u32(code, al, min_no_pages);
        emit_u32(code, al, max_no_pages);
    } else {
        emit_b8(code, al,
                0x00);  // for specifying only min page limit of memory
        emit_u32(code, al, min_no_pages);
    }
}

void emit_import_mem(Vec<uint8_t> &code, Allocator &al,
                     const std::string &mod_name, const std::string &mem_name,
                     uint32_t min_no_pages, uint32_t max_no_pages = 0) {
    emit_str(code, al, mod_name);
    emit_str(code, al, mem_name);
    emit_b8(code, al, 0x02);  // for importing memory
    if (max_no_pages > 0) {
        emit_b8(code, al,
                0x01);  // for specifying min and max page limits of memory
        emit_u32(code, al, min_no_pages);
        emit_u32(code, al, max_no_pages);
    } else {
        emit_b8(code, al,
                0x00);  // for specifying only min page limit of memory
        emit_u32(code, al, min_no_pages);
    }
}

void emit_export_mem(Vec<uint8_t> &code, Allocator &al, const std::string &name,
                    uint32_t idx) {
    emit_str(code, al, name);
    emit_b8(code, al, 0x02);  // for exporting memory
    emit_u32(code, al, idx);
}

void encode_section(Vec<uint8_t> &des, Vec<uint8_t> &section_content,
                    Allocator &al, uint32_t section_id,
                    uint32_t no_of_elements) {
    // every section in WebAssembly is encoded by adding its section id,
    // followed by the content size and lastly the contents
    emit_u32(des, al, section_id);
    emit_u32(des, al, 4U /* size of no_of_elements */ + section_content.size());
    uint32_t len_idx = emit_len_placeholder(des, al);
    emit_u32_b32_idx(des, al, len_idx, no_of_elements);
    for (auto &byte : section_content) {
        des.push_back(al, byte);
    }
}

void save_js_glue(std::string filename) {
    std::string js_glue =
        R"(function define_imports(memory, outputBuffer, exit_code, stdout_print) {
    const printNum = (num) => outputBuffer.push(num.toString());
    const printStr = (startIdx, strSize) => outputBuffer.push(
        new TextDecoder("utf8").decode(new Uint8Array(memory.buffer, startIdx, strSize)));
    const flushBuffer = () => {
        stdout_print(outputBuffer.join(" ") + "\n");
        outputBuffer.length = 0;
    }
    const set_exit_code = (exit_code_val) => exit_code.val = exit_code_val;
    const cpu_time = (time) => (Date.now() / 1000); // Date.now() returns milliseconds, so divide by 1000
    var imports = {
        js: {
            memory: memory,
            /* functions */
            print_i32: printNum,
            print_i64: printNum,
            print_f32: printNum,
            print_f64: printNum,
            print_str: printStr,
            flush_buf: flushBuffer,
            set_exit_code: set_exit_code,
            cpu_time: cpu_time
        },
    };
    return imports;
}

async function run_wasm(bytes, imports) {
    try {
        var res = await WebAssembly.instantiate(bytes, imports);
        const { _lcompilers_main } = res.instance.exports;
        _lcompilers_main();
    } catch(e) { console.log(e); }
}

async function execute_code(bytes, stdout_print) {
    var exit_code = {val: 1}; /* non-zero exit code */
    var outputBuffer = [];
    var memory = new WebAssembly.Memory({ initial: 100, maximum: 100 }); // fixed 6.4 Mb memory currently
    var imports = define_imports(memory, outputBuffer, exit_code, stdout_print);
    await run_wasm(bytes, imports);
    return exit_code.val;
}

function main() {
    const fs = require("fs");
    const wasmBuffer = fs.readFileSync(")" +
        filename + R"(");
    execute_code(wasmBuffer, (text) => process.stdout.write(text))
        .then((exit_code) => {
            process.exit(exit_code);
        })
        .catch((e) => console.log(e))
}

main();
)";
    filename += ".js";
    std::ofstream out(filename);
    out << js_glue;
    out.close();
}

void save_js_glue_wasi(std::string filename) {
    std::string js_glue =
R"(async function main() {
    const fs = require("fs");
    const { WASI } = require("wasi");
    const wasi = new WASI();
    const importObject = {
        wasi_snapshot_preview1: wasi.wasiImport,
        js: {
            cpu_time: (time) => (Date.now() / 1000) // Date.now() returns milliseconds, so divide by 1000
        }
    };
    const wasm = await WebAssembly.compile(fs.readFileSync(")" + filename + R"("));
    const instance = await WebAssembly.instantiate(wasm, importObject);
    wasi.start(instance);
}
main();
)";
    filename += ".js";
    std::ofstream out(filename);
    out << js_glue;
    out.close();
}

void save_bin(Vec<uint8_t> &code, std::string filename) {
    std::ofstream out(filename);
    out.write((const char *)code.p, code.size());
    out.close();
    save_js_glue_wasi(filename);
}

}  // namespace wasm

}  // namespace LCompilers
