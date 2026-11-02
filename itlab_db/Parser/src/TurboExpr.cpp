#include "TurboExpr.h"

TurboExpr::TurboExpr(hsql::Expr* expr) {
    if (!expr) {
        left = nullptr; right = nullptr; Optype = WhereOpType::NUL; name = ""; value = Value(0);
        return;
    }
    this->left = expr->expr ? new TurboExpr(expr->expr) : nullptr;
    this->right = expr->expr2 ? new TurboExpr(expr->expr2) : nullptr;
    if (expr->name) { this->name = std::string(expr->name); }
    
    if (expr->type == hsql::kExprColumnRef) {
        this->Optype = WhereOpType::NAME;
    } else if (expr->type == hsql::kExprLiteralString) {
        this->Optype = WhereOpType::NUL;
        if (expr->name) this->value = Value(std::string(expr->name));
    } else if (expr->type == hsql::kExprLiteralInt) {
        this->Optype = WhereOpType::NUL;
        this->value = Value(static_cast<int32_t>(expr->ival));
    } else if (expr->type == hsql::kExprLiteralFloat || expr->type == hsql::kExprLiteralDouble) {
        this->Optype = WhereOpType::NUL;
        this->value = Value(static_cast<double>(expr->fval));
    } else if (expr->type == hsql::kExprOperator) {
        switch(expr->opType) {
            case hsql::kOpEquals: this->Optype = WhereOpType::EQUAL; break;
            case hsql::kOpNotEquals: this->Optype = WhereOpType::NOTEQUAL; break;
            case hsql::kOpGreater: this->Optype = WhereOpType::BIGGER; break;
            case hsql::kOpLess: this->Optype = WhereOpType::LOWER; break;
            case hsql::kOpGreaterEq: this->Optype = WhereOpType::BIGGEREQUAL; break;
            case hsql::kOpLessEq: this->Optype = WhereOpType::LOWEREQUAL; break;
            case hsql::kOpAnd: this->Optype = WhereOpType::AND; break;
            case hsql::kOpOr: this->Optype = WhereOpType::OR; break;
            default: this->Optype = WhereOpType::NUL; break;
        }
    }
}

TurboExpr::~TurboExpr() {
    delete left;
    delete right;
}

Value TurboExpr::execute_sup(TurboExpr* this_expr, size_t row_id, const Table& table) {
    if (this_expr->getOpType() == WhereOpType::NUL) {
        return this_expr->get_value();
    }
    if (this_expr->getOpType() == WhereOpType::NAME) {
        int32_t col_id = table.get_column_index(this_expr->get_name());
        if (col_id != -1) {
            size_t chunk_idx = row_id / 64;
            size_t local_row_id = row_id % 64;
            
            const Column& col = table.get_chunks()[chunk_idx].get_column(col_id);
            switch(col.type()){
                case DataType::INT32: return Value(col.get<int32_t>()[local_row_id]);
                case DataType::FLOAT32: return Value(col.get<double>()[local_row_id]);
                case DataType::FIXED_STRING: return Value(std::string(col.get<char[256]>()[local_row_id]));
                default: throw std::logic_error("no way type");
            }
        }
    }
    return Value();
}

bool TurboExpr::execute(TurboExpr* this_expr, size_t row_id, const Table& table) {
    if (!this_expr) return false;

    if (this_expr->getOpType() != WhereOpType::AND && this_expr->getOpType() != WhereOpType::OR && 
        this_expr->getOpType() != WhereOpType::NUL && this_expr->getOpType() != WhereOpType::NAME) {
        
        Value left_value = execute_sup(this_expr->get_left(), row_id, table);
        Value right_value = execute_sup(this_expr->get_right(), row_id, table);

        switch(this_expr->getOpType()) {
            case WhereOpType::EQUAL: return left_value == right_value;
            case WhereOpType::NOTEQUAL: return left_value != right_value;
            case WhereOpType::BIGGER: return left_value > right_value;
            case WhereOpType::LOWER: return left_value < right_value;
            case WhereOpType::BIGGEREQUAL: return left_value >= right_value;
            case WhereOpType::LOWEREQUAL: return left_value <= right_value;
            default: throw std::logic_error("No way operation");
        }
    }
    else if (this_expr->getOpType() == WhereOpType::AND || this_expr->getOpType() == WhereOpType::OR) {
        bool one = execute(this_expr->get_left(), row_id, table);
        bool two = execute(this_expr->get_right(), row_id, table);
        return (this_expr->getOpType() == WhereOpType::AND) ? (one && two) : (one || two);
    }
    return false;
}
