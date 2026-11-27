#include "../itlab_db/src/data_structs/swiss_table.hpp"
#include <benchmark/benchmark.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <random>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifdef HAS_ABSEIL
#include "absl/container/flat_hash_map.h"
#endif

// ============================================================
//  Комбинации типов
// ============================================================
using LargeValue = std::array<uint64_t, 7>;

template <typename K, typename V> using HT      = HashTable<K, V>;
template <typename K, typename V> using StdMap  = std::unordered_map<K, V>;
#ifdef HAS_ABSEIL
template <typename K, typename V> using AbslMap = absl::flat_hash_map<K, V>;
#endif

// ============================================================
//  Генерация ключей
// ============================================================
template <typename K>
static std::vector<K> GenerateKeys(size_t count) {
  std::vector<K> keys(count);
  std::mt19937 gen(42);
  if constexpr (std::is_integral_v<K>) {
    std::uniform_int_distribution<K> dist(1, static_cast<K>(100000000));
    for (auto& k : keys) k = dist(gen);
  } else if constexpr (std::is_same_v<K, std::string>) {
    std::uniform_int_distribution<int> dist(0, 25);
    for (auto& k : keys) {
      k.resize(16);
      for (auto& c : k) c = static_cast<char>('a' + dist(gen));
    }
  }
  return keys;
}

template <typename K>
static std::vector<K> GenerateNonexistentKeys(const std::vector<K>& existing) {
  std::vector<K> keys;
  keys.reserve(existing.size());
  if constexpr (std::is_integral_v<K>) {
    for (K k : existing) keys.push_back(k + static_cast<K>(200000000));
  } else if constexpr (std::is_same_v<K, std::string>) {
    // [a-z] → [A-Z]
    for (const K& k : existing) {
      K copy = k;
      for (auto& c : copy)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      keys.push_back(copy);
    }
  }
  return keys;
}

// ============================================================
//  Container-agnostic обёртки над методами
// ============================================================
template <typename C, typename K, typename V>
static void InsertKV(C& c, const K& k, const V& v) {
  if constexpr (std::is_same_v<C, HashTable<K, V>>) c.insert(k, v);
  else                                              c.emplace(k, v);
}

template <typename C, typename K, typename V>
static void RemoveKV(C& c, const K& k) {
  if constexpr (std::is_same_v<C, HashTable<K, V>>) c.remove(k);
  else                                              c.erase(k);
}

template <typename C, typename K, typename V>
static void ReplaceKV(C& c, const K& k, const V& v) {
  if constexpr (std::is_same_v<C, HashTable<K, V>>) c.insert(k, v);
  else                                              c.insert_or_assign(k, v);
}

template <typename C, typename K, typename V>
static bool ContainsKV(const C& c, const K& k) {
  if constexpr (std::is_same_v<C, HashTable<K, V>>) return c.contains(k);
  else                                              return c.find(k) != c.end();
}

template <typename C, typename K, typename V>
static void ReserveIfSupported(C& c, size_t n) {
  if constexpr (!std::is_same_v<C, HashTable<K, V>>) c.reserve(n);
}

template <typename V>
static int64_t ReduceValue(const V& v) {
  if constexpr (std::is_integral_v<V>) return static_cast<int64_t>(v);
  else {
    int64_t s = 0;
    for (auto x : v) s += static_cast<int64_t>(x);
    return s;
  }
}

// ============================================================
//  Benchmark
// ============================================================
template <typename C, typename K, typename V>
static void BM_InsertNonexisting(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys<K>(n);
  for (auto _ : state) {
    C c;
    ReserveIfSupported<C, K, V>(c, n);
    for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template <typename C, typename K, typename V>
static void BM_EraseExisting(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys<K>(n);
  C c;
  for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
  for (auto _ : state) {
    for (size_t i = 0; i < n; ++i) RemoveKV<C, K, V>(c, keys[i]);
    state.PauseTiming();
    for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template <typename C, typename K, typename V>
static void BM_ReplaceExisting(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys<K>(n);
  C c;
  for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
  for (auto _ : state) {
    for (size_t i = 0; i < n; ++i) ReplaceKV<C, K, V>(c, keys[i], V{});
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template <typename C, typename K, typename V>
static void BM_EraseNonexisting(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys  = GenerateKeys<K>(n);
  auto nkeys = GenerateNonexistentKeys(keys);
  C c;
  for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
  for (auto _ : state) {
    for (size_t i = 0; i < n; ++i) RemoveKV<C, K, V>(c, nkeys[i]);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template <typename C, typename K, typename V>
static void BM_LookUpExisting(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys<K>(n);
  C c;
  for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
  for (auto _ : state) {
    for (size_t i = 0; i < n; ++i)
      benchmark::DoNotOptimize(ContainsKV<C, K, V>(c, keys[i]));
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template <typename C, typename K, typename V>
static void BM_LookUpNonexisting(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys  = GenerateKeys<K>(n);
  auto nkeys = GenerateNonexistentKeys(keys);
  C c;
  for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
  for (auto _ : state) {
    for (size_t i = 0; i < n; ++i)
      benchmark::DoNotOptimize(ContainsKV<C, K, V>(c, nkeys[i]));
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template <typename C, typename K, typename V>
static void BM_Iterate(benchmark::State &state) {
  const size_t n = state.range(0);
  auto keys = GenerateKeys<K>(n);
  C c;
  for (size_t i = 0; i < n; ++i) InsertKV<C, K, V>(c, keys[i], V{});
  for (auto _ : state) {
    int64_t sum = 0;
    for (const auto& kv : c) sum += ReduceValue(kv.second);
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

// ============================================================
//  Регистрация: одна операция × один контейнер × 3 комбинации типов
//  (короткий рабочий макрос — разворачивается в 3 BENCHMARK_TEMPLATE)
// ============================================================
#define BENCH_ALL(fn, C)                                                          \
  BENCHMARK_TEMPLATE(fn, C<int32_t, int32_t>, int32_t, int32_t)                   \
      ->RangeMultiplier(4)->Range(64, 65536);                                     \
  BENCHMARK_TEMPLATE(fn, C<int64_t, LargeValue>, int64_t, LargeValue)             \
      ->RangeMultiplier(4)->Range(64, 65536);                                     \
  BENCHMARK_TEMPLATE(fn, C<std::string, int64_t>, std::string, int64_t)           \
      ->RangeMultiplier(4)->Range(64, 65536)

BENCH_ALL(BM_InsertNonexisting, HT);
BENCH_ALL(BM_InsertNonexisting, StdMap);
#ifdef HAS_ABSEIL
BENCH_ALL(BM_InsertNonexisting, AbslMap);
#endif

BENCH_ALL(BM_EraseExisting, HT);
BENCH_ALL(BM_EraseExisting, StdMap);
#ifdef HAS_ABSEIL
BENCH_ALL(BM_EraseExisting, AbslMap);
#endif

BENCH_ALL(BM_ReplaceExisting, HT);
BENCH_ALL(BM_ReplaceExisting, StdMap);
#ifdef HAS_ABSEIL
BENCH_ALL(BM_ReplaceExisting, AbslMap);
#endif

BENCH_ALL(BM_EraseNonexisting, HT);
BENCH_ALL(BM_EraseNonexisting, StdMap);
#ifdef HAS_ABSEIL
BENCH_ALL(BM_EraseNonexisting, AbslMap);
#endif

BENCH_ALL(BM_LookUpExisting, HT);
BENCH_ALL(BM_LookUpExisting, StdMap);
#ifdef HAS_ABSEIL
BENCH_ALL(BM_LookUpExisting, AbslMap);
#endif

BENCH_ALL(BM_LookUpNonexisting, HT);
BENCH_ALL(BM_LookUpNonexisting, StdMap);
#ifdef HAS_ABSEIL
BENCH_ALL(BM_LookUpNonexisting, AbslMap);
#endif

// BENCH_ALL(BM_Iterate, HT);
BENCH_ALL(BM_Iterate, StdMap);
#ifdef HAS_ABSEIL
BENCH_ALL(BM_Iterate, AbslMap);
#endif

#undef BENCH_ALL