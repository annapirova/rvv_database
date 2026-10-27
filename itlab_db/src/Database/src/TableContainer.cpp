#include "TableContainer.h"

Index* TableContainer::get_index(const std::string& col_name, IndexType i_type) {
    for (size_t i = 0;i < indexes_.size();++i) {
        if (indexes_[i].target_column_name == col_name && indexes_[i].index_type == i_type) {
            return &indexes_[i];
        }
    }
    return nullptr; // Индекс не построен
}