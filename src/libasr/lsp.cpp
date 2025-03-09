#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/diagnostics.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/asr_builder.h>
#include <libasr/location.h>
#include <lfortran/fortran_evaluator.h>
#include <libasr/lsp_interface.h>
#include <libasr/asr_lookup_name.h>



namespace LCompilers {

LSP::DiagnosticSeverity diagnostic_level_to_lsp_severity(diag::Level level)
{
    switch (level) {
    case diag::Level::Error:
        return LSP::DiagnosticSeverity::Error;
    case diag::Level::Warning:
        return LSP::DiagnosticSeverity::Warning;
    case diag::Level::Note:
        return LSP::DiagnosticSeverity::Information;
    case diag::Level::Help:
        return LSP::DiagnosticSeverity::Hint;
    default:
        return LSP::DiagnosticSeverity::Warning;
    }
}

LSP::SymbolKind asr_symbol_type_to_lsp_symbol_kind(ASR::symbolType symbol_type)
{
    switch (symbol_type) {
    case ASR::symbolType::Module:
        return LSP::SymbolKind::Module;
    case ASR::symbolType::Function:
        return LSP::SymbolKind::Function;
    case ASR::symbolType::GenericProcedure:
        return LSP::SymbolKind::Function;
    case ASR::symbolType::CustomOperator:
        return LSP::SymbolKind::Operator;
    case ASR::symbolType::Struct:
        return LSP::SymbolKind::Struct;
    case ASR::symbolType::Enum:
        return LSP::SymbolKind::Enum;
    case ASR::symbolType::Variable:
        return LSP::SymbolKind::Variable;
    case ASR::symbolType::Class:
        return LSP::SymbolKind::Class;
    case ASR::symbolType::ClassProcedure:
        return LSP::SymbolKind::Method;
    case ASR::symbolType::Template:
        return LSP::SymbolKind::TypeParameter;
    default:
        return LSP::SymbolKind::Function;
    }
}

enum LFortranJSONType {
    kArrayType, kObjectType
};

class LFortranJSON {
private:
    LFortranJSONType type;
    std::string json_value;
    std::vector<std::pair<std::string, std::string>> object_members;
    std::vector<std::string> array_values;
    bool rebuild_needed; 

public:
    LFortranJSON(LFortranJSONType type) : type(type), rebuild_needed(true) {
        if (type == kArrayType) {
            json_value = "[]";
        } else {
            json_value = "{}";
        }
    }
    void SetObject() {
        type = kObjectType;
        object_members.clear();
        json_value = "{}";
        rebuild_needed = false;
    }
    void SetArray() {
        type = kArrayType;
        array_values.clear();
        json_value = "[]";
        rebuild_needed = false;
    }
    void AddMember(std::string key, int v) {
        object_members.push_back({key, std::to_string(v)});
        rebuild_needed = true;
    }
    void AddMember(std::string key, uint32_t v) {
        object_members.push_back({key, std::to_string(v)});
        rebuild_needed = true;
    }
    void AddMember(std::string key, std::string v) {
        object_members.push_back({key, "\"" + v + "\""});
        rebuild_needed = true;
    }
    void AddMember(std::string key, LFortranJSON v) {
        object_members.push_back({key, v.GetValue()});
        rebuild_needed = true;
    }
    void PushBack(LFortranJSON v) {
        array_values.push_back(v.GetValue());
        rebuild_needed = true;
    }
    std::string GetValue() {
        if (rebuild_needed) {
            RebuildJSON();
            rebuild_needed = false;
        }
        return json_value;
    }

private:
    void RebuildJSON() {
        if (type == kObjectType) {
            json_value = "{";
            for (size_t i = 0; i < object_members.size(); i++) {
                json_value += "\"" + object_members[i].first + "\":" + object_members[i].second;
                if (i < object_members.size() - 1) {
                    json_value += ",";
                }
            }
            json_value += "}";
        } else if (type == kArrayType) {
            json_value = "[";
            for (size_t i = 0; i < array_values.size(); i++) {
                json_value += array_values[i];
                if (i < array_values.size() - 1) {
                    json_value += ",";
                }
            }
            json_value += "]";
        }
    }
};

template <typename T>
void populate_symbol_lists(T* x, LCompilers::LocationManager lm, std::vector<LCompilers::document_symbols> &symbol_lists) {
    LCompilers::document_symbols loc;
    for (auto &a : x->m_symtab->get_scope()) {
        std::string symbol_name = a.first;
        uint32_t first_line;
        uint32_t last_line;
        uint32_t first_column;
        uint32_t last_column;
        std::string filename;
        lm.pos_to_linecol(a.second->base.loc.first, first_line,
            first_column, filename);
        lm.pos_to_linecol(a.second->base.loc.last, last_line,
            last_column, filename);
        loc.first_column = first_column;
        loc.last_column = last_column;
        loc.first_line = first_line;
        loc.last_line = last_line;
        loc.symbol_name = symbol_name;
        loc.filename = filename;
        loc.symbol_type = a.second->type;
        symbol_lists.push_back(loc);
        if ( LCompilers::ASR::is_a<LCompilers::ASR::Module_t>(*a.second) ) {
            LCompilers::ASR::Module_t *m = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(a.second);
            populate_symbol_lists(m, lm, symbol_lists);
        } else if ( LCompilers::ASR::is_a<LCompilers::ASR::Function_t>(*a.second) ) {
            LCompilers::ASR::Function_t *f = LCompilers::ASR::down_cast<LCompilers::ASR::Function_t>(a.second);
            populate_symbol_lists(f, lm, symbol_lists);
        } else if ( LCompilers::ASR::is_a<LCompilers::ASR::Program_t>(*a.second) ) {
            LCompilers::ASR::Program_t *p = LCompilers::ASR::down_cast<LCompilers::ASR::Program_t>(a.second);
            populate_symbol_lists(p, lm, symbol_lists);
        }
    }
}

int get_symbols(const std::string &infile, CompilerOptions &compiler_options)
{
    std::string input = read_file(infile);
    LCompilers::FortranEvaluator fe(compiler_options);
    std::vector<LCompilers::document_symbols> symbol_lists;

    LCompilers::LocationManager lm;
    {
        LCompilers::LocationManager::FileLocations fl;
        fl.in_filename = infile;
        lm.files.push_back(fl);
        lm.file_ends.push_back(input.size());
    }
    {
        LCompilers::diag::Diagnostics diagnostics;
        LCompilers::Result<LCompilers::ASR::TranslationUnit_t*>
            x = fe.get_asr2(input, lm, diagnostics);
        if (x.ok) {
            populate_symbol_lists(x.result, lm, symbol_lists);
        } else {
            std::cout << "{}";
            return 0;
        }
    }

    LFortranJSON test_output(LFortranJSONType::kArrayType);
    LFortranJSON range_object(LFortranJSONType::kObjectType);
    LFortranJSON start_detail(LFortranJSONType::kObjectType);
    LFortranJSON end_detail(LFortranJSONType::kObjectType);
    LFortranJSON location_object(LFortranJSONType::kObjectType);
    LFortranJSON test_capture(LFortranJSONType::kObjectType);

    test_output.SetArray();

    for (auto symbol : symbol_lists) {
        uint32_t start_character = symbol.first_column;
        uint32_t start_line = symbol.first_line;
        uint32_t end_character = symbol.last_column;
        uint32_t end_line = symbol.last_line;
        std::string name = symbol.symbol_name;
        LSP::SymbolKind kind = asr_symbol_type_to_lsp_symbol_kind(symbol.symbol_type);

        range_object.SetObject();

        start_detail.SetObject();
        start_detail.AddMember("character", start_character);
        start_detail.AddMember("line", start_line);
        range_object.AddMember("start", start_detail);

        end_detail.SetObject();
        end_detail.AddMember("character", end_character);
        end_detail.AddMember("line", end_line);
        range_object.AddMember("end", end_detail);

        location_object.SetObject();
        location_object.AddMember("range", range_object);
        location_object.AddMember("uri", "uri");

        test_capture.SetObject();
        test_capture.AddMember("kind", kind);
        test_capture.AddMember("location", location_object);
        test_capture.AddMember("name", name);
        test_capture.AddMember("filename", symbol.filename);
        test_output.PushBack(test_capture);
    }
    std::cout << test_output.GetValue();

    return 0;
}

int get_errors(const std::string &infile, CompilerOptions &compiler_options)
{
    std::string input = read_file(infile);
    LCompilers::FortranEvaluator fe(compiler_options);

    LCompilers::LocationManager lm;
    {
        LCompilers::LocationManager::FileLocations fl;
        fl.in_filename = infile;
        lm.files.push_back(fl);
        lm.file_ends.push_back(input.size());
    }
    LCompilers::diag::Diagnostics diagnostics;
    {
        LCompilers::Result<LCompilers::ASR::TranslationUnit_t*>
            result = fe.get_asr2(input, lm, diagnostics);
    }

    std::vector<LCompilers::error_highlight> diag_lists;
    LCompilers::error_highlight h;
    for (auto &d : diagnostics.diagnostics) {
        if (compiler_options.no_warnings && d.level != LCompilers::diag::Level::Error) {
            continue;
        }
        h.message = d.message;
        h.severity = d.level;
        for (auto label : d.labels) {
            for (auto span : label.spans) {
                uint32_t first_line;
                uint32_t first_column;
                uint32_t last_line;
                uint32_t last_column;
                std::string filename;
                lm.pos_to_linecol(span.loc.first, first_line, first_column,
                    filename);
                lm.pos_to_linecol(span.loc.last, last_line, last_column,
                    filename);
                h.first_column = first_column;
                h.last_column = last_column;
                h.first_line = first_line;
                h.last_line = last_line;
                h.filename = filename;
                diag_lists.push_back(h);
            }
        }
    }

    LFortranJSON range_obj(LFortranJSONType::kObjectType);
    LFortranJSON start_detail(LFortranJSONType::kObjectType);
    LFortranJSON end_detail(LFortranJSONType::kObjectType);
    LFortranJSON diag_results(LFortranJSONType::kArrayType);
    LFortranJSON diag_capture(LFortranJSONType::kObjectType);
    LFortranJSON message_send(LFortranJSONType::kObjectType);
    LFortranJSON all_errors(LFortranJSONType::kArrayType);
    all_errors.SetArray();

    message_send.SetObject();
    message_send.AddMember("uri", "uri");

    for (auto diag : diag_lists) {
        uint32_t start_line = diag.first_line;
        uint32_t start_column = diag.first_column;
        uint32_t end_line = diag.last_line;
        uint32_t end_column = diag.last_column;
        LSP::DiagnosticSeverity severity = diagnostic_level_to_lsp_severity(diag.severity);
        std::string msg = diag.message;

        range_obj.SetObject();

        start_detail.SetObject();
        start_detail.AddMember("line", start_line);
        start_detail.AddMember("character", start_column);
        range_obj.AddMember("start", start_detail);

        end_detail.SetObject();
        end_detail.AddMember("line", end_line);
        end_detail.AddMember("character", end_column);
        range_obj.AddMember("end", end_detail);

        diag_capture.SetObject();
        diag_capture.AddMember("source", "lpyth");
        diag_capture.AddMember("range", range_obj);
        diag_capture.AddMember("message", msg);
        diag_capture.AddMember("severity", severity);

        all_errors.PushBack(diag_capture);
    }
    message_send.AddMember("diagnostics", all_errors);
    std::cout << message_send.GetValue();

    return 0;
}

bool is_id_chr(const char c)
{
    return (('a' <= c) && (c <= 'z'))
        || (('A' <= c) && (c <= 'Z'))
        || (('0' <= c) && (c <= '9'))
        || (c == '_');
}

int get_definitions(const std::string &infile, LCompilers::CompilerOptions &compiler_options)
{
    std::string input = read_file(infile);
    LCompilers::FortranEvaluator fe(compiler_options);
    std::vector<LCompilers::document_symbols> symbol_lists;

    LCompilers::LocationManager lm;
    {
        LCompilers::LocationManager::FileLocations fl;
        fl.in_filename = infile;
        lm.files.push_back(fl);
        lm.file_ends.push_back(input.size());
    }
    {
        LCompilers::diag::Diagnostics diagnostics;
        LCompilers::Result<LCompilers::ASR::TranslationUnit_t*>
            x = fe.get_asr2(input, lm, diagnostics);
        if (x.ok) {
            // populate_symbol_lists(x.result, lm, symbol_lists);
            uint16_t l = std::stoi(compiler_options.line);
            uint16_t c = std::stoi(compiler_options.column);
            uint64_t input_pos = lm.linecol_to_pos(l, c);
            if (c > 0 && input_pos > 0 && !is_id_chr(input[input_pos]) && is_id_chr(input[input_pos - 1])) {
                // input_pos is to the right of the word boundary
                input_pos--;
            }
            uint64_t output_pos = lm.input_to_output_pos(input_pos, false);
            LCompilers::ASR::asr_t* asr = fe.handle_lookup_name(x.result, output_pos);
            LCompilers::document_symbols loc;
            if (!ASR::is_a<ASR::symbol_t>(*asr)) {
                std::cout << "[]";
                return 0;
            }
            ASR::symbol_t* s = ASR::down_cast<ASR::symbol_t>(asr);
            std::string symbol_name = ASRUtils::symbol_name( s );
            uint32_t first_line;
            uint32_t last_line;
            uint32_t first_column;
            uint32_t last_column;
            std::string filename;
            lm.pos_to_linecol(lm.output_to_input_pos(asr->loc.first, false), first_line,
                first_column, filename);
            lm.pos_to_linecol(lm.output_to_input_pos(asr->loc.last, true), last_line,
                last_column, filename);
            loc.first_column = first_column;
            loc.last_column = last_column;
            loc.first_line = first_line;
            loc.last_line = last_line;
            loc.symbol_name = symbol_name;
            loc.filename = filename;
            loc.symbol_type = s->type;
            symbol_lists.push_back(loc);
        } else {
            std::cout << "[]";
            return 0;
        }
    }

    LFortranJSON start_detail(LFortranJSONType::kObjectType);
    LFortranJSON range_object(LFortranJSONType::kObjectType);
    LFortranJSON end_detail(LFortranJSONType::kObjectType);
    LFortranJSON location_object(LFortranJSONType::kObjectType);
    LFortranJSON test_capture(LFortranJSONType::kObjectType);
    LFortranJSON test_output(LFortranJSONType::kArrayType);

    test_output.SetArray();

    for (auto symbol : symbol_lists) {
        uint32_t start_character = symbol.first_column;
        uint32_t start_line = symbol.first_line;
        uint32_t end_character = symbol.last_column;
        uint32_t end_line = symbol.last_line;
        std::string name = symbol.symbol_name;
        LSP::SymbolKind kind = asr_symbol_type_to_lsp_symbol_kind(symbol.symbol_type);

        range_object.SetObject();

        start_detail.SetObject();
        start_detail.AddMember("character", start_character);
        start_detail.AddMember("line", start_line);
        range_object.AddMember("start", start_detail);

        end_detail.SetObject();
        end_detail.AddMember("character", end_character);
        end_detail.AddMember("line", end_line);
        range_object.AddMember("end", end_detail);

        location_object.SetObject();
        location_object.AddMember("range", range_object);
        location_object.AddMember("uri", symbol.filename);

        test_capture.SetObject();
        test_capture.AddMember("kind", kind);
        test_capture.AddMember("location", location_object);
        test_capture.AddMember("name", name);
        test_capture.AddMember("filename", symbol.filename);
        test_output.PushBack(test_capture);
    }

    std::cout << test_output.GetValue();

    return 0;
}

int get_all_occurences(const std::string &infile, LCompilers::CompilerOptions &compiler_options)
{
    std::string input = read_file(infile);
    LCompilers::FortranEvaluator fe(compiler_options);
    std::vector<LCompilers::document_symbols> symbol_lists;

    LCompilers::LocationManager lm;
    {
        LCompilers::LocationManager::FileLocations fl;
        fl.in_filename = infile;
        lm.files.push_back(fl);
        lm.file_ends.push_back(input.size());
    }
    {
        LCompilers::diag::Diagnostics diagnostics;
        LCompilers::Result<LCompilers::ASR::TranslationUnit_t*>
            x = fe.get_asr2(input, lm, diagnostics);
        if (x.ok) {
            // populate_symbol_lists(x.result, lm, symbol_lists);
            uint16_t l = std::stoi(compiler_options.line);
            uint16_t c = std::stoi(compiler_options.column);
            uint64_t input_pos = lm.linecol_to_pos(l, c);
            uint64_t output_pos = lm.input_to_output_pos(input_pos, false);
            LCompilers::ASR::asr_t* asr = fe.handle_lookup_name(x.result, output_pos);
            LCompilers::document_symbols loc;
            if (!ASR::is_a<ASR::symbol_t>(*asr)) {
                std::cout << "[]";
                return 0;
            }
            ASR::symbol_t* s = ASR::down_cast<ASR::symbol_t>(asr);
            std::string symbol_name = ASRUtils::symbol_name( s );
            LCompilers::LFortran::OccurenceCollector occ(symbol_name, symbol_lists, lm);
            occ.visit_TranslationUnit(*x.result);
        } else {
            std::cout << "[]";
            return 0;
        }
    }

    LFortranJSON start_detail(LFortranJSONType::kObjectType);
    LFortranJSON range_object(LFortranJSONType::kObjectType);
    LFortranJSON end_detail(LFortranJSONType::kObjectType);
    LFortranJSON location_object(LFortranJSONType::kObjectType);
    LFortranJSON test_capture(LFortranJSONType::kObjectType);
    LFortranJSON test_output(LFortranJSONType::kArrayType);

    test_output.SetArray();

    for (auto symbol : symbol_lists) {
        uint32_t start_character = symbol.first_column;
        uint32_t start_line = symbol.first_line;
        uint32_t end_character = symbol.last_column;
        uint32_t end_line = symbol.last_line;
        std::string name = symbol.symbol_name;
        LSP::SymbolKind kind = asr_symbol_type_to_lsp_symbol_kind(symbol.symbol_type);

        range_object.SetObject();

        start_detail.SetObject();
        start_detail.AddMember("character", start_character);
        start_detail.AddMember("line", start_line);
        range_object.AddMember("start", start_detail);

        end_detail.SetObject();
        end_detail.AddMember("character", end_character);
        end_detail.AddMember("line", end_line);
        range_object.AddMember("end", end_detail);

        location_object.SetObject();
        location_object.AddMember("range", range_object);
        location_object.AddMember("uri", "uri");

        test_capture.SetObject();
        test_capture.AddMember("kind", kind);
        test_capture.AddMember("location", location_object);
        test_capture.AddMember("name", name);
        test_output.PushBack(test_capture);
    }

    std::cout << test_output.GetValue();

    return 0;
}

}

