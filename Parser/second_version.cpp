#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

#include "SQLParser.h"
#include "util/sqlhelper.h"

using namespace std;

class BaseColumn{
    vector<string> col;
public:
    BaseColumn(vector<string> other) : col(other) {}
    string operator[](size_t index) const {
        return col[index];
    }

    size_t size() const { return col.size(); }
};


class SimpleTable {
private:
    vector<BaseColumn> table;
public:
    SimpleTable(vector<BaseColumn> cols):table(cols) {}

    vector<BaseColumn>& get_table() { return table; }

    uint32_t get_index(string col_name)const {
        for (size_t i = 0; i < table.size(); ++i) {
            if (table[i][0] == col_name) {
                return i;
            }
        }
        return -1;
    }
    vector<BaseColumn> get_columns(pair<vector<BaseColumn>,vector<size_t>> par){
        vector<BaseColumn> ans;
        for(uint32_t i = 0;i < par.second.size();++i){
            if(par.second[i] < par.first.size()){ans.push_back(par.first[par.second[i]]);}
        }
        return ans;
    }
    BaseColumn operator[](size_t index) const {
        return table[index];
    }
};

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

    string execute_sup(WhereTurboExpr* this_expr, const vector<BaseColumn>& cols, size_t row_idx, const SimpleTable& table) {
        if (this_expr->getOpType() == NUL) {
            return this_expr->get_value();
        }
        if (this_expr->getOpType() == NAME) {
            uint32_t col_idx = table.get_index(this_expr->get_name());
            if (col_idx != -1) {
                return cols[col_idx][row_idx];
            }
        }
        return "";
    }

    bool execute(WhereTurboExpr* this_expr, const vector<BaseColumn>& cols, size_t row_idx, const SimpleTable& table) { //рекурсиивный парс дерева
        if (!this_expr) return false;

        if (this_expr->getOpType() != AND && this_expr->getOpType() != OR && this_expr->getOpType() != NUL && this_expr->getOpType() != NAME) {
            string left_value = execute_sup(this_expr->get_left(), cols, row_idx, table);
            string right_value = execute_sup(this_expr->get_right(), cols, row_idx, table);

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
            bool one = execute(this_expr->get_left(), cols, row_idx, table);
            bool two = execute(this_expr->get_right(), cols, row_idx, table);
            return (this_expr->getOpType() == AND) ? (one && two) : (one || two);
        }
        return false;
    }
};

class BaseOperator {
public:
    virtual ~BaseOperator() = default;
    virtual pair<vector<BaseColumn>, vector<size_t>> execute_operator() = 0;
};

class GetTableOperator : public BaseOperator {
private:
    SimpleTable _table;
public:
    GetTableOperator(SimpleTable table) : _table(table) {}

    pair<vector<BaseColumn>, vector<size_t>> execute_operator() override {
        vector<size_t> all_indices;
        if (!_table.get_table().empty()) {
            size_t row_count = _table.get_table()[0].size();
            for (size_t i = 0; i < row_count; ++i) {
                all_indices.push_back(i);
            }
        }
        return {_table.get_table(), all_indices};
    }
};

class WhereOperator : public BaseOperator { // возвращаем индексы,которые подходят под условие
private:
    WhereTurboExpr* current;
    BaseOperator* prev;
    SimpleTable& table;

public:
    WhereOperator(WhereTurboExpr* _current, BaseOperator* _prev, SimpleTable& _table):current(_current),prev(_prev),table(_table) {}

    pair<vector<BaseColumn>, vector<size_t>> execute_operator() override {
        auto prev_data = prev->execute_operator();
        vector<BaseColumn> col = prev_data.first;
        vector<size_t> incoming_indices = prev_data.second;

        vector<size_t> bucket;

        for (size_t i : incoming_indices) {
            if (current->execute(current, col, i, table)) {
                bucket.push_back(i);
            }
        }

        return {col, bucket};
    }
};

class SelectOperator : public BaseOperator { //обрезаем колонки и строки,выводим результат
    vector<string> names;
    BaseOperator* where;
    SimpleTable& table;
public:
    SelectOperator(vector<string> _names,WhereOperator* _where,SimpleTable& _table):names(_names),where(_where),table(_table){}
    pair<vector<BaseColumn>, vector<size_t>> execute_operator() override {
        pair<vector<BaseColumn>,vector<size_t>> par = where->execute_operator();
        vector<BaseColumn> cols = par.first;vector<size_t>indexes = par.second;

        vector<BaseColumn> final_col;
        if(!names.empty() && names[0] == "*"){
            final_col = cols;
        }
        else{
            for(string name : names){
                uint32_t index = table.get_index(name);
                final_col.push_back(cols[index]);
            }
        }
        for(size_t ind : indexes){
            for(uint32_t i = 0 ; i < final_col.size();++i){
                cout << final_col[i][ind] << " ";
            }
            cout << endl;
        }
        return {cols,indexes};
    }
};
int main(){
    BaseColumn c1({"id","1", "2", "3"});
    BaseColumn c2({"name", "zlex", "cusi", "robot"});
    BaseColumn c3({"city","Moscow", "Toronto", "Samara"});
    SimpleTable db({c1, c2, c3});

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

    WhereTurboExpr* math_tree = new WhereTurboExpr(select_stmt->whereClause);
    GetTableOperator* get_table = new GetTableOperator(db);
    WhereOperator* where_node = new WhereOperator(math_tree, get_table, db);
    SelectOperator* select_node = new SelectOperator(select_fields, where_node,db);
    select_node->execute_operator();
    delete math_tree;delete get_table;delete where_node;delete select_node;
    return 0;
}
