#pragma once
#include <stdexcept>
#include <array>
#include <new>

enum class DataType {
    INT32,
    INT64,
    FLOAT32,
    FIXED_STRING
};

class Column {

public:
    static constexpr size_t MAX_ROWS = 1024;
    static constexpr size_t ALIGNMENT = 64;
    Column(DataType type) : row_count_(0), type_(type), raw_data_(nullptr), deleter(nullptr) {}
    ~Column() {
        free_aligned(raw_data_);
    }

    template<typename T>
    T* get() const {
        return reinterpret_cast<T*>(raw_data_);
    }

    // Умная функция выделения выровненной памяти
    template<typename T>
    void allocate_aligned() {

        free_aligned(raw_data_);

        // 1. Выделение "чистой" выровненной памяти и заполнение нулями ()
        raw_data_ = ::operator new[](sizeof(T)* MAX_ROWS, std::align_val_t(ALIGNMENT));

        // 2. Безопасный Deleter (деструкторы вызывать не нужно, тип тривиальный)
        deleter = [](void* ptr) {
            if (!ptr) return;
            ::operator delete[](ptr, std::align_val_t(ALIGNMENT));
        };
    }

    inline void free_aligned(void* ptr) {
        if (raw_data_ && deleter) {
            deleter(raw_data_);
            raw_data_ = nullptr;
            deleter = nullptr;
        }
    }

    template<typename T>
    void append(const T& value) {
        if (is_full()) throw std::out_of_range("Column is full!");
        get<T>()[row_count_++] = value;
    }

    template<typename T>
    void set(size_t index, const T& value) {
        if (index >= row_count_) throw std::out_of_range("Index is out of range!");
        get<T>()[index] = value;
    }

    void* data() { return raw_data_; }
    size_t get_count() const noexcept { return row_count_; }
    DataType type() const noexcept { return type_; }
    bool is_full() const noexcept { return row_count_ >= MAX_ROWS; }
    bool is_empty() const noexcept { return row_count_ == 0; }

private:
    void* raw_data_;
    DataType type_;
    size_t row_count_;
    void (*deleter)(void* ptr);
};
