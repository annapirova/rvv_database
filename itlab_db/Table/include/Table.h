#pragma once
#include "Chunk.h"
#include "Column.h"
#include "FixedString.h"
#include "LogicalColumnView.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

class Table
{
    std::string table_name;
    std::vector<std::string> column_names_;
    std::vector<DataType> column_types_;
    std::vector<Chunk> chunks;

    // Вспомогательный метод создания и аллокации нового чанка по схеме
    void create_new_chunk();

public:
    Table(std::string name) : table_name(std::move(name)) {}
    Table(const Table&) = delete; // Запрещаем копирование
    Table& operator=(const Table&) = delete; // Запрещаем присваивание

    Table(Table&& other) noexcept;
    Table& operator=(Table&& other) noexcept;

    // Метод добавления колонки по имени и типу
    void add_column(const std::string& name, DataType type);

    // Метод удаления колонки по имени
    void remove_column(const std::string& name);

    // Получение типа DataType из строки
    DataType parse_type(const std::string& type_str) const;

    // Основная функция загрузки данных из файла
    void load_from_file(const std::string& filename);

    // Шаблонная функция загрузки данных в колонку по типу
    template<typename T>
    void load_column_data(std::istringstream& ss, size_t index, size_t rows);

    // Получение индекса колонки по имени
    int get_column_index(const std::string& name) const noexcept;

    Column& get_column(size_t chunk_id, size_t column_id) {
        return chunks.at(chunk_id).get_column(column_id);
    }

    const Column& get_column(size_t chunk_id, size_t column_id) const {
        return chunks.at(chunk_id).get_column(column_id);
    }
    
    // Получение логического представления колонки по индексу колонки в чанке
    template<typename T>
    LogicalColumnView at(size_t column_id) {
        if (column_id >= column_names_.size()) throw std::invalid_argument("Column not found: " + std::to_string(column_id));
        return LogicalColumnView(chunks, column_id, column_types_[column_id]);
    }

    std::vector<Chunk>& get_chunks() {
        return chunks;
    }

    const std::vector<Chunk>& get_chunks() const {
        return chunks;
    }

    std::vector<std::string>& get_column_names() {
        return column_names_;
    }

    const std::vector<std::string>& get_column_names() const {
        return column_names_;
    }

    std::vector<DataType>& get_column_types() {
        return column_types_;
    }

    const std::vector<DataType>& get_column_types() const {
        return column_types_;
    }

    size_t get_chunk_size() const noexcept {
        return chunks.size();
    }

    std::string& get_name() {
        return table_name;
    }

    const std::string& get_name() const {
        return table_name;
    }
};
