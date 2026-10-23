#include "itlab_db.h"
#include <gtest/gtest.h>

TEST(db_storage_test, test1) {
	test_db a;
	ASSERT_NO_THROW(a.print());
}
