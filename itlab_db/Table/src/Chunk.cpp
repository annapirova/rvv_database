#include "../include/Chunk.h"

void Chunk::allocate_column(size_t index) {
    if (index >= columns.size()) {
        throw std::out_of_range("Column index out of bounds");
    }
    auto& col = columns[index];
    switch (col.type()) {
    case DataType::INT32:        col.allocate_aligned<int32_t>(); break;
    case DataType::INT64:        col.allocate_aligned<int64_t>(); break;
    case DataType::FLOAT32:      col.allocate_aligned<float>(); break;
    case DataType::FIXED_STRING32: col.allocate_aligned<FixedString32>(); break;
    case DataType::FIXED_STRING64: col.allocate_aligned<FixedString64>(); break;
    case DataType::FIXED_STRING128: col.allocate_aligned<FixedString128>(); break;
    default: throw std::runtime_error("Unknown data type!");
    }
}

void Chunk::allocate_all_columns(){
    for (auto& col : columns) {
        switch (col.type()) {
        case DataType::INT32:        col.allocate_aligned<int32_t>(); break;
        case DataType::INT64:        col.allocate_aligned<int64_t>(); break;
        case DataType::FLOAT32:      col.allocate_aligned<float>(); break;
        case DataType::FIXED_STRING32: col.allocate_aligned<FixedString32>(); break;
        case DataType::FIXED_STRING64: col.allocate_aligned<FixedString64>(); break;
        case DataType::FIXED_STRING128: col.allocate_aligned<FixedString128>(); break;
        default: throw std::runtime_error("Unknown data type!");
        }
    }
}

