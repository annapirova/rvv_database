#pragma once
#include <stdexcept>
#include <cstdlib>
#include <new>
#include <cstdint>
#include <cstring>
#include "FixedString.h"

enum class DataType : uint8_t {
    UNKNOWN,
    INT32,
    INT64,
    FLOAT32,
    FIXED_STRING32,
    FIXED_STRING64,
    FIXED_STRING128
};

class Column {

public:
    static constexpr size_t MAX_ROWS = 64;
    static constexpr size_t ALIGNMENT = 16;
    Column(DataType type) : row_count_(0), type_(type), raw_data_(nullptr) {}

    template <typename T>
    Column(DataType type, std::initializer_list<T> list) : row_count_(0), type_(type), raw_data_(nullptr) {
        if (list.size() > MAX_ROWS) {
            throw std::invalid_argument("Initializer list is too large for Column");
        }
        allocate_aligned<T>();
        T* ptr = get<T>();
        for (const auto& item : list) {
            ptr[row_count_++] = item;
        }
    }

    Column(Column&& other) noexcept : row_count_(0), type_(DataType::UNKNOWN), raw_data_(nullptr) {
        swap(*this, other);
    }

    Column& operator=(Column&& other) noexcept {
        if (this != &other) {
            free_aligned();
            swap(*this, other);
        }
        return (*this);
    }

    ~Column() {
        free_aligned();
    }

    // Умная функция выделения выровненной памяти
    template<typename T>
    inline void allocate_aligned() {
        free_aligned();
        raw_data_ = ::operator new[](sizeof(T)* MAX_ROWS, ::std::align_val_t(ALIGNMENT));
        std::memset(raw_data_, 0, sizeof(T) * MAX_ROWS);
    }

    inline void free_aligned() {
        if (raw_data_ != nullptr) {
            ::operator delete[](raw_data_, ::std::align_val_t(ALIGNMENT));
            raw_data_ = nullptr;
            row_count_ = 0;
            type_ = DataType::UNKNOWN;
        }
    }

    // Возвращает указатель на данные, приведенный к типу T
    template<typename T>
    inline T* getDataT() {
        return static_cast<T*>(raw_data_);
    }

    template<typename T>
    inline const T* getDataT() const {
        return static_cast<T*>(raw_data_);
    }

    // Возвращает ссылку на элемент по индексу, проверяя границы
    template<typename T>
    inline T& get(size_t index) {
        if (index >= row_count_) throw std::out_of_range("Index is out of range!");
        return static_cast<T*>(raw_data_)[index];
    }

    template<typename T>
    inline const T& get(size_t index) const {
        if (index >= row_count_) throw std::out_of_range("Index is out of range!");
        return static_cast<T*>(raw_data_)[index];
    }

    // Добавляет элемент в колонку, проверяя, не превышен ли лимит
    template<typename T>
    size_t append(const T& value) {
        if (is_full()) throw std::out_of_range("Column is full!");
        static_cast<T*>(raw_data_)[row_count_] = value;
        return row_count_++;
    }

    // Устанавливает значение элемента по индексу, проверяя границы
    template<typename T>
    void set(size_t index, const T& value) {
        if (index >= row_count_) throw std::out_of_range("Index is out of range!");
        static_cast<T*>(raw_data_)[index] = value;
    }

    // Возвращает указатель на сырые данные
    void* data() { return raw_data_; }
    const void* data() const { return raw_data_; }

    size_t get_row_count() const noexcept { return row_count_; }
    DataType type() const noexcept { return type_; }
    bool is_full() const noexcept { return row_count_ >= MAX_ROWS; }
    bool is_empty() const noexcept { return row_count_ == 0; }

    friend void swap(Column& a, Column& b) noexcept {
        using std::swap;
        swap(a.raw_data_, b.raw_data_);
        swap(a.type_, b.type_);
        swap(a.row_count_, b.row_count_);
    }

private:
    void* raw_data_;
    DataType type_;
    size_t row_count_;
};
