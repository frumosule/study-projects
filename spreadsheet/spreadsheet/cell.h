#pragma once

#include "common.h"
#include "formula.h"

#include <memory>
#include <optional>
#include <vector>

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell() = default;

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    
    void InvalidateCache();
    bool IsCacheValid() const;

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual Value GetValue(const SheetInterface& sheet) const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        Value GetValue(const SheetInterface&) const override { return ""; }
        std::string GetText() const override { return ""; }
        std::vector<Position> GetReferencedCells() const override { return {}; }
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text) : text_(std::move(text)) {}
        
        Value GetValue(const SheetInterface&) const override {
            return text_.empty() || text_[0] != ESCAPE_SIGN ? 
                   text_ : text_.substr(1);
        }
        
        std::string GetText() const override { return text_; }
        std::vector<Position> GetReferencedCells() const override { return {}; }
        
    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string expression, SheetInterface& sheet);
        
        Value GetValue(const SheetInterface& sheet) const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        
    private:
        std::unique_ptr<FormulaInterface> formula_;
    };

    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cached_value_;
};
