#include "../include/Table.h"

void Table::create_new_chunk() {
    Chunk new_chunk;

    for (size_t i = 0; i < column_types_.size(); ++i) {
        new_chunk.add_column(column_types_[i]);

        // Аллоцируем память физически в зависимости от типа
        switch (column_types_[i]) {
        case DataType::INT32:        new_chunk.get_column(i).allocate_aligned<int32_t>(); break;
        case DataType::INT64:        new_chunk.get_column(i).allocate_aligned<int64_t>(); break;
        case DataType::FLOAT32:      new_chunk.get_column(i).allocate_aligned<float>(); break;
        case DataType::FIXED_STRING: new_chunk.get_column(i).allocate_aligned<char[256]>(); break;
        default: throw std::runtime_error("Unknown data type!");
        }
    }
    chunks.push_back(std::move(new_chunk));
}

// Шаблонная функция загрузки данных из строк в колонку
template<typename T>
void Table::load_column_data(size_t index, size_t rows, std::ifstream& file) {
    size_t chunk_count = (rows + Column::MAX_ROWS - 1) / Column::MAX_ROWS;
    for (size_t i = 0;i < chunk_count; ++i) {
        for (size_t j = 0; (j < Column::MAX_ROWS) && ((i * Column::MAX_ROWS + j) < rows); ++j) {
            T value;
            std::string line;
            std::getline(file, line);
            std::istringstream iss(line);
            iss >> value;
            chunks[i].get_column(index).append<T>(value);
        }
    }
}

// Диспетчер: switch по типу вызывает шаблонную функцию
void Table::load_column_by_type(size_t index, size_t rows, std::ifstream& file) {
    switch (column_types_[index]) {
    case DataType::INT32:
        load_column_data<int32_t>(index, rows, file);
        break;
    case DataType::INT64:
        load_column_data<int64_t>(index, rows, file);
        break;
    case DataType::FLOAT32:
        load_column_data<float>(index, rows, file);
        break;
    //case DataType::FIXED_STRING:
    //    load_column_data<FixedString>(col, lines);
    //    break;
    default:
        throw std::runtime_error("Unknown column type");
    }
}

// Парсинг типов из строки
DataType Table::parse_type(const std::string& type_str) const {
    if (type_str == "INT32") return DataType::INT32;
    if (type_str == "INT64") return DataType::INT64;
    if (type_str == "FLOAT32") return DataType::FLOAT32;
    if (type_str == "FIXED_STRING") return DataType::FIXED_STRING;
    throw std::runtime_error("Unknown type: " + type_str);
}

// Основная функция загрузки из файла
void Table::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;

    // Строка 1: число N (количество колонок)
    std::getline(file, line);
    size_t num_columns = std::stoull(line);

    // Строка 2: N слов через пробел (типы колонок)
    std::getline(file, line);
    std::istringstream columns_stream(line);

    for (size_t i = 0; i < num_columns; ++i) {
        std::string name;
        columns_stream >> name;
        column_names_.push_back(name);
    }

    std::getline(file, line);
    std::istringstream types_stream(line);

    for (size_t i = 0; i < num_columns; ++i) {
        std::string type_str;
        types_stream >> type_str;
        column_types_.push_back(parse_type(type_str));
    }

    // Для каждой колонки читаем данные
    for (size_t col_idx = 0; col_idx < num_columns; ++col_idx) {
        // Строка M_i: количество строк для этой колонки
        std::getline(file, line);
        size_t num_rows = std::stoul(line);

        size_t chunk_count = (num_rows + Column::MAX_ROWS - 1)  / Column::MAX_ROWS;
        size_t current_size = chunks.size();
        for (size_t i = current_size; i < chunk_count;++i) {
            create_new_chunk();
        }

        // Загружаем данные в колонку через диспетчер
        load_column_by_type(col_idx, num_rows, file);
    }

    file.close();
}
