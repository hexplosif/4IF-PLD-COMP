#ifndef SYMBOLE_H
#define SYMBOLE_H

enum VarType {
    VOID,
    INT,
    CHAR
};

enum ScopeType {
    GLOBAL,
    FUNCTION_PARAMS,
    BLOCK
};

class Symbol 
{
    public:
        Symbol() {}
        Symbol(VarType type, int offset, ScopeType scopeType) : type(type), offset(offset), scopeType(scopeType) {}
        Symbol(VarType type, int offset, ScopeType scopeType, int value) : type(type), offset(offset), scopeType(scopeType), value(value) { isCst = true; }

        bool isConstant() { return isCst; }

        bool isUsed() { return used; }

        void markUsed() { used = true; }

        int getCstValue() {    
            if (!isCst) {
                std::cerr << "error: getValue() called on non-constant variable" << std::endl;
                exit(1);
            }
            return value; 
        }

        void info() {
            std::cout << "Symbol: type=" << (type == INT ? "int" : "char") 
                      << ", offset=" << offset 
                      << ", scopeType=" << (scopeType == GLOBAL ? "global" : (scopeType == FUNCTION_PARAMS ? "function_params" : "block")) 
                      << ", isCst=" << isCst 
                      << ", value=" << value 
                      << std::endl;
        }

        VarType type;
        int offset;
        ScopeType scopeType;

        static VarType getType(std::string strType ) {
            if (strType == "int") return VarType::INT;
            if (strType == "char") return VarType::CHAR;
            if (strType == "void") return VarType::VOID;
            std::cerr << "error: unknown type " << strType << std::endl;
            exit(1);
        }

        static std::string getTypeStr(VarType type) {
            switch (type) {
                case VarType::INT: return "int";
                case VarType::CHAR: return "char";
                case VarType::VOID: return "void";
                default: return "unknown";
            }
        }
        
    private:
        bool isCst = false;
        bool used = false;
        int value; // only for constant

};

#endif
