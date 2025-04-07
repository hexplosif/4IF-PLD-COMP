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
    static int labelCounter = 0;
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
        // sub: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    subl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case mul:
        // mul: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    imull " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case div:
        // div: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cltd\n";
        o << "    idivl " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case mod:
        // mod: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cltd\n";
        o << "    idivl " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
        o << "    movl %edx, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case copyTblx:
    {
        // copy: params[0] = destination, params[1] = expr, params[2] = position
        int idxxx = bb->cfg->get_var_index(params[0]);
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (idxxx) << "(%rbp, %rbx, 4), %rax\n"; // Correct displacement and scaling
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n";
        o << "    movl %edx, (%rax)\n";
        break;
    }
    case addTblx: 
    {
        // add: params[0] = destination, params[1] = expr, params[2] = position
        int idxxx = bb->cfg->get_var_index(params[0]);
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n"; 
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (idxxx) << "(%rbp, %rbx, 4), %rax\n"; 
        o << "    movl (%rax), %edx\n"; 
        o << "    addl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n"; //Ajoute la valeur 
        o << "    movl %edx, (%rax)\n"; 
        break;
    }
    case subTblx:
    {
        // sub: params[0] = destination, params[1] = expr, params[2] = position
        int idxxx = bb->cfg->get_var_index(params[0]);
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n"; 
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (idxxx) << "(%rbp, %rbx, 4), %rax\n"; 
        o << "    movl (%rax), %edx\n"; 
        o << "    subl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n"; //Sub la valeur 
        o << "    movl %edx, (%rax)\n"; 
        break;
    }
    case mulTblx:
    {
        // mul: params[0] = destination, params[1] = expr, params[2] = position
        int idxxx = bb->cfg->get_var_index(params[0]);
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n"; 
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (idxxx) << "(%rbp, %rbx, 4), %rax\n"; 
        o << "    movl (%rax), %edx\n"; 
        o << "    imull " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n"; //Mul la valeur 
        o << "    movl %edx, (%rax)\n"; 
        break;
    }
    case divTblx:
    {
        // div: params[0] = destination, params[1] = expr, params[2] = position
        int idxxx = bb->cfg->get_var_index(params[0]);
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n"; 
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (idxxx) << "(%rbp, %rbx, 4), %rcx\n"; 
        o << "    movl (%rcx), %eax\n";
        o << "    cltd\n";
        o << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n"; //Div la valeur
        o << "    movl %eax, (%rcx)\n";
        break;
    }
    case modTblx:
    {
        // mod: params[0] = destination, params[1] = expr, params[2] = position
        int idxxx = bb->cfg->get_var_index(params[0]);
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n"; 
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (idxxx) << "(%rbp, %rbx, 4), %rcx\n"; 
        o << "    movl (%rcx), %eax\n";
        o << "    cltd\n";
        o << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n"; //Mod la valeur
        o << "    movl %edx, (%rcx)\n";
        break;
    }
    case getTblx:{
        // copy: params[0] = destination, params[1] = tableaux, params[2] = position
        int idxxx = bb->cfg->get_var_index(params[1]);
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (idxxx) << "(%rbp, %rbx, 4), %rax\n"; // Correct displacement and scaling
        o << "    movl (%rax), %edx\n";
        o << "    movl %edx, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
    break;}
    case incr:
        // incr: params[0] = var
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", %eax\n";
        o << "    addl $1, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case decr:
        // decr: params[0] = var
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", %eax\n";
        o << "    subl $1, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case cmp_eq:
        // cmp_eq: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_lt:
        // cmp_lt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    setl %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_le:
        // cmp_le: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    setle %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case cmp_ne:
        // cmp_ne: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    setne %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case cmp_gt:
        // cmp_gt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    setg %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case cmp_ge:
        // cmp_ge: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    setge %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case bit_and:
        // bit_and: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    andl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case bit_or:
        // bit_or: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    orl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case bit_xor:
        // bit_xor: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    xorl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case unary_minus:
        // unary_minus: params[0] = dest, params[1] = source
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    negl %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case not_op:
        // not_op: params[0] = dest, params[1] = source
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl $0, %eax\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case log_and:
    {
        int currentLabel = labelCounter++;
        std::string labelFalse = ".Lfalse" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jz " << labelFalse << "\n";

        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jz " << labelFalse << "\n";

        o << "    movl $1, %eax\n";
        o << "    jmp " << labelEnd << "\n";

        o << labelFalse << ":\n";
        o << "    movl $0, %eax\n";

        o << labelEnd << ":\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    }
    case log_or:
    {
        int currentLabel = labelCounter++;
        std::string labelTrue = ".Ltrue" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
    
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jnz " << labelTrue << "\n";
    
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jnz " << labelTrue << "\n";
    
        o << "    movl $0, %eax\n";
        o << "    jmp " << labelEnd << "\n";
    
        o << labelTrue << ":\n";
        o << "    movl $1, %eax\n";
    
        o << labelEnd << ":\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    }
     
    case rmem:
        // rmem: params[0] = destination, params[1] = adresse
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    movl (%eax), %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;

    case wmem:
        // wmem: params[0] = adresse, params[1] = valeur
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", %edx\n";
        o << "    movl %eax, (%edx)\n";
        break;

    case call:
        // call: params[0] = label, params[1] = destination, params[2]... = paramètres
        o << "    call " << params[0] << "\n";
        break;

    case jmp:
        // jmp: params[0] = label
        o << "    jmp " << params[0] << "\n";
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

    if (!test_var_name.empty() && exit_true != nullptr && exit_false != nullptr)
    {
        // Conditional jump based on test_var_name
        o << "    movl " << cfg->IR_reg_to_asm(test_var_name) << ", %eax\n";
        o << "    cmpl $0, %eax\n";
        o << "    je " << exit_false->label << "\n";
        o << "    jmp " << exit_true->label << "\n";
    }
    else if (exit_true != nullptr)
    {
        // Unconditional jump to exit_true
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
    o << ".global main\n";
    for (size_t i = 0; i < bbs.size(); i++)
    {
        o << bbs[i]->label << ":\n"; // Emit label for every block
        if (i == 0)
        {
            gen_asm_prologue(o);
        }
        bbs[i]->gen_asm(o);
    }
    o << ".Lepilogue:\n";
    gen_asm_epilogue(o);
}


std::string CFG::IR_reg_to_asm(std::string reg)
{
    // Si le nom commence par '%' on considère que c'est un registre explicite et on le renvoie tel quel.
    if (!reg.empty() && reg[0] == '%')
        return reg;
        
    // Sinon, utilise la table des symboles pour obtenir l'index et calcule l'offset mémoire.
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

void CFG::add_to_symbol_table(std::string name, Type t, int size)
{
    symbolTable.addSymbol(name, size);
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
