#pragma once
#include "Table.h"
#include "vector"
#include <memory>

// Типы индексов в системе
enum class IndexType : uint8_t { BPLUS_TREE, FLAT_HASH };

// Обёртка над void* указателем, чтобы не использовать наследование классов индексов
struct Index {
    std::string target_column_name; // По какой колонке построен индекс
    IndexType index_type;           // Какого типа индекс (Дерево или Хэш)
    void* raw_index_ptr = nullptr;  // Сырой указатель на шаблонный класс (Дерево или Хэш)
};

class TableContainer {

    Table table_data;
    std::vector<Index> indexes_;

public:
    TableContainer(std::string table_name) : table_data(std::move(table_name)) {}

    // Метод регистрации нового индекса
    void register_index(const std::string& col_name, IndexType i_type, void* raw_tree) {
        indexes_.push_back({ col_name, i_type, raw_tree });
    }

    // Поиск нужного индекса по имени колонки и типу (если нет - nullptr)
    Index* get_index(const std::string& col_name, IndexType i_type);
};
