#pragma once
#include "Column.h"
#include <stdexcept>
#include <vector>
#include <string>

class Chunk {
    std::vector<Column> columns;

public:

    Chunk() = default;
    Chunk(const Chunk&) = delete; // Запрещаем копирование
    Chunk& operator=(const Chunk&) = delete; // Запрещаем присваивание
    Chunk(Chunk&& other) noexcept : columns(std::move(other.columns)) {}
    Chunk& operator=(Chunk&& other) noexcept {
        if (this != &other) {
            columns = std::move(other.columns);
        }
        return *this;
    }

    Chunk(std::initializer_list<DataType> list) {
        for (const auto& type : list) {
            columns.emplace_back(type);
        }
    }

    Chunk(std::vector<DataType> list) {
        for (const auto& type : list) {
            columns.emplace_back(type);
        }
    }

    bool is_full() const noexcept {
        if (columns.empty()) return false;
        return columns[0].is_full();
    }

    bool is_empty() const noexcept {
        if (columns.empty()) return true;
        return columns[0].is_empty();
    }

    size_t get_rows_count() const noexcept {
        if (columns.empty()) return 0;
        return columns[0].get_row_count();
    }

    std::vector<Column>& get_columns() {
        return columns;
    }

    const std::vector<Column>& get_columns() const {
        return columns;
    }

    Column& get_column(size_t index) {
        return columns.at(index);
    }

    const Column& get_column(size_t index) const {
        return columns.at(index);
    }

    void add_column(const DataType& type) {
        columns.emplace_back(type);
    }

    void remove_column(size_t index) {
        if (index >= columns.size()) {
            throw std::out_of_range("Column index out of bounds");
        }
        columns.erase(columns.begin() + index);
    }

    size_t get_column_count() const noexcept {
        return columns.size();
    }

    // Аллоцируем память 1 колонки в зависимости от типа по индексу (используется, если нужно аллоцировать 1 колонку в непустом чанке)
    void allocate_column(size_t index);

    // Аллоцируем память колонок в зависимости от типа (используется, если нужно аллоцировать пустой чанк)
    void allocate_all_columns();
};
