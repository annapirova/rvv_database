#pragma once
#include "Value.h"
#include "Table.h"
#include "SQLParser.h"

class TurboExpr {
private:
    TurboExpr* left;
    TurboExpr* right;
    std::string name;
    Value value;
    WhereOpType Optype;

public:
    TurboExpr(hsql::Expr* expr);
    ~TurboExpr();

    WhereOpType getOpType() const { return Optype; }
    std::string get_name() const { return name; }
    Value get_value() const { return value; }
    TurboExpr* get_left() const { return left; }
    TurboExpr* get_right() const { return right; }

    Value execute_sup(TurboExpr* this_expr, size_t row_id, const Table& table);
    bool execute(TurboExpr* this_expr, size_t row_id, const Table& table);
};
