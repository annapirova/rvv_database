#pragma once
#include "Value.h"
#include "../../Table/include/Table.h"
#include "../../../thirdparty/sql-parser/src/SQLParser.h"

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

    WhereOpType getOpType() { return Optype; }
    std::string get_name() { return name; }
    Value get_value() { return value; }
    TurboExpr* get_left() { return left; }
    TurboExpr* get_right() { return right; }

    Value execute_sup(TurboExpr* this_expr, size_t row_id, Table& table);
    bool execute(TurboExpr* this_expr, size_t row_id, Table& table);
};
