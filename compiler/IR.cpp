#include "IR.h"
#include "SymbolTable.h"
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

void BasicBlock::add_IRInstr(IRInstr::Operation op, VarType t, std::vector<std::string> params)
{
    // General case
    bool needFindAdd = (op == IRInstr::Operation::call || op == IRInstr::Operation::jmp);
    std::string p0 = needFindAdd ? params[0] : cfg->IR_reg_to_asm(params[0]);
    std::string p1 = params.size() >= 2 ? cfg->IR_reg_to_asm(params[1]) : "";
    std::string p2 = params.size() >= 3 ? cfg->IR_reg_to_asm(params[2]) : "";

    // Tableau case
    if (op == IRInstr::Operation::copyTblx || op == IRInstr::Operation::addTblx || op == IRInstr::Operation::subTblx ||
        op == IRInstr::Operation::mulTblx || op == IRInstr::Operation::divTblx || op == IRInstr::Operation::modTblx)
    {
        Symbol *p = cfg->currentScope->findVariable(params[0]);
        p0 = to_string(p->offset);
    }

    if (op == IRInstr::Operation::getTblx)
    {
        Symbol *p = cfg->currentScope->findVariable(params[1]);
        p1 = to_string(p->offset);
    }

    std::vector<std::string> paramsRealAddr = {p0, p1, p2};

    if (op == IRInstr::Operation::copy && CFG::isRegConstant(p1)) {
        op = IRInstr::Operation::ldconst;
    }

    IRInstr *instr = new IRInstr(this, op, t, paramsRealAddr);
    instrs.push_back(instr);
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

void GVM::setGlobalVariableValue(std::string name, int value)
{
    if (globalScope->findVariableThisScope(name) != nullptr)
    {
        globalVariableValues[name] = value;
    }
    else
    {
        std::cerr << "error: variable '" << name << "' not declared\n";
        exit(1);
    }
}

std::string GVM::addTempConstVariable(VarType type, int value)
{
    return globalScope->addTempConstVariable(type, value);
}