#pragma once
#include "Value.h"
#include "../../Table/include/Table.h"
#include <vector>

class QueryResult {
    Table& table;
    std::vector<size_t> rows;
    std::vector<size_t> cols;

public:
    QueryResult(Table& _table, std::vector<size_t>&& _rows, std::vector<size_t>&& _cols);
    QueryResult(QueryResult&& other) noexcept;

    std::vector<std::string>& get_names();
    std::vector<size_t>& get_rows();
    std::vector<size_t>& get_cols();
    
    void set_cols(std::vector<size_t> new_cols);
    void set_rows(std::vector<size_t> new_rows);
    
    std::vector<Value> get_row(size_t row_id);
    Table& get_table();
};
