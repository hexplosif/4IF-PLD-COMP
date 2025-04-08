#include <sstream>
#include "IR.h"
using namespace std;

// Génération de code assembleur pour l'instruction pour les machines ARM

#ifdef __arm__

void IRInstr::gen_asm(std::ostream &o)
{
    static int labelCounter = 0;

    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = constante
        o << "    mov r0, " << params[1] << "\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    case copy:
        // copy: params[0] = destination, params[1] = source
        o << "    ldr r0, " << params[1] << "\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    case add:
        // add: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    add r0, r0, r1\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    case sub:
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    sub r0, r0, r1\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    case mul:
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    mul r0, r0, r1\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    case div: // params : dest, source1, source2
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    bl __aeabi_idiv\n"; // Appelle la fonction de division ARM
        o << "    str r0, " << params[0] << "\n";
        break;
    case mod: // params : dest, source1, source2
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    bl __aeabi_idivmod\n"; // Quotient en r0, reste en r1
        o << "    str r1, " << params[0] << "\n"; // Stocke le reste
        break;

    case copyTblx: {
        // copy: params[0] = destination, params[1] = expr, params[2] = position
        o << "    ldr r0, " << params[2] << "\n";
        o << "    ldr r1, " << params[1] << "\n";
        o << "    ldr r2, =" << params[0] << "\n";
        o << "    str r1, [r2, r0, lsl #2]\n"; // Offset = index*4
        break;
    }
    case addTblx: {
        // add: params[0] = destination, params[1] = expr, params[2] = position
        o << "    ldr r0, " << params[2] << "\n";
        o << "    ldr r1, " << params[1] << "\n";
        o << "    ldr r2, =" << params[0] << "\n";
        o << "    ldr r3, [r2, r0, lsl #2]\n"; // Charge valeur actuelle
        o << "    add r3, r3, r1\n"; // Ajoute la valeur
        o << "    str r3, [r2, r0, lsl #2]\n"; // Stocke le résultat
        break;
    }
    case subTblx: {
        // sub: params[0] = destination, params[1] = expr, params[2] = position
        o << "    ldr r0, " << params[2] << "\n";
        o << "    ldr r1, " << params[1] << "\n";
        o << "    ldr r2, =" << params[0] << "\n";
        o << "    ldr r3, [r2, r0, lsl #2]\n"; // Charge valeur actuelle
        o << "    sub r3, r3, r1\n"; // Soustrait la valeur
        o << "    str r3, [r2, r0, lsl #2]\n"; // Stocke le résultat
        break;
    }
    case mulTblx: {
        // mul: params[0] = destination, params[1] = expr, params[2] = position
        o << "    ldr r0, " << params[2] << "\n";
        o << "    ldr r1, " << params[1] << "\n";
        o << "    ldr r2, =" << params[0] << "\n";
        o << "    ldr r3, [r2, r0, lsl #2]\n"; // Charge valeur actuelle
        o << "    mul r3, r3, r1\n"; // Multiplie la valeur
        o << "    str r3, [r2, r0, lsl #2]\n"; // Stocke le résultat
        break;
    }
    case divTblx: {
        // div: params[0] = destination, params[1] = expr, params[2] = position
        o << "    ldr r0, " << params[2] << "\n";
        o << "    ldr r3, =" << params[0] << "\n";
        o << "    ldr r0, [r3, r0, lsl #2]\n"; // Charge valeur actuelle dans r0
        o << "    ldr r1, " << params[1] << "\n";
        o << "    bl __aeabi_idiv\n"; // Division r0/r1 -> r0
        o << "    ldr r1, " << params[2] << "\n";
        o << "    str r0, [r3, r1, lsl #2]\n"; // Stocke le résultat
        break;
    }
    case modTblx: {
        // mod: params[0] = destination, params[1] = expr, params[2] = position
        o << "    ldr r0, " << params[2] << "\n";
        o << "    ldr r3, =" << params[0] << "\n";
        o << "    ldr r0, [r3, r0, lsl #2]\n"; // Charge valeur actuelle dans r0
        o << "    ldr r1, " << params[1] << "\n";
        o << "    bl __aeabi_idivmod\n"; // Division r0/r1 -> r0 (quotient), r1 (reste)
        o << "    ldr r2, " << params[2] << "\n";
        o << "    str r1, [r3, r2, lsl #2]\n"; // Stocke le reste
        break;
    }
    case getTblx: {
        // copy: params[0] = destination, params[1] = tableaux, params[2] = position
        o << "    ldr r0, " << params[2] << "\n";
        o << "    ldr r1, =" << params[1] << "\n";
        o << "    ldr r0, [r1, r0, lsl #2]\n"; // Charge la valeur du tableau
        o << "    str r0, " << params[0] << "\n"; // Stocke dans destination
        break;
    }
    case incr:
        // incr: params[0] = var
        o << "    ldr r0, " << params[0] << "\n";
        o << "    add r0, r0, #1\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case decr:
        // decr: params[0] = var
        o << "    ldr r0, " << params[0] << "\n";
        o << "    sub r0, r0, #1\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case cmp_eq:
        // cmp_eq: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    cmp r0, r1\n";
        o << "    moveq r0, #1\n";
        o << "    movne r0, #0\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    case cmp_lt:
        // cmp_lt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    cmp r0, r1\n";
        o << "    movlt r0, #1\n";
        o << "    movge r0, #0\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    case cmp_le:
        // cmp_le: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    cmp r0, r1\n";
        o << "    movle r0, #1\n";
        o << "    movgt r0, #0\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case cmp_ne:
        // cmp_ne: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    cmp r0, r1\n";
        o << "    movne r0, #1\n";
        o << "    moveq r0, #0\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case cmp_gt:
        // cmp_gt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    cmp r0, r1\n";
        o << "    movgt r0, #1\n";
        o << "    movle r0, #0\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case cmp_ge:
        // cmp_ge: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    cmp r0, r1\n";
        o << "    movge r0, #1\n";
        o << "    movlt r0, #0\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case bit_and:
        // bit_and: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    and r0, r0, r1\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case bit_or:
        // bit_or: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    orr r0, r0, r1\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case bit_xor:
        // bit_xor: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[2] << "\n";
        o << "    eor r0, r0, r1\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case unary_minus:
        // unary_minus: params[0] = dest, params[1] = source
        o << "    ldr r0, " << params[1] << "\n";
        o << "    rsb r0, r0, #0\n"; // Reverse subtract: 0 - r0
        o << "    str r0, " << params[0] << "\n";
        break;

    case not_op:
        // not_op: params[0] = dest, params[1] = source
        o << "    ldr r0, " << params[1] << "\n";
        o << "    cmp r0, #0\n";
        o << "    moveq r0, #1\n";
        o << "    movne r0, #0\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case log_and: {
        int currentLabel = labelCounter++;
        std::string labelFalse = ".Lfalse" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        o << "    ldr r0, " << params[1] << "\n";
        o << "    cmp r0, #0\n";
        o << "    beq " << labelFalse << "\n";

        o << "    ldr r0, " << params[2] << "\n";
        o << "    cmp r0, #0\n";
        o << "    beq " << labelFalse << "\n";

        o << "    mov r0, #1\n";
        o << "    b " << labelEnd << "\n";

        o << labelFalse << ":\n";
        o << "    mov r0, #0\n";

        o << labelEnd << ":\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    }

    case log_or: {
        int currentLabel = labelCounter++;
        std::string labelTrue = ".Ltrue" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        o << "    ldr r0, " << params[1] << "\n";
        o << "    cmp r0, #0\n";
        o << "    bne " << labelTrue << "\n";

        o << "    ldr r0, " << params[2] << "\n";
        o << "    cmp r0, #0\n";
        o << "    bne " << labelTrue << "\n";

        o << "    mov r0, #0\n";
        o << "    b " << labelEnd << "\n";

        o << labelTrue << ":\n";
        o << "    mov r0, #1\n";

        o << labelEnd << ":\n";
        o << "    str r0, " << params[0] << "\n";
        break;
    }

    case rmem:
        // rmem: params[0] = destination, params[1] = adresse
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r0, [r0]\n";
        o << "    str r0, " << params[0] << "\n";
        break;

    case wmem:
        // wmem: params[0] = adresse, params[1] = valeur
        o << "    ldr r0, " << params[1] << "\n";
        o << "    ldr r1, " << params[0] << "\n";
        o << "    str r0, [r1]\n";
        break;

    case call:
        // call: params[0] = label, params[1] = destination, params[2]... = paramètres
        o << "    bl " << params[0] << "\n";
        o << "    str r0, " << params[1] << "\n";
        break;

    case jmp:
        // jmp: params[0] = label
        o << "    b " << params[0] << "\n";
        break;

    default:
        o << "    @ Opération IR non supportée\n";
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
        o << "    ldr r0, " << test_var_register << "\n";
        o << "    cmp r0, #0\n";
        o << "    beq " << exit_false->label << "\n";
        o << "    b " << exit_true->label << "\n";
    }
    else if (exit_true != nullptr)
    {
        // Unconditional jump to exit_true
        if (exit_true->label != cfg->get_epilogue_label())
        { // Already have jmp to end with return statement
            o << "    b " << exit_true->label << "\n";
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
            return "#" + std::to_string(p->getCstValue());
        }

        if (p->scopeType == GLOBAL)
        {
            return "=" + reg;
        }
        return "[fp, #-" + to_string(p->offset) + "]";
    }
    return reg;
}

void CFG::gen_asm_prologue(std::ostream &o)
{
    o << "    push {fp, lr}\n";
    o << "    add fp, sp, #4\n";
    o << "    sub sp, sp, #" << getStackSize() << "\n";
}

void CFG::gen_asm_epilogue(std::ostream &o)
{
    o << "    sub sp, fp, #4\n";
    o << "    pop {fp, pc}\n";
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
        o << "    .global " << var.first << "\n";
        o << var.first << ":\n";
        if (globalVariableValues.count(var.first) == 0)
        {
            o << "    .word 0\n";
        }
        else
        {
            o << "    .word " << globalVariableValues[var.first] << "\n";
        }
    }

    o << "    .text\n";
}

#endif // __arm__