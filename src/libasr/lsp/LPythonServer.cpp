#include <iostream>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include "LPythonServer.hpp"
#include "JSONRPC2Connection.hpp"
#include "MessageHandler.hpp"

struct handle_functions
{
  JSONRPC2Connection* conn;

  handle_functions() {
    this->conn = new JSONRPC2Connection();
  }

  std::string getPath(std::string uri)  {
    // Converts URI to path
    if (uri.compare(0, 7, "file://"))
      return uri;
    std::string ret;
    #ifdef _WIN32
      // Skipping the initial "/" on Windows
      size_t i = 8;
    #else
      size_t i = 7;
    #endif
      auto from_hex = [](unsigned char c) {
        return c - '0' < 10 ? c - '0' : (c | 32) - 'a' + 10;
      };
      for (; i < uri.size(); i++) {
        if (i + 3 <= uri.size() && uri[i] == '%') {
          ret.push_back(from_hex(uri[i + 1]) * 16 + from_hex(uri[i + 2]));
          i += 2;
        } else
          ret.push_back(uri[i]);
      }
    #ifdef _WIN32
      std::replace(ret.begin(), ret.end(), '\\', '/');
      if (ret.size() > 1 && ret[0] >= 'a' && ret[0] <= 'z' && ret[1] == ':') {
        ret[0] = toupper(ret[0]);
      }
    #endif

    return ret;
  }

  rapidjson::Document serve_initialize(rapidjson::Document &/*request*/) {
    rapidjson::Document capabilities(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType &allocator = capabilities.GetAllocator(); 
    capabilities.SetObject();
    rapidjson::Document server_capabilities(rapidjson::kObjectType);
    server_capabilities.SetObject();
    server_capabilities.AddMember("textDocumentSync", rapidjson::Value().SetInt(2), allocator);
    server_capabilities.AddMember("documentSymbolProvider", rapidjson::Value().SetBool(true), allocator);
    capabilities.AddMember("capabilities", server_capabilities, allocator);
    return capabilities;
  }

  void serve_onSave(rapidjson::Document &request) {
        std::string uri = request["params"]["textDocument"]["uri"].GetString();
        int start_line = 2;
        int start_column = 0;
        int end_line = 3;
        int end_column = 10;
        std::string msg = "lpyth";

        rapidjson::Document range_obj(rapidjson::kObjectType);
        rapidjson::Document::AllocatorType &allocator = range_obj.GetAllocator(); 
        range_obj.SetObject();

        rapidjson::Document start_detail(rapidjson::kObjectType); 
        start_detail.SetObject();
        start_detail.AddMember("line", rapidjson::Value().SetInt(start_line), allocator);
        start_detail.AddMember("character", rapidjson::Value().SetInt(start_column), allocator);
        range_obj.AddMember("start", start_detail, allocator);

        rapidjson::Document end_detail(rapidjson::kObjectType);
        end_detail.SetObject();
        end_detail.AddMember("line", rapidjson::Value().SetInt(end_line), allocator);
        end_detail.AddMember("character", rapidjson::Value().SetInt(end_column), allocator);
        range_obj.AddMember("end", end_detail, allocator);

        rapidjson::Document diag_results(rapidjson::kArrayType);
        diag_results.SetArray();

        rapidjson::Document diag_capture(rapidjson::kObjectType);
        diag_capture.AddMember("source", rapidjson::Value().SetString("lsp", allocator), allocator);
        diag_capture.AddMember("range", range_obj, allocator);
        diag_capture.AddMember("message", rapidjson::Value().SetString(msg.c_str(), allocator), allocator);
        diag_capture.AddMember("severity", rapidjson::Value().SetInt(2), allocator);
        diag_results.PushBack(diag_capture, allocator);

        rapidjson::Document message_send(rapidjson::kObjectType);
        message_send.SetObject();
        message_send.AddMember("uri", rapidjson::Value().SetString(uri.c_str(), allocator), allocator);
        message_send.AddMember("diagnostics", diag_results, allocator);

        this->conn->send_notification(
          "textDocument/publishDiagnostics",
          message_send
        );
  }

  void serve_document_symbol(rapidjson::Document &request, JSONRPC2Connection& obj, int rid) {
    std::string uri = request["params"]["textDocument"]["uri"].GetString();
    std::string path = getPath(uri);
    using LFortran::CompilerOptions;
    CompilerOptions compiler_options;
    LCompilers::PassManager lpython_pass_manager;
    std::string runtime_library_dir = LFortran::get_runtime_library_dir();

    std::vector<LFortran::LPython::lsp_locations> 
      symbol_lists = LFortran::LPython::get_SymbolLists(path, runtime_library_dir, compiler_options);

    rapidjson::Document test_output(rapidjson::kArrayType);
    rapidjson::Document range_object(rapidjson::kObjectType);
    rapidjson::Document start_detail(rapidjson::kObjectType); 
    rapidjson::Document end_detail(rapidjson::kObjectType); 
    rapidjson::Document location_object(rapidjson::kObjectType);
    rapidjson::Document test_capture(rapidjson::kObjectType);

    test_output.SetArray();

    for (auto symbol : symbol_lists) {
      uint32_t start_character = symbol.first_column;
      uint32_t start_line = symbol.first_line;
      uint32_t end_character = symbol.last_column;
      uint32_t end_line = symbol.last_line;
      std::string name = symbol.symbol_name;

      range_object.SetObject();
      rapidjson::Document::AllocatorType &allocator = range_object.GetAllocator(); 

      start_detail.SetObject();
      start_detail.AddMember("character", rapidjson::Value().SetInt(start_character), allocator);
      start_detail.AddMember("line", rapidjson::Value().SetInt(start_line), allocator);
      range_object.AddMember("start", start_detail, allocator);

      end_detail.SetObject();
      end_detail.AddMember("character", rapidjson::Value().SetInt(end_character), allocator);
      end_detail.AddMember("line", rapidjson::Value().SetInt(end_line), allocator);
      range_object.AddMember("end", end_detail, allocator);

      location_object.SetObject();
      location_object.AddMember("range", range_object, allocator);
      location_object.AddMember("uri", rapidjson::Value().SetString(uri.c_str(), allocator), allocator);

      test_capture.SetObject();
      test_capture.AddMember("kind", rapidjson::Value().SetInt(12), allocator);
      test_capture.AddMember("location", location_object, allocator);
      test_capture.AddMember("name", rapidjson::Value().SetString(name.c_str(), allocator), allocator);
      test_output.PushBack(test_capture, test_output.GetAllocator());
    }
    obj.write_message(rid, test_output);
  }
};

typedef rapidjson::Document (handle_functions::*json_handle)(rapidjson::Document &);
typedef void (handle_functions::*void_handle)(rapidjson::Document &);
typedef void (handle_functions::*void_and_json_handle)(rapidjson::Document &, JSONRPC2Connection &, int rid);

typedef std::map<std::string, json_handle> json_mapfun;
typedef std::map<std::string, void_handle> void_mapfun;
typedef std::map<std::string, void_and_json_handle> void_json_mapfun;

void LPythonServer::handle(rapidjson::Document &request) {
    json_mapfun json_handler;
    void_mapfun void_handler;
    void_json_mapfun json_void_handler;

    json_handler["initialize"] = &handle_functions::serve_initialize;
    json_void_handler["textDocument/documentSymbol"] = &handle_functions::serve_document_symbol;
    void_handler["textDocument/didSave"] = &handle_functions::serve_onSave;

    std::string request_method = request["method"].GetString();

    if (request.HasMember("id")) {
      json_mapfun::iterator x = json_handler.find(request_method);
      void_json_mapfun::iterator y = json_void_handler.find(request_method);

      if (y != json_void_handler.end()) {
        handle_functions h;
        (h.*(y->second))(request, *this->conn, request["id"].GetInt());
      } else if (x != json_handler.end()) {
          handle_functions h;
          rapidjson::Document resp;
          resp = (h.*(x->second))(request);
          rapidjson::StringBuffer buffer;
          buffer.Clear();
          rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
          resp.Accept(writer);
          std::string resp_str( buffer.GetString() );
          int rid = request["id"].GetInt();
          this->conn->write_message(rid, resp);
      }
    }
    else {
      void_mapfun::iterator x = void_handler.find(request_method);
      if (x != void_handler.end()) {
          handle_functions h;
          (h.*(x->second))(request);
      }
    }
}

void LPythonServer::run(std::string /*path*/) {
  while (this->running) {
    rapidjson::Document request  = this->conn->read_message();
    this->handle(request);
  }
}
