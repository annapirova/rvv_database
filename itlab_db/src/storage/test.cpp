#include "itlab_db.h"
#include <iostream>

#if defined(HAS_SQL_PARSER) && HAS_SQL_PARSER
    #include "SQLParser.h"
#endif

void test1() {
	test_db a;
}

int test2() {
	
  std::string query = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100));";
  
#if defined(HAS_SQL_PARSER) && HAS_SQL_PARSER  
  hsql::SQLParserResult result;
  hsql::SQLParser::parse(query, &result);
  if (result.isValid()) {
    std::cout << "Parsed successfully!\n";
    std::cout << "Number of statements: " << result.size() << std::endl;
	return 0;
  } 
  #else
	std::cout << "SQL parser is disabled\n";
  #endif
  	return 1;
}