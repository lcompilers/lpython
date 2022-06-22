#ifndef JSONRPC2_CONNECTION_HPP
#define JSONRPC2_CONNECTION_HPP


#include <string>
#include <rapidjson/document.h>

class JSONRPC2Connection {
  public:
    JSONRPC2Connection() {}
    rapidjson::Document read_message();
    int _read_header_content_length(std::string);
    std::string read(int);
    rapidjson::Document _receive();
    rapidjson::Document _json_parse(std::string);
    void _send(rapidjson::Document&);
    void write_message(int, rapidjson::Document&);
    void send_notification(std::string, rapidjson::Document&);
    std::string json_to_string(rapidjson::Document&);
};

#endif