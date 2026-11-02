# rvv_database
Проект ITLab-2026

__Цель проекта__ - разработка упрощенной in-memory базы данных для процессоров RISC-V. В ядре библиотеки используются эффективные структуры данных, реализованные с использованием векторных SIMD-инструкций.

__Приложение:__ хранение и обработка метаданных медиафайлов, c поддержкой числовых фильтров, полнотекстовый поиска и семантического поиска по картинкам/тексту.

__Сборка__
```
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/default-gcc.toolchain.cmake ..
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/riscv64-1p0-gcc.toolchain.cmake ..
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/riscv64-1p0-gcc.toolchain.cmake -DWITH_SQLPARSER=OFF .. 
```
// последнее - с выключенным sql парсером 

__Примеры, аналоги__
- DuckDB https://duckdb.org/docs/current/
- PostgreSQL https://www.postgresql.org/
- Hyrise https://github.com/hyrise/hyrise (исследовательская)
