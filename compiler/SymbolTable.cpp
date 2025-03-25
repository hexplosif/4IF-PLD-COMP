#include "SymbolTable.h"

SymbolTable::SymbolTable( int initialOffset ) {
    table = std::map<std::string, Symbol>();
    parent = nullptr;
    this->currentDeclOffset = initialOffset;
}

Symbol SymbolTable::addLocalVariable(std::string name, std::string type) {
    if (findVariableThisScope(name) == nullptr) {
        Symbol p = { getType(type) , currentDeclOffset, ScopeType::BLOCK };
        table[name] = p;
        currentDeclOffset += 4;
        return p;
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