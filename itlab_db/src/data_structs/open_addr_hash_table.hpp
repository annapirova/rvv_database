#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

template <typename K, typename V> class HashTable {
  enum State : uint8_t { EMPTY, OCCUPIED, TOMBSTONE };

  struct Slot {
    K key{};
    V value{};
    State state = EMPTY;
  };

  std::vector<Slot> table;
  size_t count = 0;      // количество живых элементов
  size_t tombstones = 0; // количество надгробий
  std::hash<K> hasher;

  static constexpr float MAX_LOAD = 0.5f;    // (count + tombstones) / size
  static constexpr float SHRINK_LOAD = 0.1f; // слишком пустая таблица
  static constexpr size_t MIN_SIZE = 8;

  // Проверка на простое число и поиск следующего простого числа (нужны для
  // подбора длины таблицы, подходящей для прыжков без накладок)
  static bool isPrime(size_t x) {
    if (x < 2)
      return false;
    if (x % 2 == 0)
      return x == 2;
    for (size_t i = 3; i * i <= x; i += 2)
      if (x % i == 0)
        return false;
    return true;
  }
  static size_t nextPrime(size_t n) {
    while (!isPrime(n))
      ++n;
    return n;
  }

  // Начало прыжков
  size_t homeIndex(const K &key) const { return hasher(key) % table.size(); }

  // Квадратичный поиск: (h + i*i) % size
  Slot *findSlot(const K &key) {
    size_t h = homeIndex(key), m = table.size();
    for (size_t i = 0; i < m; ++i) {
      size_t idx = (h + i * i) % m;
      if (table[idx].state == EMPTY)
        return nullptr;
      if (table[idx].state == OCCUPIED && table[idx].key == key)
        return &table[idx];
    }
    return nullptr;
  }
  const Slot *findSlot(const K &key) const {
    size_t h = homeIndex(key), m = table.size();
    for (size_t i = 0; i < m; ++i) {
      size_t idx = (h + i * i) % m;
      if (table[idx].state == EMPTY)
        return nullptr;
      if (table[idx].state == OCCUPIED && table[idx].key == key)
        return &table[idx];
    }
    return nullptr;
  }

  // Размещение
  void place(K key, V value) {
    size_t h = hasher(key) % table.size(), m = table.size();
    for (size_t i = 0; i < m; ++i) {
      size_t idx = (h + i * i) % m;
      if (table[idx].state != OCCUPIED) {
        table[idx].key = std::move(key);
        table[idx].value = std::move(value);
        table[idx].state = OCCUPIED;
        ++count;
        return;
      }
    }
  }

  void rehashTo(size_t newSize) {
    newSize = nextPrime(std::max(newSize, MIN_SIZE));
    std::vector<Slot> old = std::move(table);
    table.assign(newSize, Slot{});
    count = 0;
    tombstones = 0;
    for (auto &slot : old) {
      if (slot.state == OCCUPIED)
        place(std::move(slot.key), std::move(slot.value));
    }
  }

  // Гарантирует строго load < 50% для n элементов
  size_t sizeForCount(size_t n) const { return nextPrime(n * 2 + 1); }

public:
  explicit HashTable(size_t initial = MIN_SIZE) {
    table.assign(nextPrime(std::max(initial, MIN_SIZE)), Slot{});
  }

  void insert(const K &key, const V &value) {
    // Обновление существующего
    if (Slot *s = findSlot(key)) {
      s->value = value;
      return;
    }

    // Проверка загрузки
    if ((float)(count + tombstones + 1) / table.size() >= MAX_LOAD) {
      size_t target = sizeForCount(count + 1);
      // Если расти не обязательно — только чистим надгробия
      if (target > table.size() && tombstones > 0 &&
          (float)(count + 1) / table.size() < MAX_LOAD) {
        target = table.size();
      }
      rehashTo(target);
    }

    // Квадратичное пробирование
    size_t h = homeIndex(key), m = table.size();
    size_t firstTomb = SIZE_MAX;
    for (size_t i = 0; i < m; ++i) {
      size_t idx = (h + i * i) % m;
      if (table[idx].state == EMPTY) {
        size_t target = (firstTomb != SIZE_MAX) ? firstTomb : idx;
        if (firstTomb != SIZE_MAX)
          --tombstones;
        table[target].key = key;
        table[target].value = value;
        table[target].state = OCCUPIED;
        ++count;
        return;
      }
      if (table[idx].state == TOMBSTONE && firstTomb == SIZE_MAX)
        firstTomb = idx;
    }
    // Страховка (при корректной нагрузке не срабатывает)
    rehashTo(m * 2);
    insert(key, value);
  }

  V &get(const K &key) {
    Slot *s = findSlot(key);
    if (!s)
      throw std::out_of_range("Key not found");
    return s->value;
  }
  const V &get(const K &key) const {
    const Slot *s = findSlot(key);
    if (!s)
      throw std::out_of_range("Key not found");
    return s->value;
  }

  bool remove(const K &key) {
    Slot *s = findSlot(key);
    if (!s)
      return false;
    s->state = TOMBSTONE;
    --count;
    ++tombstones;

    if (table.size() <= MIN_SIZE)
      return true;

    float liveLoad = (float)count / table.size();
    bool tooManyTombstones = tombstones > count;
    bool tooSparse = liveLoad < SHRINK_LOAD;

    if (tooManyTombstones || tooSparse) {
      // Рехеш вниз: подбираем размер под оставшиеся элементы
      size_t newSize = std::min(sizeForCount(count), table.size());
      rehashTo(newSize);
    }
    return true;
  }

  bool contains(const K &key) const { return findSlot(key) != nullptr; }
  size_t size() const { return count; }
  size_t tombstoneCount() const { return tombstones; }
  size_t capacity() const { return table.size(); }
  bool empty() const { return count == 0; }

  friend std::ostream &operator<<(std::ostream &out, const HashTable<K, V> &h) {
    out << "capacity=" << h.table.size() << " count=" << h.count
        << " tombstones=" << h.tombstones
        << " load=" << (float)(h.count + h.tombstones) / h.table.size();
    return out;
  }
  void printStats() const { std::cout << *this << std::endl; }
};
