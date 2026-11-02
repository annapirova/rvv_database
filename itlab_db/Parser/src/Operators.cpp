#include "Operators.h"
#include <stdexcept>

GetTableOperator::GetTableOperator(const Table& _table) : table(_table) {}

QueryResult GetTableOperator::execute_operator() {
    size_t ch_sz = table.get_chunk_size();
    std::vector<size_t> tmp_rows;
    for(size_t i = 0; i < ch_sz; ++i){
        size_t row_sz = table.get_chunks()[i].get_rows_count();
        for(size_t j = 0; j < row_sz; ++j){
            tmp_rows.push_back(j + 64 * i);
        }
    }
    
    std::vector<size_t> tmp_cols;
    size_t sz = table.column_names().size();
    for(size_t j = 0; j < sz; ++j){
        tmp_cols.push_back(j);
    }
    
    return QueryResult(table, std::move(tmp_rows), std::move(tmp_cols));
}

WhereOperator::WhereOperator(TurboExpr* _current, BaseOperator* _prev) : current(_current), prev(_prev) {}
WhereOperator::~WhereOperator() { delete current; }

QueryResult WhereOperator::execute_operator() {
    QueryResult prev_data = prev->execute_operator();
    std::vector<size_t> valid_rows;
    for (size_t row_id : prev_data.get_rows()) {
        if (current->execute(current, row_id, prev_data.get_table())) {
            valid_rows.push_back(row_id);
        }
    }
    prev_data.set_rows(std::move(valid_rows));
    return prev_data;
}

SelectOperator::SelectOperator(WhereOperator* _where, const std::vector<std::string>& _names) : where(_where), names(_names) {}
SelectOperator::~SelectOperator() { delete where; }

QueryResult SelectOperator::execute_operator() {
    QueryResult prev_data = where->execute_operator();
    if(!names.empty() && names[0] == "*"){
        return prev_data;
    }
    
    std::vector<size_t> new_cols;
    for(const std::string& name : names){
        int32_t index = prev_data.get_table().get_column_index(name);
        if(index == -1){
            throw std::logic_error("Column '" + name + "' not found in table!");
        }
        new_cols.push_back(index);
    }
    prev_data.set_cols(std::move(new_cols));
    return prev_data;
}
