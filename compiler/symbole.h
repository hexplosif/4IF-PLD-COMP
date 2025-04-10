#ifndef SYMBOLE_H
#define SYMBOLE_H

enum VarType {
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

        int getCstValue() {    
            if (!isCst) {
                std::cerr << "error: getValue() called on non-constant variable" << std::endl;
                exit(1);
            }
            return value; 
        }

        VarType type;
        int offset;
        ScopeType scopeType;
        
    private:
        bool isCst = false;
        int value; // only for constant

};

#endif
