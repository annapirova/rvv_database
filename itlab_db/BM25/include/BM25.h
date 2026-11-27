#pragma once
#include "../../Table/include/Table.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>

struct ResultRow {
    int idx; // номер песни в таблице   
    double tfidf; // результат TF-IDF
    double bm25; // результат BM25
    double agg; // итоговая оценка       
};

class TwoStageSearch {
public:
    std::string get_document_text(int row_id) const;
    void fit(Table& table, const std::string& text_col = "text");
    std::vector<ResultRow> search(const std::string& query,
        int limit_stage1 = 100,
        int limit_stage2 = 5) const;

private:
    // Анкета песни
    struct Doc {
        std::unordered_map<uint32_t, int> trigram_tf; // триграмма - сколько раз встретилась
        int trigram_total = 0; // общее количество триграмм

        std::unordered_map<std::string, int> word_tf; // слово - сколько раз встретилось
        int word_total = 0; // общее количество слов
        int word_unique_count = 0; // количество уникальных слов
    };

    std::vector<Doc> docs_;

    Table* table_ = nullptr;
    std::string text_col_; // имя колонки (где ищем)
    DataType text_type_ = DataType::UNKNOWN; // тип колонки
    size_t text_col_idx_ = 0; // индекс колонки в таблице

    std::unordered_map<uint32_t, double> idf_trigram_; // все уникальные триграммы всей таблицы - их вес
    std::unordered_map<std::string, double> idf_word_; // все уникальные слова всей таблицы - их вес
    double avg_dl_ = 0.0; // средняя длина песни
    size_t N_ = 0; // количество песен

    void build_indexes();
    double score_tfidf(const Doc& d, const std::vector<uint32_t>& q_trigrams) const;
    double score_bm25(const Doc& d, const std::vector<std::string>& q_words) const;
};