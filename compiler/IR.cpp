#include "IR.h"
#include "SymbolTable.h"
#include "type.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
using namespace std;

/* ---------------------- IRInstr ---------------------- */

IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t, std::vector<std::string> params)
    : bb(bb_), op(op), t(t), params(params)
{
}

void IRInstr::gen_asm(std::ostream &o)
{
    // Pour simplifier, on gère ici ldconst, copy, add, sub et mul.
    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = constante
        o << "    movl $" << params[1] << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case copy:
        // copy: params[0] = destination, params[1] = source
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n"; // Stocke le résultat
        break;
    
    case add:
        // add: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    addl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case sub:
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    subl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case mul:
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    imull " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    default:
        o << "    # Opération IR non supportée\n";
        break;
    }
}

/* ---------------------- BasicBlock ---------------------- */

BasicBlock::BasicBlock(CFG *cfg, std::string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr)
{
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, std::vector<std::string> params)
{
    IRInstr *instr = new IRInstr(this, op, t, params);
    instrs.push_back(instr);
}

void BasicBlock::gen_asm(std::ostream &o)
{
    for (auto instr : instrs)
    {
        instr->gen_asm(o);
    }
    // Ici, nous générons un saut inconditionnel vers le bloc suivant s'il existe.
    if (exit_true != nullptr)
    {
        o << "    jmp " << exit_true->label << "\n";
    }
}

/* ---------------------- CFG ---------------------- */

CFG::CFG(DefFonction *ast) : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr)
{
    // Création du bloc d'entrée ("main")
    current_bb = new BasicBlock(this, "main");
    bbs.push_back(current_bb);
}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
}

void CFG::gen_asm(std::ostream &o)
{
    o << ".global main\n";  // Ajoute cette ligne pour rendre main visible
    for (size_t i = 0; i < bbs.size(); i++)
    {
        if (i == 0)
        {
            o << bbs[i]->label << ":\n"; // S'assure que main est bien affiché
            gen_asm_prologue(o);
        }
        bbs[i]->gen_asm(o);
    }
    gen_asm_epilogue(o);
}

std::string CFG::IR_reg_to_asm(std::string reg)
{
    // Utilise la table des symboles pour obtenir l'index et calcule l'offset mémoire.
    int idx = symbolTable.getSymbolIndex(reg);
    if (idx < 0)
    {
        // En cas d'erreur, on renvoie une adresse factice.
        return "0(%rbp)";
    }
    return "-" + std::to_string(idx + 4) + "(%rbp)";
}

void CFG::gen_asm_prologue(std::ostream &o)
{
    o << "    pushq %rbp\n";
    o << "    movq %rsp, %rbp\n";
}

void CFG::gen_asm_epilogue(std::ostream &o)
{
    o << "    popq %rbp\n";
    o << "    ret\n";
}

void CFG::add_to_symbol_table(std::string name, Type t)
{
    symbolTable.addSymbol(name);
}

std::string CFG::create_new_tempvar(Type t)
{
    // Génère un nom unique pour une variable temporaire.
    std::string temp = "t" + std::to_string(nextFreeSymbolIndex);
    nextFreeSymbolIndex++;
    symbolTable.addSymbol(temp);
    return temp;
}

int CFG::get_var_index(std::string name)
{
    return symbolTable.getSymbolIndex(name);
}

std::string CFG::new_BB_name()
{
    return "BB" + std::to_string(nextBBnumber++);
}
