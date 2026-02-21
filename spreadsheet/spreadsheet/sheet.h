#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PositionHash {
    size_t operator()(const Position& pos) const {
        return std::hash<int>()(pos.row) ^ (std::hash<int>()(pos.col) << 1);
    }
};

struct PositionEqual {
    bool operator()(const Position& lhs, const Position& rhs) const {
        return lhs.row == rhs.row && lhs.col == rhs.col;
    }
};

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;
    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    struct CellData {
        std::unique_ptr<Cell> cell;
        std::unordered_set<Position, PositionHash, PositionEqual> dependents;    // Ячейки, зависящие от этой
        std::unordered_set<Position, PositionHash, PositionEqual> dependencies;  // Ячейки, от которых зависит эта
        
        CellData() : cell(nullptr) {}
        
        CellData(std::unique_ptr<Cell> cell_ptr) 
            : cell(std::move(cell_ptr)) 
        {}
    };

    std::unordered_map<Position, CellData, PositionHash> cells_;
    mutable std::optional<Size> printable_size_cache_;

    void ValidatePosition(Position pos) const;
    void EnsureCellExists(Position pos);
    
    void UpdateDependencies(Position pos, const std::vector<Position>& new_refs);
    void RemoveDependencies(Position pos);
    bool CheckCircularDependency(Position pos, const std::vector<Position>& refs) const;
    
    void InvalidateDependentCaches(Position pos, std::unordered_set<Position, PositionHash, PositionEqual>& visited);
    void InvalidateCache();
};
