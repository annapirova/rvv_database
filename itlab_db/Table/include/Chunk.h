#pragma once
#include "Column.h"
#include <stdexcept>
#include <vector>
#include <string>

class Chunk {
    std::vector<Column> columns;

public:
    // Чанк полон, если хотя бы одна его колонка заполнена
    Chunk() {}
    Chunk(Chunk&& other) noexcept {
        columns = std::move(other.columns);
    }

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

    Chunk& operator=(Chunk&& other) noexcept {
        if (this != &other) {
            columns = std::move(other.columns);
        }
        return (*this);
    }
};

