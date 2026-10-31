#include "../itlab_db/src/data_structs/open_addr_hash_table.hpp"
#include <gtest/gtest.h>

#include <random>
#include <string>

struct ZeroHashKey {
  int value = 0;
  bool operator==(const ZeroHashKey &o) const { return value == o.value; }
};

namespace std {
template <> struct hash<ZeroHashKey> {
  size_t operator()(const ZeroHashKey &) const { return 0; }
};
} // namespace std

// ============================================================
//  Базовые тесты
// ============================================================

TEST(HashTableTest, DefaultConstructIsEmpty) {
  HashTable<int, int> ht;
  EXPECT_EQ(ht.size(), 0u);
  EXPECT_TRUE(ht.empty());
  EXPECT_GT(ht.capacity(), 0u);
  EXPECT_EQ(ht.tombstoneCount(), 0u);
}

TEST(HashTableTest, InsertAndGetSingle) {
  HashTable<int, int> ht;
  ht.insert(1, 100);
  EXPECT_EQ(ht.get(1), 100);
  EXPECT_EQ(ht.size(), 1u);
  EXPECT_FALSE(ht.empty());
}

TEST(HashTableTest, InsertAndGetMultiple) {
  HashTable<int, std::string> ht;
  ht.insert(1, "one");
  ht.insert(2, "two");
  ht.insert(3, "three");
  EXPECT_EQ(ht.get(1), "one");
  EXPECT_EQ(ht.get(2), "two");
  EXPECT_EQ(ht.get(3), "three");
  EXPECT_EQ(ht.size(), 3u);
}

TEST(HashTableTest, UpdateExistingKey) {
  HashTable<int, int> ht;
  ht.insert(42, 100);
  ht.insert(42, 200);
  EXPECT_EQ(ht.get(42), 200);
  EXPECT_EQ(ht.size(), 1u);
}

TEST(HashTableTest, GetThrowsOnMissing) {
  HashTable<int, int> ht;
  ht.insert(1, 1);
  EXPECT_THROW(ht.get(2), std::out_of_range);
}

TEST(HashTableTest, ConstGet) {
  HashTable<int, int> ht;
  ht.insert(1, 42);
  const auto &c = ht;
  EXPECT_EQ(c.get(1), 42);
  EXPECT_THROW(c.get(2), std::out_of_range);
}

TEST(HashTableTest, Contains) {
  HashTable<int, int> ht;
  ht.insert(5, 50);
  EXPECT_TRUE(ht.contains(5));
  EXPECT_FALSE(ht.contains(6));
}

// ============================================================
//  Удаление и надгробия
// ============================================================

TEST(HashTableTest, RemoveExisting) {
  HashTable<int, int> ht;
  ht.insert(1, 10);
  ht.insert(2, 20);
  EXPECT_TRUE(ht.remove(1));
  EXPECT_FALSE(ht.contains(1));
  EXPECT_EQ(ht.size(), 1u);
  EXPECT_EQ(ht.get(2), 20);
}

TEST(HashTableTest, RemoveMissingReturnsFalse) {
  HashTable<int, int> ht;
  ht.insert(1, 1);
  EXPECT_FALSE(ht.remove(2));
  EXPECT_EQ(ht.size(), 1u);
}

TEST(HashTableTest, RemoveCreatesTombstone) {
  HashTable<int, int> ht;
  for (int i = 0; i < 20; ++i)
    ht.insert(i, i);

  size_t t0 = ht.tombstoneCount();
  ht.remove(0);
  // Либо появилось надгробие, либо удаление вызвало рехеш (тогда их 0)
  EXPECT_TRUE(ht.tombstoneCount() == t0 + 1 || ht.tombstoneCount() == 0);
}

TEST(HashTableTest, ReinsertAfterRemove) {
  HashTable<int, std::string> ht;
  ht.insert(1, "a");
  ht.insert(2, "b");
  ht.remove(1);
  ht.insert(1, "new_a");
  EXPECT_EQ(ht.get(1), "new_a");
  EXPECT_EQ(ht.get(2), "b");
  EXPECT_EQ(ht.size(), 2u);
}

TEST(HashTableTest, RemoveDoesNotBreakProbing) {
  HashTable<int, int> ht;
  for (int i = 0; i < 50; ++i)
    ht.insert(i, i * 3);

  // Удаляем середину диапазона
  for (int i = 10; i < 40; ++i)
    EXPECT_TRUE(ht.remove(i));

  // Оставшиеся доступны
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(ht.get(i), i * 3);
  for (int i = 40; i < 50; ++i)
    EXPECT_EQ(ht.get(i), i * 3);

  // Удалённые не находятся
  for (int i = 10; i < 40; ++i)
    EXPECT_FALSE(ht.contains(i));
}

// ============================================================
//  Много элементов / рехеш вверх
// ============================================================

TEST(HashTableTest, ManyInserts) {
  HashTable<int, int> ht;
  constexpr int N = 1000;
  for (int i = 0; i < N; ++i)
    ht.insert(i, i * 2);
  EXPECT_EQ(ht.size(), N);
  for (int i = 0; i < N; ++i)
    EXPECT_EQ(ht.get(i), i * 2);
}

TEST(HashTableTest, RehashUpGrowsCapacity) {
  HashTable<int, int> ht;
  size_t initialCap = ht.capacity();
  for (int i = 0; i < 500; ++i)
    ht.insert(i, i);
  EXPECT_GT(ht.capacity(), initialCap);
  for (int i = 0; i < 500; ++i)
    EXPECT_EQ(ht.get(i), i);
}

TEST(HashTableTest, LoadFactorStaysBelowMax) {
  HashTable<int, int> ht;
  for (int i = 0; i < 500; ++i) {
    ht.insert(i, i);
    float load = (float)(ht.size() + ht.tombstoneCount()) / ht.capacity();
    EXPECT_LT(load, 0.5f + 1e-6f) << "at i=" << i;
  }
}

// ============================================================
//  Рехеш вниз и очистка надгробий
// ============================================================

TEST(HashTableTest, RehashDownShrinks) {
  HashTable<int, int> ht;
  for (int i = 0; i < 500; ++i)
    ht.insert(i, i);
  size_t bigCap = ht.capacity();

  for (int i = 0; i < 495; ++i)
    ht.remove(i);

  EXPECT_LT(ht.capacity(), bigCap);
  for (int i = 495; i < 500; ++i)
    EXPECT_EQ(ht.get(i), i);
  EXPECT_EQ(ht.size(), 5u);
}

TEST(HashTableTest, RemoveAllThenReinsert) {
  HashTable<int, int> ht;
  for (int i = 0; i < 100; ++i)
    ht.insert(i, i);
  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(ht.remove(i));
  EXPECT_EQ(ht.size(), 0u);
  EXPECT_TRUE(ht.empty());

  for (int i = 0; i < 100; ++i)
    ht.insert(i, i * 10);
  for (int i = 0; i < 100; ++i)
    EXPECT_EQ(ht.get(i), i * 10);
}

TEST(HashTableTest, NoUnboundedGrowthFromTombstones) {
  HashTable<int, int> ht;
  for (int i = 0; i < 50; ++i)
    ht.insert(i, i);
  size_t capAfterInsert = ht.capacity();

  for (int round = 0; round < 2000; ++round) {
    ht.insert(10000, round);
    ht.remove(10000);
  }

  EXPECT_LT(ht.capacity(), capAfterInsert * 4);
  EXPECT_EQ(ht.size(), 50u);
  for (int i = 0; i < 50; ++i)
    EXPECT_EQ(ht.get(i), i);
}

// ============================================================
//  Квадратичное пробирование при тотальных коллизиях
// ============================================================

TEST(HashTableTest, AllKeysCollideQuadraticProbing) {
  HashTable<ZeroHashKey, int> ht;
  constexpr int N = 5;
  for (int i = 0; i < N; ++i)
    ht.insert({i}, i * 10);
  for (int i = 0; i < N; ++i)
    EXPECT_EQ(ht.get({i}), i * 10);

  ht.remove({2});
  EXPECT_FALSE(ht.contains({2}));
  for (int i : {0, 1, 3, 4})
    EXPECT_EQ(ht.get({i}), i * 10);
}

// ============================================================
//  Строковые ключи
// ============================================================

TEST(HashTableTest, StringKeys) {
  HashTable<std::string, int> ht;
  ht.insert("apple", 1);
  ht.insert("banana", 2);
  ht.insert("cherry", 3);
  EXPECT_EQ(ht.get("apple"), 1);
  EXPECT_EQ(ht.get("banana"), 2);
  EXPECT_EQ(ht.get("cherry"), 3);

  ht.remove("banana");
  EXPECT_FALSE(ht.contains("banana"));
  EXPECT_TRUE(ht.contains("apple"));
  EXPECT_EQ(ht.size(), 2u);

  ht.insert("apple", 100);
  EXPECT_EQ(ht.get("apple"), 100);
}