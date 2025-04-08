#include <sstream>
#include "IR.h"
using namespace std;

// Génération de code assembleur pour l'instruction for x86 machine

#ifdef __x86_64__

void IRInstr::gen_asm(std::ostream &o)
{
    static int labelCounter = 0;
    // Pour simplifier, on gère ici ldconst, copy, add, sub et mul.

    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = constante
        o << "    movl " << params[1] << ", " << params[0] << "\n";
        break;
    case copy:
        // copy: params[0] = destination, params[1] = source
        o << "    movl " << params[1] << ", %eax\n";
        if (params[0] != "%eax")
            o << "    movl %eax, " << params[0] << "\n"; // Stocke le résultat
        break;
    case add:
        // add: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    addl " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case sub:
        o << "    movl " << params[1] << ", %eax\n";
        o << "    subl " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case mul:
        o << "    movl " << params[1] << ", %eax\n";
        o << "    imull " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case div: // params : dest, source1, source2
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cltd\n";
        o << "    idivl " << params[2] << "\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case mod: // params : dest, source1, source2
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cltd\n";
        o << "    idivl " << params[2] << "\n";
        o << "    movl %edx, " << params[0] << "\n";
        break;

    case copyTblx: {
        // copy: params[0] = destination, params[1] = expr, params[2] = position
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n";                                // Sign extend to 64-bit
        o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n"; // Correct displacement and scaling
        o << "    movl " << params[1] << ", %edx\n";
        o << "    movl %edx, (%rax)\n";
        break;
    }
    case addTblx: {
        // add: params[0] = destination, params[1] = expr, params[2] = position
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n";
        o << "    movl (%rax), %edx\n";
        o << "    addl " << params[1] << ", %edx\n"; // Ajoute la valeur
        o << "    movl %edx, (%rax)\n";
        break;
    }
    case subTblx: {
        // sub: params[0] = destination, params[1] = expr, params[2] = position
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n";
        o << "    movl (%rax), %edx\n";
        o << "    subl " << params[1] << ", %edx\n"; // Sub la valeur
        o << "    movl %edx, (%rax)\n";
        break;
    }
    case mulTblx: {
        // mul: params[0] = destination, params[1] = expr, params[2] = position
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << params[0] << "(%rbp, %rbx, 4), %rax\n";
        o << "    movl (%rax), %edx\n";
        o << "    imull " << params[1] << ", %edx\n"; // Mul la valeur
        o << "    movl %edx, (%rax)\n";
        break;
    }
    case divTblx: {
        // div: params[0] = destination, params[1] = expr, params[2] = position
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << params[0] << "(%rbp, %rbx, 4), %rcx\n";
        o << "    movl (%rcx), %eax\n";
        o << "    cltd\n";
        o << "    idivl " << params[1] << "\n"; // Div la valeur
        o << "    movl %eax, (%rcx)\n";
        break;
    }
    case modTblx: {
        // mod: params[0] = destination, params[1] = expr, params[2] = position
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
        o << "    leaq -" << params[0] << "(%rbp, %rbx, 4), %rcx\n";
        o << "    movl (%rcx), %eax\n";
        o << "    cltd\n";
        o << "    idivl " << params[1] << "\n"; // Mod la valeur
        o << "    movl %edx, (%rcx)\n";
        break;
    }
    case getTblx: {
        // copy: params[0] = destination, params[1] = tableaux, params[2] = position
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n";                                // Sign extend to 64-bit
        o << "    leaq -" << (params[1]) << "(%rbp, %rbx, 4), %rax\n"; // Correct displacement and scaling
        o << "    movl (%rax), %edx\n";
        o << "    movl %edx, " << params[0] << "\n";
        break;
    }
    case incr:
        // incr: params[0] = var
        o << "    movl " << params[0] << ", %eax\n";
        o << "    addl $1, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case decr:
        // decr: params[0] = var
        o << "    movl " << params[0] << ", %eax\n";
        o << "    subl $1, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_eq:
        // cmp_eq: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case cmp_lt:
        // cmp_lt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setl %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case cmp_le:
        // cmp_le: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setle %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_ne:
        // cmp_ne: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setne %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_gt:
        // cmp_gt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setg %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_ge:
        // cmp_ge: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setge %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case bit_and:
        // bit_and: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    andl " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case bit_or:
        // bit_or: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    orl " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case bit_xor:
        // bit_xor: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    movl " << params[1] << ", %eax\n";
        o << "    xorl " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case unary_minus:
        // unary_minus: params[0] = dest, params[1] = source
        o << "    movl " << params[1] << ", %eax\n";
        o << "    negl %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case not_op:
        // not_op: params[0] = dest, params[1] = source
        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl $0, %eax\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case log_and: {
        int currentLabel = labelCounter++;
        std::string labelFalse = ".Lfalse" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        o << "    movl " << params[1] << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jz " << labelFalse << "\n";

        o << "    movl " << params[2] << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jz " << labelFalse << "\n";

        o << "    movl $1, %eax\n";
        o << "    jmp " << labelEnd << "\n";

        o << labelFalse << ":\n";
        o << "    movl $0, %eax\n";

        o << labelEnd << ":\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    }
    case log_or: {
        int currentLabel = labelCounter++;
        std::string labelTrue = ".Ltrue" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        o << "    movl " << params[1] << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jnz " << labelTrue << "\n";

        o << "    movl " << params[2] << ", %eax\n";
        o << "    testl %eax, %eax\n";
        o << "    jnz " << labelTrue << "\n";

        o << "    movl $0, %eax\n";
        o << "    jmp " << labelEnd << "\n";

        o << labelTrue << ":\n";
        o << "    movl $1, %eax\n";

        o << labelEnd << ":\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    }

    case rmem:
        // rmem: params[0] = destination, params[1] = adresse
        o << "    movl " << params[1] << ", %eax\n";
        o << "    movl (%eax), %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case wmem:
        // wmem: params[0] = adresse, params[1] = valeur
        o << "    movl " << params[1] << ", %eax\n";
        o << "    movl " << params[0] << ", %edx\n";
        o << "    movl %eax, (%edx)\n";
        break;

    case call:
        // call: params[0] = label, params[1] = destination, params[2]... = paramètres
        o << "    call " << params[0] << "\n";
        o << "    movl %eax, " << params[1] << "\n";
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

//* ---------------------- BasicBlock ---------------------- */

void BasicBlock::gen_asm(std::ostream &o)
{
    for (auto instr : instrs)
    {
        instr->gen_asm(o);
    }

    if (!test_var_name.empty() && exit_true != nullptr && exit_false != nullptr)
    {
        // Conditional jump based on test_var_name
        o << "    movl " << test_var_register << ", %eax\n";
        o << "    cmpl $0, %eax\n";
        o << "    je " << exit_false->label << "\n";
        o << "    jmp " << exit_true->label << "\n";
    }
    else if (exit_true != nullptr)
    {
        // Unconditional jump to exit_true
        if (exit_true->label != cfg->get_epilogue_label())
        { // Already have jmp to end with return statement
            o << "    jmp " << exit_true->label << "\n";
        }
    }
    else
    { // si on est à la fin de cfg (fin de function)
        cfg->gen_asm_epilogue(o);
    }
}

//* ---------------------- CFG ---------------------- */

void CFG::gen_asm(std::ostream &o)
{
    o << ".global " << ast->getName() << "\n";
    for (size_t i = 0; i < bbs.size(); i++)
    {
        o << bbs[i]->label << ":\n";
        if (i == 0)
        {
            gen_asm_prologue(o);
        }
        bbs[i]->gen_asm(o);
    }
}

std::string CFG::IR_reg_to_asm(std::string &reg, bool ignoreCst)
{
    // Utilise la table des symboles pour obtenir l'index et calcule l'offset mémoire.
    Symbol *p = currentScope->findVariable(reg);
    if (p != nullptr)
    {
        if (p->isConstant() & !ignoreCst) {
            return "$" + std::to_string(p->getCstValue());
        }

        if (p->scopeType == GLOBAL)
        {
            return reg + "(%rip)";
        }
        return "-" + to_string(p->offset) + "(%rbp)";
    }
    return reg;
}

void CFG::gen_asm_prologue(std::ostream &o)
{
    o << "    pushq %rbp\n";
    o << "    movq %rsp, %rbp\n";
    o << "    subq $" << getStackSize() << ", %rsp\n";
}

void CFG::gen_asm_epilogue(std::ostream &o)
{
    o << "    leave\n";
    o << "    ret\n";
}

bool CFG::isRegConstant(std::string& reg)
{
    return reg[0] == '$';
}

//* ---------------------- GlobalVarManager ---------------------- */
void GVM::gen_asm(std::ostream &o)
{
    // Verifier si on a des variables globales
    if (globalScope->getNumberVariable() == 0)
        return;

    o << "    .data\n";

    // On génère le code assembleur pour les variables globales
    for (auto var : globalScope->getTable())
    {
        // check if var is temp variable
        if (SymbolTable::isTempVariable(var.first))
            continue;
        o << "    .globl " << var.first << "\n";
        o << var.first << ":\n";
        if (globalVariableValues.count(var.first) == 0)
        {
            o << "    .zero 4\n";
        }
        else
        {
            o << "    .long " << globalVariableValues[var.first] << "\n";
        }
    }

    o << "    .text\n";
}

#endif // __x86_64__