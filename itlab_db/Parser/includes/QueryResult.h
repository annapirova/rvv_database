#pragma once
#include "Value.h"
#include "Table.h"
#include <vector>

class QueryResult {
    const Table& table;
    std::vector<size_t> rows;
    std::vector<size_t> cols;

public:
    QueryResult(const Table& _table, std::vector<size_t>&& _rows, std::vector<size_t>&& _cols);
    QueryResult(QueryResult&& other) noexcept;

    const std::vector<std::string>& get_names() const;
    const std::vector<size_t>& get_rows() const;
    const std::vector<size_t>& get_cols() const;
    
    void set_cols(std::vector<size_t> new_cols);
    void set_rows(std::vector<size_t> new_rows);
    
    std::vector<Value> get_row(size_t row_id) const;
    const Table& get_table() const;
};
