#include <cpprest/http_listener.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Table.hpp"

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

static unsigned int _TABLE_TYPE = 3;
static std::unordered_map <std::string, std::pair<Table, bool>> DB; \
std::vector<std::vector<std::string>> G_DIFF_DATA;
std::vector<std::string> G_DIFF_column_names;
std::vector<std::string> G_DIFF_column_types;
http_listener* G_LISTENER;

struct TableInfo {
    int rows;
    int cols;
    int table_type;
    bool alive = true;
    std::string name;
    std::vector<std::string> column_names;
    std::vector<std::string> column_types; 
    std::vector<std::vector<std::string>> m_data;

    TableInfo(int r, int c, int t, const std::string& n, const std::vector<std::string>& col_names, const std::vector<std::string>& col_types, const std::vector<std::vector<std::string>>& data)
        : rows(r), cols(c), table_type(t), name(n), column_names(col_names), column_types(col_types), m_data(data) {}
};

std::vector<TableInfo> tables = {
    {1, 3, 1, "Table 1", {"ID", "Name", "Age"}, {"int", "string", "int"}, {{"asd", ":ddd", "asdada"}}},
    {2, 2, 2, "Table 2", {"Product", "Price", "Stock", "Rating"}, {"string", "real", "int", "real"}, {{"aaaad", ":dfaasdd"}, {"nam", "bam"}}},
    {3, 2, 3, "Table 3", {"X", "Y"}, {"real", "complex"}, {{"aaaad", ":dfaasdd"}, {"1313", "2.1m"}, {"ddd", "xdd"}}}
};

void UM_to_table(std::vector<TableInfo>& tables, std::unordered_map <std::string, std::pair<Table, bool>> DB, std::string updateName);

std::string read_html_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + file_path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string generate_table_list() {
    std::string list_html;
    for (size_t i = 0; i < tables.size(); ++i) {
        if(tables[i].alive)
        {
            list_html += "<li><a href=\"/table?rows=" + std::to_string(tables[i].rows) +
                "&cols=" + std::to_string(tables[i].cols) +
                "&table=" + std::to_string(tables[i].table_type) + "\">" +
                tables[i].name + " (" + std::to_string(tables[i].rows) + "x" + std::to_string(tables[i].cols) + ")</a></li>";
        }
    }
    return list_html;
}

std::string generate_table_rows(int table_type) {
    std::string table_html;

    const TableInfo* table_info = nullptr;
    for (const auto& table : tables) {
        if (table.table_type == table_type) {
            table_info = &table;
            break;
        }
    }

    if (!table_info) {
        return "<tr><td colspan='100%'>Table not found</td></tr>";
    }
    table_html += "<tr>";
    for (int j = 0; j < table_info->cols; ++j) {
        table_html += "<td>" + table_info->column_names[j] + " (" + table_info->column_types[j] + ")" + "</td>";
    }
    table_html += "</tr>";
    for (int i = 0; i < table_info->rows; ++i) {
        table_html += "<tr>";
        for (int j = 0; j < table_info->cols; ++j) {
            table_html += "<td>" + table_info->m_data[i][j] + "</td>";
        }
        table_html += "</tr>";
    }

    return table_html;
}

std::string get_query_param(const uri& url, const std::string& key) {
    auto query_map = uri::split_query(url.query());
    auto iter = query_map.find(utility::conversions::to_string_t(key));
    if (iter != query_map.end()) {
        return utility::conversions::to_utf8string(iter->second);
    }
    return "";
}

void handle_get(http_request request) {
    auto path = uri::split_path(uri::decode(request.relative_uri().path()));

    if (path.empty() || path[0] == U("")) {
        try {
            std::string main_page_html = read_html_file("main_page.html");
            std::string table_list = generate_table_list();
            main_page_html.replace(main_page_html.find("{{table_list}}"), 13, table_list);
            request.reply(status_codes::OK, main_page_html, "text/html");
        }
        catch (const std::exception& ex) {
            request.reply(status_codes::InternalError, ex.what());
        }
        return;
    }

    if (path[0] == U("table")) {
        try {
            auto url = request.relative_uri();
            std::string table_str = get_query_param(url, "table");

            int table_type = table_str.empty() ? 1 : std::stoi(table_str);

            std::string table_page_html = read_html_file("table_page.html");

            std::string table_rows = generate_table_rows(table_type);

            table_page_html.replace(table_page_html.find("{{table_name}}"), 13, tables[table_type - 1].name);

            table_page_html.replace(table_page_html.find("{{table_rows}}"), 13, table_rows);

            int cols = tables[table_type - 1].cols; 
            table_page_html.replace(table_page_html.find("{{cols}}"), 7, std::to_string(cols));

            request.reply(status_codes::OK, table_page_html, "text/html");
        }
        catch (const std::exception& ex) {
            request.reply(status_codes::InternalError, ex.what());
        }
        return;
    }


    request.reply(status_codes::NotFound, U("404 Not Found"));
}

void handle_update_db(http_request request)
{
    G_LISTENER->support(methods::GET, handle_get);
    return;
}

void handle_add_table(http_request request) {
    request.extract_json().then([=](json::value json_data) {
        try {
            std::string table_name = utility::conversions::to_utf8string(json_data[U("name")].as_string());
            auto columns = json_data[U("columns")].as_array();

            std::vector<std::string> column_names;
            std::vector<std::string> column_types;

            for (const auto& col : columns) {
                column_names.push_back(utility::conversions::to_utf8string(col.at(U("name")).as_string()));
                column_types.push_back(utility::conversions::to_utf8string(col.at(U("type")).as_string()));
            }

            std::vector<std::vector<std::string>> data;
            tables.push_back({ static_cast<int>(0), static_cast<int>(column_names.size()), static_cast<int>(tables.size() + 1), table_name, column_names, column_types, data });

            request.reply(status_codes::OK, "Table '" + table_name + "' added with " + std::to_string(column_names.size()) + " columns.");
        }
        catch (const std::exception& ex) {
            request.reply(status_codes::BadRequest, "Invalid data");
        }
        }).wait();
}

void handle_diff_table(http_request request) {
    request.extract_json().then([=](json::value json_data) {
        try {
            std::string table_name = "difference";
            int index = -1;

            for (size_t i = 0; i < tables.size(); i++)
            {
                if (tables[i].name == table_name)
                {
                    index = i;
                }
            }

            if (index == -1)
            {
                tables.push_back({ static_cast<int>(G_DIFF_DATA.size()), static_cast<int>(G_DIFF_column_names.size()), static_cast<int>(tables.size() + 1), table_name, G_DIFF_column_names, G_DIFF_column_types, G_DIFF_DATA});
            }
            else
            {
                tables[index] = TableInfo(static_cast<int>(G_DIFF_DATA.size()), static_cast<int>(G_DIFF_column_names.size()), static_cast<int>(tables.size() + 1), table_name, G_DIFF_column_names, G_DIFF_column_types, G_DIFF_DATA);
            }

            /*std::vector<std::vector<std::string>> data;
            tables.push_back({ static_cast<int>(0), static_cast<int>(column_names.size()), static_cast<int>(tables.size() + 1), table_name, column_names, column_types, data });*/

            request.reply(status_codes::OK, "Table");
        }
        catch (const std::exception& ex) {
            request.reply(status_codes::BadRequest, "Invalid data");
        }
        }).wait();
}

void handle_calculate_diff_table(http_request request) {
    std::string table_name1;
    std::string table_name2;
    request.extract_json().then([&](json::value json_data) {
        try {
            table_name1 = utility::conversions::to_utf8string(json_data[U("name1")].as_string());
            table_name2 = utility::conversions::to_utf8string(json_data[U("name2")].as_string());

            for (size_t i = 0; i < tables.size(); i++)
            {
                if (tables[i].name == table_name1)
                {
                    G_DIFF_column_names = tables[i].column_names;
                    G_DIFF_column_types = tables[i].column_types;
                }
            }
            /*std::vector<std::vector<std::string>> data;
            tables.push_back({ static_cast<int>(0), static_cast<int>(column_names.size()), static_cast<int>(tables.size() + 1), table_name, column_names, column_types, data });*/

            request.reply(status_codes::OK, "Table");
        }
        catch (const std::exception& ex) {
            request.reply(status_codes::BadRequest, "Invalid data");
        }
        }).wait();

    G_DIFF_DATA = tableDifference(DB, table_name1, table_name2);
}

void handle_delete_table(http_request request) {
    request.extract_json().then([=](json::value json_data) {
        try {
            std::string table_name = utility::conversions::to_utf8string(json_data[U("table_name")].as_string());

            for (size_t i = 0; i < tables.size(); i++)
            {
                if (tables[i].name == table_name)
                {
                    tables.erase(tables.begin() + i);
                }
            }
            
            request.reply(status_codes::OK, "Table deleted");
        }
        catch (const std::exception& ex) {
            request.reply(status_codes::BadRequest, "Invalid data");
        }
        }).wait();
}

void processAddRow(std::vector<std::string> vs, std::string tableName)
{
    std::cout << "row added to db" << std::endl;
    DB[tableName].first.addRowFW(vs);
}

void processUpdRow(std::vector<std::string> vs, int rowIdx, std::string tableName)
{
    std::cout << "row added to db" << std::endl;
    DB[tableName].first.updateRowFW(rowIdx, vs);
}

void handle_add_row(http_request request) {
    std::vector<std::string> vs;
    std::string tableName;
    int table_index;
    request.extract_json().then([&](json::value json_data) {
        try {
            std::string table_name = utility::conversions::to_utf8string(json_data[U("table_name")].as_string());
            auto row_data = json_data[U("row_data")].as_array();
            std::cout << "Adding new row to table: " << table_name << std::endl;
            table_name.erase(table_name.end() - 1);
            for (const auto& cell : row_data) {
                vs.push_back(utility::conversions::to_utf8string(cell.as_string()));
            }
            tableName = table_name;
            for (size_t i = 0; i < tables.size(); i++)
            {
                if (tables[i].name == table_name)
                {
                    tables[i].m_data.push_back(vs);
                    tables[i].rows++;
                }
            }
            request.reply(status_codes::OK, "Row added successfully to " + table_name);
        }
        catch (const std::exception& ex) {
            request.reply(status_codes::BadRequest, "Invalid row data");
        }
        }).wait();

    processAddRow(vs, tableName);
}

void handle_update_row(http_request request) {
    std::vector<std::string> vs;
    std::string tableName;
    int row_index;

    request.extract_json().then([&](json::value json_data) {
        try {
            std::string table_name = utility::conversions::to_utf8string(json_data[U("table_name")].as_string());
            std::cout << "Table name: " << table_name << std::endl;
            std::cout << "Extracting row_index..." << std::endl;
            std::string srow_idx = utility::conversions::to_utf8string(json_data[U("row_index")].as_string());
            int row_idx = std::stoi(srow_idx);
            std::cout << "Row index: " << row_idx << std::endl;

            auto row_data = json_data[U("row_data")].as_array();

            std::cout << "Updating row " << row_idx << " in table: " << table_name << std::endl;

            table_name.erase(table_name.end() - 1);
            for (const auto& cell : row_data) {
                vs.push_back(utility::conversions::to_utf8string(cell.as_string()));
            }
            tableName = table_name;

            bool row_updated = false;
            for (size_t i = 0; i < tables.size(); i++)
            {
                if (tables[i].name == table_name)
                {
                    if (row_index < tables[i].rows)
                    {
                        tables[i].m_data[row_idx] = vs;
                        row_updated = true;
                    }
                    else
                    {
                        throw std::out_of_range("Row index is out of bounds.");
                    }
                }
            }

            if (row_updated) {
                request.reply(status_codes::OK, "Row updated successfully.");
            }
            else {
                request.reply(status_codes::BadRequest, "Table not found or row index invalid.");
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "Error updating row: " << ex.what() << std::endl;
            request.reply(status_codes::BadRequest, "Invalid row data");
        }
    }).wait();


    processUpdRow(vs, row_index, tableName);
}


std::vector<std::string> convertToStringVector(const ColumnType& column) {
    std::vector<std::string> stringVector;

    std::visit([&stringVector](const auto& vec) {
        for (const auto& value : vec) {
            std::ostringstream oss;
            oss << value;
            stringVector.push_back(oss.str());
        }
        }, column);

    return stringVector;
}

bool tableCheckName(const std::vector<TableInfo>& tables, std::string tableName)
{
    for (size_t i = 0; i < tables.size(); i++)
        if (tables[i].name == tableName)
            return false;
    return true;
}

void UM_to_table(std::vector<TableInfo>& tables, std::unordered_map <std::string, std::pair<Table, bool>> DB, std::string updateName)
{
    for (std::pair<std::string, std::pair<Table, bool>> p : DB)
    {
        Table t1 = p.second.first;
        std::string currentName = p.first;

        std::vector<std::vector<DataType>> v1;
        std::vector<std::vector<std::string>>  result;
        std::vector<std::vector<std::string>>  maskS1;

        //form t1 row
        for (int i = 0; i < p.second.first.getColAmount(); i++)
        {
            ColumnType temp = p.second.first.getColumns()[i];
            result.push_back(convertToStringVector(temp));
        }

        for (int k = 0; k < result[0].size(); k++)
        {
            std::vector<std::string> temp;
            for (int i = 0; i < result.size(); i++)
            {
                temp.push_back(result[i][k]);
            }
            maskS1.push_back(temp);
        }

        TableInfo tableinfo = TableInfo(maskS1.size(), maskS1[0].size(), ++_TABLE_TYPE, p.first, p.second.first.getColNames(), p.second.first.getDataTypes(), maskS1);

        if(tableCheckName(tables, currentName) || currentName == updateName)
            tables.push_back(tableinfo);
    }
}

int main() {
    std::string dataPath = "C:\\Users\\dwarf\\OneDrive\\Documents\\DataBaseSaveFile.txt";
    loadFromFile(DB, dataPath);

    UM_to_table(tables, DB, "");

    uri_builder uri(U("http://localhost:8080"));
    auto addr = uri.to_uri().to_string();
    http_listener listener(addr);
    G_LISTENER = &listener;

    listener.support(methods::POST, [](http_request request) {
        auto path = uri::split_path(uri::decode(request.relative_uri().path()));

        if (path.size() == 1 && path[0] == U("add_table")) {
            handle_add_table(request);
        }
        else if (path.size() == 1 && path[0] == U("update_db")) {
            handle_update_db(request);
        }
        else if (path.size() == 1 && path[0] == U("add_row")) {
            handle_add_row(request); 
        }
        else if (path.size() == 1 && path[0] == U("update_row")) {
            handle_update_row(request); 
        }
        else if (path.size() == 1 && path[0] == U("diff_table")) {
            handle_calculate_diff_table(request);
        }
        else if (path.size() == 1 && path[0] == U("diff_complete")) {
            handle_diff_table(request);
        }
        /*else if (path.size() == 1 && path[0] == U("delete_table")) {
            handle_delete_table(request); 
        }*/
        else {
            request.reply(status_codes::NotFound, "Route not found");
        }
        });

    listener.support(methods::GET, handle_get);
    listener.support(methods::DEL, [](http_request request) {
        auto path = uri::split_path(uri::decode(request.relative_uri().path()));

        if (path.size() == 1 && path[0] == U("delete_table")) {
            std::cout << "del triggered" << std::endl;
            handle_delete_table(request);
        }
        else {
            request.reply(status_codes::NotFound, "Route not found");
        }
        });

    try {
        listener
            .open()
            .then([&listener]() { std::cout << "Starting server at: " << std::endl; })
            .wait();

        std::cout << "Press ENTER to exit." << std::endl;
        std::string line;
        std::getline(std::cin, line);

        listener.close().wait();
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}


