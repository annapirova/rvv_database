#include <gtest/gtest.h>
#include "Column.h"
#include <cstdint>
#include <cstring>

// === Базовое создание ===

TEST(ColumnTest, DefaultState) {
    Column col(DataType::INT32);
    EXPECT_EQ(col.get_count(), 0);
    EXPECT_TRUE(col.is_empty());
    EXPECT_FALSE(col.is_full());
    EXPECT_EQ(col.type(), DataType::INT32);
}

TEST(ColumnTest, AllocateAligned) {
    Column col(DataType::INT32);
    col.allocate_aligned<int32_t>();

    // Память должна быть выделена
    EXPECT_NE(col.data(), nullptr);

    // Проверка выравнивания
    uintptr_t addr = reinterpret_cast<uintptr_t>(col.data());
    EXPECT_EQ(addr % Column::ALIGNMENT, 0);

    EXPECT_EQ(col.get_count(), 0);
}

// === Append и доступ ===

TEST(ColumnTest, AppendInt32) {
    Column col(DataType::INT32);
    col.allocate_aligned<int32_t>();

    col.append<int32_t>(42);
    col.append<int32_t>(-100);
    col.append<int32_t>(0);

    EXPECT_EQ(col.get_count(), 3);
    EXPECT_FALSE(col.is_empty());

    int32_t* data = col.get<int32_t>();
    EXPECT_EQ(data[0], 42);
    EXPECT_EQ(data[1], -100);
    EXPECT_EQ(data[2], 0);
}

TEST(ColumnTest, AppendInt64) {
    Column col(DataType::INT64);
    col.allocate_aligned<int64_t>();

    col.append<int64_t>(1234567890123LL);
    col.append<int64_t>(-9876543210987LL);

    int64_t* data = col.get<int64_t>();
    EXPECT_EQ(data[0], 1234567890123LL);
    EXPECT_EQ(data[1], -9876543210987LL);
}

TEST(ColumnTest, AppendFloat32) {
    Column col(DataType::FLOAT32);
    col.allocate_aligned<float>();

    col.append<float>(3.14f);
    col.append<float>(-2.71f);

    float* data = col.get<float>();
    EXPECT_FLOAT_EQ(data[0], 3.14f);
    EXPECT_FLOAT_EQ(data[1], -2.71f);
}

// === Set ===

TEST(ColumnTest, Set) {
    Column col(DataType::INT32);
    col.allocate_aligned<int32_t>();

    col.append<int32_t>(1);
    col.append<int32_t>(2);

    col.set<int32_t>(0, 100);
    col.set<int32_t>(1, 200);

    int32_t* data = col.get<int32_t>();
    EXPECT_EQ(data[0], 100);
    EXPECT_EQ(data[1], 200);
}

TEST(ColumnTest, SetOutOfRange) {
    Column col(DataType::INT32);
    col.allocate_aligned<int32_t>();

    col.append<int32_t>(1);

    EXPECT_THROW(col.set<int32_t>(5, 100), std::out_of_range);
}

// === Переполнение ===

TEST(ColumnTest, FullColumn) {
    Column col(DataType::INT32);
    col.allocate_aligned<int32_t>();

    // Заполняем все MAX_ROWS элементов
    for (size_t i = 0; i < Column::MAX_ROWS; ++i) {
        col.append<int32_t>(static_cast<int32_t>(i));
    }

    EXPECT_TRUE(col.is_full());
    EXPECT_EQ(col.get_count(), Column::MAX_ROWS);

    // Следующий append должен бросить исключение
    EXPECT_THROW(col.append<int32_t>(999), std::out_of_range);
}

// === Move semantics ===

TEST(ColumnTest, MoveConstructor) {
    Column col1(DataType::INT32);
    col1.allocate_aligned<int32_t>();
    col1.append<int32_t>(42);

    void* original_ptr = col1.data();

    Column col2(std::move(col1));

    // col2 получил данные
    EXPECT_EQ(col2.data(), original_ptr);
    EXPECT_EQ(col2.get_count(), 1);
    EXPECT_EQ(col2.get<int32_t>()[0], 42);

    // col1 очищен
    EXPECT_EQ(col1.data(), nullptr);
    EXPECT_EQ(col1.get_count(), 0);
}

TEST(ColumnTest, MoveAssignment) {
    Column col1(DataType::INT32);
    col1.allocate_aligned<int32_t>();
    col1.append<int32_t>(42);

    Column col2(DataType::INT64);
    col2 = std::move(col1);

    EXPECT_EQ(col2.type(), DataType::INT32);
    EXPECT_EQ(col2.get_count(), 1);
}

// === Выравнивание для SIMD ===

TEST(ColumnTest, Alignment) {
    Column col(DataType::INT32);
    col.allocate_aligned<int32_t>();

    uintptr_t addr = reinterpret_cast<uintptr_t>(col.data());
    EXPECT_EQ(addr % 16, 0) << "Память должна быть выровнена по 16 байт для SIMD?";
}