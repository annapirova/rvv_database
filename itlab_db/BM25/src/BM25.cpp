#include "../include/BM25.h"
#include "../../Table/include/FixedString.h"
#include "../../Table/include/LogicalColumnView.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <cctype>

// приводим к нижнему регистру
static inline std::string to_lower(std::string s) {
    for (char& c : s) c = std::tolower((unsigned char)c);
    return s;
}

static const std::vector<std::string> SUFFIXES = {
    "ing", "ly", "ed", "ious", "ies", "ive", "es", "s", "ment"
};

// обрезаем окончания
static std::string stem_word(const std::string& w) {
    for (const std::string& suf : SUFFIXES) {
        if (w.size() >= suf.size() &&
            w.compare(w.size() - suf.size(), suf.size(), suf) == 0) {
            return w.substr(0, w.size() - suf.size());
        }
    }
    return w;
}

// обработка одного документа
static std::string preprocess_document(const std::string& doc) {
    std::string cleaned;
    cleaned.reserve(doc.size());
    // удалили все, кроме цифр, букв и пробелов
    for (unsigned char c : doc) {
        if (std::isalnum(c) || c == ' ') cleaned.push_back((char)c);
    }
    // понизили регистр
    cleaned = to_lower(cleaned);
    // находим начало и конец
    size_t b = cleaned.find_first_not_of(' ');
    size_t e = cleaned.find_last_not_of(' ');
    if (b == std::string::npos) return "";
    cleaned = cleaned.substr(b, e - b + 1); // обрезали
    std::string out;
    out.reserve(cleaned.size());
    size_t i = 0;
    bool first = true;
    // делаем так, чтобы слова были соединены только одним пробелом
    while (i <= cleaned.size()) {
        size_t j = cleaned.find(' ', i);
        if (j == std::string::npos) j = cleaned.size();
        std::string word = cleaned.substr(i, j - i);
        if (!word.empty()) {
            std::string st = stem_word(word); // обрезали окончания
            if (!first) out.push_back(' ');
            out += st;
            first = false;
        }
        if (j == cleaned.size()) break;
        i = j + 1;
    }
    return out;
}

// разбиваем строку на слова
static std::vector<std::string> split_by_space(const std::string& s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i <= s.size()) {
        size_t j = s.find(' ', i);
        if (j == std::string::npos) j = s.size();
        std::string tok = s.substr(i, j - i);
        if (!tok.empty()) out.push_back(std::move(tok));
        if (j == s.size()) break;
        i = j + 1;
    }
    return out;
}

using Trigram = uint32_t;

// упаковываем триграмму в 32-битный int
static inline Trigram pack3(char a, char b, char c) {
    return ((Trigram)(uint8_t)a << 16) | ((Trigram)(uint8_t)b << 8) | (Trigram)(uint8_t)c;
}

// разбиваем слово на триграммы
static void word_to_trigrams(const std::string& w, std::vector<Trigram>& out) {
    if (w.size() < 3) return; // слова длины <3 не участвуют в 1 этапе. В основном это предлоги, так что норм (можно и включить их конечно)
    for (size_t i = 0; i + 3 <= w.size(); ++i) {
        out.push_back(pack3(w[i], w[i + 1], w[i + 2]));
    }
}

// читаем текст из таблицы
static std::string read_cell(Table& t, size_t col_idx, DataType type, size_t row) {
    switch (type) {
    case DataType::FIXED_STRING32: {
        LogicalColumnView v = t.at<FixedString32>(col_idx);
        return std::string((const char*)v.get<FixedString32>(row));
    }
    case DataType::FIXED_STRING64: {
        LogicalColumnView v = t.at<FixedString64>(col_idx);
        return std::string((const char*)v.get<FixedString64>(row));
    }
    case DataType::FIXED_STRING128: {
        LogicalColumnView v = t.at<FixedString128>(col_idx);
        return std::string((const char*)v.get<FixedString128>(row));
    }
    default:
        throw std::invalid_argument("Column is not a FIXED_STRING* type");
    }
}

// Сравнение для первого этапа
static bool compare_stage1(const std::pair<int, double>& a, const std::pair<int, double>& b) {
    return a.second > b.second;
}

// Округление до 3 знаков
static double round_3(double x) {
    return std::round(x * 1000.0) / 1000.0;
}

// Сравнение для итоговых результатов
static bool compare_result_rows(const ResultRow& a, const ResultRow& b) {
    double ra = round_3(a.agg), rb = round_3(b.agg);
    if (ra != rb) return ra > rb;
    double rba = round_3(a.bm25), rbb = round_3(b.bm25);
    if (rba != rbb) return rba > rbb;

    return round_3(a.tfidf) > round_3(b.tfidf);
}

void TwoStageSearch::fit(Table& table, const std::string& text_col) {
    int idx = table.get_column_index(text_col); // индекс колонки по имени
    if (idx < 0) throw std::invalid_argument("Table has no column: " + text_col);
    // проверяем, что там строки
    DataType t = table.get_column_types().at((size_t)idx);
    if (t != DataType::FIXED_STRING32 && t != DataType::FIXED_STRING64 && t != DataType::FIXED_STRING128) {
        throw std::invalid_argument("Column " + text_col + " is not FIXED_STRING*");
    }

    table_ = &table;
    text_col_ = text_col;
    text_col_idx_ = (size_t)idx;
    text_type_ = t;
    // идем по чанкам и считаем общее количество строк
    size_t n = 0;
    for (const Chunk& ch : table.get_chunks()) n += ch.get_rows_count();
    N_ = n;

    docs_.clear();
    docs_.reserve(n);

    for (size_t row = 0; row < n; ++row) {
        std::string text = read_cell(table, text_col_idx_, text_type_, row); // считали песню
        std::string pre = preprocess_document(text);
        // создаем анкету каждой песни
        Doc d;
        std::vector<std::string> words = split_by_space(pre);
        // считаем общее к-во слов и заполняем: слово - его частота
        for (std::string& w : words) {
            d.word_tf[w]++;
            d.word_total++;
        }
        d.word_unique_count = (int)d.word_tf.size();

        for (std::string& w : words) {
            std::vector<Trigram> tmp;
            word_to_trigrams(w, tmp); // режем на триграммы
            // считаем общее к-во триграмм и заполняем: триграмма - ее частота
            for (Trigram tg : tmp) {
                d.trigram_tf[tg]++;
                d.trigram_total++;
            }
        }
        docs_.push_back(std::move(d));
    }

    build_indexes();
}

void TwoStageSearch::build_indexes() {
    std::unordered_map<Trigram, int> df_trigram; // триграмма - в скольких песнях встречается
    std::unordered_map<std::string, int> df_word; // слово - в скольких песнях встречается
    long long sum_dl = 0; // общая длина всех песен
    // в скольких песнях встретились триграммы и слова
    for (const Doc& d : docs_) {
        for (const std::pair<const uint32_t, int>& kv : d.trigram_tf) df_trigram[kv.first]++;
        for (const std::pair<const std::string, int>& kv : d.word_tf) df_word[kv.first]++;
        sum_dl += d.word_total;
    }

    avg_dl_ = (N_ == 0) ? 0.0 : (double)sum_dl / (double)N_; // считаем среднюю длину песни
    // считаем IDF (вес) триграммы
    for (const std::pair<const uint32_t, int>& kv : df_trigram) {
        idf_trigram_[kv.first] = std::log((1.0 + (double)N_) / (1.0 + (double)kv.second));
    }
    // считаем IDF (вес) слова
    for (const std::pair<const std::string, int>& kv : df_word) {
        double dfv = (double)kv.second;
        double x = ((double)N_ - dfv + 0.5) / (dfv + 0.5);
        idf_word_[kv.first] = std::log(x + 1.0);
    }
}
// 1 этап: TF-IDF для одной песни
double TwoStageSearch::score_tfidf(const Doc& d, const std::vector<uint32_t>& q_trigrams) const {
    if (d.trigram_total == 0) return 0.0;
    double s = 0.0;
    const double inv_total = 1.0 / (double)d.trigram_total;

    for (Trigram q : q_trigrams) {
        auto it = d.trigram_tf.find(q);
        if (it == d.trigram_tf.end()) continue;

        double tf = (double)it->second * inv_total;
        auto itidf = idf_trigram_.find(q);
        double idf_v = (itidf == idf_trigram_.end()) ? 1e-3 : itidf->second;
        s += tf * idf_v;
    }
    return s;
}
// 2 этап: BM25 для одной песни
double TwoStageSearch::score_bm25(const Doc& d, const std::vector<std::string>& q_words) const {
    if (d.word_total == 0 || avg_dl_ <= 0.0) return 0.0;

    double k1 = 4, b = 0.5, delta = 1.0;
    double s = 0.0;

    for (const std::string& q : q_words) {
        auto itf = d.word_tf.find(q);
        if (itf == d.word_tf.end()) continue; 

        double tf = (double)itf->second;
        auto itidf = idf_word_.find(q);
        double idf_v = (itidf == idf_word_.end()) ? 1e-6 : itidf->second;

        double num = tf * (k1 + 1.0);
        double den = tf + k1 * (1.0 - b + b * (double)d.word_total / avg_dl_);
        double tf_part = num / den + delta;
        s += idf_v * tf_part;
    }
    return s;
}

std::vector<ResultRow> TwoStageSearch::search(const std::string& query, int limit_stage1, int limit_stage2) const {
    if (!table_) throw std::logic_error("fit() was not called");
    std::string q_pre = preprocess_document(query);
    std::vector<std::string> q_words = split_by_space(q_pre);
    std::vector<Trigram> q_trigrams;
    for (const std::string& w : q_words) {
        word_to_trigrams(w, q_trigrams);
    }
    std::vector<std::pair<int, double>> stage1;
    stage1.reserve(docs_.size());
    for (int i = 0; i < (int)docs_.size(); ++i) {
        double s = score_tfidf(docs_[i], q_trigrams);
        if (s > 0.0) stage1.emplace_back(i, s);
    }
    int k1 = std::min<int>(limit_stage1, stage1.size());
    std::partial_sort(stage1.begin(), stage1.begin() + k1, stage1.end(), compare_stage1);
    stage1.resize(k1);
    if (stage1.empty()) return {};
    std::vector<ResultRow> rows;
    rows.reserve(stage1.size());

    for (const std::pair<int, double>& p : stage1) {
        int idx = p.first;
        double s1 = p.second;
        double s2 = score_bm25(docs_[idx], q_words);

        if (s2 > 0.0) {
            double agg = std::sqrt(std::max(0.0, s1) * std::max(0.0, s2));
            rows.push_back({ idx, s1, s2, agg });
        }
    }
    std::sort(rows.begin(), rows.end(), compare_result_rows);
    if ((int)rows.size() > limit_stage2) rows.resize(limit_stage2);
    return rows;
}

std::string TwoStageSearch::get_document_text(int row_id) const {
    if (!table_) throw std::logic_error("fit() was not called");
    if (row_id < 0 || (size_t)row_id >= docs_.size()) throw std::out_of_range("row_id out of range");
    return read_cell(*table_, text_col_idx_, text_type_, (size_t)row_id);
}

