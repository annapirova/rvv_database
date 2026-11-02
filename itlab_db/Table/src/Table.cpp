#include "../include/Table.h"

void Table::create_new_chunk() {
    Chunk new_chunk(column_types_);
    new_chunk.allocate_all_columns();
    chunks.push_back(std::move(new_chunk));
}

Table::Table(Table&& other) noexcept
    : table_name(std::move(other.table_name)),
    column_names_(std::move(other.column_names_)),
    column_types_(std::move(other.column_types_)),
    chunks(std::move(other.chunks)) {
}

Table& Table::operator=(Table&& other) noexcept {
    if (this != &other) {
        table_name = std::move(other.table_name);
        column_names_ = std::move(other.column_names_);
        column_types_ = std::move(other.column_types_);
        chunks = std::move(other.chunks);
    }
    return *this;
}

void Table::add_column(const std::string& name, DataType type) {
    column_names_.push_back(name);
    column_types_.push_back(type);
    for (auto& chunk : chunks) {
        chunk.add_column(type);
    }
}

void Table::remove_column(const std::string& name) {
    int index = get_column_index(name);
    if (index == -1) throw std::invalid_argument("Column not found: " + name);
    column_names_.erase(column_names_.begin() + index);
    column_types_.erase(column_types_.begin() + index);
    for (auto& chunk : chunks) {
        chunk.remove_column(index);
    }
}

// Шаблонная функция загрузки данных из строк в колонку
template<typename T>
void Table::load_column_data(std::istringstream& ss, size_t index, size_t rows) {
    size_t chunk_count = (rows + Column::MAX_ROWS - 1) / Column::MAX_ROWS;
    T value;
    std::string line;
    std::istringstream iss;
    for (size_t i = 0;i < chunk_count; ++i) {
        for (size_t j = 0; (j < Column::MAX_ROWS) && ((i * Column::MAX_ROWS + j) < rows); ++j) {
            std::getline(ss, line, ';');
            iss.str(line);
            iss.clear();
            iss >> value;
            chunks[i].get_column(index).append<T>(value);
        }
    }
}

DataType Table::parse_type(const std::string& type_str) const {
    if (type_str == "INT32") return DataType::INT32;
    if (type_str == "INT64") return DataType::INT64;
    if (type_str == "FLOAT32") return DataType::FLOAT32;
    if (type_str == "FIXED_STRING32") return DataType::FIXED_STRING32;
    if (type_str == "FIXED_STRING64") return DataType::FIXED_STRING64;
    if (type_str == "FIXED_STRING128") return DataType::FIXED_STRING128;
    throw std::runtime_error("Unknown type: " + type_str);
}

int Table::get_column_index(const std::string& name) const noexcept {
    for (int i = 0; i < column_names_.size(); ++i) {
        if (column_names_[i] == name) return i;
    }
    return -1; // Не найдено
}

void Table::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;

    // Строка 1: число N (количество колонок)
    std::getline(file, line);
    size_t num_columns = std::stoull(line);

    // Заводим строку для хранения имени колонки или типа колонки
    std::string cell;
    int i_num=0;

    // Строка 2: считываем строку с именами колонок через разделитель ; и обрабатываем её
    std::getline(file, line);
    std::istringstream ss(line);
    while (std::getline(ss, cell, ';') && i_num<num_columns) {
        column_names_.push_back(cell);++i_num;
    }
    i_num=0;
    
    // Строка 3: считываем строку с типами колонок через разделитель ; и обрабатываем её
    std::getline(file, line);
    ss.str(line);ss.clear();
    while (std::getline(ss, cell, ';') && i_num<num_columns) {
        column_types_.push_back(parse_type(cell));++i_num;
    }

    // Строка 4: считываем число M (количество строк в чанке)
    std::getline(file, line);
    size_t num_rows = std::stoull(line);

    // Создаём необходимое количество чанков
    size_t chunk_count = (num_rows + Column::MAX_ROWS - 1) / Column::MAX_ROWS;
    for (size_t i = 0; i < chunk_count;++i) {
        create_new_chunk();
    }

    // Для каждой колонки читаем данные
    for (size_t col_idx = 0; col_idx < num_columns; ++col_idx) {
        std::getline(file, line);
        ss.str(line);ss.clear();
        switch (column_types_[col_idx]) {
        case DataType::INT32:
            load_column_data<int32_t>(ss, col_idx, num_rows);
            break;
        case DataType::INT64:
            load_column_data<int64_t>(ss, col_idx, num_rows);
            break;
        case DataType::FLOAT32:
            load_column_data<float>(ss, col_idx, num_rows);
            break;
        case DataType::FIXED_STRING32:
            load_column_data<FixedString32>(ss, col_idx, num_rows);
            break;
        case DataType::FIXED_STRING64:
            load_column_data<FixedString64>(ss, col_idx, num_rows);
            break;
        case DataType::FIXED_STRING128:
            load_column_data<FixedString128>(ss, col_idx, num_rows);
            break;
        default:
            throw std::runtime_error("Unknown column type");
        }
    }

    file.close();
}
