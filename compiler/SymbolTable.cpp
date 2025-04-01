#include "SymbolTable.h"

SymbolTable::SymbolTable(int initialOffset)
{
    table = std::map<std::string, SymbolParameters>();
    parent = nullptr;
    this->currentDeclOffset = initialOffset;
}

int SymbolTable::addLocalVariable(std::string name, std::string type, int size, bool isArray) 
{
    if (findVariableThisScope(name) == nullptr) 
    {
        if (isArray) 
        {
            int offset = currentDeclOffset - size; 
            SymbolParameters p = {getType(type), offset, SymbolScopeType::BLOCK};
            table[name] = p;
            currentDeclOffset = offset; 
            return offset;
        }
        else
        {
            SymbolParameters p = {getType(type), currentDeclOffset, SymbolScopeType::BLOCK};
            table[name] = p;
            currentDeclOffset += 4;
            return currentDeclOffset - 4;
        }
    } 
    else 
    {
        std::cerr << "error: variable '" << name << "' has already declared" << std::endl;
        exit(1);
    }
}

void SymbolTable::addGlobalVariable(std::string name, std::string type)
{
    if (findVariableThisScope(name) == nullptr)
    {
        SymbolParameters p = {getType(type), currentDeclOffset, SymbolScopeType::GLOBAL};
        table[name] = p;
    }
    else
    {
        std::cerr << "error: variable '" << name << "' has already declared" << std::endl;
        exit(1);
    }
}

SymbolParameters *SymbolTable::findVariable(std::string name)
{
    SymbolTable *current = this;
    while (current != nullptr)
    {
        if (current->table.find(name) != current->table.end())
        {
            return &current->table[name];
        }
        current = current->parent;
    }
    return nullptr;
}

SymbolParameters *SymbolTable::findVariableThisScope(std::string name)
{
    if (table.find(name) != table.end())
    {
        return &table[name];
    }
    return nullptr;
}

void SymbolTable::synchronize(SymbolTable *symbolTable)
{
    currentDeclOffset = symbolTable->currentDeclOffset;
}

bool SymbolTable::isGlobalScope()
{
    return parent == nullptr;
}

SymbolType SymbolTable::getType(std::string strType)
{
    if (strType == "int")
        return SymbolType::INT;
    if (strType == "char")
        return SymbolType::CHAR;
    std::cerr << "error: unknown type " << strType << std::endl;
    exit(1);
}