#include "itlab_db.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include "../itlab_db/BM25/include/BM25.h"
#include "../itlab_db/Table/include/Table.h"

int main() {
  //test1();
  //test2();
  Table table("songs");
  std::string filepath = "D:/data/songs_200.txt";
  try {
      table.load_from_file(filepath);
  }
  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
  }
  TwoStageSearch search;
  try {
      search.fit(table, "text");
  }
  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
  }
  std::string query;
  while (true) {
      std::cout << "Query: ";
      std::getline(std::cin, query);
      std::vector<ResultRow> results = search.search(query, 100, 5);
      if (results.empty()) {
          std::cout << "No results" << std::endl;
          continue;
      }
      for (const ResultRow& r : results) {
          std::cout << r.tfidf << " | " << r.bm25 << " | " << r.agg << " -> ";
          std::cout << search.get_document_text(r.idx) << std::endl;
      }
  }
  return 0;
}