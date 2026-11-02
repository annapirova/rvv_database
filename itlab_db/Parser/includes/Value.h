#pragma once
#include <string>
#include <cstdint>
#include <stdexcept>
#include "../Table/include/Column.h" // Для DataType

enum class WhereOpType {
    EQUAL, NOTEQUAL, BIGGER, LOWER, BIGGEREQUAL, LOWEREQUAL, AND, OR, NUL, NAME
};

class Value {
    DataType type;
    int32_t int_val;
    double float_val;
    std::string str_val;

public:
    Value() : type(DataType::INT32), int_val(0) {}
    Value(int32_t v) : type(DataType::INT32), int_val(v) {}
    Value(double v) : type(DataType::FLOAT32), float_val(v) {}
    Value(const std::string& v) : type(DataType::FIXED_STRING), str_val(v) {}
    Value(const char* v) : type(DataType::FIXED_STRING), str_val(v) {}

    bool is_numeric() const {
        return type == DataType::INT32 || type == DataType::FLOAT32;
    }

    bool operator==(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const { return !((*this) > other); }
    bool operator>=(const Value& other) const { return !((*this) < other); }
    bool operator!=(const Value& other) const { return !((*this) == other); }
    
    std::string to_string() const;
};
