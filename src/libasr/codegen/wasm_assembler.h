#include <cassert>

#include <libasr/alloc.h>
#include <libasr/containers.h>

namespace LFortran {
namespace wasm {

enum type {
    i32 = 0x7F,
    i64 = 0x7E,
    f32 = 0x7D,
    f64 = 0x7C
};

void emit_leb128_u32(Vec<uint8_t> &code, Allocator &al, uint32_t n) { // for u32
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        if (n != 0) {
            byte |= 0x80;
        }
        code.push_back(al, byte);
    } while (n != 0);
}

void emit_leb128_i32(Vec<uint8_t> &code, Allocator &al, int32_t n) { // for i32
    bool more = true;
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        more = !((((n == 0) && ((byte & 0x40) == 0)) ||
                  ((n == -1) && ((byte & 0x40) != 0))));
        if (more) {
            byte |= 0x80;
        }
        code.push_back(al, byte);
    } while (more);
}

void emit_leb128_i64(Vec<uint8_t> &code, Allocator &al, int64_t n) { // for i64
    bool more = true;
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        more = !((((n == 0) && ((byte & 0x40) == 0)) ||
                  ((n == -1) && ((byte & 0x40) != 0))));
        if (more) {
            byte |= 0x80;
        }
        code.push_back(al, byte);
    } while (more);
}

void emit_ieee754_f32(Vec<uint8_t> &code, Allocator &al, float z) { // for f32
    uint8_t encoded_float[sizeof(z)];
    std::memcpy(&encoded_float, &z, sizeof(z));
    for(auto &byte:encoded_float){
        code.push_back(al, byte);
    }
}

void emit_ieee754_f64(Vec<uint8_t> &code, Allocator &al, double z) { // for f64
    uint8_t encoded_float[sizeof(z)];
    std::memcpy(&encoded_float, &z, sizeof(z));
    for(auto &byte:encoded_float){
        code.push_back(al, byte);
    }
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

// function to append a given bytecode to the end of the code
void emit_b8(Vec<uint8_t> &code, Allocator &al, uint8_t x) {
    code.push_back(al, x);
}

// function to emit unsigned 32 bit integer
void emit_u32(Vec<uint8_t> &code, Allocator &al, uint32_t x) {
    emit_leb128_u32(code, al, x);
}

// function to emit signed 32 bit integer
void emit_i32(Vec<uint8_t> &code, Allocator &al, int32_t x) {
    emit_leb128_i32(code, al, x);
}

// function to emit signed 64 bit integer
void emit_i64(Vec<uint8_t> &code, Allocator &al, int64_t x) {
    emit_leb128_i64(code, al, x);
}

// function to emit 32 bit float
void emit_f32(Vec<uint8_t> &code, Allocator &al, float x) {
    emit_ieee754_f32(code, al, x);
}

// function to emit 64 bit float
void emit_f64(Vec<uint8_t> &code, Allocator &al, double x) {
    emit_ieee754_f64(code, al, x);
}

// function to emit string
void emit_str(Vec<uint8_t> &code, Allocator &al, std::string text){
    std::vector<uint8_t> text_bytes(text.size());
    std::memcpy(text_bytes.data(), text.data(), text.size());
    emit_u32(code, al, text_bytes.size());
    for(auto &byte:text_bytes)
        emit_b8(code, al, byte);
}

void emit_u32_b32_idx(Vec<uint8_t> &code, Allocator &al, uint32_t idx,
                      uint32_t section_size) {
    /*
    Encodes the integer `i` using LEB128 and adds trailing zeros to always
    occupy 4 bytes. Stores the int `i` at the index `idx` in `code`.
    */
    Vec<uint8_t> num;
    num.reserve(al, 4);
    emit_leb128_u32(num, al, section_size);
    std::vector<uint8_t> num_4b = {0x80, 0x80, 0x80, 0x00};
    assert(num.size() <= 4);
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

void emit_export_fn(Vec<uint8_t> &code, Allocator &al, const std::string& name,
                    uint32_t idx) {
    emit_str(code, al, name);
    emit_b8(code, al, 0x00); // for exporting function
    emit_u32(code, al, idx);
}

void emit_import_fn(Vec<uint8_t> &code, Allocator &al, const std::string &mod_name,
                const std::string& fn_name, uint32_t type_idx) 
{
    emit_str(code, al, mod_name);
    emit_str(code, al, fn_name);
    emit_b8(code, al, 0x00); // for importing function
    emit_u32(code, al, type_idx);
}

void emit_import_mem(Vec<uint8_t> &code, Allocator &al, const std::string &mod_name,
                 const std::string& mem_name, uint32_t min_limit, uint32_t /* max_limit */) {
    emit_str(code, al, mod_name);
    emit_str(code, al, mem_name);
    emit_b8(code, al, 0x02); // for importing memory
    emit_b8(code, al, 0x00); // for specifying min page limit of memory
    // max page limit can also be specifid, but currently omitting it.
    emit_u32(code, al, min_limit);
}

void encode_section(Vec<uint8_t> &des, Vec<uint8_t> &section_content, Allocator &al, uint32_t section_id){
    // every section in WebAssembly is encoded by adding its section id, followed by the content size and lastly the contents
    emit_u32(des, al, section_id);
    emit_u32(des, al, section_content.size());
    for(auto &byte:section_content){
        des.push_back(al, byte);
    }
}

// function to emit get local variable at given index
void emit_get_local(Vec<uint8_t> &code, Allocator &al, uint32_t idx) {
    code.push_back(al, 0x20);
    emit_u32(code, al, idx);
}

// function to emit set local variable at given index
void emit_set_local(Vec<uint8_t> &code, Allocator &al, uint32_t idx) {
    code.push_back(al, 0x21);
    emit_u32(code, al, idx);
}

// function to emit call instruction
void emit_call(Vec<uint8_t> &code, Allocator &al, uint32_t idx) {
    code.push_back(al, 0x10);
    emit_u32(code, al, idx);
}

// function to emit end of wasm expression
void emit_expr_end(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x0B);
}

/**************************** Integer Operations ****************************/

// function to emit a i32.const instruction
void emit_i32_const(Vec<uint8_t> &code, Allocator &al, int32_t x) {
    code.push_back(al, 0x41);
    emit_i32(code, al, x);
}

// function to emit i32.clz instruction
void emit_i32_clz(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x67); }

// function to emit i32.ctz instruction
void emit_i32_ctz(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x68); }

// function to emit i32.popcnt instruction
void emit_i32_popcnt(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x69); }

// function to emit i32.add instruction
void emit_i32_add(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x6A); }

// function to emit i32.sub instruction
void emit_i32_sub(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x6B); }

// function to emit i32.mul instruction
void emit_i32_mul(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x6C); }

// function to emit i32.div_s instruction
void emit_i32_div_s(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x6D); }

// function to emit i32.div_u instruction
void emit_i32_div_u(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x6E); }

// function to emit i32.rem_s instruction
void emit_i32_rem_s(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x6F); }

// function to emit i32.rem_u instruction
void emit_i32_rem_u(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x70); }

// function to emit i32.and instruction
void emit_i32_and(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x71); }

// function to emit i32.or instruction
void emit_i32_or(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x72); }

// function to emit i32.xor instruction
void emit_i32_xor(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x73); }

// function to emit i32.shl instruction
void emit_i32_shl(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x74); }

// function to emit i32.shr_s instruction
void emit_i32_shr_s(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x75); }

// function to emit i32.shr_u instruction
void emit_i32_shr_u(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x76); }

// function to emit i32.rotl instruction
void emit_i32_rotl(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x77); }

// function to emit i32.rotr instruction
void emit_i32_rotr(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x78); }


// function to emit a i64.const instruction
void emit_i64_const(Vec<uint8_t> &code, Allocator &al, int64_t x) {
    code.push_back(al, 0x42);
    emit_i64(code, al, x);
}

// function to emit i64.clz instruction
void emit_i64_clz(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x79); }

// function to emit i64.ctz instruction
void emit_i64_ctz(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x7A); }

// function to emit i64.popcnt instruction
void emit_i64_popcnt(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x7B); }

// function to emit i64.add instruction
void emit_i64_add(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x7C); }

// function to emit i64.sub instruction
void emit_i64_sub(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x7D); }

// function to emit i64.mul instruction
void emit_i64_mul(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x7E); }

// function to emit i64.div_s instruction
void emit_i64_div_s(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x7F); }

// function to emit i64.div_u instruction
void emit_i64_div_u(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x80); }

// function to emit i64.rem_s instruction
void emit_i64_rem_s(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x81); }

// function to emit i64.rem_u instruction
void emit_i64_rem_u(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x82); }

// function to emit i64.and instruction
void emit_i64_and(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x83); }

// function to emit i64.or instruction
void emit_i64_or(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x84); }

// function to emit i64.xor instruction
void emit_i64_xor(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x85); }

// function to emit i64.shl instruction
void emit_i64_shl(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x86); }

// function to emit i64.shr_s instruction
void emit_i64_shr_s(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x87); }

// function to emit i64.shr_u instruction
void emit_i64_shr_u(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x88); }

// function to emit i64.rotl instruction
void emit_i64_rotl(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x89); }

// function to emit i64.rotr instruction
void emit_i64_rotr(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x8A); }



/**************************** Floating Point Operations ****************************/

// function to emit a f32.const instruction
void emit_f32_const(Vec<uint8_t> &code, Allocator &al, float x) {
    code.push_back(al, 0x43);
    emit_f32(code, al, x);
}

// function to emit f32.abs instruction
void emit_f32_abs(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x8B); }

// function to emit f32.neg instruction
void emit_f32_neg(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x8C); }

// function to emit f32.ceil instruction
void emit_f32_ceil(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x8D); }

// function to emit f32.floor instruction
void emit_f32_floor(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x8E); }

// function to emit f32.trunc instruction
void emit_f32_trunc(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x8F); }

// function to emit f32.nearest instruction
void emit_f32_nearest(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x90); }

// function to emit f32.sqrt instruction
void emit_f32_sqrt(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x91); }

// function to emit f32.add instruction
void emit_f32_add(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x92); }

// function to emit f32.sub instruction
void emit_f32_sub(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x93); }

// function to emit f32.mul instruction
void emit_f32_mul(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x94); }

// function to emit f32.div instruction
void emit_f32_div(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x95); }

// function to emit f32.min instruction
void emit_f32_min(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x96); }

// function to emit f32.max instruction
void emit_f32_max(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x97); }

// function to emit f32.copysign instruction
void emit_f32_copysign(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x98); }


// function to emit a f64.const instruction
void emit_f64_const(Vec<uint8_t> &code, Allocator &al, double x) {
    code.push_back(al, 0x44);
    emit_f64(code, al, x);
}

// function to emit f64.abs instruction
void emit_f64_abs(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x99); }

// function to emit f64.neg instruction
void emit_f64_neg(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x9A); }

// function to emit f64.ceil instruction
void emit_f64_ceil(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x9B); }

// function to emit f64.floor instruction
void emit_f64_floor(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x9C); }

// function to emit f64.trunc instruction
void emit_f64_trunc(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x9D); }

// function to emit f64.nearest instruction
void emit_f64_nearest(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x9E); }

// function to emit f64.sqrt instruction
void emit_f64_sqrt(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0x9F); }

// function to emit f64.add instruction
void emit_f64_add(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0xA0); }

// function to emit f64.sub instruction
void emit_f64_sub(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0xA1); }

// function to emit f64.mul instruction
void emit_f64_mul(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0xA2); }

// function to emit f64.div instruction
void emit_f64_div(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0xA3); }

// function to emit f64.min instruction
void emit_f64_min(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0xA4); }

// function to emit f64.max instruction
void emit_f64_max(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0xA5); }

// function to emit f64.copysign instruction
void emit_f64_copysign(Vec<uint8_t> &code, Allocator &al) { code.push_back(al, 0xA6); }


// function to emit string
void emit_str_const(Vec<uint8_t> &code, Allocator &al, uint32_t mem_idx, const std::string &text) {
    emit_u32(code, al, 0U); // for active mode of memory with default mem_idx of 0
    emit_i32_const(code, al, (int32_t)mem_idx); // specifying memory location as instructions
    emit_expr_end(code, al); // end instructions
    emit_str(code, al, text);
}


void emit_unreachable(Vec<uint8_t> &code, Allocator &al){
    code.push_back(al, 0x00);
}

void save_js_glue(std::string filename){
    std::string js_glue = 
R"(function define_imports(memory, outputBuffer, stdout_print) {
    const printNum = (num) => outputBuffer.push(num.toString());
    const printStr = (startIdx, strSize) => outputBuffer.push(
        new TextDecoder("utf8").decode(new Uint8Array(memory.buffer, startIdx, strSize)));
    const flushBuffer = () => {
        stdout_print(outputBuffer.join(" ") + "\n");
        outputBuffer.length = 0;
    }
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
        },
    };
    return imports;
}

async function run_wasm(bytes, imports) {
    var res;
    try { res = await WebAssembly.instantiate(bytes, imports); }
    catch (e) { console.log(e); return 1 /* non-zero return value */; }
    const { _lcompilers_main } = res.instance.exports;
    try { _lcompilers_main() }
    catch (e) { return 1; }
    return 0;
}

async function execute_code(bytes, stdout_print) {
    var outputBuffer = [];
    var memory = new WebAssembly.Memory({ initial: 10, maximum: 100 }); // initial 640Kb and max 6.4Mb
    var imports = define_imports(memory, outputBuffer, stdout_print);
    const exec_status = await run_wasm(bytes, imports);
    return (exec_status ? outputBuffer[0] : 0); // the last element denotes the actual execution status
}

const fs = require("fs");
const wasmBuffer = fs.readFileSync(")" + filename + 
R"(");
execute_code(wasmBuffer, (text) => process.stdout.write(text)).then().catch((e) => console.log(e))
)";
    filename += ".js";
    std::ofstream out(filename);
    out << js_glue;
    out.close();
}

void save_bin(Vec<uint8_t> &code, std::string filename){
    std::ofstream out(filename);
    out.write((const char*) code.p, code.size());
    out.close();
    save_js_glue(filename);
}

}  // namespace wasm

}  // namespace LFortran
