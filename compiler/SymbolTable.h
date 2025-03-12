#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <map>
#include <string>
#include <iostream>

class SymbolTable {
public:
    std::map<std::string, int> table; // Associe chaque variable à son index mémoire
    int nextIndex = 0; // Indice mémoire (multiples de 4)

    bool addVariable(const std::string& name);
    bool exists(const std::string& name) const;
    int getIndex(const std::string& name) const;
    void print() const;
};

#endif
