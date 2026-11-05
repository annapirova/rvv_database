#include <gtest/gtest.h>
#include "Table.h"
#include "FixedString.h"
#include <fstream>
#include <cstdio>

TEST(TableContainerTest, test_load_file) {
	Table table("test");
	table.load_from_file("bin/file_data.csv");
	EXPECT_EQ(table.get_chunk_size(), 1);
	size_t index = table.get_column_index("count");
	int temp = table.get_column(0, index).get<int32_t>(0);
	EXPECT_EQ(temp, 100);
    index = table.get_column_index("name");
    auto res = table.get_column(0, index).get<FixedString32>(0);
    EXPECT_EQ(res, FixedString32("Igor"));
}

class TableTest : public ::testing::Test {
protected:
    const std::string test_file = "test_data.csv";

    void SetUp() override {
        // Создаём тестовый файл
        std::ofstream file(test_file);
        file << "3\n";                              // 3 колонки
        file << "id;value;score\n";                 // имена колонок
        file << "INT32;INT64;FLOAT32\n";            // типы
        file << "5\n";                              // 5 строк в таблице
        file << "1;2;3;4;5\n";                      // данные 1 колонки
        file << "100;200;300;400;500\n";                    // данные 2 колонки
        file << "3.14;2.71;1.05;7.77;8.88\n";                      // данные 3 колонки
        file.close();
    }

    void TearDown() override {
        std::remove(test_file.c_str());
    }
};

TEST_F(TableTest, LoadFromFile) {
    Table table("test_table");
    table.load_from_file(test_file);

    EXPECT_EQ(table.get_column_names().size(), 3);
    EXPECT_EQ(table.get_column_names()[0], "id");
    EXPECT_EQ(table.get_column_names()[1], "value");
    EXPECT_EQ(table.get_column_names()[2], "score");

    EXPECT_EQ(table.get_column_types().size(), 3);
    EXPECT_EQ(table.get_column_types()[0], DataType::INT32);
    EXPECT_EQ(table.get_column_types()[1], DataType::INT64);
    EXPECT_EQ(table.get_column_types()[2], DataType::FLOAT32);
}

TEST_F(TableTest, DataLoadedCorrectly) {
    Table table("test_table");
    table.load_from_file(test_file);

    // Проверяем данные первой колонки (INT32)
    Column& col0 = table.get_column(0, 0);
    EXPECT_EQ(col0.get_row_count(), 5);
    int32_t* data0 = col0.getDataT<int32_t>();
    EXPECT_EQ(data0[0], 1);
    EXPECT_EQ(data0[4], 5);

    // Проверяем данные второй колонки (INT64)
    Column& col1 = table.get_column(0, 1);
    EXPECT_EQ(col1.get_row_count(), 5);
    int64_t* data1 = col1.getDataT<int64_t>();
    EXPECT_EQ(data1[0], 100);
    EXPECT_EQ(data1[2], 300);

    // Проверяем данные третьей колонки (FLOAT32)
    Column& col2 = table.get_column(0, 2);
    EXPECT_EQ(col2.get_row_count(), 5);
    float* data2 = col2.getDataT<float>();
    EXPECT_FLOAT_EQ(data2[0], 3.14f);
    EXPECT_FLOAT_EQ(data2[1], 2.71f);
}

TEST_F(TableTest, MultipleChunks) {
    // Создаём файл с большим количеством строк
    const std::string big_file = "test_big_data.csv";
    {
        std::ofstream file(big_file);
        size_t num_rows = Column::MAX_ROWS + 100;  // Больше одного чанка
        file << "1\n";
        file << "id\n";
        file << "INT32\n";
        file << num_rows << "\n";
        for (size_t i = 0; i < num_rows; ++i) {
            file << i << "\n";
        }
        file.close();
    }

    Table table("big_table");
    table.load_from_file(big_file);

    // Должно быть 3 чанка
    EXPECT_EQ(table.get_chunk_size(), 3);

    // Первый чанк полный
    EXPECT_EQ(table.get_column(0, 0).get_row_count(), Column::MAX_ROWS);

    // Второй чанк содержит остаток
    EXPECT_EQ(table.get_column(2, 0).get_row_count(), 36);

    std::remove(big_file.c_str());
}

TEST_F(TableTest, GetColumnIndex) {
    Table table("test_table");
    table.load_from_file(test_file);

    EXPECT_EQ(table.get_column_index("id"), 0);
    EXPECT_EQ(table.get_column_index("value"), 1);
    EXPECT_EQ(table.get_column_index("score"), 2);
    EXPECT_EQ(table.get_column_index("nonexistent"), -1);
}

TEST_F(TableTest, FileNotFound) {
    Table table("test_table");
    EXPECT_THROW(table.load_from_file("nonexistent.txt"), std::runtime_error);
}

TEST_F(TableTest, ParseType) {
    Table table("test_table");
    EXPECT_EQ(table.parse_type("INT32"), DataType::INT32);
    EXPECT_EQ(table.parse_type("INT64"), DataType::INT64);
    EXPECT_EQ(table.parse_type("FLOAT32"), DataType::FLOAT32);
    EXPECT_EQ(table.parse_type("FIXED_STRING32"), DataType::FIXED_STRING32);
    EXPECT_THROW(table.parse_type("UNKNOWN"), std::runtime_error);
}