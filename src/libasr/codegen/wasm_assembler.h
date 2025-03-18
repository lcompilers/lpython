#include <libasr/assert.h>
#include <libasr/codegen/wasm_utils.h>
#include <libasr/wasm_visitor.h>
#include <fstream>

namespace LCompilers {

namespace wasm {

void emit_expr_end(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x0B);
}

// function to emit string
void emit_str(Vec<uint8_t> &code, Allocator &al, std::string text) {
    std::vector<uint8_t> text_bytes(text.size());
    std::memcpy(text_bytes.data(), text.data(), text.size());
    emit_u32(code, al, text_bytes.size());
    for (auto &byte : text_bytes) emit_b8(code, al, byte);
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

class WASMAssembler: public WASM_INSTS_VISITOR::WASMInstsAssembler<WASMAssembler> {
  private:
    Allocator &m_al;

    Vec<uint8_t> m_type_section;
    Vec<uint8_t> m_import_section;
    Vec<uint8_t> m_func_section;
    Vec<uint8_t> m_memory_section;
    Vec<uint8_t> m_global_section;
    Vec<uint8_t> m_export_section;
    Vec<uint8_t> m_code_section;
    Vec<uint8_t> m_data_section;

    // no_of_types indicates total (imported + defined) no of functions
    uint32_t no_of_types;
    uint32_t no_of_functions;
    uint32_t no_of_memories;
    uint32_t no_of_globals;
    uint32_t no_of_exports;
    uint32_t no_of_imports;
    uint32_t no_of_data_segs;

  public:

    int nest_lvl;
    int cur_loop_nest_lvl;

    WASMAssembler(Allocator &al) : WASMInstsAssembler(al, m_code_section), m_al(al) {
        nest_lvl = 0;
        cur_loop_nest_lvl = 0;

        no_of_types = 0;
        no_of_functions = 0;
        no_of_memories = 0;
        no_of_globals = 0;
        no_of_exports = 0;
        no_of_imports = 0;
        no_of_data_segs = 0;

        m_type_section.reserve(m_al, 1024 * 128);
        m_import_section.reserve(m_al, 1024 * 128);
        m_func_section.reserve(m_al, 1024 * 128);
        m_memory_section.reserve(m_al, 1024 * 128);
        m_global_section.reserve(m_al, 1024 * 128);
        m_export_section.reserve(m_al, 1024 * 128);
        m_code_section.reserve(m_al, 1024 * 128);
        m_data_section.reserve(m_al, 1024 * 128);
    }

    uint32_t get_no_of_types() {
        return no_of_types;
    }

    // function to emit header of Wasm Binary Format
    void emit_header(Vec<uint8_t> &code) {
        code.push_back(m_al, 0x00);
        code.push_back(m_al, 0x61);
        code.push_back(m_al, 0x73);
        code.push_back(m_al, 0x6D);
        code.push_back(m_al, 0x01);
        code.push_back(m_al, 0x00);
        code.push_back(m_al, 0x00);
        code.push_back(m_al, 0x00);
    }

    Vec<uint8_t> get_wasm() {
        Vec<uint8_t> code;
        code.reserve(m_al, 8U /* preamble size */ +
                               8U /* (section id + section size) */ *
                                   8U /* number of sections */
                               + m_type_section.size() +
                               m_import_section.size() + m_func_section.size() +
                               m_memory_section.size() + m_global_section.size() +
                               m_export_section.size() + m_code_section.size() +
                               m_data_section.size());

        emit_header(code);  // emit header and version
        encode_section(code, m_type_section, 1U, no_of_types);
        encode_section(code, m_import_section, 2U, no_of_imports);
        encode_section(code, m_func_section, 3U, no_of_functions);
        encode_section(code, m_memory_section, 5U, no_of_memories);
        encode_section(code, m_global_section, 6U, no_of_globals);
        encode_section(code, m_export_section, 7U, no_of_exports);
        encode_section(code, m_code_section, 10U, no_of_functions);
        encode_section(code, m_data_section, 11U, no_of_data_segs);
        return code;
    }

    void emit_if_else(std::function<void()> test_cond, std::function<void()> if_block, std::function<void()> else_block) {
        test_cond();
        wasm::emit_b8(m_code_section, m_al, 0x04);  // emit if start
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type
        nest_lvl++;
        if_block();
        wasm::emit_b8(m_code_section, m_al, 0x05);  // starting of else
        else_block();
        nest_lvl--;
        wasm::emit_expr_end(m_code_section, m_al); // instructions end
    }

    void emit_loop(std::function<void()> test_cond, std::function<void()> loop_block) {
        uint32_t prev_cur_loop_nest_lvl = cur_loop_nest_lvl;
        cur_loop_nest_lvl = nest_lvl;

        wasm::emit_b8(m_code_section, m_al, 0x03);  // emit loop start
        wasm::emit_b8(m_code_section, m_al, 0x40);  // empty block type

        nest_lvl++;

        emit_if_else(test_cond, [&](){
            loop_block();
            // From WebAssembly Docs:
            // Unlike with other index spaces, indexing of labels is relative by
            // nesting depth, that is, label 0 refers to the innermost structured
            // control instruction enclosing the referring branch instruction, while
            // increasing indices refer to those farther out.
            emit_br(nest_lvl - cur_loop_nest_lvl - 1);  // emit_branch and label the loop
        }, [&](){});

        nest_lvl--;
        wasm::emit_expr_end(m_code_section, m_al); // instructions end
        cur_loop_nest_lvl = prev_cur_loop_nest_lvl;
    }

    uint32_t emit_func_type(std::vector<wasm::var_type> &params, std::vector<wasm::var_type> &results) {
        wasm::emit_b8(m_type_section, m_al, 0x60);

        wasm::emit_u32(m_type_section, m_al, params.size());
        for (auto param:params) {
           wasm::emit_b8(m_type_section, m_al, param);
        }

        wasm::emit_u32(m_type_section, m_al, results.size());
        for (auto result:results) {
           wasm::emit_b8(m_type_section, m_al, result);
        }

        return no_of_types++;
    }

    void emit_import_fn(const std::string &mod_name, const std::string &fn_name,
                    uint32_t type_idx) {
        wasm::emit_str(m_import_section, m_al, mod_name);
        wasm::emit_str(m_import_section, m_al, fn_name);
        wasm::emit_b8(m_import_section, m_al, 0x00);  // for importing function
        wasm::emit_u32(m_import_section, m_al, type_idx);
        no_of_imports++;
    }

    void emit_export_fn(const std::string &name, uint32_t idx) {
        wasm::emit_str(m_export_section, m_al, name);
        wasm::emit_b8(m_export_section, m_al, 0x00);  // for exporting function
        wasm::emit_u32(m_export_section, m_al, idx);
        no_of_exports++;
    }

    void emit_declare_mem(uint32_t min_no_pages, uint32_t max_no_pages = 0) {
        if (max_no_pages > 0) {
            wasm::emit_b8(m_memory_section, m_al,
                    0x01);  // for specifying min and max page limits of memory
            wasm::emit_u32(m_memory_section, m_al, min_no_pages);
            wasm::emit_u32(m_memory_section, m_al, max_no_pages);
        } else {
            wasm::emit_b8(m_memory_section, m_al,
                    0x00);  // for specifying only min page limit of memory
            wasm::emit_u32(m_memory_section, m_al, min_no_pages);
        }
        no_of_memories++;
    }

    void emit_import_mem(const std::string &mod_name, const std::string &mem_name,
                        uint32_t min_no_pages, uint32_t max_no_pages = 0) {
        wasm::emit_str(m_import_section, m_al, mod_name);
        wasm::emit_str(m_import_section, m_al, mem_name);
        wasm::emit_b8(m_import_section, m_al, 0x02);  // for importing memory
        if (max_no_pages > 0) {
            wasm::emit_b8(m_import_section, m_al,
                    0x01);  // for specifying min and max page limits of memory
            wasm::emit_u32(m_import_section, m_al, min_no_pages);
            wasm::emit_u32(m_import_section, m_al, max_no_pages);
        } else {
            wasm::emit_b8(m_import_section, m_al,
                    0x00);  // for specifying only min page limit of memory
            wasm::emit_u32(m_import_section, m_al, min_no_pages);
        }
        no_of_imports++;
    }

    void emit_export_mem(const std::string &name,
                        uint32_t idx) {
        wasm::emit_str(m_export_section, m_al, name);
        wasm::emit_b8(m_export_section, m_al, 0x02);  // for exporting memory
        wasm::emit_u32(m_export_section, m_al, idx);
        no_of_exports++;
    }

    void encode_section(Vec<uint8_t> &des, Vec<uint8_t> &section_content,
                        uint32_t section_id,
                        uint32_t no_of_elements) {
        // every section in WebAssembly is encoded by adding its section id,
        // followed by the content size and lastly the contents
        wasm::emit_u32(des, m_al, section_id);
        wasm::emit_u32(des, m_al, 4U /* size of no_of_elements */ + section_content.size());
        uint32_t len_idx = wasm::emit_len_placeholder(des, m_al);
        wasm::emit_u32_b32_idx(des, m_al, len_idx, no_of_elements);
        for (auto &byte : section_content) {
            des.push_back(m_al, byte);
        }
    }

    void emit_func_body(uint32_t func_idx, std::string func_name, std::vector<wasm::var_type> &locals, std::function<void()> func_body) {
        /*** Reference Function Prototype ***/
        wasm::emit_u32(m_func_section, m_al, func_idx);

        /*** Function Body Starts Here ***/
        uint32_t len_idx_code_section_func_size =
            wasm::emit_len_placeholder(m_code_section, m_al);


        {
            uint32_t len_idx_code_section_no_of_locals =
                wasm::emit_len_placeholder(m_code_section, m_al);
            uint32_t no_of_locals = emit_local_vars(locals);
            wasm::emit_u32_b32_idx(m_code_section, m_al,
                len_idx_code_section_no_of_locals, no_of_locals);
        }

        func_body();

        wasm::emit_expr_end(m_code_section, m_al);
        wasm::fixup_len(m_code_section, m_al, len_idx_code_section_func_size);

        /*** Export the function ***/
        emit_export_fn(func_name, func_idx);
        no_of_functions++;
    }

    void define_func(
        std::vector<wasm::var_type> params,
        std::vector<wasm::var_type> results,
        std::vector<wasm::var_type> locals,
        std::string func_name,
        std::function<void()> func_body) {

        uint32_t func_idx = emit_func_type(params, results);
        emit_func_body(func_idx, func_name, locals, func_body);
    }

    template <typename T>
    uint32_t declare_global_var(wasm::var_type var_type, T init_val) {
        m_global_section.push_back(m_al, var_type);
        m_global_section.push_back(m_al, true /* mutable */);
        switch (var_type)
        {
            case wasm::i32:
                wasm::emit_b8(m_global_section, m_al, 0x41); // emit instruction
                wasm::emit_i32(m_global_section, m_al, init_val); // emit val
                break;
            case wasm::i64:
                wasm::emit_b8(m_global_section, m_al, 0x42); // emit instruction
                wasm::emit_i64(m_global_section, m_al, init_val); // emit val
                break;
            case wasm::f32:
                wasm::emit_b8(m_global_section, m_al, 0x43); // emit instruction
                wasm::emit_f32(m_global_section, m_al, init_val); // emit val
                break;
            case wasm::f64:
                wasm::emit_b8(m_global_section, m_al, 0x44); // emit instruction
                wasm::emit_f64(m_global_section, m_al, init_val); // emit val
                break;
            default:
                std::cerr << "declare_global_var: Unsupported type" << std::endl;
                LCOMPILERS_ASSERT(false);
        }
        wasm::emit_expr_end(m_global_section, m_al);
        return no_of_globals++;
    }

    uint32_t emit_local_vars(std::vector<wasm::var_type> locals) {
        uint32_t no_of_locals = 0;
        for (auto v:locals) {
            wasm::emit_u32(m_code_section, m_al, 1);
            wasm::emit_b8(m_code_section, m_al, v);
            no_of_locals++;
        }
        return no_of_locals;
    }

    void emit_data_str(uint32_t mem_idx, const std::string &text) {
        wasm::emit_u32(m_data_section, m_al, 0U);  // for active mode of memory with default mem_idx of 0
        wasm::emit_b8(m_data_section, m_al, 0x41); // i32.const
        wasm::emit_i32(m_data_section, m_al, (int32_t)mem_idx); // specifying memory location
        wasm::emit_expr_end(m_data_section, m_al); // end instructions
        wasm::emit_str(m_data_section, m_al, text);
        no_of_data_segs++;
    }
};

}  // namespace LCompilers
