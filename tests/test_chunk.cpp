#include <gtest/gtest.h>
#include "Chunk.h"

TEST(ChunkTest, EmptyChunk) {
    Chunk chunk;
    EXPECT_FALSE(chunk.is_full());
    EXPECT_EQ(chunk.get_rows_count(), 0);
}

TEST(ChunkTest, AddColumns) {
    Chunk chunk;
    chunk.add_column(DataType::INT32);
    chunk.add_column(DataType::INT64);
    chunk.add_column(DataType::FLOAT32);

    EXPECT_EQ(chunk.get_column(0).type(), DataType::INT32);
    EXPECT_EQ(chunk.get_column(1).type(), DataType::INT64);
    EXPECT_EQ(chunk.get_column(2).type(), DataType::FLOAT32);
}

TEST(ChunkTest, GetColumnOutOfRange) {
    Chunk chunk;
    chunk.add_column(DataType::INT32);

    EXPECT_THROW(chunk.get_column(5), std::out_of_range);
}

TEST(ChunkTest, IsFull) {
    Chunk chunk;
    chunk.add_column(DataType::INT32);
    chunk.get_column(0).allocate_aligned<int32_t>();

    EXPECT_FALSE(chunk.is_full());

    // Заполняем колонку
    for (size_t i = 0; i < Column::MAX_ROWS; ++i) {
        chunk.get_column(0).append<int32_t>(static_cast<int32_t>(i));
    }

    EXPECT_TRUE(chunk.is_full());
    EXPECT_EQ(chunk.get_rows_count(), Column::MAX_ROWS);
}

TEST(ChunkTest, MoveSemantics) {
    Chunk chunk1;
    chunk1.add_column(DataType::INT32);
    chunk1.get_column(0).allocate_aligned<int32_t>();
    chunk1.get_column(0).append<int32_t>(42);

    Chunk chunk2 = std::move(chunk1);

    EXPECT_EQ(chunk2.get_column(0).get_count(), 1);
    EXPECT_EQ(chunk2.get_column(0).get<int32_t>()[0], 42);
}