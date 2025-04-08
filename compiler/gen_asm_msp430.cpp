#include <sstream>
#include "IR.h"
using namespace std;

// Génération de code assembleur pour l'instruction pour les machines MSP430

#ifdef __MSP430__

void IRInstr::gen_asm(std::ostream &o)
{
    static int labelCounter = 0;

    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = constante
        o << "    mov " << params[1] << ", " << params[0] << "\n";
        break;
    case copy:
        // copy: params[0] = destination, params[1] = source
        o << "    mov " << params[1] << ", r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;
    case add:
        // add: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    add " << params[2] << ", r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;
    case sub:
        o << "    mov " << params[1] << ", r15\n";
        o << "    sub " << params[2] << ", r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;
    case mul:
        // MSP430 n'a pas d'instruction de multiplication native dans tous les modèles
        // On utilise un appel à une routine de multiplication
        o << "    mov " << params[1] << ", r15\n";
        o << "    mov " << params[2] << ", r14\n";
        o << "    call #__mspabi_mpyi\n"; // Résultat dans r15
        o << "    mov r15, " << params[0] << "\n";
        break;
    case div: // params : dest, source1, source2
        o << "    mov " << params[1] << ", r15\n";
        o << "    mov " << params[2] << ", r14\n";
        o << "    call #__mspabi_divi\n"; // Division signée
        o << "    mov r15, " << params[0] << "\n";
        break;
    case mod: // params : dest, source1, source2
        o << "    mov " << params[1] << ", r15\n";
        o << "    mov " << params[2] << ", r14\n";
        o << "    call #__mspabi_remi\n"; // Modulo signé
        o << "    mov r15, " << params[0] << "\n";
        break;

    case copyTblx: {
        // copy: params[0] = destination, params[1] = expr, params[2] = position
        o << "    mov " << params[2] << ", r14\n";
        o << "    rla r14\n"; // Multiplie par 2 (2 octets par int sur MSP430)
        o << "    mov " << params[1] << ", r15\n";
        o << "    mov r15, " << params[0] << "(r14)\n"; // destination(r14) accède à l'élément
        break;
    }
    case addTblx: {
        // add: params[0] = destination, params[1] = expr, params[2] = position
        o << "    mov " << params[2] << ", r14\n";
        o << "    rla r14\n"; // Multiplie par 2
        o << "    mov " << params[1] << ", r15\n";
        o << "    add " << params[0] << "(r14), r15\n"; // Ajoute la valeur courante
        o << "    mov r15, " << params[0] << "(r14)\n"; // Stocke le résultat
        break;
    }
    case subTblx: {
        // sub: params[0] = destination, params[1] = expr, params[2] = position
        o << "    mov " << params[2] << ", r14\n";
        o << "    rla r14\n"; // Multiplie par 2
        o << "    mov " << params[0] << "(r14), r15\n"; // Charge valeur actuelle
        o << "    sub " << params[1] << ", r15\n"; // Soustrait la valeur
        o << "    mov r15, " << params[0] << "(r14)\n"; // Stocke le résultat
        break;
    }
    case mulTblx: {
        // mul: params[0] = destination, params[1] = expr, params[2] = position
        o << "    mov " << params[2] << ", r14\n";
        o << "    rla r14\n"; // Multiplie par 2
        o << "    mov " << params[0] << "(r14), r15\n"; // Charge valeur actuelle
        o << "    mov " << params[1] << ", r14\n";
        o << "    call #__mspabi_mpyi\n"; // r15 * r14 -> r15
        o << "    mov " << params[2] << ", r14\n";
        o << "    rla r14\n"; // Multiplie par 2
        o << "    mov r15, " << params[0] << "(r14)\n"; // Stocke le résultat
        break;
    }
    case divTblx: {
        // div: params[0] = destination, params[1] = expr, params[2] = position
        o << "    mov " << params[2] << ", r13\n";
        o << "    rla r13\n"; // Multiplie par 2
        o << "    mov " << params[0] << "(r13), r15\n"; // Charge valeur actuelle dans r15
        o << "    mov " << params[1] << ", r14\n";
        o << "    call #__mspabi_divi\n"; // Division r15/r14 -> r15
        o << "    mov r13, r14\n"; // Recharge index*2 dans r14
        o << "    mov r15, " << params[0] << "(r14)\n"; // Stocke le résultat
        break;
    }
    case modTblx: {
        // mod: params[0] = destination, params[1] = expr, params[2] = position
        o << "    mov " << params[2] << ", r13\n";
        o << "    rla r13\n"; // Multiplie par 2
        o << "    mov " << params[0] << "(r13), r15\n"; // Charge valeur actuelle dans r15
        o << "    mov " << params[1] << ", r14\n";
        o << "    call #__mspabi_remi\n"; // Modulo r15/r14 -> r15
        o << "    mov r13, r14\n"; // Recharge index*2 dans r14
        o << "    mov r15, " << params[0] << "(r14)\n"; // Stocke le résultat
        break;
    }
    case getTblx: {
        // copy: params[0] = destination, params[1] = tableaux, params[2] = position
        o << "    mov " << params[2] << ", r14\n";
        o << "    rla r14\n"; // Multiplie par 2
        o << "    mov " << params[1] << "(r14), r15\n"; // Charge la valeur du tableau
        o << "    mov r15, " << params[0] << "\n"; // Stocke dans destination
        break;
    }
    case incr:
        // incr: params[0] = var
        o << "    mov " << params[0] << ", r15\n";
        o << "    inc r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case decr:
        // decr: params[0] = var
        o << "    mov " << params[0] << ", r15\n";
        o << "    dec r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case cmp_eq:
        // cmp_eq: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    cmp " << params[2] << ", r15\n";
        o << "    jeq $+6\n";           // Saute 3 instructions (6 octets) si égal
        o << "    mov #0, r15\n";       // Si inégal, met 0
        o << "    jmp $+4\n";           // Saute 2 instructions (4 octets)
        o << "    mov #1, r15\n";       // Si égal, met 1
        o << "    mov r15, " << params[0] << "\n";
        break;

    case cmp_lt:
        // cmp_lt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    cmp " << params[2] << ", r15\n";
        o << "    jl $+6\n";            // Saute si inférieur
        o << "    mov #0, r15\n";
        o << "    jmp $+4\n";
        o << "    mov #1, r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case cmp_le:
        // cmp_le: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    cmp " << params[2] << ", r15\n";
        o << "    jle $+6\n";           // Saute si inférieur ou égal
        o << "    mov #0, r15\n";
        o << "    jmp $+4\n";
        o << "    mov #1, r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case cmp_ne:
        // cmp_ne: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    cmp " << params[2] << ", r15\n";
        o << "    jne $+6\n";           // Saute si non égal
        o << "    mov #0, r15\n";
        o << "    jmp $+4\n";
        o << "    mov #1, r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case cmp_gt:
        // cmp_gt: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[2] << ", r14\n"; // Inversion pour utiliser jl
        o << "    mov " << params[1] << ", r15\n";
        o << "    cmp r14, r15\n";      // r15 > r14 ?
        o << "    jl $+6\n";            // Saute si inférieur (ce qui veut dire que params[1] > params[2])
        o << "    mov #0, r15\n";
        o << "    jmp $+4\n";
        o << "    mov #1, r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case cmp_ge:
        // cmp_ge: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[2] << ", r14\n"; // Inversion pour utiliser jle
        o << "    mov " << params[1] << ", r15\n";
        o << "    cmp r14, r15\n";      // r15 >= r14 ?
        o << "    jle $+6\n";           // Saute si inférieur ou égal
        o << "    mov #0, r15\n";
        o << "    jmp $+4\n";
        o << "    mov #1, r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case bit_and:
        // bit_and: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    and " << params[2] << ", r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case bit_or:
        // bit_or: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    bis " << params[2] << ", r15\n"; // MSP430 utilise BIS pour OR
        o << "    mov r15, " << params[0] << "\n";
        break;

    case bit_xor:
        // bit_xor: params[0] = dest, params[1] = gauche, params[2] = droite
        o << "    mov " << params[1] << ", r15\n";
        o << "    xor " << params[2] << ", r15\n";
        o << "    mov r15, " << params[0] << "\n";
        break;

    case unary_minus:
        // unary_minus: params[0] = dest, params[1] = source
        o << "    mov " << params[1] << ", r15\n";
        o << "    inv r15\n";          // Inversion de bits
        o << "    inc r15\n";          // Complément à 2
        o << "    mov r15, " << params[0] << "\n";
        break;

    case not_op:
        // not_op: params[0] = dest, params[1] = source
        o << "    mov " << params[1] << ", r15\n";
        o << "    tst r15\n";
        o << "    jeq $+6\n";          // Si zéro, saute
        o << "    mov #0, r15\n";      // Si non-zéro, met 0
        o << "    jmp $+4\n";
        o << "    mov #1, r15\n";      // Si zéro, met 1
        o << "    mov r15, " << params[0] << "\n";
        break;

    case log_and: {
        int currentLabel = labelCounter++;
        std::string labelFalse = ".Lfalse" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        o << "    mov " << params[1] << ", r15\n";
        o << "    tst r15\n";
        o << "    jeq " << labelFalse << "\n";

        o << "    mov " << params[2] << ", r15\n";
        o << "    tst r15\n";
        o << "    jeq " << labelFalse << "\n";

        o << "    mov #1, r15\n";
        o << "    jmp " << labelEnd << "\n";

        o << labelFalse << ":\n";
        o << "    mov #0, r15\n";

        o << labelEnd << ":\n";
        o << "    mov r15, " << params[0] << "\n";
        break;
    }

    case log_or: {
        int currentLabel = labelCounter++;
        std::string labelTrue = ".Ltrue" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        o << "    mov " << params[1] << ", r15\n";
        o << "    tst r15\n";
        o << "    jne " << labelTrue << "\n";

        o << "    mov " << params[2] << ", r15\n";
        o << "    tst r15\n";
        o << "    jne " << labelTrue << "\n";

        o << "    mov #0, r15\n";
        o << "    jmp " << labelEnd << "\n";

        o << labelTrue << ":\n";
        o << "    mov #1, r15\n";

        o << labelEnd << ":\n";
        o << "    mov r15, " << params[0] << "\n";
        break;
    }

    case rmem:
        // rmem: params[0] = destination, params[1] = adresse
        o << "    mov " << params[1] << ", r14\n";
        o << "    mov @r14, r15\n";      // Déréférencement
        o << "    mov r15, " << params[0] << "\n";
        break;

    case wmem:
        // wmem: params[0] = adresse, params[1] = valeur
        o << "    mov " << params[1] << ", r15\n";
        o << "    mov " << params[0] << ", r14\n";
        o << "    mov r15, 0(r14)\n";    // Écriture à l'adresse
        break;

    case call:
        // call: params[0] = label, params[1] = destination, params[2]... = paramètres
        o << "    call #" << params[0] << "\n";
        o << "    mov r15, " << params[1] << "\n";  // Résultat dans r15
        break;

    case jmp:
        // jmp: params[0] = label
        o << "    jmp " << params[0] << "\n";
        break;

    default:
        o << "    ; Opération IR non supportée\n";
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
        o << "    mov " << test_var_register << ", r15\n";
        o << "    tst r15\n";
        o << "    jeq " << exit_false->label << "\n";
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
            return "#" + std::to_string(p->getCstValue());
        }

        if (p->scopeType == GLOBAL)
        {
            return "&" + reg;
        }
        return "-" + to_string(p->offset) + "(r4)"; // r4 est utilisé comme frame pointer
    }
    return reg;
}

void CFG::gen_asm_prologue(std::ostream &o)
{
    o << "    push r4\n";
    o << "    mov r1, r4\n";          // r1 est le stack pointer sur MSP430
    o << "    sub #" << getStackSize() << ", r1\n";
}

void CFG::gen_asm_epilogue(std::ostream &o)
{
    o << "    mov r4, r1\n";          // Restaure le stack pointer
    o << "    pop r4\n";              // Restaure le frame pointer
    o << "    ret\n";                 // Retour de fonction
}

bool CFG::isRegConstant(std::string& reg)
{
    return reg[0] == '#';
}

//* ---------------------- GlobalVarManager ---------------------- */
void GVM::gen_asm(std::ostream &o)
{
    // Verifier si on a des variables globales
    if (globalScope->getNumberVariable() == 0)
        return;

    o << "    .section .data\n";

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

    o << "    .section .text\n";
}

#endif // __MSP430__