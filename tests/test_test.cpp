#include "Table.h"
#include <gtest/gtest.h>

TEST(TableContainer, test_load_file) {
	Table table("test");
	table.load_from_file("tests/file_data.csv");
	EXPECT_EQ(table.get_chunk_size(), 1);
	size_t index = table.get_column_index("count");
	EXPECT_EQ(table.get_column(0,index).get<int>()[0], 100);
}