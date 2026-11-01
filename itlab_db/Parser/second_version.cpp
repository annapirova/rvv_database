#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include <array>
#include <cstring>
#include <utility>


#include "SQLParser.h"
#include "util/sqlhelper.h"

#include "../Table/include/Table.h"
#include "../Table/include/Column.h"
#include "../Table/include/Chunk.h"

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

class WhereTurboExpr { // обертка для Expr
private:
    WhereTurboExpr* left;
    WhereTurboExpr* right;
    string name;
    string value;
    WhereOpType Optype;

public:
    WhereOpType getOpType() const { return Optype; }
    string get_name() const { return name; }
    string get_value() const { return value; }
    WhereTurboExpr* get_left() const { return left; }
    WhereTurboExpr* get_right() const { return right; }

    WhereTurboExpr(hsql::Expr* expr) {
        if (!expr) {
            left = nullptr;
            right = nullptr;
            Optype = NUL;
            name = "";
            value = "";
            return;
        }
        this->left = expr->expr ? new WhereTurboExpr(expr->expr) : nullptr;
        this->right = expr->expr2 ? new WhereTurboExpr(expr->expr2) : nullptr;
        if (expr->name) {this->name = string(expr->name);}
        if (expr->type == hsql::kExprColumnRef) {this->Optype = NAME;}
        else if (expr->type == hsql::kExprLiteralString) {
            this->Optype = NUL;
            if (expr->name) this->value = expr->name;
        }
        else if (expr->type == hsql::kExprLiteralInt) {
            this->Optype = NUL;
            this->value = to_string(expr->ival);
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

    ~WhereTurboExpr() { if(left){delete left;} if(right){delete right;} left = nullptr;right=nullptr; }

// problem - work only with string ,but in table every data exist


    string execute_sup(WhereTurboExpr* this_expr,size_t row_idx,Table& table) { //const Table in the future
        if (this_expr->getOpType() == NUL) {
            return this_expr->get_value();
        }
        if (this_expr->getOpType() == NAME) {
            uint32_t col_idx = table.get_column_index(this_expr->get_name());
            uint32_t chunk_idx = row_idx / 64; // пока Игорь не вынес константу или получение колонки
            if (col_idx != -1) {

                char* ptr = table.get_column(chunk_idx,col_idx).get<char[256]>()[row_idx];

                return string(ptr);
            }
        }
        return "";
    }

    bool execute(WhereTurboExpr* this_expr, size_t row_idx, Table& table) { //рекурсиивный парс дерева (also const Table need)
        if (!this_expr) return false;

        if (this_expr->getOpType() != AND && this_expr->getOpType() != OR && this_expr->getOpType() != NUL && this_expr->getOpType() != NAME) {
            string left_value = execute_sup(this_expr->get_left(), row_idx, table);
            string right_value = execute_sup(this_expr->get_right(),row_idx, table);

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
            bool one = execute(this_expr->get_left(), row_idx, table);
            bool two = execute(this_expr->get_right(), row_idx, table);
            return (this_expr->getOpType() == AND) ? (one && two) : (one || two);
        }
        return false;
    }
};

class QueryResult{
    Table table;
    vector<size_t> indexes;
public:
    QueryResult(Table _table,vector<size_t> _indexes):table(_table),indexes(_indexes){}
    QueryResult(const QueryResult& other):table(other.table),indexes(other.indexes){}
    void delete_row(size_t row_id){
        indexes.erase(indexes.begin() + row_id);
    }
    vector<string>& get_names(){
        return table.column_names();
    }
    size_t get_str_count(){
        size_t count = 0;
        size_t ch_sz = table.get_chunk_size();
        for(int i =0 ; i<ch_sz;++i){
            size_t row_sz = table.get_chunks()[i].get_rows_count();
            count += row_sz;
        }
        return count;
    }
    string get_row(size_t row_id){
        if(row_id >= get_str_count()){throw out_of_range("out of range");}
        string res = "";
        size_t ch_id = row_id /64;
        size_t col_cnt = table.column_names().size();
        for(int i = 0;i < col_cnt;++i){
            char* ptr = table.get_column(ch_id,i).get<char[256]>()[row_id];
            res += string(ptr);
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
    Table table;
public:
    GetTableOperator(Table _table) : table(_table) {}

    QueryResult execute_operator() override {
        size_t ch_sz = table.get_chunk_size();
        vector<size_t> tmp_ind;
        for(int i =0 ; i<ch_sz;++i){
            size_t row_sz = table.get_chunks()[i].get_rows_count();
            for(int j =0 ;j<row_sz;++j){
                tmp_ind.push_back(j + 64*i);
            }
        }
        return QueryResult(table,tmp_ind);
    }
};

class WhereOperator : public BaseOperator { // возвращаем индексы,которые подходят под условие
private:
    WhereTurboExpr* current;
    BaseOperator* prev;
public:
    WhereOperator(WhereTurboExpr* _current, BaseOperator* _prev):current(_current),prev(_prev) {}

    QueryResult execute_operator() override {
        QueryResult prev_data = prev->execute_operator();
        size_t ch_sz = prev_data.get_table().get_chunk_size();
        for(int i =0 ; i<ch_sz;++i){
            size_t row_sz = prev_data.get_table().get_chunks()[i].get_rows_count();
            for(int j =0 ;j<row_sz;++j){
                size_t row_id = j+64*i;
                if(!current->execute(current,row_id,prev_data.get_table())){
                    prev_data.delete_row(row_id);
                }
            }
        }
        return prev_data;
    }
    ~WhereOperator(){if(current){delete[] current;}current=nullptr;}
};

class SelectOperator : public BaseOperator { //обрезаем колонки и строки,выводим результат
    vector<string> names;
    BaseOperator* where;
public:
    SelectOperator(WhereOperator* _where):where(_where){}
    QueryResult execute_operator() override {
        QueryResult prev_data = where->execute_operator();
        this->names = prev_data.get_table().column_names();
        return prev_data;
        // vector<BaseColumn> final_col;
        // if(!names.empty() && names[0] == "*"){
        //     final_col = cols;
        // }
        // else{
        //     for(string name : names){
        //         uint32_t index = table.get_index(name);
        //         final_col.push_back(cols[index]);
        //     }
        // }
        // for(size_t ind : indexes){
        //     for(uint32_t i = 0 ; i < final_col.size();++i){
        //         cout << final_col[i][ind] << " ";
        //     }
        //     cout << endl;
        // }
        // return {cols,indexes};
    }
    ~SelectOperator(){if(where){delete[] where;}where=nullptr;}
};

int main(){

    const string query = "SELECT city FROM table_name WHERE id > 1;";
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
    //   не понятно как набить чанк или таблицу
    db.get_chunks().push_back(std::move(s1));
    db.column_names() = {"id", "name", "city"};

    WhereTurboExpr* math_tree = new WhereTurboExpr(select_stmt->whereClause);
    GetTableOperator* get_table = new GetTableOperator(db);
    WhereOperator* where_node = new WhereOperator(math_tree, get_table);
    SelectOperator* select_node = new SelectOperator(where_node);
    select_node->execute_operator();
    return 0;
}
