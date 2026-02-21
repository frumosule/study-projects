#include "sheet.h"
#include "cell.h"
#include "common.h"
#include "formula.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <stack>
#include <queue>

using namespace std::literals;

Sheet::Sheet() = default;

void Sheet::ValidatePosition(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position: " + pos.ToString());
    }
}

void Sheet::EnsureCellExists(Position pos) {
    if (cells_.find(pos) == cells_.end()) {
        cells_[pos] = CellData{std::make_unique<Cell>(*this)};
    }
}

void Sheet::InvalidateCache() {
    printable_size_cache_.reset();
}

void Sheet::RemoveDependencies(Position pos) {
    auto it = cells_.find(pos);
    if (it == cells_.end()) return;
    
    for (const auto& dep_pos : it->second.dependencies) {
        if (auto dep_it = cells_.find(dep_pos); dep_it != cells_.end()) {
            dep_it->second.dependents.erase(pos);
        }
    }
    it->second.dependencies.clear();
}

bool Sheet::CheckCircularDependency(Position pos, const std::vector<Position>& refs) const {
    for (const auto& ref : refs) {
        if (ref == pos) {
            return true;
        }
    }
    
    std::unordered_set<Position, PositionHash, PositionEqual> visited;
    std::unordered_set<Position, PositionHash, PositionEqual> in_stack;
    
    std::function<bool(Position)> dfs = [&](Position current) -> bool {
        if (current == pos) return true; 
        if (in_stack.find(current) != in_stack.end()) return true; 
        if (visited.find(current) != visited.end()) return false; 
        
        visited.insert(current);
        in_stack.insert(current);
        
        auto it = cells_.find(current);
        if (it != cells_.end()) {
            for (const auto& dep : it->second.dependencies) {
                if (dfs(dep)) {
                    return true;
                }
            }
        }
        
        in_stack.erase(current);
        return false;
    };
    
    for (const auto& ref : refs) {
        if (dfs(ref)) {
            return true;
        }
    }
    
    return false;
}

void Sheet::UpdateDependencies(Position pos, const std::vector<Position>& new_refs) {
    RemoveDependencies(pos); 
    
    auto& cell_data = cells_[pos];
    cell_data.dependencies.clear();
    
    for (const auto& ref_pos : new_refs) {
        EnsureCellExists(ref_pos);
        cell_data.dependencies.insert(ref_pos);
        cells_[ref_pos].dependents.insert(pos);
    }
}

void Sheet::InvalidateDependentCaches(Position pos, std::unordered_set<Position, PositionHash, PositionEqual>& visited) {
    std::queue<Position> queue;
    queue.push(pos);
    visited.insert(pos);
    
    while (!queue.empty()) {
        Position current = queue.front();
        queue.pop();
        
        auto it = cells_.find(current);
        if (it != cells_.end()) {
            it->second.cell->InvalidateCache();
            
            for (const auto& dependent : it->second.dependents) {
                if (visited.find(dependent) == visited.end()) {
                    visited.insert(dependent);
                    queue.push(dependent);
                }
            }
        }
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    ValidatePosition(pos);
    
    if (!text.empty() && text[0] == FORMULA_SIGN && text.size() > 1) {
        std::string formula_expression = text.substr(1);
        auto formula = ParseFormula(formula_expression);
        auto new_refs = formula->GetReferencedCells();
        
        if (CheckCircularDependency(pos, new_refs)) {
            throw CircularDependencyException("Circular dependency detected");
        }
    }
    
    std::vector<Position> old_dependencies;
    if (auto it = cells_.find(pos); it != cells_.end()) {
        old_dependencies.assign(it->second.dependencies.begin(), 
                               it->second.dependencies.end());
    }
    
    RemoveDependencies(pos);
    
    try {
        EnsureCellExists(pos);
        auto& cell_data = cells_[pos];
        
        cell_data.cell->Set(std::move(text));
        
        auto new_refs = cell_data.cell->GetReferencedCells();
        UpdateDependencies(pos, new_refs);
        
        std::unordered_set<Position, PositionHash, PositionEqual> visited;
        InvalidateDependentCaches(pos, visited);
        
        InvalidateCache();
        
    } catch (...) {
        UpdateDependencies(pos, old_dependencies);
        throw;
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    ValidatePosition(pos);
    
    auto it = cells_.find(pos);
    return it != cells_.end() ? it->second.cell.get() : nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    ValidatePosition(pos);
    
    auto it = cells_.find(pos);
    return it != cells_.end() ? it->second.cell.get() : nullptr;
}

void Sheet::ClearCell(Position pos) {
    ValidatePosition(pos);
    
    auto it = cells_.find(pos);
    if (it != cells_.end()) {
        RemoveDependencies(pos);
        cells_.erase(it);
        InvalidateCache();
    }
}

Size Sheet::GetPrintableSize() const {
    if (printable_size_cache_.has_value()) {
        return *printable_size_cache_;
    }
    
    int max_row = -1;
    int max_col = -1;
    
    for (const auto& [pos, cell_data] : cells_) {
        if (cell_data.cell && !cell_data.cell->GetText().empty()) {
            max_row = std::max(max_row, pos.row);
            max_col = std::max(max_col, pos.col);
        }
    }
    
    Size result{0, 0};
    if (max_row >= 0 && max_col >= 0) {
        result.rows = max_row + 1;
        result.cols = max_col + 1;
    }
    
    printable_size_cache_ = result;
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    
    for (int row = 0; row < size.rows; ++row) {
        bool need_separator = false;
        
        for (int col = 0; col < size.cols; ++col) {
            if (need_separator) {
                output << '\t';
            }
            need_separator = true;
            
            Position pos{row, col};
            if (auto it = cells_.find(pos); it != cells_.end() && it->second.cell) {
                auto value = it->second.cell->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                } else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                } else {
                    output << std::get<FormulaError>(value).ToString();
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    if (printable_size_cache_.has_value()) {
        Size size = *printable_size_cache_;
        
        for (int row = 0; row < size.rows; ++row) {
            bool need_separator = false;
            
            for (int col = 0; col < size.cols; ++col) {
                if (need_separator) {
                    output << '\t';
                }
                need_separator = true;
                
                Position pos{row, col};
                if (auto it = cells_.find(pos); it != cells_.end() && it->second.cell) {
                    output << it->second.cell->GetText();
                }
            }
            output << '\n';
        }
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
