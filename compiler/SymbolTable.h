#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>    // std::find

enum VarType {
    INT,
    CHAR
};

struct Constant {
    int value;
    VarType type;
};

enum ScopeType {
    GLOBAL,
    FUNCTION_PARAMS,
    BLOCK
};

struct Parameters
{
    VarType type;
    int offset;
    ScopeType scopeType;
};

class SymbolTable
// classe pour la table des symboles
// - chaque SCOPE a sa propre table des symboles
// - chaque table des symboles a un pointeur vers la table des symboles parente (SymbolTable *parent)
//    -> c'est le scope de la block extérieur plus proche
{
    public:
        SymbolTable( int initialOffset );
        int addLocalVariable(std::string name, std::string type); //return offset
        void addGlobalVariable(std::string name, std::string type);


        Parameters* findVariable(std::string name); // find all var can see in the scope
        Parameters* findVariableThisScope(std::string name); //find only var in the scope

        void synchronize(SymbolTable *symbolTable); // synchronise les offsets des variables

        bool isGlobalScope();

        void setParent(SymbolTable *parent) { this->parent = parent; }
        SymbolTable *getParent() { return parent; }
        int getCurrentDeclOffset() { return currentDeclOffset; }

        static VarType getHigherType(VarType type1, VarType type2);

    private:
        std::map<std::string, Parameters> table;
        int currentDeclOffset = 0;

        SymbolTable *parent = nullptr;

        VarType getType(std::string strType );
        static constexpr std::array<VarType, 2> typeRank = {VarType::CHAR, VarType::INT};
            // char < int < unsigned int < long < unsigned long < long long < unsigned long long < float < double < long double
};
