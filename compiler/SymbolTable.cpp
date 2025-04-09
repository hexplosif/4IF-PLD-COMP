#include "SymbolTable.h"
#include <iomanip>

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
        std::cerr << "error: variable '" << name << "' has already declared\n";
        exit(1);
    }
}

Symbol SymbolTable::addGlobalVariable(std::string name, VarType type) {
    if (findVariableThisScope(name) == nullptr) {
        Symbol p = { type , currentDeclOffset, ScopeType::GLOBAL };
        table[name] = p; 
        return p;
    } else {
        std::cerr << "error: variable '" << name << "' has already declared\n";
        exit(1);
    }
}

std::string SymbolTable::addTempVariable(VarType type) {
    currentDeclOffset += 4;
    std::string name = "!tmp" + std::to_string(currentDeclOffset);
    Symbol p = { type, currentDeclOffset, ScopeType::BLOCK };
    table[name] = p;
    return name;
}

std::string SymbolTable::addTempConstVariable(VarType type, int value) {
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
    std::cout << "================== Symbol Table ==================" << std::endl;
    std::cout << "| Name       | Type   | Scope           | Offset |" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
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
                  << " |" << std::endl;
    }
    
    std::cout << "================================================" << std::endl;
}

bool SymbolTable::isTempVariable(std::string name) {
    return name.find("!tmp") != std::string::npos;
}

VarType SymbolTable::getHigherType(VarType type1, VarType type2) {
    std::array<VarType, 2> numberTypeRank = {VarType::CHAR, VarType::INT}; //TODO: move this as a constant / static member
    // char < int < unsigned int < long < unsigned long < long long < unsigned long long < float < double < long double

    int rankType1 = std::find(numberTypeRank.begin(), numberTypeRank.end(), type1) - numberTypeRank.begin();
    int rankType2 = std::find(numberTypeRank.begin(), numberTypeRank.end(), type2) - numberTypeRank.begin();
    if (rankType1 == numberTypeRank.end() - numberTypeRank.begin() || rankType2 == numberTypeRank.end() - numberTypeRank.begin()) {
        std::cerr << "error: can not convert type from " << Symbol::getTypeStr(type1) << " to " << Symbol::getTypeStr(type2) << std::endl;
        exit(1);
    }
    return numberTypeRank[std::max(rankType1, rankType2)];
}

bool SymbolTable::isTypeCompatible(VarType type1, VarType type2) {
    return (type1 == type2 || (type1 == INT && type2 == CHAR) || (type1 == CHAR && type2 == INT));
}