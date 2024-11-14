#pragma once
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

class RestServer {
public:
    RestServer(const utility::string_t& url) : listener(url) {
        listener.support(methods::GET, std::bind(&RestServer::handle_get, this, std::placeholders::_1));
        listener.support(methods::POST, std::bind(&RestServer::handle_post, this, std::placeholders::_1));
        //listener.support(methods::PUT, std::bind(&RestServer::handle_put, this, std::placeholders::_1));
        listener.support(methods::DEL, std::bind(&RestServer::handle_delete, this, std::placeholders::_1));
    }

    void handle_get(http_request request);
    void handle_post(http_request request);
    //void handle_put(http_request request);
    void handle_delete(http_request request);

    void open() { listener.open().wait(); }
    void close() { listener.close().wait(); }

private:
    http_listener listener;
};


