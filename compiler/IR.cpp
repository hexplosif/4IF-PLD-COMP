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

void IRInstr::gen_asm(std::ostream &o)
{
    // Pour simplifier, on gère ici ldconst, copy, add, sub et mul.
    std::string p0 = bb->cfg->IR_reg_to_asm(params[0]);

    std::string p1 = params.size() >= 2 ? bb->cfg->IR_reg_to_asm(params[1]) : "";
    std::string p2 = params.size() >= 3 ? bb->cfg->IR_reg_to_asm(params[2]) : "";

    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = constante
        o << "    movl $" << p1 << ", " << p0 << "\n";
        break;
    case copy:
        // copy: params[0] = destination, params[1] = source
        o << "    movl " << p1 << ", %eax\n";
        o << "    movl %eax, " << p0 << "\n"; // Stocke le résultat
        break;
    case add:
        // add: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    addl " << p2 << ", %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    case sub:
        o << "    movl " << p1 << ", %eax\n";
        o << "    subl " << p2 << ", %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    case mul:
        o << "    movl " << p1 << ", %eax\n";
        o << "    imull " << p2 << ", %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    case div:    // params : dest, source1, source2
        o << "    movl " << p1 << ", %eax\n";
        o << "    cltd\n";
        o << "    idivl " << p2 << "\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    case mod:    // params : dest, source1, source2
        o << "    movl " << p1 << ", %eax\n";
        o << "    cltd\n";
        o << "    idivl " << p2 << "\n";
        o << "    movl %edx, " << p0 << "\n";
        break;  

    case cmp_eq:
        // cmp_eq: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    cmpl " << p2 << ", %eax\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    case cmp_lt:
        // cmp_lt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    cmpl " << p2 << ", %eax\n";
        o << "    setl %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    case cmp_le:
        // cmp_le: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    cmpl " << p2 << ", %eax\n";
        o << "    setle %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    
    case cmp_ne:
        // cmp_ne: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    cmpl " << p2 << ", %eax\n";
        o << "    setne %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    
    case cmp_gt:
        // cmp_gt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    cmpl " << p2 << ", %eax\n";
        o << "    setg %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;

    case cmp_ge:
        // cmp_ge: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    cmpl " << p2 << ", %eax\n";
        o << "    setge %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;

    case bit_and:
        // bit_and: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    andl " << p2 << ", %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;

    case bit_or:
        // bit_or: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    orl " << p2 << ", %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;

    case bit_xor:
        // bit_xor: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << p1 << ", %eax\n";
        o << "    xorl " << p2 << ", %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;
    
    case unary_minus:
        // unary_minus: params[0] = dest, params[1] = source
        o << "    movl " << p1 << ", %eax\n";
        o << "    negl %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;

    case not_op:
        // not_op: params[0] = dest, params[1] = source
        o << "    cmpl $0, " << p1 << "\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        break;

    case rmem:
        // rmem: params[0] = destination, params[1] = adresse
        o << "    movl " << p1 << ", %eax\n";
        o << "    movl (%eax), %eax\n";
        o << "    movl %eax, " << p0 << "\n";
        break;

    case wmem:
        // wmem: params[0] = adresse, params[1] = valeur
        o << "    movl " << p1 << ", %eax\n";
        o << "    movl " << p0 << ", %edx\n";
        o << "    movl %eax, (%edx)\n";
        break;

    case call:
        // call: params[0] = label, params[1] = destination, params[2]... = paramètres
        o << "    call " << params[0] << "\n";
        o << "    movl %eax, " << p1 << "\n";
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

void BasicBlock::add_IRInstr(IRInstr::Operation op, VarType t, std::vector<std::string> params)
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
    } else { // si on est à la fin de cfg (fin de function)
        cfg->gen_asm_epilogue(o);
    }
}

/* ---------------------- CFG ---------------------- */

CFG::CFG(DefFonction *ast) : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr)
{
    this->ast = ast;
    // Création du bloc d'entrée ("main")
    current_bb = new BasicBlock(this, "main");
    bbs.push_back(current_bb);
}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
    if (current_bb != nullptr)
    {
        current_bb->exit_true = bb;
    }
    current_bb = bb;
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
        } else {
            o << bbs[i]->label << ":\n";
        }
        bbs[i]->gen_asm(o);
    }
}

std::string CFG::IR_reg_to_asm(std::string& reg)
{
    // Utilise la table des symboles pour obtenir l'index et calcule l'offset mémoire.
    Symbol* p = currentScope->findVariableThisScope(reg);
    if (p != nullptr)
    {
        return "-" + to_string(p->offset) + "(%rbp)";
    }
    return reg;
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

std::string CFG::new_BB_name()
{
    return "BB" + std::to_string(nextBBnumber++);
}
