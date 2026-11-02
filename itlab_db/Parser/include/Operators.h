#pragma once
#include "QueryResult.h"
#include "TurboExpr.h"
#include <vector>
#include <string>

class BaseOperator {
public:
    virtual ~BaseOperator() = default;
    virtual QueryResult execute_operator() = 0;
};

class GetTableOperator : public BaseOperator {
private:
    Table& table;
public:
    GetTableOperator(Table& _table);
    QueryResult execute_operator() override;
};

class WhereOperator : public BaseOperator {
private:
    TurboExpr* current;
    BaseOperator* prev;
public:
    WhereOperator(TurboExpr* _current, BaseOperator* _prev);
    ~WhereOperator();
    QueryResult execute_operator() override;
};

class SelectOperator : public BaseOperator {
private:
    std::vector<std::string> names;
    BaseOperator* where;
public:
    SelectOperator(WhereOperator* _where, std::vector<std::string>& _names);
    ~SelectOperator();
    QueryResult execute_operator() override;
};
