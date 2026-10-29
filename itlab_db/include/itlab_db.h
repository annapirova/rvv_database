#pragma once

#include <vector>
#include <iostream>

class test_db {
	std::vector<int> vec;
	
public:

	test_db() {}
	
	void print() {
		for (const auto& x: vec)
			std::cout << x << " ";
	}
};

void test1();
int test2();