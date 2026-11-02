#pragma once
#include "Chunk.h"

class LogicalColumnView {
    const std::vector<Chunk>& chunks_;
    size_t col_index_;
    DataType type_;

public:
    LogicalColumnView(const std::vector<Chunk>& chunks, size_t col_idx, DataType type)
        : chunks_(chunks), col_index_(col_idx), type_(type) {
    }

    // Получение значения по глобальному индексу строки
    template<typename T>
    const T& get(size_t global_row_id) const;

    // Установка значения по глобальному индексу строки
    template<typename T>
    void set(size_t global_row_id, const T& value) const;

    // Добавляет элемент в колонку, проверяя, не превышен ли лимит
    template<typename T>
    size_t append(const T& value) {
        if (is_full()) throw std::out_of_range("LogicalColumn is full!");
        chunks_.back().get_column(col_index_).append(value);
        return total_rows() - 1;
    }

    template<typename T> std::pair<T*, size_t> get_chunk_data(size_t chunk_id) const;

    size_t total_rows() const noexcept {
        if (chunks_.empty()) return 0;
        return (chunks_.size() - 1) * Column::MAX_ROWS + chunks_.back().get_rows_count();
    }

    DataType type() const noexcept { return type_; }
    bool is_full() const noexcept { return chunks_.back().get_rows_count() >= Column::MAX_ROWS; }
    bool is_empty() const noexcept { return chunks_.front().get_rows_count() == 0; }
};

template<typename T>
inline const T& LogicalColumnView::get(size_t global_row_id) const {
    return chunks_.at(global_row_id / Column::MAX_ROWS).get_column(col_index_).get<T>(global_row_id % Column::MAX_ROWS);
}

template<typename T>
inline void LogicalColumnView::set(size_t global_row_id, const T& value) const {
    if (global_row_id >= total_rows()) throw std::out_of_range("Global row id is out of range!");
    chunks_.at(global_row_id / Column::MAX_ROWS).get_column(col_index_).set<T>(global_row_id % Column::MAX_ROWS, value);
}

template<typename T>
inline std::pair<T*, size_t> LogicalColumnView::get_chunk_data(size_t chunk_id) const {
    Column& col = chunks_.at(chunk_id).get_column(col_index_);
    return std::make_pair(col.getDataT<T>(), col.get_row_count());
}
