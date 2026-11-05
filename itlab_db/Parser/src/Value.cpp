#include "../include/Value.h"

bool Value::operator==(const Value& other) const {
    if (type == other.type) {
        if (type == DataType::INT32) return int_val == other.int_val;
        if (type == DataType::FLOAT32) return float_val == other.float_val;
        if (type == DataType::FIXED_STRING) return str_val == other.str_val;
    }
    if (this->is_numeric() && other.is_numeric()) {
        double left_d = (type == DataType::INT32) ? static_cast<double>(int_val) : float_val;
        double right_d = (other.type == DataType::INT32) ? static_cast<double>(other.int_val) : other.float_val;
        return left_d == right_d;
    }
    throw std::logic_error("different type");
}

bool Value::operator>(const Value& other) const {
    if (type == other.type) {
        if (type == DataType::INT32) return int_val > other.int_val;
        if (type == DataType::FLOAT32) return float_val > other.float_val;
        if (type == DataType::FIXED_STRING) return str_val > other.str_val;
    }
    if (this->is_numeric() && other.is_numeric()) {
        double left_d = (type == DataType::INT32) ? static_cast<double>(int_val) : float_val;
        double right_d = (other.type == DataType::INT32) ? static_cast<double>(other.int_val) : other.float_val;
        return left_d > right_d;
    }
    throw std::logic_error("different type");
}

bool Value::operator<(const Value& other) const {
    if (type == other.type) {
        if (type == DataType::INT32) return int_val < other.int_val;
        if (type == DataType::FLOAT32) return float_val < other.float_val;
        if (type == DataType::FIXED_STRING) return str_val < other.str_val;
    }
    if (this->is_numeric() && other.is_numeric()) {
        double left_d = (type == DataType::INT32) ? static_cast<double>(int_val) : float_val;
        double right_d = (other.type == DataType::INT32) ? static_cast<double>(other.int_val) : other.float_val;
        return left_d < right_d;
    }
    throw std::logic_error("different type");
}

std::string Value::to_string() const {
    if (type == DataType::INT32) return std::to_string(int_val);
    if (type == DataType::FLOAT32) return std::to_string(float_val);
    return str_val;
}
