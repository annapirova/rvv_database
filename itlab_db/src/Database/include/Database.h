#pragma once
#include "TableContainer.h"

//class Database {
//private:
//    // Имя таблицы -> Контейнер (Данные + Все её Индексы)
//    std::unordered_map<std::string, std::unique_ptr<TableContainer>> tables_catalog;
//
//public:
//    void create_table(const std::string& name, const std::vector<std::pair<std::string, DataType>>& schema) {
//        auto container = std::make_unique<TableContainer>(name);
//        for (const auto& [col_name, col_type] : schema) {
//            container->table_data.add_column_to_schema(col_name, col_type);
//        }
//        tables_catalog[name] = std::move(container);
//    }
//
//    TableContainer* get_table_container(const std::string& name) {
//        auto it = tables_catalog.find(name);
//        if (it != tables_catalog.end()) return it->second.get();
//        return nullptr;
//    }
//};
