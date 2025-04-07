#include "SymbolTable.h"
#include <iomanip>

SymbolTable::SymbolTable( int initialOffset ) {
    table = std::map<std::string, Symbol>();
    parent = nullptr;
    this->currentDeclOffset = initialOffset;
}

Symbol SymbolTable::addLocalVariable(std::string name, std::string type, int size, bool isArray) {
    if (findVariableThisScope(name) == nullptr) {
        if (isArray) {
            int offset = currentDeclOffset - size; 
            Symbol p = {getType(type), offset, ScopeType::BLOCK};
            table[name] = p;
            currentDeclOffset = offset; 
            return p;
        } else {
            Symbol p = {getType(type), currentDeclOffset, ScopeType::BLOCK};
            table[name] = p;
            currentDeclOffset += 4;
            return p;
        }
    } else {
        std::cerr << "error: variable '" << name << "' has already declared\n";
        exit(1);
    }
}

Symbol SymbolTable::addGlobalVariable(std::string name, std::string type) {
    if (findVariableThisScope(name) == nullptr) {
        Symbol p = { getType(type) , currentDeclOffset, ScopeType::GLOBAL };
        table[name] = p; 
        return p;
    } else {
        std::cerr << "error: variable '" << name << "' has already declared\n";
        exit(1);
    }
}

std::string SymbolTable::addTempVariable(std::string type) {
    std::string name = "!tmp" + std::to_string(currentDeclOffset);
    Symbol p = { getType(type) , currentDeclOffset, ScopeType::BLOCK };
    table[name] = p;
    currentDeclOffset += 4;
    return name;
}

std::string SymbolTable::addTempConstVariable(std::string type, int value) {
    std::string name = "!tmp" + std::to_string(currentDeclOffset);
    Symbol p = { getType(type) , currentDeclOffset, ScopeType::BLOCK, value };
    table[name] = p;
    currentDeclOffset += 4;
    return name;
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

VarType SymbolTable::getType( std::string strType ) {
    if (strType == "int") return VarType::INT;
    if (strType == "char") return VarType::CHAR;
    std::cerr << "error: unknown type " << strType << std::endl;
    exit(1);
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