#include <string>
#include <iostream>
#include <iterator>
#include <iomanip>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "JSONRPC2Connection.hpp"

class EOFError : public std::exception {
  public:
    std::string error_msg () {
      return "Reached end of file...";
    }
};

class JSONRPC2ProtocolError : public std::exception {
  public:
    std::string msg;
    JSONRPC2ProtocolError(std::string msg) {
        this->msg = msg;
    } 
    std::string error_msg () {
      return msg;
    }
};

int JSONRPC2Connection::_read_header_content_length(std::string line) {
  if (line.size() < 2 || (line[line.size() - 1] == '\n' && line[line.size() - 2] == '\r'))  {
    throw JSONRPC2ProtocolError("Line endings must be \r\n");
  }
  if (line.substr(0, 16) == "Content-Length: ") {
    // Example -> Content-length: 100
    int length = std::stoi(line.substr(16, line.size() - 16));
    return length;
  }
  return -1;
}

std::string JSONRPC2Connection::read(int length) {
  std::string body = "";
  int exit_seq = 0;
  int idx = 0;
  while (idx < length) {
    int c = getchar();
    if (c == EOF) break;
    if (idx == 0 && c != 123) continue;
    if (exit_seq == 0 && c == '\r') {
      ++exit_seq;
    } else if (exit_seq == 0 && c == '\n'){
      break;
    } else {
      body += c;
      idx++;
    }
  }
  return body;
}

rapidjson::Document JSONRPC2Connection::_json_parse(std::string body) {
  rapidjson::Document doc;
  doc.Parse(body.c_str());
  return doc;
}

rapidjson::Document JSONRPC2Connection::_receive() {
  std::string line;
  std::getline(std::cin, line);
  if (line.size() == 0) {
    throw(EOFError());
  }
  int length = this->_read_header_content_length(line);
  std::string body = this->read(length);
  return _json_parse(body);
}

void JSONRPC2Connection::_send(rapidjson::Document& body) {
  std::string body_str = json_to_string(body);
  int content_length = body_str.size();
  std::string response = "Content-Length: " + std::to_string(content_length) + "\r\n" +
    "Content-Type: application/vscode-jsonrpc; charset=utf8\r\n\r\n";
  std::cout << response;
  std::cout << body_str;
}

void JSONRPC2Connection::write_message(int rid, rapidjson::Document& response) {
  rapidjson::Document body(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType &allocator = body.GetAllocator(); 
  body.SetObject();
  body.AddMember("jsonrpc", rapidjson::Value().SetString("2.0", allocator), allocator);
  body.AddMember("id", rapidjson::Value().SetInt(rid), allocator);
  body.AddMember("result", response, allocator);
  this->_send(body);
}

rapidjson::Document JSONRPC2Connection::read_message() {
  rapidjson::Document res_empty;
  try {
    rapidjson::Document res = this->_receive();
    return res;
  } catch (EOFError) {
    return res_empty;
  }
  return res_empty;
}

void JSONRPC2Connection::send_notification(std::string method, rapidjson::Document &params) {
  rapidjson::Document body(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType &allocator = body.GetAllocator(); 
  body.SetObject();
  body.AddMember("jsonrpc", rapidjson::Value().SetString("2.0", allocator), allocator);
  body.AddMember("method", rapidjson::Value().SetString(method.c_str(), allocator), allocator);
  body.AddMember("params", params, allocator);
  rapidjson::StringBuffer buffer;
  buffer.Clear();
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  body.Accept(writer);
  std::string str_body(buffer.GetString());
  this->_send(body);
}

std::string JSONRPC2Connection::json_to_string(rapidjson::Document &json) {
  rapidjson::StringBuffer strbuf;
  strbuf.Clear();
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  json.Accept(writer);
  return strbuf.GetString();
}