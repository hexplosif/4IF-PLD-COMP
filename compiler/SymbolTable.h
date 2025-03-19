#pragma once

#include <iostream>
#include <map>

enum SymbolType
{
    INT,
    CHAR
};

enum SymbolScopeType
{
    GLOBAL,
    FUNCTION_PARAMS,
    BLOCK
};

struct SymbolParameters
{
    SymbolType type;
    int offset;
    SymbolScopeType scopeType;
};

class SymbolTable
// classe pour la table des symboles
// - chaque SCOPE a sa propre table des symboles
// - chaque table des symboles a un pointeur vers la table des symboles parente (SymbolTable *parent)
//    -> c'est le scope de la block extérieur plus proche
{
  public:
    SymbolTable(int initialOffset);
    int addLocalVariable(std::string name, std::string type); // return offset
    void addGlobalVariable(std::string name, std::string type);

    SymbolParameters *findVariable(std::string name);          // find all var can see in the scope
    SymbolParameters *findVariableThisScope(std::string name); // find only var in the scope

    void synchronize(SymbolTable *symbolTable); // synchronise les offsets des variables

    bool isGlobalScope();

    void setParent(SymbolTable *parent)
    {
        this->parent = parent;
    }
    SymbolTable *getParent()
    {
        return parent;
    }
    int getCurrentDeclOffset()
    {
        return currentDeclOffset;
    }

  private:
    std::map<std::string, SymbolParameters> table;
    int currentDeclOffset = 0;

    SymbolTable *parent = nullptr;

    SymbolType getType(std::string strType);
};