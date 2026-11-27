#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename K, typename V, typename Hash = std::hash<K>,
          size_t GroupSize = 16>
class HashTable;

template <typename K, typename V, typename Hash, size_t GroupSize>
std::ostream &operator<<(std::ostream &out,
                         const HashTable<K, V, Hash, GroupSize> &t);

template <typename K, typename V, typename Hash, size_t GroupSize>
class HashTable {
  static_assert(GroupSize > 0 && (GroupSize & (GroupSize - 1)) == 0,
                "GroupSize must be a power of 2");

  // ---- control-байты ----
  static constexpr uint8_t EMPTY_BYTE = 0x80;   // 1000 0000
  static constexpr uint8_t DELETED_BYTE = 0xFE; // 1111 1110
  // любое значение < 0x80 — H2-фрагмент хеша

  static constexpr size_t GROUP_SIZE = GroupSize;
  static constexpr size_t MIN_CAPACITY = GROUP_SIZE;
  const int loadfactor_numerator = 7;
  const int loadfactor_denominator = 8;

  struct Slot {
    K key{};
    V value{};
  };

  std::vector<Slot> slots;
  std::vector<uint8_t> ctrl; // control-байты
  std::vector<uint8_t> bloom_mas;
  bool bloom_enabled = true;
  size_t count = 0;
  size_t tombstones = 0;
  size_t cap = 0;
  size_t mask = 0;    // cap - 1 // Нужен чтобы не выходить за границы
  uint32_t bmask = 0; // cap/2 - 1
  Hash hasher;

  static uint8_t H2(size_t h);
  size_t H1(size_t h) const;

  // Результат поиска:
  //   found      — ключ найден (index — позиция с ключом)
  //   wasDeleted — не найден, index — первое найденное надгробие
  //   иначе      — не найден, index — первая EMPTY-позиция (или SIZE_MAX)
  struct FindResult {
    size_t index;
    bool found;
    bool wasDeleted;
  };
  FindResult find(size_t h, const K &key) const;

  // Округление вверх до ближайшей степени двойки
  static size_t roundUpPow2(size_t n);

  void rehash(size_t newCap);

  void bloom_change(size_t h, int step);
  bool bloom_check(size_t h) const;

public:
  explicit HashTable(size_t initial = MIN_CAPACITY, int _loadfactor_num = 7,
                     int _loadfactor_den = 8, bool bloom = true, Hash h = Hash{});

  void insert(const K &key, const V &value);

  V &get(const K &key);
  const V &get(const K &key) const;

  bool remove(const K &key);
  bool contains(const K &key) const;

  size_t size() const;
  size_t tombstoneCount() const;
  size_t capacity() const;
  bool empty() const;

  friend std::ostream &operator<< <>(std::ostream &out,
                                     const HashTable<K, V, Hash, GroupSize> &t);
  void printStats() const;

  // Отладочный вывод: '.'=EMPTY, 'X'=DELETED, 'O'=OCCUPIED
  void printCtrl() const;
};

// ==================================================================
//  Реализации методов HashTable
// ==================================================================

template <typename K, typename V, typename Hash, size_t GroupSize>
uint8_t HashTable<K, V, Hash, GroupSize>::H2(size_t h) {
  return static_cast<uint8_t>(h & 0x7F);
}

template <typename K, typename V, typename Hash, size_t GroupSize>
size_t HashTable<K, V, Hash, GroupSize>::H1(size_t h) const {
  return (h >> 7) & mask;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
size_t HashTable<K, V, Hash, GroupSize>::roundUpPow2(size_t n) {
  if (n <= 1)
    return 1;
  --n;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  return n + 1;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
HashTable<K, V, Hash, GroupSize>::HashTable(size_t initial, int _loadfactor_num,
                                            int _loadfactor_den, bool bloom, Hash h)
    : loadfactor_numerator(_loadfactor_num),
      loadfactor_denominator(_loadfactor_den),
      bloom_enabled(bloom),
      hasher(std::move(h)) {
  size_t c = std::max(MIN_CAPACITY, roundUpPow2(initial));
  slots.resize(c);
  ctrl.assign(c, EMPTY_BYTE);
  cap = c;
  mask = c - 1;
  if (bloom_enabled) {
    bmask = c >> 1;
    bloom_mas.assign(bmask--, 0);
  } else {
    bmask = 0;
    bloom_mas.clear();
  }
}

// ----------------------------------------------------------------
//  Поиск
// ----------------------------------------------------------------
template <typename K, typename V, typename Hash, size_t GroupSize>
typename HashTable<K, V, Hash, GroupSize>::FindResult
HashTable<K, V, Hash, GroupSize>::find(size_t h, const K &key) const {
  uint8_t h2 = H2(h);
  size_t start = H1(h);
  size_t firstDeleted = SIZE_MAX;
  size_t groups = cap / GROUP_SIZE;

  const uint8_t *__restrict ctrlPtr = ctrl.data();
  const auto *__restrict slotsPtr = slots.data();

  for (size_t g = 0; g < groups; ++g) {
    const size_t groupStart = (start + g * GROUP_SIZE) & mask;

    for (size_t i = 0; i < GROUP_SIZE; ++i) {
      const size_t idx = (groupStart + i) & mask;
      const uint8_t c = ctrlPtr[idx];

      if (c == h2 && slotsPtr[idx].key == key) {
        return {idx, true, false};
      }
      if (c == EMPTY_BYTE) {
        if (firstDeleted != SIZE_MAX) {
          return {firstDeleted, false, true};
        }
        return {idx, false, false};
      }
      if (c == DELETED_BYTE && firstDeleted == SIZE_MAX) {
        firstDeleted = idx;
      }
    }
  }

  if (firstDeleted != SIZE_MAX)
    return {firstDeleted, false, true};
  return {SIZE_MAX, false, false};
}

// ----------------------------------------------------------------
//  Рехеш
// ----------------------------------------------------------------
template <typename K, typename V, typename Hash, size_t GroupSize>
void HashTable<K, V, Hash, GroupSize>::rehash(size_t newCap) {
  size_t needed = std::max(
      MIN_CAPACITY,
      roundUpPow2((count * loadfactor_denominator) / loadfactor_numerator + 1));
  size_t nc = std::max(needed, roundUpPow2(newCap));
  if (nc == cap && tombstones == 0)
    return;

  std::vector<Slot> oldSlots = std::move(slots);
  std::vector<uint8_t> oldCtrl = std::move(ctrl);
  size_t oldCap = cap;

  slots.assign(nc, Slot{});
  ctrl.assign(nc, EMPTY_BYTE);
    cap = nc;
  mask = nc - 1;
  if (bloom_enabled) {
    bmask = nc >> 1;
    bloom_mas.assign(bmask--, 0);
  } else {
    bmask = 0;
    bloom_mas.clear();
  }
  count = 0;
  tombstones = 0;

  for (size_t i = 0; i < oldCap; ++i) {
    uint8_t c = oldCtrl[i];
    if (c != EMPTY_BYTE && c != DELETED_BYTE)
      insert(std::move(oldSlots[i].key), std::move(oldSlots[i].value));
  }
}

// ----------------------------------------------------------------
//  Bloom
// ----------------------------------------------------------------
template <typename K, typename V, typename Hash, size_t GroupSize>
void HashTable<K, V, Hash, GroupSize>::bloom_change(size_t h, int step) {
  if (!bloom_enabled) return;

  uint32_t a = static_cast<uint32_t>(h);
  uint32_t b = static_cast<uint32_t>(h >> 32) | 1;
  for (int i = 0; i < 2; ++i) {
    uint8_t &counter = bloom_mas[a & bmask];
    if (step > 0) {
      if (counter < UINT8_MAX) ++counter;
    } else if (step < 0) {
      if (counter > 0 && counter < UINT8_MAX) --counter;
    }
    a += b;
  }
}

template <typename K, typename V, typename Hash, size_t GroupSize>
bool HashTable<K, V, Hash, GroupSize>::bloom_check(size_t h) const {
  if (!bloom_enabled) return true;   // фильтр выключен — "пропускаем" всегда

  uint32_t a = static_cast<uint32_t>(h);
  uint32_t b = static_cast<uint32_t>(h >> 32) | 1;
  for (int i = 0; i < 2; ++i) {
    if (bloom_mas[a & bmask] == 0) return false;
    a += b;
  }
  return true;
}

// ----------------------------------------------------------------
//  Вставка / обновление
// ----------------------------------------------------------------
template <typename K, typename V, typename Hash, size_t GroupSize>
void HashTable<K, V, Hash, GroupSize>::insert(const K &key, const V &value) {
  size_t h = hasher(key);
  FindResult r = find(h, key);

  if (r.found) {
    slots[r.index].value = value;
    return;
  }

  if ((count + tombstones + 1) * loadfactor_denominator >
      cap * loadfactor_numerator) {
    rehash(cap * 2);
    h = hasher(key);
    r = find(h, key);
  }

  if (r.index == SIZE_MAX) {
    rehash(cap * 2);
    insert(key, value);
    return;
  }

  if (r.wasDeleted)
    --tombstones;
  slots[r.index].key = key;
  slots[r.index].value = value;
  ctrl[r.index] = H2(h);
  ++count;
  bloom_change(h, 1);
}

// ----------------------------------------------------------------
//  Получение
// ----------------------------------------------------------------
template <typename K, typename V, typename Hash, size_t GroupSize>
V &HashTable<K, V, Hash, GroupSize>::get(const K &key) {
  size_t h = hasher(key);
  if (!bloom_check(h)) {
    throw std::out_of_range("Key not found");
  }
  FindResult r = find(h, key);
  if (!r.found)
    throw std::out_of_range("Key not found");
  return slots[r.index].value;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
const V &HashTable<K, V, Hash, GroupSize>::get(const K &key) const {
  size_t h = hasher(key);
  if (!bloom_check(h)) {
    throw std::out_of_range("Key not found");
  }
  FindResult r = find(h, key);
  if (!r.found)
    throw std::out_of_range("Key not found");
  return slots[r.index].value;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
bool HashTable<K, V, Hash, GroupSize>::contains(const K &key) const {
  size_t h = hasher(key);
  return bloom_check(h) & find(h, key).found;
}

// ----------------------------------------------------------------
//  Удаление
// ----------------------------------------------------------------
template <typename K, typename V, typename Hash, size_t GroupSize>
bool HashTable<K, V, Hash, GroupSize>::remove(const K &key) {
  size_t h = hasher(key);
  FindResult r = find(h, key);
  if (!r.found)
    return false;

  bloom_change(h, -1);

  ctrl[r.index] = DELETED_BYTE;
  slots[r.index].key = K{};
  slots[r.index].value = V{};
  --count;
  ++tombstones;

  if (cap > MIN_CAPACITY) {
    bool tooSparse = count * 8 < cap; // load < 1/8
    bool tooManyTombs = tombstones > count;
    if (tooSparse || tooManyTombs) {
      size_t target = tooSparse
                          ? std::max(MIN_CAPACITY, roundUpPow2((count + 1) * 2))
                          : cap;
      rehash(target);
    }
  }
  return true;
}

// ----------------------------------------------------------------
//  Геттеры и диагностика
// ----------------------------------------------------------------
template <typename K, typename V, typename Hash, size_t GroupSize>
size_t HashTable<K, V, Hash, GroupSize>::size() const {
  return count;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
size_t HashTable<K, V, Hash, GroupSize>::tombstoneCount() const {
  return tombstones;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
size_t HashTable<K, V, Hash, GroupSize>::capacity() const {
  return cap;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
bool HashTable<K, V, Hash, GroupSize>::empty() const {
  return count == 0;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
void HashTable<K, V, Hash, GroupSize>::printCtrl() const {
  for (size_t i = 0; i < cap; ++i) {
    uint8_t c = ctrl[i];
    if (c == EMPTY_BYTE)
      std::cout << '.';
    else if (c == DELETED_BYTE)
      std::cout << 'X';
    else
      std::cout << 'O';
  }
  std::cout << '\n';
}

template <typename K, typename V, typename Hash, size_t GroupSize>
std::ostream &operator<<(std::ostream &out,
                         const HashTable<K, V, Hash, GroupSize> &t) {
  out << "capacity=" << t.cap << " count=" << t.count
      << " tombstones=" << t.tombstones
      << " load=" << (float)(t.count + t.tombstones) / t.cap;
  return out;
}

template <typename K, typename V, typename Hash, size_t GroupSize>
void HashTable<K, V, Hash, GroupSize>::printStats() const {
  std::cout << *this << std::endl;
}
