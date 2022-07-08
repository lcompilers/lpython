#ifndef LPYTHON_SERVER_HPP
#define LPYTHON_SERVER_HPP

#include <string>
#include <rapidjson/document.h>

#include "JSONRPC2Connection.hpp"

class LPythonServer {
    public:
        bool running = false;
        JSONRPC2Connection* conn;
        LPythonServer() {
            this->running = true;
            this->conn = new JSONRPC2Connection();
        }
        void run(std::string);
        void handle(rapidjson::Document &);
};

#endif