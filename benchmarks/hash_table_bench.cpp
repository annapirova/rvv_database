#include "../itlab_db/src/data_structs/swiss_table.hpp"
#include <benchmark/benchmark.h>

#include <random>
#include <vector>

static std::vector<int> GenerateKeys(size_t count) {
  std::vector<int> keys(count);
  std::mt19937 gen(42);
  std::uniform_int_distribution<int> dist(1, 100000000);
  for (size_t i = 0; i < count; ++i) {
    keys[i] = dist(gen);
  }
  return keys;
}

// ============================================================
//  Бенчмарк вставки
// ============================================================
static void BM_HashTable_Insert(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys(n);

  for (auto _ : state) {
    HashTable<int, int> ht;
    for (size_t i = 0; i < n; ++i) {
      ht.insert(keys[i], static_cast<int>(i));
    }
    benchmark::DoNotOptimize(ht.size());
  }
}
BENCHMARK(BM_HashTable_Insert)->RangeMultiplier(4)->Range(64, 65536);

// ============================================================
//  std::unordered_map для сравнения
// ============================================================
static void BM_StdUnorderedMap_Insert(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys(n);

  for (auto _ : state) {
    std::unordered_map<int, int> map;
    for (size_t i = 0; i < n; ++i) {
      map.emplace(keys[i], static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map.size());
  }
}
BENCHMARK(BM_StdUnorderedMap_Insert)->RangeMultiplier(4)->Range(64, 65536);

// ============================================================
//  Бенчмарк поиска
// ============================================================
static void BM_HashTable_Contains(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys(n);
  HashTable<int, int> ht;
  for (size_t i = 0; i < n; ++i) {
    ht.insert(keys[i], static_cast<int>(i));
  }

  for (auto _ : state) {
    for (size_t i = 0; i < n; ++i) {
      bool found = ht.contains(keys[i]);
      benchmark::DoNotOptimize(found);
    }
  }
}
BENCHMARK(BM_HashTable_Contains)->RangeMultiplier(4)->Range(64, 65536);

// ============================================================
//  Бенчмарк удаления и повторной вставки
// ============================================================
static void BM_HashTable_RemoveAndReinsert(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys(n);
  HashTable<int, int> ht;
  for (size_t i = 0; i < n; ++i) {
    ht.insert(keys[i], static_cast<int>(i));
  }

  size_t idx = 0;
  for (auto _ : state) {
    int k = keys[idx % n];
    ht.remove(k);
    ht.insert(k, static_cast<int>(idx));
    ++idx;
    benchmark::DoNotOptimize(ht.size());
  }
}
BENCHMARK(BM_HashTable_RemoveAndReinsert)->RangeMultiplier(4)->Range(64, 65536);
