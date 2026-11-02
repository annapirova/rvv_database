#include "QueryResult.h"

QueryResult::QueryResult(const Table& _table, std::vector<size_t>&& _rows, std::vector<size_t>&& _cols)
    : table(_table), rows(std::move(_rows)), cols(std::move(_cols)) {}

QueryResult::QueryResult(QueryResult&& other) noexcept
    : table(other.table), rows(std::move(other.rows)), cols(std::move(other.cols)) {}

const std::vector<std::string>& QueryResult::get_names() const {
    return table.column_names();
}

const std::vector<size_t>& QueryResult::get_rows() const { return rows; }
const std::vector<size_t>& QueryResult::get_cols() const { return cols; }

void QueryResult::set_cols(std::vector<size_t> new_cols) { cols = std::move(new_cols); }
void QueryResult::set_rows(std::vector<size_t> new_rows) { rows = std::move(new_rows); }

std::vector<Value> QueryResult::get_row(size_t row_id) const {
    std::vector<Value> res;
    size_t ch_id = row_id / 64;
    size_t local_row_id = row_id % 64;
    size_t col_cnt = cols.size();
    
    for(size_t i = 0; i < col_cnt; ++i){
        const Column& col = table.get_chunks()[ch_id].get_column(cols[i]);
        DataType col_type = col.type();
        switch(col_type){
            case DataType::INT32: 
                res.push_back(Value(col.get<int32_t>()[local_row_id])); 
                break;
            case DataType::FLOAT32: 
                res.push_back(Value(col.get<double>()[local_row_id])); 
                break;
            case DataType::FIXED_STRING: 
                res.push_back(Value(std::string(col.get<char[256]>()[local_row_id]))); 
                break;
            default: 
                throw std::logic_error("no way type");
        }
    }
    return res;
}

const Table& QueryResult::get_table() const { return table; }
