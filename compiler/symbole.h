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

struct Symbol
{
    VarType type;
    int offset;
    ScopeType scopeType;
};

#endif
