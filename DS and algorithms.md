### 1. Swiss Hash Table 
Архитектура хеш-таблицы с открытой адресацией, оптимизированная за счет использования SIMD-инструкций. 
* **Полезные материалы и спецификация:**
  * [Официальный блог Google Open Source: Designing a Fast Asymmetric Hash Table]( ) — концепция и бенчмарки.
  * [Доклад на CppCon (YouTube): Matt Kulukundis](https://www.youtube.com/watch?v=ncHmEUmJZf4&xstg=CAMSEBUJ_b-oH-PhF0yjBgbwzzE%3D)
  * [Реализация Abseil](https://github.com/abseil/abseil-cpp)
  * [описание Apache](https://arrow.apache.org/docs/developers/cpp/acero/swiss_table.html)
  * [бенчмарки](https://jacksonallan.github.io/c_cpp_hash_tables_benchmark/)

### 2. FAST-дерево (Fast Architecture Sensitive Tree)
Специализированная вариация B+ Дерева поиска, адаптированная под архитектуру процессорного кэша и регистров SIMD.
* **Полезные материалы и спецификация:**
  * [статья] (https://www.researchgate.net/profile/Jatin-Chhugani/publication/221213860_FAST_fast_architecture_sensitive_tree_search_on_modern_CPUs_and_GPUs/links/0c96051f5d29890097000000/FAST-fast-architecture-sensitive-tree-search-on-modern-CPUs-and-GPUs.pdf)
  * [презентация от разработчика VK] (https://squidex.jugru.team/api/assets/srm/596bc84b-fe73-499c-bfe9-6b293595a433/cpp-russia-03-06.pdf)

### 3. ART-дерево (Adaptive Radix Tree)
Адаптивное префиксное дерево, разработанное для in-memory баз данных, решающее проблему пустой траты памяти в классических Trie.
* **Полезные материалы и спецификация:**
  * [статья](https://ieeexplore.ieee.org/iel7/6530811/6544790/06544812.pdf?casa_token=UU2QcqvR5HUAAAAA:9DJnRbTqbQtqz5CO7emTUcJIRBIdYslWzMPtbtzfwGkRC3GjYdP4igf4NN1DxSeJWBlIHQQPKd2tkA) 

### 4. Ранжирование Okapi BM25
Функция ранжирования полнотекстового поиска, оценивающая релевантность документа на основе частоты терминов.
* **Полезные материалы и спецификация:**
  * [Подробный разбор BM25 на Wikipedia]([https://wikipedia.org](https://ru.wikipedia.org/wiki/Okapi_BM25))
  * [Elasticsearch Guide](https://www.elastic.co/blog/practical-bm25-part-2-the-bm25-algorithm-and-its-variables)

### 5. RISC-V Vector Extension (RVV)
* **Полезные материалы и спецификация:**
* Intrinsic Viewer https://dzaima.github.io/intrinsics-viewer/
