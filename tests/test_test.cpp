#include "itlab_db.h"
#include "TableContainer.h"
#include <gtest/gtest.h>

TEST(db_storage_test, test1) {
	test_db a;
	ASSERT_NO_THROW(a.print());
}

TEST(TableContainer, test_load_file) {
	Table table("test");
	table.load_from_file("file_data.txt");
	EXPECT_EQ(table.get_chunk_size(), 1);
	size_t index = table.get_column_index("count");
	EXPECT_EQ(table.get_column(0,index).get<int>()[0], 100);
}