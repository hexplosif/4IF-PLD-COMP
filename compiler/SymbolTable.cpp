#include "SymbolTable.h"
#include <iomanip>
#include "FeedbackStyleOutput.h"

SymbolTable::SymbolTable( int initialOffset ) {
    table = std::map<std::string, Symbol>();
    parent = nullptr;
    this->currentDeclOffset = initialOffset;
}

Symbol SymbolTable::addLocalVariable(std::string name, VarType type, int size) {
    if (findVariableThisScope(name) == nullptr) {
        if (size > 0) {
            currentDeclOffset += size * 4;
        } else {
            currentDeclOffset += 4;
        }
        Symbol p = { type, currentDeclOffset, ScopeType::BLOCK };
        table[name] = p;
        return p;
    } else {
        FeedbackOutputFormat::showFeedbackOutput("error", "variable '" + name + "' has already declared");
        exit(1);
    }
}

Symbol SymbolTable::addGlobalVariable(std::string name, VarType type) {
    if (findVariableThisScope(name) == nullptr) {
        Symbol p = { type , currentDeclOffset, ScopeType::GLOBAL };
        table[name] = p; 
        return p;
    } else {
        FeedbackOutputFormat::showFeedbackOutput("error", "variable '" + name + "' has already declared");
        exit(1);
    }
}

std::string SymbolTable::addTempVariable(VarType type, int size) {
    if (size > 0) {
        currentDeclOffset += size * 4;
    } else {
        currentDeclOffset += 4;
    }
    currentDeclOffset += 4;
    std::string name = "!tmp" + std::to_string(currentDeclOffset);
    Symbol p = { type, currentDeclOffset, ScopeType::BLOCK };
    table[name] = p;
    return name;
}

std::string SymbolTable::addTempConstVariable(VarType type, std::string value) {
    currentDeclOffset += 4;
    std::string name = "!tmp" + std::to_string(currentDeclOffset);
    Symbol p = { type , currentDeclOffset, ScopeType::BLOCK, value };
    table[name] = p;
    return name;
}

void SymbolTable::freeLastTempVariable() {
    if (currentDeclOffset > 0) {
        std::string name = "!tmp" + std::to_string(currentDeclOffset);
        currentDeclOffset -= 4;

        if (table.find(name) == table.end()) {
            std::cerr << "error: temp variable '" << name << "' not found\n";
            exit(1);
        }
        table.erase(name);
    }
    else {
        std::cerr << "error: no temp variable to free\n";
        exit(1);
    }
}

Symbol* SymbolTable::findVariable(std::string name) {
    SymbolTable *current = this;
    while (current != nullptr) {
        if (current->table.find(name) != current->table.end()) {
            return &current->table[name];
        }
        current = current->parent;
    }
    return nullptr;
}

Symbol* SymbolTable::findVariableThisScope(std::string name) {
    if (table.find(name) != table.end()) {
        return &table[name];
    }
    return nullptr;
}

void SymbolTable::synchronize(SymbolTable *symbolTable) {
    currentDeclOffset = symbolTable->currentDeclOffset;
}

bool SymbolTable::isGlobalScope() {
    return parent == nullptr;
}

void SymbolTable::printTable() {
    std::cout << "======================== Symbol Table ========================" << std::endl;
    std::cout << "| Name       | Type   | Scope           | Offset | Is used?  |" << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    
    for (const auto& entry : table) {
        std::string varName = entry.first;
        Symbol symbol = entry.second;
        
        std::string typeStr = (symbol.type == INT) ? "INT" : "CHAR";
        
        std::string scopeStr;
        switch (symbol.scopeType) {
            case GLOBAL: scopeStr = "GLOBAL"; break;
            case FUNCTION_PARAMS: scopeStr = "FUNCTION_PARAMS"; break;
            case BLOCK: scopeStr = "BLOCK"; break;
        }
        
        std::cout << "| " << std::setw(10) << std::left << varName 
                  << " | " << std::setw(6) << std::left << typeStr
                  << " | " << std::setw(15) << std::left << scopeStr
                  << " | " << std::setw(6) << std::left << symbol.offset 
                  << " | " << std::setw(9) << std:: left << symbol.isUsed()
                  << " |" << std::endl;
    }
    
    std::cout << "==============================================================" << std::endl;
}

bool SymbolTable::isTempVariable(std::string name) {
    return name.find("!tmp") != std::string::npos;
}

void SymbolTable::checkUnusedVariables() {
    for (const auto& entry : table) {
        std::string varName = entry.first;
        Symbol symbol = entry.second;
        
        if (!isTempVariable(varName) && !symbol.isUsed()) {
            FeedbackOutputFormat::showFeedbackOutput("warning", "variable '" + varName + "' is declared but not used");
        }
    }
}