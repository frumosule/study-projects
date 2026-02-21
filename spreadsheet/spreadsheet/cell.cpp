//я попыталась перенести работу с зависимосями сюда из класса самой таблицы, но не получилось (решение падает)
//метод cell:set и sheet::setcell вот так я пыталась сделать, но тут что-то не так, а я не могу понять, что
//void Sheet::SetCell(Position pos, std::string text) {
    //ValidatePosition(pos);
    //EnsureCellExists(pos);
    //cells_[pos]->Set(std::move(text));
    //InvalidateCache();
//}

//void Cell::Set(std::string text) {
    //std::string current_text = GetText();
    //if (text == current_text) {
        //return;
    //}
    
    //auto old_impl = std::move(impl_);
    //std::unordered_set<Position> old_dependencies = dependencies_;
    //std::vector<Position> old_deps_vec(old_dependencies.begin(), old_dependencies.end());
    
    //try {
        //if (text.empty()) {
            //impl_ = std::make_unique<EmptyImpl>();
            //UpdateDependencies({});
        //} else if (text[0] == FORMULA_SIGN && text.size() > 1) {
            // Формульная ячейка
            //std::string formula_expression = text.substr(1);
            //auto formula = ParseFormula(formula_expression);
            //auto new_refs = formula->GetReferencedCells();
            
            //if (CheckCircularDependency(new_refs)) {
                //throw CircularDependencyException("Circular dependency detected");
            //}
            
            //impl_ = std::make_unique<FormulaImpl>(std::move(formula_expression), sheet_);
            //UpdateDependencies(new_refs);
            
        //} else {
            // Текстовая ячейка (обычная или экранированная)
            //impl_ = std::make_unique<TextImpl>(std::move(text));
            //UpdateDependencies({});
        //}
    
        //InvalidateCache();
        
        //for (const auto& dependent : dependents_) {
            //if (CellInterface* dep_cell = sheet_.GetCell(dependent)) {
                //if (auto concrete_cell = dynamic_cast<Cell*>(dep_cell)) {
                    //concrete_cell->InvalidateCache();
                //}
            //}
        //} 
    //} catch (const FormulaException& e) {
        //impl_ = std::move(old_impl);
        //UpdateDependencies(old_deps_vec);
        //throw;
    //} catch (const CircularDependencyException& e) {
        //impl_ = std::move(old_impl);
        //UpdateDependencies(old_deps_vec);
        //throw;
    //} catch (const std::exception& e) {
        //impl_ = std::move(old_impl);
        //UpdateDependencies(old_deps_vec);
        //throw;
    //}
//}

//остальные два замечания исправила 

#include "cell.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <memory>
#include <variant>

// Реализация FormulaImpl
Cell::FormulaImpl::FormulaImpl(std::string expression, SheetInterface& sheet)
    : formula_(ParseFormula(std::move(expression)))
{}

CellInterface::Value Cell::FormulaImpl::GetValue(const SheetInterface& sheet) const {
    auto result = formula_->Evaluate(sheet);
    if (std::holds_alternative<double>(result)) {
        return std::get<double>(result);
    } else {
        return std::get<FormulaError>(result);
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

Cell::Cell(SheetInterface& sheet) 
    : sheet_(sheet)
    , impl_(std::make_unique<EmptyImpl>()) 
    , cached_value_(std::nullopt)
{}

void Cell::Set(std::string text) {
    std::string current_text = GetText();
    if ((text.empty() && current_text.empty()) || 
        (!text.empty() && text == (current_text[0] == ESCAPE_SIGN ? current_text.substr(1) : current_text))) {
        return; 
    }
    
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
        InvalidateCache();
        return;
    }
    
    auto old_impl = std::move(impl_);
    
    try {
        if (text[0] == FORMULA_SIGN && text.size() > 1) {
            std::string formula_expression = text.substr(1);
            impl_ = std::make_unique<FormulaImpl>(std::move(formula_expression), sheet_);
        } else if (text[0] == ESCAPE_SIGN) {
            impl_ = std::make_unique<TextImpl>(std::move(text));
        } else {
            impl_ = std::make_unique<TextImpl>(std::move(text));
        }
        
        InvalidateCache();
        
    } catch (const FormulaException& e) {
        impl_ = std::move(old_impl);
        throw;
    } catch (const std::exception& e) {
        impl_ = std::move(old_impl);
        throw;
    }
}

void Cell::Clear() {
    Set("");
}

CellInterface::Value Cell::GetValue() const {
    if (cached_value_.has_value()) {
        return cached_value_.value();
    }
    
    cached_value_ = impl_->GetValue(sheet_);
    return cached_value_.value();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::InvalidateCache() {
    cached_value_ = std::nullopt;
}

bool Cell::IsCacheValid() const {
    return cached_value_.has_value();
}


