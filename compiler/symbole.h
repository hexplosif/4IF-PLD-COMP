#ifndef SYMBOLE_H
#define SYMBOLE_H

#include <array>
#include <algorithm>

enum VarType {
    VOID,
    INT,
    FLOAT,
    CHAR,

    INT_PTR, //TODO: j'ai ajouté ça pour les tableaux, mais j'ai rien fait, il faut que qui est responsable pour le pointer le fasse
    CHAR_PTR,
};

enum ScopeType {
    GLOBAL,
    FUNCTION_PARAMS,
    BLOCK
};

class Symbol 
{
    public:
        VarType type;
        int offset;
        ScopeType scopeType;
        std::string label; // for label of float constant value

        // Constructors

        Symbol() {}
        Symbol(VarType type, int offset, ScopeType scopeType) : type(type), offset(offset), scopeType(scopeType) {}
        Symbol(VarType type, int offset, ScopeType scopeType, std::string value) : type(type), offset(offset), scopeType(scopeType), value(value) { isCst = true; }

        // Ultility functions
        bool isUsed() { return used; }
        void markUsed() { used = true; }
        bool isConstant() { return isCst; }

        std::string getCstValue() {    
            if (!isCst) {
                std::cerr << "error: getValue() called on non-constant variable" << std::endl;
                exit(1);
            }
            return value; 
        }

        VarType getType() { return type; }

        // Static functions: Type

        static VarType getTypeFromStr(std::string strType ) {
            if (strType == "int") return VarType::INT;
            if (strType == "char") return VarType::CHAR;
            if (strType == "void") return VarType::VOID;
            if (strType == "float") return VarType::FLOAT;
            if (strType == "int*") return VarType::INT_PTR;
            if (strType == "char*") return VarType::CHAR_PTR;
            std::cerr << "error: unknown type " << strType << std::endl;
            exit(1);
        }

        static std::string getTypeStr(VarType type) {
            switch (type) {
                case VarType::INT: return "int";
                case VarType::CHAR: return "char";
                case VarType::VOID: return "void";
                case VarType::FLOAT: return "float";
                case VarType::INT_PTR: return "int*";
                case VarType::CHAR_PTR: return "char*";
                default: return "unknown";
            }
        }

        static bool isIntegerType(VarType type) {
            return (type == VarType::INT || type == VarType::CHAR);
        }

        static bool isFloatingType(VarType type) {
            return (type == VarType::FLOAT);
        }

        static VarType getHigherType(VarType type1, VarType type2) {
            std::array<VarType, 3> numberTypeRank = {VarType::CHAR, VarType::INT, VarType::FLOAT}; //TODO: move this as a constant / static member
            // char < int < unsigned int < long < unsigned long < long long < unsigned long long < float < double < long double
        
            int rankType1 = std::find(numberTypeRank.begin(), numberTypeRank.end(), type1) - numberTypeRank.begin();
            int rankType2 = std::find(numberTypeRank.begin(), numberTypeRank.end(), type2) - numberTypeRank.begin();
            if (rankType1 == numberTypeRank.end() - numberTypeRank.begin() || rankType2 == numberTypeRank.end() - numberTypeRank.begin()) {
                std::cerr << "error: can not convert type from " << Symbol::getTypeStr(type1) << " to " << Symbol::getTypeStr(type2) << std::endl;
                exit(1);
            }
            return numberTypeRank[std::max(rankType1, rankType2)];
        }

        // Debug

        void info() {
            std::cout << "Symbol: type=" << (type == INT ? "int" : "char") 
                      << ", offset=" << offset 
                      << ", scopeType=" << (scopeType == GLOBAL ? "global" : (scopeType == FUNCTION_PARAMS ? "function_params" : "block")) 
                      << ", isCst=" << isCst 
                      << ", value=" << value 
                      << std::endl;
        }
        
    private:
        bool isCst = false;
        bool used = false;
        std::string value; // only for constant

};

#endif
