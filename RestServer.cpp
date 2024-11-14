#include "RestServer.h"

void RestServer::handle_get(http_request request) {
    ucout << "GET request received" << std::endl;

    std::ifstream html_file("table.html");
    if (!html_file) {
        request.reply(status_codes::NotFound, U("HTML file not found"));
        return;
    }
    std::string html_content((std::istreambuf_iterator<char>(html_file)),
        std::istreambuf_iterator<char>());

    auto path = uri::split_path(uri::decode(request.relative_uri().path()));

    if (path.size() == 2 && path[0] == U("tables")) {
        auto tableName = path[1];

        if (tableName == U("example_table")) {
            json::value response = json::value::array();

            // Пример данных таблицы
            response[0][U("id")] = json::value::number(1);
            response[0][U("name")] = json::value::string(U("John Doe"));
            response[0][U("age")] = json::value::number(30);

            response[1][U("id")] = json::value::number(2);
            response[1][U("name")] = json::value::string(U("Jane Smith"));
            response[1][U("age")] = json::value::number(25);

            request.reply(status_codes::OK, response);
        }
        else {
            request.reply(status_codes::NotFound, U("Table not found"));
        }
    }
    else {
        request.reply(status_codes::NotFound, U("Not found"));
    }

    request.reply(status_codes::OK, html_content);
}


void RestServer::handle_post(http_request request) {
    ucout << "POST request received" << std::endl;

    request.extract_json().then([=](json::value jsonBody) {
        auto tableName = jsonBody[U("name")].as_string();
        // Логика для создания таблицы
        ucout << "Creating table: " << tableName << std::endl;
        request.reply(status_codes::Created, U("Table created"));
        }).wait();
}
