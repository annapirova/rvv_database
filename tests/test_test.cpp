#include "itlab_db.h"
#include <gtest/gtest.h>

#if defined(HAS_SQL_PARSER) && HAS_SQL_PARSER
    #include "SQLParser.h"
#endif

TEST(db_storage_test, test1) {
	test_db a;
	ASSERT_NO_THROW(a.print());
}

#if defined(HAS_SQL_PARSER) && HAS_SQL_PARSER
TEST(db_storage_test, test2) {
	std::string query = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100));";
	hsql::SQLParserResult result;
	hsql::SQLParser::parse(query, &result);
	EXPECT_TRUE(result.isValid());
}
#endif
