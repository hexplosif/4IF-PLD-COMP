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

struct Parameters
{
    VarType type;
    int offset;
    ScopeType scopeType;
};

class DefFonction {
    public:
        DefFonction(std::string name, VarType type) : name(name), type(type) {}
        std::string name;
        VarType type;
        std::map<std::string, Parameters> params;
};

class Symbol 
{
    public:
        Symbol() {}
        Symbol(VarType type, int offset, ScopeType scopeType) : type(type), offset(offset), scopeType(scopeType) {}
        Symbol(VarType type, int offset, ScopeType scopeType, int value) : type(type), offset(offset), scopeType(scopeType), value(value) { isCst = true; }

        bool isConstant() { return isCst; }

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
        
    private:
        bool isCst = false;
        int value; // only for constant

};

#endif
