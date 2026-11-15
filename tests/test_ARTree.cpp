#include <fstream>
#include <string>

#include <gtest/gtest.h>
#include "ART.h"

TEST(TestArt, TestInsertAndSearch) {
    std::ifstream file("words.txt");
    std::string key;
    EXPECT_TRUE(file.is_open());
    ARTree<int> super_tree;
    for (int i = 0;i < 200;++i) {
        EXPECT_NO_THROW(std::getline(file, key));
        EXPECT_NO_THROW(super_tree.insert(key, i));
    }
    file.close();
    std::ifstream file2("words.txt");
    for (int i = 0;i < 200;++i) {
        EXPECT_NO_THROW(std::getline(file2, key));
        EXPECT_EQ(super_tree.search<int>(key), i);
    }
    file2.close();
}