#pragma once
#include <cstring>
#include <sstream>
#include <string>

template <size_t N = 256>
struct FixedString {
    char data[N];

    // Конструктор по умолчанию (обнуление)
    FixedString() {
        std::memset(data, 0, N);
    }
    FixedString(const FixedString& other) = default;
    FixedString& operator=(const FixedString& other) = default;

    // Конструктор из C-строки
    FixedString(const char* str) {
        std::memset(data, 0, N);
        strncpy(data, str, N - 1); // N-1, чтобы гарантировать нуль-терминацию
    }

    FixedString& operator=(const char* str) {
        std::memset(data, 0, N);
        strncpy(data, str, N - 1);
        return *this;
    }

    operator char* () { return data; }
    operator const char* () const { return data; }

    bool operator==(const FixedString& other) const {
        return std::memcmp(data, other.data, N) == 0;
    }

    friend std::istringstream& operator>>(std::istringstream& iss, FixedString& fs) {
        std::string line;
        std::getline(iss, line, ';');
        fs = line.c_str();
        return iss;
    }
};

using FixedString128 = FixedString<128>;
using FixedString64 = FixedString<64>;
using FixedString32 = FixedString<32>;
