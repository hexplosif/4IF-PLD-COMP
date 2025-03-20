#include "SymbolTable.h"

SymbolTable::SymbolTable() : nextIndex(0) {}

void SymbolTable::addSymbol(const std::string &name) {
    // On ajoute la variable uniquement si elle n'est pas déjà présente
    if (table.find(name) == table.end()) {
        table[name] = nextIndex;
        nextIndex += 4; // On incrémente par 4 pour respecter des index multiples de 4 (entiers 32 bits)
    }
}

int SymbolTable::getSymbolIndex(const std::string &name) const {
    auto it = table.find(name);
    if (it != table.end()) {
        return it->second;
    }
    return -1; // Valeur d'erreur si la variable n'est pas trouvée
}
