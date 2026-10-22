### 1. Swiss Hash Table 
Архитектура хеш-таблицы с открытой адресацией, оптимизированная за счет использования SIMD-инструкций. 
  * [abseil](https://abseil.io/about/design/swisstables)
  * [Доклад на CppCon (YouTube): Matt Kulukundis](https://www.youtube.com/watch?v=ncHmEUmJZf4&xstg=CAMSEBUJ_b-oH-PhF0yjBgbwzzE%3D)
  * [Реализация Abseil](https://github.com/abseil/abseil-cpp)
  * [описание Apache](https://arrow.apache.org/docs/developers/cpp/acero/swiss_table.html)
  * [описание Go](https://go.dev/blog/swisstable)
  * [бенчмарки хеш-таблиц](https://jacksonallan.github.io/c_cpp_hash_tables_benchmark/)

### 2. FAST-дерево (Fast Architecture Sensitive Tree)
Специализированная вариация B+ Дерева поиска, адаптированная под архитектуру процессорного кэша и регистров SIMD.
  * [статья](https://www.researchgate.net/profile/Jatin-Chhugani/publication/221213860_FAST_fast_architecture_sensitive_tree_search_on_modern_CPUs_and_GPUs/links/0c96051f5d29890097000000/FAST-fast-architecture-sensitive-tree-search-on-modern-CPUs-and-GPUs.pdf)
  * [презентация от разработчика VK Утусикова](https://squidex.jugru.team/api/assets/srm/596bc84b-fe73-499c-bfe9-6b293595a433/cpp-russia-03-06.pdf)
* [видеолекция Утусикова](https://www.google.com/url?sa=t&source=web&rct=j&opi=89978449&url=https://www.youtube.com/watch%3Fv%3Dk3t96JrX92Y%26xstg%3DCAMSEBUJ_b-oH-PhF0yjBgbwzzE%253D&ved=2ahUKEwjjrNX-1bCXAxVJJRAIHQ6LALYQtwJ6BAgWEAE&usg=AOvVaw2XSCUwvBe509mUpQZSA97m) 

### 3. ART-дерево (Adaptive Radix Tree)
Адаптивное префиксное дерево, разработанное для in-memory баз данных.
  * [статья](https://ieeexplore.ieee.org/iel7/6530811/6544790/06544812.pdf?casa_token=UU2QcqvR5HUAAAAA:9DJnRbTqbQtqz5CO7emTUcJIRBIdYslWzMPtbtzfwGkRC3GjYdP4igf4NN1DxSeJWBlIHQQPKd2tkA) 
### 4. Ранжирование Okapi BM25
Функция ранжирования полнотекстового поиска, оценивающая релевантность документа на основе частоты терминов.
  * [Wikipedia](https://ru.wikipedia.org/wiki/Okapi_BM25)
  * [Elasticsearch Guide](https://www.elastic.co/blog/practical-bm25-part-2-the-bm25-algorithm-and-its-variables)

### 5. RISC-V Vector Extension (RVV)
* Intrinsic Viewer https://dzaima.github.io/intrinsics-viewer/

### 6. Оптимизация и обработка запросов
* [лекция на англ.](https://15445.courses.cs.cmu.edu/fall2025/notes/15-optimization1.pdf)
* [книга Postgre SQL](https://postgrespro.ru/education/books/qptbook)
* [книга на англ.](https://howqueryengineswork.com/01-what-is-a-query-engine.html)
