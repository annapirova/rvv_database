#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <array>
#include <cstring>
#include <utility>

#include "SQLParser.h"
#include "util/sqlhelper.h"

#include "../Table/include/Table.h"

using namespace std;

enum WhereOpType {
    EQUAL, // =
    NOTEQUAL, // !=
    BIGGER, // >
    LOWER, // <
    BIGGEREQUAL, // >=
    LOWEREQUAL, // <=
    AND,
    OR,
    NUL, // значение
    NAME  // имя колонки
};


class Value{
    DataType type;
    int32_t int_val;
    double float_val;
    string str_val;

public:
    Value() : type(DataType::INT32) {}
    Value(int32_t v) : type(DataType::INT32), int_val(v) {}
    Value(double v) : type(DataType::FLOAT32), float_val(v) {}
    Value(const std::string& v) : type(DataType::FIXED_STRING), str_val(v) {}
    Value(const char* v) : type(DataType::FIXED_STRING), str_val(v) {}
    bool is_numeric() const {
        return type == DataType::INT32 || type == DataType::FLOAT32;
    }
    bool operator==(const Value& other) const {
        if (type == other.type) {
            if (type == DataType::INT32) return int_val == other.int_val;
            if (type == DataType::FLOAT32) return float_val == other.float_val;
            if (type == DataType::FIXED_STRING) return str_val == other.str_val;
        }
        if (this->is_numeric() && other.is_numeric()) {
            double left_d = (type == DataType::INT32) ? static_cast<double>(int_val) : float_val;
            double right_d = (other.type == DataType::INT32) ? static_cast<double>(other.int_val) : other.float_val;
            return left_d == right_d;
        }
        throw logic_error("different type");
    }

    bool operator>(const Value& other) const {
        if (type == other.type) {
            if (type == DataType::INT32) return int_val > other.int_val;
            if (type == DataType::FLOAT32) return float_val > other.float_val;
            if (type == DataType::FIXED_STRING) return str_val > other.str_val;
        }
        if (this->is_numeric() && other.is_numeric()) {
            double left_d = (type == DataType::INT32) ? static_cast<double>(int_val) : float_val;
            double right_d = (other.type == DataType::INT32) ? static_cast<double>(other.int_val) : other.float_val;
            return left_d > right_d;
        }
        throw logic_error("different type");
    }

    bool operator<(const Value& other) const {
        if (type == other.type) {
            if (type == DataType::INT32) return int_val < other.int_val;
            if (type == DataType::FLOAT32) return float_val < other.float_val;
            if (type == DataType::FIXED_STRING) return str_val < other.str_val;
        }
        if (this->is_numeric() && other.is_numeric()) {
            double left_d = (type == DataType::INT32) ? static_cast<double>(int_val) : float_val;
            double right_d = (other.type == DataType::INT32) ? static_cast<double>(other.int_val) : other.float_val;
            return left_d < right_d;
        }
        throw logic_error("different type");
    }
    bool operator<=(const Value& other) const {
        return (!((*this) > other));
    }
    bool operator>=(const Value& other) const {
        return (!((*this) < other));
    }
    bool operator!=(const Value& other) const {
        return (!((*this) == other));
    }
    string to_string() const {
        if (type == DataType::INT32) return std::to_string(int_val);
        if (type == DataType::FLOAT32) return std::to_string(float_val);
        return str_val;
    }
};


class TurboExpr { // обертка для Expr
private:
    TurboExpr* left;
    TurboExpr* right;
    string name;
    Value value;
    WhereOpType Optype;

public:
    WhereOpType getOpType() const { return Optype; }
    string get_name() const { return name; }
    Value get_value() const { return value; }
    TurboExpr* get_left() const { return left; }
    TurboExpr* get_right() const { return right; }

    TurboExpr(hsql::Expr* expr) {
        if (!expr) {
            left = nullptr;
            right = nullptr;
            Optype = NUL;
            name = "";
            value = 0;
            return;
        }
        this->left = expr->expr ? new TurboExpr(expr->expr) : nullptr;
        this->right = expr->expr2 ? new TurboExpr(expr->expr2) : nullptr;
        if (expr->name) {this->name = string(expr->name);}
        if (expr->type == hsql::kExprColumnRef) {this->Optype = NAME;}
        else if (expr->type == hsql::kExprLiteralString) {
            this->Optype = NUL;
            if (expr->name) this->value = Value(string(expr->name));
        }
        else if (expr->type == hsql::kExprLiteralInt) {
            this->Optype = NUL;
            this->value = Value(static_cast<int32_t>(expr->ival));
        }
        else if(expr->type == hsql::kExprLiteralFloat){
            this->Optype = NUL;
            this->value = Value(static_cast<double>(expr->fval));
        }
        else if (expr->type == hsql::kExprOperator) {
            hsql::OperatorType hsql_optype = expr->opType;
            switch(hsql_optype) {
            case hsql::kOpEquals:this->Optype = EQUAL; break;
            case hsql::kOpNotEquals:this->Optype = NOTEQUAL; break;
            case hsql::kOpGreater:this->Optype = BIGGER; break;
            case hsql::kOpLess:this->Optype = LOWER; break;
            case hsql::kOpGreaterEq:this->Optype = BIGGEREQUAL; break;
            case hsql::kOpLessEq:this->Optype = LOWEREQUAL; break;
            case hsql::kOpAnd:this->Optype = AND; break;
            case hsql::kOpOr:this->Optype = OR; break;
            default:this->Optype = NUL; break;
            }
        }
    }

    ~TurboExpr() {delete left;delete right; }


    Value execute_sup(TurboExpr* this_expr,size_t row_id,Table& table) { //const Table in the future
        if (this_expr->getOpType() == NUL) {
            return Value(this_expr->get_value());
        }
        if (this_expr->getOpType() == NAME) {
            uint32_t col_id = table.get_column_index(this_expr->get_name());
            uint32_t chunk_idx = row_id / 64; // пока Игорь не вынес константу или получение колонки
            uint32_t local_row_id = row_id % 64;
            if (col_id != -1) {
                Column& col = table.get_chunks()[chunk_idx].get_column(col_id);
                switch(col.type()){
                case DataType::INT32 : {int32_t ptr = col.get<int32_t>()[local_row_id];return(Value(ptr));}
                case DataType::FLOAT32 : {double ptr = col.get<double>()[local_row_id];return(Value(ptr));}
                case DataType::FIXED_STRING : {char* ptr = col.get<char[256]>()[local_row_id];return(Value(string(ptr)));}
                default: throw logic_error("no way type");
                }
            }
        }
        return Value();
    }

    bool execute(TurboExpr* this_expr, size_t row_id, Table& table) { //рекурсиивный парс дерева (also const Table need)
        if (!this_expr) return false;

        if (this_expr->getOpType() != AND && this_expr->getOpType() != OR && this_expr->getOpType() != NUL && this_expr->getOpType() != NAME) {
            Value left_value = execute_sup(this_expr->get_left(), row_id, table);
            Value right_value = execute_sup(this_expr->get_right(),row_id, table);

            switch(this_expr->getOpType()) {
            case EQUAL:return left_value == right_value;
            case NOTEQUAL:return left_value != right_value;
            case BIGGER:return left_value > right_value;
            case LOWER: return left_value < right_value;
            case BIGGEREQUAL:return left_value >= right_value;
            case LOWEREQUAL:return left_value <= right_value;
            default: throw logic_error("No way operation");
            }
        }
        else if (this_expr->getOpType() == AND || this_expr->getOpType() == OR) {
            bool one = execute(this_expr->get_left(), row_id, table);
            bool two = execute(this_expr->get_right(), row_id, table);
            return (this_expr->getOpType() == AND) ? (one && two) : (one || two);
        }
        return false;
    }
};

class QueryResult{
    Table& table;
    vector<size_t> rows;
    vector<size_t> cols;
public:
    QueryResult(Table& _table, vector<size_t>&& _rows,vector<size_t>&& _cols)
        : table(_table), rows(std::move(_rows)),cols(std::move(_cols)) {}

    QueryResult(QueryResult&& other) noexcept
        : table(other.table), rows(std::move(other.rows)),cols(std::move(other.cols)) {}
    vector<string>& get_names(){
        return table.column_names();
    }
    vector<size_t>& get_rows(){
        return rows;
    }
    vector<size_t>& get_cols(){
        return cols;
    }
    void set_cols(vector<size_t> new_cols){
        cols = std::move(new_cols);
    }
    void set_rows(vector<size_t> new_rows){
        rows = std::move(new_rows);
    }
    vector<Value> get_row(size_t row_id){
        vector<Value> res;
        size_t ch_id = row_id /64;
        size_t col_cnt = cols.size();
        for(int i = 0;i < col_cnt;++i){
            const Column& col = table.get_column(ch_id, cols[i]);
            DataType col_type = col.type();
            switch(col_type){
            case DataType::INT32 : {int32_t ptr = col.get<int32_t>()[row_id%64];res.push_back(Value(ptr));break;}
            case DataType::FLOAT32 : {double ptr = col.get<double>()[row_id%64];res.push_back(Value(ptr));break;}
            case DataType::FIXED_STRING : {char* ptr = col.get<char[256]>()[row_id%64];res.push_back(Value(string(ptr)));break;}
            default: throw logic_error("no way type");
            }
        }
        return res;
    }
    Table& get_table(){
        return table;
    }
};

class BaseOperator {
public:
    virtual ~BaseOperator() = default;
    virtual QueryResult execute_operator() = 0;
};

class GetTableOperator : public BaseOperator {
private:
    Table& table;
public:
    GetTableOperator(Table& _table) : table(_table) {}

    QueryResult execute_operator() override {
        size_t ch_sz = table.get_chunk_size();
        vector<size_t> tmp_rows;
        vector<size_t> tmp_cols;
        for(int i =0 ; i<ch_sz;++i){
            size_t row_sz = table.get_chunks()[i].get_rows_count();
            for(int j =0 ;j<row_sz;++j){
                tmp_rows.push_back(j + 64*i);
            }
        }
        size_t sz = table.column_names().size();
        for(int j =0 ;j<sz;++j){
            tmp_cols.push_back(j);
        }
        return QueryResult(table,std::move(tmp_rows),std::move(tmp_cols));
    }
};

class WhereOperator : public BaseOperator { // возвращаем индексы,которые подходят под условие
private:
    TurboExpr* current;
    BaseOperator* prev;
public:
    WhereOperator(TurboExpr* _current, BaseOperator* _prev):current(_current),prev(_prev) {}

    QueryResult execute_operator() override {
        QueryResult prev_data = prev->execute_operator();
        vector<size_t> valid_rows;
        for (size_t row_id : prev_data.get_rows()) {
            if (current->execute(current, row_id, prev_data.get_table())) {
                valid_rows.push_back(row_id);
            }
        }
        prev_data.set_rows(std::move(valid_rows));
        return prev_data;
    }
    ~WhereOperator(){delete current;}
};

class SelectOperator : public BaseOperator { //обрезаем колонки и строки
    vector<string> names;
    BaseOperator* where;
public:
    SelectOperator(WhereOperator* _where,vector<string>& _names):where(_where),names(_names){}
    QueryResult execute_operator() override {
        QueryResult prev_data = where->execute_operator();
        if(!names.empty() && names[0] == "*"){
            return prev_data;
        }
        vector<size_t> new_cols;
        for(string name : names){
            int32_t index = prev_data.get_table().get_column_index(name);
            if(index == -1){throw logic_error("syntax error");}
            new_cols.push_back(index);
        }
        prev_data.set_cols(new_cols);
        return prev_data;

    }
    ~SelectOperator(){delete where;}
};

int main(){

    const string query = "SELECT name,age FROM table_name WHERE age < 30.6;";
    hsql::SQLParserResult result;
    hsql::SQLParser::parse(query, &result);

    if (!result.isValid()) {
        cerr << "SQL Syntax Error!" << endl;
        return 1;
    }

    auto* select_stmt = (hsql::SelectStatement*)result.getStatement(0);
    vector<string> select_fields;
    for (auto* expr : *select_stmt->selectList) {
        if (expr->name) {
            select_fields.push_back(string(expr->name));
        }else if(expr->type == hsql::kExprStar){
            select_fields.push_back("*");
        }
    }

    Table db("Database");
    Chunk s1;

    s1.add_column(DataType::INT32);
    s1.get_column(0).allocate_aligned<int32_t>();
    s1.get_column(0).append<int32_t>(1);
    s1.get_column(0).append<int32_t>(2);
    s1.get_column(0).append<int32_t>(3);

    s1.add_column(DataType::FIXED_STRING);
    s1.get_column(1).allocate_aligned<std::array<char, 256>>();
    std::array<char, 256> tmp;

    std::snprintf(tmp.data(), sizeof(tmp), "Alice");
    s1.get_column(1).append<std::array<char, 256>>(tmp);

    std::snprintf(tmp.data(), sizeof(tmp), "Bob");
    s1.get_column(1).append<std::array<char, 256>>(tmp);

    std::snprintf(tmp.data(), sizeof(tmp), "Charlie");
    s1.get_column(1).append<std::array<char, 256>>(tmp);


    s1.add_column(DataType::INT32);
    s1.get_column(2).allocate_aligned<int32_t>();
    s1.get_column(2).append<int32_t>(25);
    s1.get_column(2).append<int32_t>(19);
    s1.get_column(2).append<int32_t>(30);

    db.get_chunks().push_back(std::move(s1));
    db.column_names() = {"id", "name", "age"};

    TurboExpr* math_tree = new TurboExpr(select_stmt->whereClause);
    GetTableOperator* get_table = new GetTableOperator(db);
    WhereOperator* where_node = new WhereOperator(math_tree, get_table);
    SelectOperator* select_node = new SelectOperator(where_node,select_fields);
    QueryResult res = select_node->execute_operator();

    for(size_t j_row : res.get_rows()){
        for(Value val : res.get_row(j_row)){
            cout << val.to_string() << " ";
        }
        cout << endl;
    }


   delete get_table;delete select_node;
    return 0;
}
