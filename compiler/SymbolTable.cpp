#include "SymbolTable.h"

SymbolTable::SymbolTable() : nextIndex(0) {}

void SymbolTable::addSymbol(const std::string &name, int size) {
    // On ajoute la variable uniquement si elle n'est pas déjà présente
    if (table.find(name) == table.end()) {
        if (size > 0) {
            // Si la variable est un tableau, on l'ajoute avec un index de taille appropriée
            nextIndex += size * 4; // On incrémente par la taille du tableau (en octets)
            table[name] = nextIndex;
        }
        else {
            table[name] = nextIndex;
            nextIndex += 4; // On incrémente par 4 pour respecter des index multiples de 4 (entiers 32 bits)
        }
    }
}

int SymbolTable::getSymbolIndex(const std::string &name) const {
    auto it = table.find(name);
    if (it != table.end()) {
        return it->second;
    }
    return -1; // Valeur d'erreur si la variable n'est pas trouvée
}
