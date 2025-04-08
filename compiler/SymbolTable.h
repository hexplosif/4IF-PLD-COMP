#pragma once

#include <iostream>
#include <map>
#include "symbole.h"

class SymbolTable
// classe pour la table des symboles
// - chaque SCOPE a sa propre table des symboles
// - chaque table des symboles a un pointeur vers la table des symboles parente (SymbolTable *parent)
//    -> c'est le scope de la block extérieur plus proche
{
    public:
        SymbolTable( int initialOffset );

        Symbol addLocalVariable(std::string name, std::string type, int size = -1); //return offset
        Symbol addGlobalVariable(std::string name, std::string type);
        std::string addTempVariable(std::string type); //return name
        std::string addTempConstVariable(std::string type, int value); //return name

        void freeLastTempVariable(); // free last temp variable


        Symbol* findVariable(std::string name); // find all var can see in the scope
        Symbol* findVariableThisScope(std::string name); //find only var in the scope

        void synchronize(SymbolTable *symbolTable); // synchronise les offsets des variables
        bool isGlobalScope();

        void setParent(SymbolTable *parent) { this->parent = parent; }
        SymbolTable *getParent() { return parent; }
        int getCurrentDeclOffset() { return currentDeclOffset; }
        int getNumberVariable() { return table.size(); }
        std::map<std::string, Symbol> getTable() { return table; }

        void printTable();
        static bool isTempVariable(std::string name);

        void infoVariable(std::string name) {
            Symbol* var = findVariable(name);
            if (var != nullptr) {
                std::cout << "Variable: " << name;
                var->info();
            } else {
                std::cout << "Variable not found: " << name << std::endl;
            }
        }

    private:
        std::map<std::string, Symbol> table;
        int currentDeclOffset = 0;

        SymbolTable *parent = nullptr;

        VarType getType(std::string strType );
};