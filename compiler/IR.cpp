#include "IR.h"
#include "SymbolTable.h"
#include "FeedbackStyleOutput.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
using namespace std;

/* ---------------------- IRInstr ---------------------- */

IRInstr::IRInstr(BasicBlock *bb_, Operation op, VarType t, std::vector<std::string> params)
    : bb(bb_), op(op), t(t), params(params)
{
}


/* ---------------------- BasicBlock ---------------------- */

BasicBlock::BasicBlock(CFG *cfg, std::string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr)
{
}

/* ---------------------- CFG ---------------------- */

int CFG::nextBBnumber = 0;

CFG::CFG(DefFonction *ast) : ast(ast), current_bb(nullptr)
{
    bbs = std::vector<BasicBlock *>();
}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
}

int CFG::get_var_index(std::string name)
{
    Symbol *s = currentScope->findVariable(name);
    return s->offset;
}

std::string CFG::new_BB_name()
{
    return ".BB" + std::to_string(nextBBnumber++);
}

std::string CFG::get_epilogue_label()
{
    return std::string(".Lepilogue") + "_" + ast->getName();
}

int CFG::getStackSize()
{
    return currentScope->getCurrentDeclOffset() / 16 * 16 + 16; // Round up to the next multiple of 16
}

/* ---------------------- GVM ---------------------- */
GVM::GVM()
{
    globalScope = new SymbolTable(0);
}

void GVM::addGlobalVariable(std::string name, VarType type)
{
    globalScope->addGlobalVariable(name, type);
}

void GVM::setGlobalVariableValue(std::string name, std::string value)
{
    if (globalScope->findVariableThisScope(name) != nullptr)
    {
        globalVariableValues[name] = value;
    }
    else
    {
        FeedbackOutputFormat::showFeedbackOutput("error", "variable '" + name + "' not declared");
        exit(1);
    }
}

std::string GVM::addTempConstVariable(VarType type, string value)
{
    return globalScope->addTempConstVariable(type, value);
}

//* ---------------------- Read only data Manager ---------------------- */
RoDM::RoDM()
{
    floatData = std::map<std::string, float>();   
    labelCounter = 0;
}

std::string RoDM::getNewFloatLabel() {
    std::string label = ".LFD" + std::to_string(labelCounter++);
    return label;
}

std::string RoDM::putFloatIfNotExists(float value)
{
    // Check if the value is already in the map
    for (const auto &pair : floatData)
    {
        if (pair.second == value)
        {
            return pair.first; // Return the existing label
        }
    }

    // If not, create a new label and add it to the map
    std::string label = getNewFloatLabel();
    floatData[label] = value;
    return label; // Return the new label
}

std::string floatToLong_Ieee754(float value) {
    union {
        float d;
        uint32_t i;
    } u;
    u.d = value;

    std::ostringstream oss;
    oss << std::hex << u.i;
    // cout << "converted" << std::hex << u.i;

    return oss.str();
}

std::string RoDM::getLabelDataForUnaryOp() {
    if (needDataForUnaryOp) {
        return labelDataForUnaryOp;
    }

    needDataForUnaryOp = true;
    labelDataForUnaryOp = getNewFloatLabel();
    return labelDataForUnaryOp;
}