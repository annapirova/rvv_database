#pragma once
#include "Chunk.h"
#include "Column.h"
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

public:
    Table(std::string name) : table_name(std::move(name)) {}

    // Метод добавления колонки в схему (вызывается при парсинге строки типов)
    void add_column_to_schema(const std::string& name, DataType type) {
        column_names_.push_back(name);
        column_types_.push_back(type);
    }

    // Вспомогательный метод создания и аллокации нового чанка по схеме
    void create_new_chunk();

    void load_from_file(const std::string& filename);

    template<typename T>
    void load_column_data(size_t index, size_t rows, std::ifstream& file);

    void load_column_by_type(size_t index, size_t rows, std::ifstream& file);

    DataType parse_type(const std::string& type_str) const;

    int get_column_index(const std::string& name) const noexcept {
        for (int i = 0; i < column_names_.size(); ++i) {
            if (column_names_[i] == name) return i;
        }
        return -1; // Не найдено
    }

    Column& get_column(size_t chunk_id, size_t column_id) {
        return chunks.at(chunk_id).get_column(column_id);
    }

    Column& get_last_chunk_column(const std::string& name) {
        return chunks.back().get_column(get_column_index(name));
    }

    std::vector<Chunk>& get_chunks() {
        return chunks;
    }

    std::vector<std::string>& column_names() {
        return column_names_;
    }

    std::vector<DataType>& column_types() {
        return column_types_;
    }

    size_t get_chunk_size() const noexcept {
        return chunks.size();
    }
};

