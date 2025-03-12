#include "SymbolTable.h"

bool SymbolTable::addVariable(const std::string& name) {
    if (table.find(name) != table.end()) {
        std::cerr << "Erreur : Variable '" << name << "' déjà déclarée." << std::endl;
        return false;
    }
    table[name] = nextIndex;
    nextIndex += 4; // Chaque variable occupe 4 octets
    return true;
}

bool SymbolTable::exists(const std::string& name) const {
    return table.find(name) != table.end();
}

int SymbolTable::getIndex(const std::string& name) const {
    if (!exists(name)) {
        std::cerr << "Erreur : Variable '" << name << "' non déclarée." << std::endl;
        return -1;
    }
    return table.at(name);
}

void SymbolTable::print() const {
    std::cout << "Table des symboles :" << std::endl;
    for (const auto& [name, index] : table) {
        std::cout << "  " << name << " -> " << index << std::endl;
    }
}
