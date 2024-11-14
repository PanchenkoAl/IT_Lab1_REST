#include <iostream>
#include <vector>
#include <variant>
#include <regex>
#include <complex>
#include <string>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <fstream>

static int CURRENT_ID = 0;
using DataType = std::variant<int, float, std::complex<double>, std::string>;
using ColumnType = std::variant<std::vector<int>, std::vector<float>, std::vector<std::complex<double>>, std::vector<std::string>>;
std::vector<std::string> convertToStringVector(const ColumnType& column);

class Table
{
    std::string  m_name;
    unsigned int m_id;
    unsigned int m_colsize;

    //std::vector<std::vector<std::variant<int, float, std::string, >>> m_columns;
    std::vector<ColumnType> m_columns;
    std::vector<std::string> m_colNames;
    std::vector<std::string> m_dTypes;

public:
    Table(int id, int colsize, std::string name) :
        m_id(id),
        m_colsize(colsize),
        m_name(name)
    {}

    Table()
    {}

    std::vector<ColumnType> getColumns() const
    {
        return m_columns;
    }

    std::vector<std::string> getColNames() const
    {
        return m_colNames;
    }

    std::vector<std::string> getDataTypes() const
    {
        return m_dTypes;
    }

    int getColAmount()
    {
        return m_columns.size();
    }

    void addColName(std::string value)
    {
        m_colNames.push_back(value);
    }

    void addDataType(std::string value)
    {
        m_dTypes.push_back(value);
    }

    template<typename T>
    void addColumn() {
        m_columns.emplace_back(std::vector<T>{});
    }

    template<typename T>
    void addElementToColumn(size_t columnIndex, const T& value) {
        if (columnIndex < m_columns.size() && std::holds_alternative<std::vector<T>>(m_columns[columnIndex])) {
            std::get<std::vector<T>>(m_columns[columnIndex]).push_back(value);
        }
        else {
            std::cerr << "Err 1!" << std::endl;
        }
    }

    void addElementToColumnAuto(size_t columnIndex, const DataType& value) {
        if (std::holds_alternative<int>(value) && std::holds_alternative<std::vector<int>>(m_columns[columnIndex])) {
            addElementToColumn<int>(columnIndex, std::get<int>(value));
        }
        else if (std::holds_alternative<float>(value) && std::holds_alternative<std::vector<float>>(m_columns[columnIndex])) {
            addElementToColumn<float>(columnIndex, std::get<float>(value));
        }
        else if (std::holds_alternative<std::string>(value) && std::holds_alternative<std::vector<std::string>>(m_columns[columnIndex])) {
            addElementToColumn<std::string>(columnIndex, std::get<std::string>(value));
        }
        else if (std::holds_alternative<std::complex<double>>(value) && std::holds_alternative<std::vector<std::complex<double>>>(m_columns[columnIndex])) {
            addElementToColumn<std::complex<double>>(columnIndex, std::get<std::complex<double>>(value));
        }
        else {
            addElementToColumn<std::string>(columnIndex, std::get<std::string>(value));
        }
    }

    template<typename T>
    void updateElementInColumn(size_t columnIndex, const T& value, size_t rowIndex) {
        if (columnIndex < m_columns.size() && std::holds_alternative<std::vector<T>>(m_columns[columnIndex])) {
            std::get<std::vector<T>>(m_columns[columnIndex])[rowIndex] = value;
        }
        else {
            std::cerr << "Err 1!" << std::endl;
        }
    }

    void updateElementInColumnAuto(size_t columnIndex, const DataType& value, size_t rowIndex) {
        if (std::holds_alternative<int>(value) && std::holds_alternative<std::vector<int>>(m_columns[columnIndex])) {
            updateElementInColumn<int>(columnIndex, std::get<int>(value), rowIndex);
        }
        else if (std::holds_alternative<float>(value) && std::holds_alternative<std::vector<float>>(m_columns[columnIndex])) {
            updateElementInColumn<float>(columnIndex, std::get<float>(value), rowIndex);
        }
        else if (std::holds_alternative<std::string>(value) && std::holds_alternative<std::vector<std::string>>(m_columns[columnIndex])) {
            updateElementInColumn<std::string>(columnIndex, std::get<std::string>(value), rowIndex);
        }
        else if (std::holds_alternative<std::complex<double>>(value) && std::holds_alternative<std::vector<std::complex<double>>>(m_columns[columnIndex])) {
            updateElementInColumn<std::complex<double>>(columnIndex, std::get<std::complex<double>>(value), rowIndex);
        }
        else {
            std::cerr << "Type mismatch or invalid column index!" << std::endl;
        }
    }

    void initiateTable()
    {
        std::string s = "";
        std::string datatype = "";

        int iterator = 0;
        for (int i = 0; i < m_colsize; i++)
        {
            std::cout << "Enter column " << i + 1 << " name: " << std::endl;
            std::cin >> s;
            std::cout << "Enter data type: ";
            std::cin >> datatype;

            switch (datatype[0])
            {
            case 'i':
                addColumn<int>();
                break;
            case 'r':
                addColumn<float>();
                break;
            case 'c':
                addColumn<std::complex<double>>();
                break;
            case 's':
                addColumn<std::string>();
                break;
            }

            m_colNames.push_back(s);
            m_dTypes.push_back(datatype);
        }
    }

    std::string detectType(const std::string& input, DataType& modinput)
    {
        std::regex intRegex("^[-+]?\\d+$");
        if (std::regex_match(input, intRegex))
        {
            modinput = std::stoi(input);
            return "int";
        }

        std::regex realRegex("^[-+]?\\d*\\.\\d+([eE][-+]?\\d+)?$");
        if (std::regex_match(input, realRegex))
        {
            modinput = std::stof(input);
            return "real";
        }

        std::regex complexRegex("^\\((-?\\d*\\.?\\d+),(-?\\d*\\.?\\d+)\\)$");
        std::smatch match;
        if (std::regex_match(input, match, complexRegex)) {
            double realPart = std::stod(match[1]);
            double imagPart = std::stod(match[2]);
            modinput = std::complex<double>(realPart, imagPart);
            return "complex";
        }

        modinput = input;
        return "string";
    }

    void updateRowManual(size_t rowId)
    {
        std::vector<DataType> row;
        DataType modified;
        std::string s = "";
        for (int i = 0; i < m_colsize; i++)
        {
            std::cout << "Enter " << m_colNames[i] << " with " << m_dTypes[0] << "type: ";
            std::cin >> s;

            if (detectType(s, modified) == m_dTypes[i])
            {
                row.push_back(modified);
            }
            else
            {
                std::cout << "Invalid input." << std::endl;
                return;
            }
        }

        for (int i = 0; i < m_colsize; i++)
        {
            updateElementInColumnAuto(i, row[i], rowId);
        }
    }

    void updateRowFW(size_t rowId, std::vector<std::string> vs)
    {
        std::vector<DataType> row;
        DataType modified;
        for (int i = 0; i < m_colsize; i++)
        {
            if (detectType(vs[i], modified) == m_dTypes[i])
            {
                row.push_back(modified);
            }
            else
            {
                std::cout << "U" << std::endl;
                return;
            }
        }

        for (int i = 0; i < m_colsize; i++)
        {
            updateElementInColumnAuto(i, row[i], rowId);
        }
    }

    void addRowManual()
    {
        std::vector<DataType> row;
        DataType modified;
        std::string s = "";
        for (int i = 0; i < m_colsize; i++)
        {
            std::cout << "Enter " << m_colNames[i] << " with " << m_dTypes[i] << " type: ";
            std::cin >> s;

            if (detectType(s, modified) == m_dTypes[i])
            {
                row.push_back(modified);
            }
            else
            {
                std::cout << "Invalid input." << std::endl;
                return;
            }
        }

        for (int i = 0; i < m_colsize; i++)
        {
            addElementToColumnAuto(i, row[i]);
        }
    }


    bool addRowFW(std::vector<std::string> vs)
    {
        std::vector<DataType> row;
        DataType modified;
        for (int i = 0; i < m_colsize; i++)
        {
            if (detectType(vs[i], modified) == m_dTypes[i])
            {
                row.push_back(modified);
            }
            else
            {
                std::cout << "F" << std::endl;
                return false;
            }
        }

        for (int i = 0; i < m_colsize; i++)
        {
            addElementToColumnAuto(i, row[i]);
        }
        return true;
    }

    template <typename T>
    void printTable()
    {
        for (int j = 0; j < std::get<std::vector<T>>(m_columns[0]).size(); j++)
        {
            for (int i = 0; i < m_colsize; i++)
            {
                if (i < m_columns.size() && std::holds_alternative<std::vector<T>>(m_columns[i])) {
                    std::cout << std::get<std::vector<T>>(m_columns[i])[j] << " ";
                }
                else {
                    std::cerr << "Err 1!" << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }

    size_t getMaxRows() const {
        size_t maxRows = 0;
        for (const auto& column : m_columns) {
            std::visit([&maxRows](const auto& col) {
                maxRows = std::max(maxRows, col.size());
                }, column);
        }
        return maxRows;
    }

    void print() const {
        for (size_t i = 0; i < m_colNames.size(); i++)
        {
            std::cout << std::setw(15) << m_colNames[i];
        }
        std::cout << std::endl;

        size_t numRows = getMaxRows();
        for (size_t row = 0; row < numRows; ++row)
        {
            for (const auto& column : m_columns)
            {
                std::visit([row](const auto& col)
                    {
                        if (row < col.size())
                        {
                            std::cout << std::setw(15) << col[row];
                        }
                        else
                        {
                            std::cout << std::setw(15) << " ";
                        }
                    }, column);
            }
            std::cout << std::endl;
        }
    }

    int getColumnSize() const
    {
        return m_colsize;
    }
};

class DataBase
{
    std::unordered_map <std::string, std::pair<Table, bool>> m_DB;

public:
    DataBase(std::unordered_map <std::string, std::pair<Table, bool>> DB) :
        m_DB(DB)
    {}

    void connectDB(std::unordered_map <std::string, std::pair<Table, bool>> DB)
    {
        m_DB = DB;
    }
};

void saveToFile(const std::unordered_map<std::string, std::pair<Table, bool>>& DB, const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return;
    }

    for (const auto& [tableName, tablePair] : DB) {
        if (!tablePair.second) continue;
        const Table& table = tablePair.first;

        file << tableName << "\n";
        file << table.getColumnSize() << "\n";

        for (int i = 0; i < table.getColumnSize(); ++i) {
            file << table.getColNames()[i] << " " << table.getDataTypes()[i] << "\n";
        }

        size_t numRows = table.getMaxRows();
        for (size_t row = 0; row < numRows; ++row) {
            for (const auto& column : table.getColumns()) {
                std::visit([&file, row](const auto& col) {
                    if (row < col.size()) {
                        file << col[row] << " ";
                    }
                    }, column);
            }
            file << "\n";
        }
        file << "---\n";
    }
    file.close();
    std::cout << "Database saved to " << filename << std::endl;
}

template <typename T>
bool vectorMatch(std::vector<T> v1, std::vector<T> v2)
{
    if (v1.size() != v2.size())
        return false;
    for (int i = 0; i < v1.size(); i++)
        if (v1[i] != v2[i])
            return false;
    return true;
}

std::vector<std::vector<std::string>> tableDifference(std::unordered_map <std::string, std::pair<Table, bool>> DB, std::string table1Name, std::string table2Name)
{
    Table t1 = DB[table1Name].first;
    Table t2 = DB[table2Name].first;
    std::vector<std::vector<std::string>>  retval;

    //std::cout << DB[table1Name].first.getColumns()[0];

    if (t1.getColAmount() != t2.getColAmount())
    {
        std::cout << "Amount of columns does not match." << std::endl;
        return retval;
    }

    std::vector<std::vector<DataType>> v1;
    std::vector<std::vector<DataType>> v2;
    std::vector<bool> mask1;
    std::vector<bool> mask2;
    std::vector<std::vector<std::string>>  result;
    std::vector<std::vector<std::string>>  maskS1;
    std::vector<std::vector<std::string>>  maskS2;


    //std::cout << DB[table1Name].first.getColAmount() << std::endl;

    //form t1 row
    for (int i = 0; i < DB[table1Name].first.getColAmount(); i++)
    {
        ColumnType temp = DB[table1Name].first.getColumns()[i];
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

    result.clear();

    for (int i = 0; i < DB[table2Name].first.getColAmount(); i++)
    {
        ColumnType temp = DB[table2Name].first.getColumns()[i];
        result.push_back(convertToStringVector(temp));
    }

    for (int k = 0; k < result[0].size(); k++)
    {
        std::vector<std::string> temp;
        for (int i = 0; i < result.size(); i++)
        {
            temp.push_back(result[i][k]);
        }
        maskS2.push_back(temp);
    }

    for (int i = 0; i < maskS1.size(); i++)
        mask1.push_back(false);
    for (int i = 0; i < maskS2.size(); i++)
        mask2.push_back(false);

    for (int i = 0; i < maskS1.size(); i++)
    {
        for (int j = 0; j < maskS2.size(); j++)
        {
            if (vectorMatch<std::string>(maskS1[i], maskS2[j]))
            {
                mask1[i] = true;
                mask2[j] = true;
            }
        }
    }

    for (int i = 0; i < mask1.size(); i++)
    {
        if (!mask1[i])
            retval.push_back(maskS1[i]);
    }
    for (int i = 0; i < mask2.size(); i++)
    {
        if (!mask2[i])
            retval.push_back(maskS2[i]);
    }

    return retval;
}


void updateDB()
{
    std::cout << "YES" << std::endl;
}

void loadFromFile(std::unordered_map<std::string, std::pair<Table, bool>>& DB, const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file for reading." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string tableName = line;
        int colAmount;
        file >> colAmount;
        file.ignore();

        Table table(++CURRENT_ID, colAmount, tableName);

        for (int i = 0; i < colAmount; ++i) {
            std::string colName, dataType;
            file >> colName >> dataType;
            table.addColName(colName);
            table.addDataType(dataType);

            if (dataType == "int") {
                table.addColumn<int>();
            }
            else if (dataType == "real") {
                table.addColumn<float>();
            }
            else if (dataType == "complex") {
                table.addColumn<std::complex<double>>();
            }
            else if (dataType == "string") {
                table.addColumn<std::string>();
            }
        }

        file.ignore();

        std::string row;
        while (std::getline(file, row) && row != "---") {
            std::istringstream rowStream(row);
            for (int i = 0; i < colAmount; ++i) {
                if (table.getDataTypes()[i] == "int") {
                    int val;
                    rowStream >> val;
                    table.addElementToColumn<int>(i, val);
                }
                else if (table.getDataTypes()[i] == "real") {
                    float val;
                    rowStream >> val;
                    table.addElementToColumn<float>(i, val);
                }
                else if (table.getDataTypes()[i] == "complex") {
                    std::complex<double> val;
                    rowStream >> val;
                    table.addElementToColumn<std::complex<double>>(i, val);
                }
                else if (table.getDataTypes()[i] == "string") {
                    std::string val;
                    rowStream >> val;
                    table.addElementToColumn<std::string>(i, val);
                }
            }
        }

        DB[tableName] = { table, true };
    }

    file.close();
    std::cout << "Database loaded from " << filename << std::endl;
}