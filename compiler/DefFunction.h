
#pragma once

#include <string>
#include <vector>
#include <map>
#include "symbole.h"

class DefFonction {
    public:
        DefFonction(std::string name, VarType type) : name(name), type(type) {}
        
        std::string getName() { return name; }
        VarType getType() { return type; }
        void setParameters(std::vector<VarType> params) { this->params = params; }
        std::vector<VarType> getParameters() { return params; }
    
    private:
        std::string name;
        VarType type;
        std::vector<VarType> params;
};