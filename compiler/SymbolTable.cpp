#include "SymbolTable.h"

SymbolTable::SymbolTable( int initialOffset ) {
    table = std::map<std::string, Parameters>();
    parent = nullptr;
    this->currentDeclOffset = initialOffset;
}

int SymbolTable::addLocalVariable(std::string name, std::string type) {
    Parameters p = { getType(type) , currentDeclOffset, ScopeType::BLOCK };
    table[name] = p;
    currentDeclOffset += 4;
    return currentDeclOffset - 4;
}

void SymbolTable::addGlobalVariable(std::string name, std::string type) {
    Parameters p = { getType(type) , currentDeclOffset, ScopeType::GLOBAL };
    table[name] = p;
}

Parameters* SymbolTable::findVariable(std::string name) {
    SymbolTable *current = this;
    while (current != nullptr) {
        if (current->table.find(name) != current->table.end()) {
            return &current->table[name];
        }
        current = current->parent;
    }
    return nullptr;
}

Parameters* SymbolTable::findVariableThisScope(std::string name) {
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

VarType SymbolTable::getHigherType(VarType type1, VarType type2) {
    int rankType1 = std::find(typeRank.begin(), typeRank.end(), type1) - typeRank.begin();
    int rankType2 = std::find(typeRank.begin(), typeRank.end(), type2) - typeRank.begin();
    return typeRank[std::max(rankType1, rankType2)];
}