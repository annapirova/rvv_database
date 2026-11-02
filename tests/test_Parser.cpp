#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <utility>

#include <gtest/gtest.h>

#include "SQLParser.h"
#include "util/sqlhelper.h"
#include "Table.h"

#include "Value.h"
#include "TurboExpr.h"
#include "QueryResult.h"
#include "Operators.h"

using namespace std;

class DatabaseFixture : public ::testing::Test {
protected:
    Table db;

    void SetUp() override {
        db = Table("TestDatabase");
        Chunk s1;

        // Колонка 0: id (INT32)
        s1.add_column(DataType::INT32);
        s1.get_column(0).allocate_aligned<int32_t>();
        s1.get_column(0).append<int32_t>(1);
        s1.get_column(0).append<int32_t>(2);
        s1.get_column(0).append<int32_t>(3);

        // Колонка 1: name (FIXED_STRING)
        s1.add_column(DataType::FIXED_STRING);
        s1.get_column(1).allocate_aligned<std::array<char, 256>>();
        std::array<char, 256> tmp;

        std::snprintf(tmp.data(), sizeof(tmp), "Alice");
        s1.get_column(1).append<std::array<char, 256>>(tmp);
        std::snprintf(tmp.data(), sizeof(tmp), "Bob");
        s1.get_column(1).append<std::array<char, 256>>(tmp);
        std::snprintf(tmp.data(), sizeof(tmp), "Charlie");
        s1.get_column(1).append<std::array<char, 256>>(tmp);

        // Колонка 2: age (INT32)
        s1.add_column(DataType::INT32);
        s1.get_column(2).allocate_aligned<int32_t>();
        s1.get_column(2).append<int32_t>(25);
        s1.get_column(2).append<int32_t>(19);
        s1.get_column(2).append<int32_t>(30);

        db.get_chunks().push_back(std::move(s1));
        db.column_names() = {"id", "name", "age"};
    }

    // Вспомогательный метод для выполнения запроса и возврата результата
    QueryResult execute_query(const string& query, vector<string>& out_fields) {
        hsql::SQLParserResult result;
        ASSERT_TRUE(hsql::SQLParser::parse(query, &result)) << "Failed to parse query";
        ASSERT_TRUE(result.isValid()) << "Invalid SQL syntax";

        auto* select_stmt = (hsql::SelectStatement*)result.getStatement(0);
        
        for (auto* expr : *select_stmt->selectList) {
            if (expr->name) out_fields.push_back(string(expr->name));
            else if (expr->type == hsql::kExprStar) out_fields.push_back("*");
        }

        TurboExpr* math_tree = new TurboExpr(select_stmt->whereClause);
        GetTableOperator* get_table = new GetTableOperator(db);
        WhereOperator* where_node = new WhereOperator(math_tree, get_table);
        SelectOperator* select_node = new SelectOperator(where_node, out_fields);

        QueryResult res = select_node->execute_operator();
        delete select_node; 
        delete get_table;

        return res;
    }
};

TEST_F(DatabaseFixture, FilterByAgeLessThan30) {
    vector<string> fields;
    QueryResult res = execute_query("SELECT name, age FROM table_name WHERE age < 30;", fields);

     EXPECT_EQ(res.get_rows().size(), 2);

    auto row0 = res.get_row(res.get_rows()[0]);
    EXPECT_EQ(row0[0].to_string(), "Alice");
    EXPECT_EQ(row0[1].to_string(), "25");

    auto row1 = res.get_row(res.get_rows()[1]);
    EXPECT_EQ(row1[0].to_string(), "Bob");
    EXPECT_EQ(row1[1].to_string(), "19");
}

TEST_F(DatabaseFixture, ImplicitTypeCoercionFloatToInt) {
    vector<string> fields;
    QueryResult res = execute_query("SELECT name FROM table_name WHERE age > 1.5;", fields);
    EXPECT_EQ(res.get_rows().size(), 3);

    EXPECT_EQ(res.get_row(res.get_rows()[0])[0].to_string(), "Alice");
    EXPECT_EQ(res.get_row(res.get_rows()[1])[0].to_string(), "Bob");
    EXPECT_EQ(res.get_row(res.get_rows()[2])[0].to_string(), "Charlie");
}

TEST_F(DatabaseFixture, InvalidColumnNameThrows) {
    vector<string> fields;
    
    EXPECT_THROW(
        execute_query("SELECT fake_column FROM table_name;", fields), 
        std::logic_error
    );
}

TEST_F(DatabaseFixture, SelectStarReturnsAllColumns) {
    vector<string> fields;
    QueryResult res = execute_query("SELECT * FROM table_name WHERE id = 2;", fields);

    EXPECT_EQ(res.get_rows().size(), 1);
    EXPECT_EQ(res.get_cols().size(), 3); // Должны быть все 3 колонки

    auto row = res.get_row(res.get_rows()[0]);
    EXPECT_EQ(row[0].to_string(), "2");
    EXPECT_EQ(row[1].to_string(), "Bob");
    EXPECT_EQ(row[2].to_string(), "19");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
