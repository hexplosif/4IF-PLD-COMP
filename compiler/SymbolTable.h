#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <map>
#include <string>

class SymbolTable {
public:
    SymbolTable();

    // Ajoute une variable à la table si elle n'existe pas encore
    void addSymbol(const std::string &name);

    // Renvoie l'index associé à une variable
    int getSymbolIndex(const std::string &name) const;

private:
    std::map<std::string, int> table;
    int nextIndex; // Chaque nouvel index sera un multiple de 4
};

#endif
