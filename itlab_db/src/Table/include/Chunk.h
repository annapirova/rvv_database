#pragma once
#include "Column.h"
#include <stdexcept>
#include <vector>
#include <string>

class Chunk {
    //std::byte* memory;
    std::vector<Column> columns;

public:
    // Чанк полон, если хотя бы одна его колонка заполнена
    bool is_full() const noexcept {
        if (columns.empty()) return false;
        return columns[0].is_full();
    }

    size_t get_rows_count() const noexcept {
        if (columns.empty()) return 0;
        return columns[0].get_count();
    }

    Column& get_column(size_t index) {
        return columns.at(index);
    }

    void add_column(const DataType& type) {
        columns.emplace_back(type);
    }
};

