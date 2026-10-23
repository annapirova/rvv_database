#include "SQLParser.h"
#include <iostream>

int main() {
	
  std::string query = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100));";

  hsql::SQLParserResult result;
  hsql::SQLParser::parse(query, &result);
  
  if (result.isValid()) {
    std::cout << "Parsed successfully!\n";
    std::cout << "Number of statements: " << result.size() << std::endl;

    return 0;
  } 
  else {
    std::cout << "Given string is not a valid SQL query.\n";
    return -1;
  }
}