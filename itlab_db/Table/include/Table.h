#pragma once
#include "../../src/data_structs/swiss_table.hpp"
#include "Chunk.h"
#include "Column.h"
#include "FixedString.h"
#include "LogicalColumnView.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Table {
  std::string table_name;
  std::vector<std::string> column_names_;
  std::vector<DataType> column_types_;
  std::vector<Chunk> chunks;

  // Вспомогательный метод создания и аллокации нового чанка по схеме
  void create_new_chunk();

public:
  Table(std::string name) : table_name(std::move(name)) {}
  Table(const Table &) = delete;            // Запрещаем копирование
  Table &operator=(const Table &) = delete; // Запрещаем присваивание

  Table(Table &&other) noexcept;
  Table &operator=(Table &&other) noexcept;

  // Метод добавления колонки по имени и типу
  void add_column(const std::string &name, DataType type);

  // Метод удаления колонки по имени
  void remove_column(const std::string &name);

  // Получение типа DataType из строки
  DataType parse_type(const std::string &type_str) const;

  // Основная функция загрузки данных из файла
  void load_from_file(const std::string &filename);

  // Шаблонная функция загрузки данных в колонку по типу
  template <typename T>
  void load_column_data(std::istringstream &ss, size_t index, size_t rows);

  // Получение индекса колонки по имени
  int get_column_index(const std::string &name) const noexcept;

  Column &get_column(size_t chunk_id, size_t column_id) {
    return chunks.at(chunk_id).get_column(column_id);
  }

  const Column &get_column(size_t chunk_id, size_t column_id) const {
    return chunks.at(chunk_id).get_column(column_id);
  }

  // Получение логического представления колонки по индексу колонки в чанке
  template <typename T> LogicalColumnView at(size_t column_id) {
    if (column_id >= column_names_.size())
      throw std::invalid_argument("Column not found: " +
                                  std::to_string(column_id));
    return LogicalColumnView(chunks, column_id, column_types_[column_id]);
  }

  //----------------------------
  // Hash Join
  //----------------------------
  // Table *hashJoin(const Table &other, const std::string &thisCol,
  //                 const std::string &otherCol) const {

  //   const Table *build;
  //   const Table *probe;
  //   std::string buildCol, probeCol;

  //   if (column_names_.size() <= other.column_names_.size()) {
  //     build = this;
  //     buildCol = thisCol;
  //     probe = &other;
  //     probeCol = otherCol;
  //   } else {
  //     build = &other;
  //     buildCol = otherCol;
  //     probe = this;
  //     probeCol = thisCol;
  //   }

  //   const size_t bIdx = build->get_column_index(buildCol);
  //   const size_t pIdx = probe->get_column_index(probeCol);
  //   if (bIdx == -1 || pIdx == -1) {
  //     throw "No such column in table";
  //   }

  //   size_t build_size = 0;
  //   for (auto chunks_iter = build->chunks.begin();
  //        chunks_iter < build->chunks.end(); ++chunks_iter) {
  //     build_size += chunks_iter->get_rows_count();
  //   }
  //   size_t probe_size = 0;
  //   for (auto chunks_iter = probe->chunks.begin();
  //        chunks_iter < probe->chunks.end(); ++chunks_iter) {
  //     probe_size += chunks_iter->get_rows_count();
  //   }

  //   HashTable<std::string, std::vector<size_t>> ht(build_size * 2);

  //   for (size_t i = 0; i < build_size; ++i) {
  //     const std::string &key = build->rows_[i][bIdx];
  //     try {
  //       ht.get(key).push_back(i);
  //     } catch (const std::out_of_range &) {
  //       ht.insert(key, {i});
  //     }
  //   }
  //   ht.printStats();

  //   std::vector<std::string> outCols;
  //   for (const auto &c : columns_)
  //     outCols.push_back(table_name + "." + c);
  //   for (const auto &c : other.columns_)
  //     outCols.push_back(other.table_name + "." + c);

  //   Table result(table_name + "_JOIN_" + other.table_name, outCols);

  //   for (const auto &prow : probe->rows_) {
  //     const std::string &key = prow[pIdx];
  //     if (!ht.contains(key))
  //       continue;

  //     const auto &matches = ht.get(key);
  //     for (size_t bidx : matches) {
  //       const auto &brow = build->rows_[bidx];
  //       Row combined;
  //       combined.reserve(outCols.size());

  //       if (build == this) {
  //         combined.insert(combined.end(), brow.begin(), brow.end());
  //         combined.insert(combined.end(), prow.begin(), prow.end());
  //       } else {
  //         combined.insert(combined.end(), prow.begin(), prow.end());
  //         combined.insert(combined.end(), brow.begin(), brow.end());
  //       }
  //       result.addRow(combined);
  //     }
  //   }
  //   return &result;
  // }

  std::vector<Chunk> &get_chunks() { return chunks; }

  const std::vector<Chunk> &get_chunks() const { return chunks; }

  std::vector<std::string> &get_column_names() { return column_names_; }

  const std::vector<std::string> &get_column_names() const {
    return column_names_;
  }

  std::vector<DataType> &get_column_types() { return column_types_; }

  const std::vector<DataType> &get_column_types() const {
    return column_types_;
  }

  size_t get_chunk_size() const noexcept { return chunks.size(); }

  std::string &get_name() { return table_name; }

  const std::string &get_name() const { return table_name; }
};
