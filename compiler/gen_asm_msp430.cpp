#include <sstream>
#include <vector>
#include "CodeGenVisitor.h"
#include "IR.h"
using namespace std;

// Génération de code assembleur pour l'instruction pour les machines MSP430

#ifdef __MSP430__

// Pour MSP430, les registres r0-r3 sont spéciaux (PC, SP, SR, CG) 
// r4-r11 sont des registres généraux pour les arguments et résultats
// r12-r15 sont également généraux mais traditionnellement utilisés comme registres de travail
vector<string> argRegs = {"r15", "r14", "r13", "r12"};  // Arguments en ordre inverse pour stack
vector<string> tempRegs = {"r15", "r14", "r13", "r12", "r11", "r10"};
string returnReg = "r15";  // Registre pour retourner la valeur

bool is_cst(const std::string& s) {
    return !s.empty() && s[0] == '#';
}

bool is_memory(const std::string& s) {
    return !s.empty() && (
        (s[0] == '@' && s.size() > 1) ||  // Adressage indirect
        (s[0] == '(' && s.back() == ')')  // Adressage indexé ou avec déplacement
    );
}

bool is_register(const std::string& s) {
    return !s.empty() && s[0] == 'r' && std::all_of(s.begin() + 1, s.end(), ::isdigit);
}

bool is_global(const std::string& s) {
    return !s.empty() && s[0] == '_';
}

void move(std::ostream &o, const std::string& src, const std::string& dest) {
    if (is_register(src)) {
        if (is_register(dest)) {
            o << "    mov " << src << ", " << dest << "\n";
        } else if (is_memory(dest)) {
            if (dest[0] == '@') {
                // Adressage indirect @Rdest
                o << "    mov " << src << ", " << dest.substr(1) << "\n";
            } else {
                // Adressage avec déplacement
                o << "    mov " << src << ", " << dest << "\n";
            }
        } else if (is_global(dest)) {
            o << "    mov " << src << ", &" << dest << "\n";
        }
    } else if (is_memory(src)) {
        if (is_register(dest)) {
            if (src[0] == '@') {
                // Adressage indirect @Rsrc
                o << "    mov " << src.substr(1) << ", " << dest << "\n";
            } else {
                // Adressage avec déplacement
                o << "    mov " << src << ", " << dest << "\n";
            }
        } else if (is_memory(dest)) {
            o << "    mov " << src << ", r11\n";    // Registre temporaire
            o << "    mov r11, " << dest << "\n";
        }
    } else if (is_cst(src)) {
        if (is_register(dest)) {
            o << "    mov " << src << ", " << dest << "\n";
        } else if (is_memory(dest)) {
            o << "    mov " << src << ", r11\n";    // Registre temporaire
            o << "    mov r11, " << dest << "\n";
        }
    } else if (is_global(src)) {
        if (is_register(dest)) {
            o << "    mov &" << src << ", " << dest << "\n";
        } else {
            o << "    mov &" << src << ", r11\n";    // Registre temporaire
            o << "    mov r11, " << dest << "\n";
        }
    } else {
        o << "    ; Unsupported move: " << src << " to " << dest << "\n";
    }
}

void IRInstr::gen_asm(std::ostream &o)
{
    static int labelCounter = 0;

    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = immediate constant (#val)
        move(o, params[1], "r11");      // Move immediate constant to r11
        move(o, "r11", params[0]);      // Move r11 to destination
        break;
    case copy:
        // copy: params[0] = destination (memory), params[1] = source (memory or immediate)
        move(o, params[1], "r11");      // Move source to r11
        move(o, "r11", params[0]);      // Move r11 to destination
        break;
    case add:
        // add: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    add r14, r15\n";       // MSP430: destination est à droite (add src, dst)
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    case sub:
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    sub r14, r15\n";       // MSP430: destination est à droite (sub src, dst)
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    case mul:
        // MSP430 n'a pas d'instruction de multiplication dans le jeu d'instructions de base
        // Utiliser la fonction d'aide __mulhi3
        move(o, params[1], "r15");       // Premier argument dans r15
        move(o, params[2], "r14");       // Deuxième argument dans r14
        o << "    call #__mulhi3\n";     // Résultat dans r15
        move(o, "r15", params[0]);       // Move r15 to destination
        break;
    case div:
        // MSP430 n'a pas d'instruction de division dans le jeu d'instructions de base
        // Utiliser la fonction d'aide __divhi3 pour division signée
        move(o, params[1], "r15");       // Dividende dans r15
        move(o, params[2], "r14");       // Diviseur dans r14
        o << "    call #__divhi3\n";     // Résultat dans r15
        move(o, "r15", params[0]);       // Move r15 to destination
        break;
    case mod:
        // Pour modulo, on utilise division puis multiplication et soustraction
        // Utiliser les fonctions d'aide __divhi3 et __mulhi3
        move(o, params[1], "r13");       // Sauvegarde du dividende dans r13
        move(o, params[1], "r15");       // Dividende dans r15
        move(o, params[2], "r14");       // Diviseur dans r14
        o << "    call #__divhi3\n";     // Quotient dans r15
        o << "    mov r14, r12\n";       // Sauvegarde du diviseur dans r12
        o << "    call #__mulhi3\n";     // r15 = quotient * diviseur
        o << "    mov r13, r14\n";       // r14 = dividende original
        o << "    sub r15, r14\n";       // r14 = dividende - (quotient * diviseur)
        o << "    mov r14, r15\n";       // Déplacer le résultat dans r15
        move(o, "r15", params[0]);       // Move r15 to destination
        break;

    case copyTblx: {
        // copyTblx: params[0] = base_offset (string literal number), params[1] = value (mem/imm), params[2] = index (mem/imm)
        // Adresse = sp - base_offset + index * 2 (MSP430 utilise des mots de 16 bits)
        move(o, params[2], "r14");      // Charger l'index dans r14
        o << "    add r14, r14\n";       // r14 = index * 2 (taille d'un entier en MSP430)
        o << "    mov r1, r13\n";        // r13 = sp
        o << "    sub #" << params[0] << ", r13\n"; // r13 = sp - base_offset
        o << "    add r14, r13\n";       // adresse finale = base + offset
        move(o, params[1], "r15");      // Charger la valeur dans r15
        o << "    mov r15, 0(r13)\n";    // Stocker la valeur à l'adresse calculée
        break;
    }
    case addTblx: {
        // addTblx: params[0]=base_offset, params[1]=value_to_add (mem/imm), params[2]=index (mem/imm)
        move(o, params[2], "r14");      // index dans r14
        o << "    add r14, r14\n";       // offset = index * 2
        o << "    mov r1, r13\n";        // r13 = sp
        o << "    sub #" << params[0] << ", r13\n"; // adresse base = sp - base_offset
        o << "    add r14, r13\n";       // adresse finale = base + offset
        o << "    mov 0(r13), r12\n";    // Charger la valeur du tableau dans r12
        move(o, params[1], "r15");      // value_to_add dans r15
        o << "    add r15, r12\n";       // Ajouter la valeur
        o << "    mov r12, 0(r13)\n";    // Stocker à nouveau
        break;
    }
    case subTblx: {
        move(o, params[2], "r14");      // index dans r14
        o << "    add r14, r14\n";       // offset = index * 2
        o << "    mov r1, r13\n";        // r13 = sp
        o << "    sub #" << params[0] << ", r13\n"; // adresse base = sp - base_offset
        o << "    add r14, r13\n";       // adresse finale = base + offset
        o << "    mov 0(r13), r12\n";    // Charger la valeur du tableau dans r12
        move(o, params[1], "r15");      // value_to_sub dans r15
        o << "    sub r15, r12\n";       // Soustraire la valeur
        o << "    mov r12, 0(r13)\n";    // Stocker à nouveau
        break;
    }
    case mulTblx: {
        move(o, params[2], "r14");      // index dans r14
        o << "    add r14, r14\n";       // offset = index * 2
        o << "    mov r1, r13\n";        // r13 = sp
        o << "    sub #" << params[0] << ", r13\n"; // adresse base = sp - base_offset
        o << "    add r14, r13\n";       // adresse finale = base + offset
        o << "    mov 0(r13), r15\n";    // Charger la valeur du tableau dans r15
        move(o, params[1], "r14");      // value_to_mul dans r14
        o << "    push r13\n";           // Sauvegarder l'adresse du tableau
        o << "    call #__mulhi3\n";     // Multiplication, résultat dans r15
        o << "    pop r13\n";            // Restaurer l'adresse du tableau
        o << "    mov r15, 0(r13)\n";    // Stocker à nouveau
        break;
    }
    case divTblx: {
        move(o, params[2], "r14");      // index dans r14
        o << "    add r14, r14\n";       // offset = index * 2
        o << "    mov r1, r13\n";        // r13 = sp
        o << "    sub #" << params[0] << ", r13\n"; // adresse base = sp - base_offset
        o << "    add r14, r13\n";       // adresse finale = base + offset
        o << "    mov 0(r13), r15\n";    // Charger la valeur du tableau (dividende) dans r15
        move(o, params[1], "r14");      // diviseur dans r14
        o << "    push r13\n";           // Sauvegarder l'adresse du tableau
        o << "    call #__divhi3\n";     // Division, résultat dans r15
        o << "    pop r13\n";            // Restaurer l'adresse du tableau
        o << "    mov r15, 0(r13)\n";    // Stocker à nouveau
        break;
    }
    case modTblx: {
        move(o, params[2], "r14");      // index dans r14
        o << "    add r14, r14\n";       // offset = index * 2
        o << "    mov r1, r13\n";        // r13 = sp
        o << "    sub #" << params[0] << ", r13\n"; // adresse base = sp - base_offset
        o << "    add r14, r13\n";       // adresse finale = base + offset
        o << "    mov 0(r13), r15\n";    // Charger la valeur du tableau (dividende) dans r15
        o << "    push r15\n";           // Sauvegarder la valeur originale
        move(o, params[1], "r14");      // diviseur dans r14
        o << "    push r14\n";           // Sauvegarder le diviseur
        o << "    push r13\n";           // Sauvegarder l'adresse du tableau
        o << "    call #__divhi3\n";     // Division, quotient dans r15
        o << "    mov r15, r12\n";       // Sauvegarder le quotient dans r12
        o << "    pop r13\n";            // Restaurer l'adresse du tableau
        o << "    pop r14\n";            // Restaurer le diviseur
        o << "    mov r12, r15\n";       // Quotient dans r15
        o << "    push r13\n";           // Sauvegarder l'adresse du tableau
        o << "    call #__mulhi3\n";     // r15 = quotient * diviseur
        o << "    pop r13\n";            // Restaurer l'adresse du tableau
        o << "    pop r14\n";            // Restaurer la valeur originale dans r14
        o << "    sub r15, r14\n";       // r14 = dividende - (quotient * diviseur)
        o << "    mov r14, 0(r13)\n";    // Stocker le résultat
        break;
    }
    case getTblx: {
        // getTblx: params[0] = destination (mem), params[1] = base_offset (string literal number), params[2] = index (mem/imm)
        move(o, params[2], "r14");      // index dans r14
        o << "    add r14, r14\n";       // offset = index * 2
        o << "    mov r1, r13\n";        // r13 = sp
        o << "    sub #" << params[1] << ", r13\n"; // adresse base = sp - base_offset
        o << "    add r14, r13\n";       // adresse finale = base + offset
        o << "    mov 0(r13), r15\n";    // Charger la valeur du tableau dans r15
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case incr:
        // incr: params[0] = var (memory)
        move(o, params[0], "r15");      // Charger la variable dans r15
        o << "    inc r15\n";            // Incrémenter r15
        move(o, "r15", params[0]);      // Move r15 to destination
        break;

    case decr:
        // decr: params[0] = var (memory)
        move(o, params[0], "r15");      // Charger la variable dans r15
        o << "    dec r15\n";            // Décrémenter r15
        move(o, "r15", params[0]);      // Move r15 to destination
        break;

    case cmp_eq: { // ==
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    cmp r14, r15\n";       // Compare r15 avec r14
        o << "    mov #0, r15\n";        // Par défaut, résultat est faux (0)
        int currentLabel = labelCounter++;
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
        o << "    jne " << labelEnd << "\n";     // Sauter si pas égal
        o << "    mov #1, r15\n";        // Mettre 1 si égal
        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case cmp_lt: { // <
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    cmp r14, r15\n";       // Compare r15 avec r14
        o << "    mov #0, r15\n";        // Par défaut, résultat est faux (0)
        int currentLabel = labelCounter++;
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
        o << "    jge " << labelEnd << "\n";     // Sauter si r15 >= r14
        o << "    mov #1, r15\n";        // Mettre 1 si r15 < r14
        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case cmp_le: { // <=
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    cmp r14, r15\n";       // Compare r15 avec r14
        o << "    mov #0, r15\n";        // Par défaut, résultat est faux (0)
        int currentLabel = labelCounter++;
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
        o << "    jg " << labelEnd << "\n";      // Sauter si r15 > r14
        o << "    mov #1, r15\n";        // Mettre 1 si r15 <= r14
        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case cmp_ne: { // !=
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    cmp r14, r15\n";       // Compare r15 avec r14
        o << "    mov #0, r15\n";        // Par défaut, résultat est faux (0)
        int currentLabel = labelCounter++;
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
        o << "    jeq " << labelEnd << "\n";     // Sauter si égal
        o << "    mov #1, r15\n";        // Mettre 1 si pas égal
        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case cmp_gt: { // >
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    cmp r14, r15\n";       // Compare r15 avec r14
        o << "    mov #0, r15\n";        // Par défaut, résultat est faux (0)
        int currentLabel = labelCounter++;
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
        o << "    jle " << labelEnd << "\n";     // Sauter si r15 <= r14
        o << "    mov #1, r15\n";        // Mettre 1 si r15 > r14
        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case cmp_ge: { // >=
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    cmp r14, r15\n";       // Compare r15 avec r14
        o << "    mov #0, r15\n";        // Par défaut, résultat est faux (0)
        int currentLabel = labelCounter++;
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
        o << "    jl " << labelEnd << "\n";      // Sauter si r15 < r14
        o << "    mov #1, r15\n";        // Mettre 1 si r15 >= r14
        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case bit_and:
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    and r14, r15\n";       // AND bit à bit, résultat dans r15
        move(o, "r15", params[0]);      // Move r15 to destination
        break;

    case bit_or:
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    bis r14, r15\n";       // OR bit à bit (BIS = Bit Set), résultat dans r15
        move(o, "r15", params[0]);      // Move r15 to destination
        break;

    case bit_xor:
        move(o, params[1], "r15");
        move(o, params[2], "r14");
        o << "    xor r14, r15\n";       // XOR bit à bit, résultat dans r15
        move(o, "r15", params[0]);      // Move r15 to destination
        break;

    case unary_minus:
        move(o, params[1], "r15");
        o << "    inv r15\n";            // Inverse les bits
        o << "    inc r15\n";            // Ajoute 1 pour avoir le complément à 2
        move(o, "r15", params[0]);      // Move r15 to destination
        break;

    case not_op: { // logical not !
        move(o, params[1], "r15");
        o << "    cmp #0, r15\n";        // Compare r15 avec 0
        o << "    mov #0, r15\n";        // Par défaut, résultat est faux (0)
        int currentLabel = labelCounter++;
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);
        o << "    jne " << labelEnd << "\n";     // Sauter si r15 != 0
        o << "    mov #1, r15\n";        // Mettre 1 si r15 == 0
        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case log_and: { // &&
        int currentLabel = labelCounter++;
        std::string labelFalse = ".Lfalse" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        move(o, params[1], "r15");
        o << "    cmp #0, r15\n";        // Compare first arg with 0
        o << "    jeq " << labelFalse << "\n"; // if first is false, result is false

        move(o, params[2], "r15");
        o << "    cmp #0, r15\n";        // Compare second arg with 0
        o << "    jeq " << labelFalse << "\n"; // if second is false, result is false

        // Both are true
        o << "    mov #1, r15\n";
        o << "    jmp " << labelEnd << "\n";

        o << labelFalse << ":\n";
        o << "    mov #0, r15\n";

        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case log_or: { // ||
        int currentLabel = labelCounter++;
        std::string labelTrue = ".Ltrue" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        move(o, params[1], "r15");
        o << "    cmp #0, r15\n";        // Compare first arg with 0
        o << "    jne " << labelTrue << "\n"; // if first is true, result is true

        move(o, params[2], "r15");
        o << "    cmp #0, r15\n";        // Compare second arg with 0
        o << "    jne " << labelTrue << "\n"; // if second is true, result is true

        // Both are false
        o << "    mov #0, r15\n";
        o << "    jmp " << labelEnd << "\n";

        o << labelTrue << ":\n";
        o << "    mov #1, r15\n";

        o << labelEnd << ":\n";
        move(o, "r15", params[0]);      // Move r15 to destination
        break;
    }
    case rmem:
        // rmem: params[0] = destination (mem), params[1] = address (mem/imm)
        move(o, params[1], "r14");     // Load address into r14
        o << "    mov @r14, r15\n";    // Load value from address in r14 into r15
        move(o, "r15", params[0]);     // Move r15 to destination
        break;

    case wmem:
        // wmem: params[0] = address (mem/imm), params[1] = value (mem/imm)
        move(o, params[0], "r14");     // Load address into r14
        move(o, params[1], "r15");     // Load value into r15
        o << "    mov r15, 0(r14)\n";  // Store value r15 to address in r14
        break;

    case call:
        // call: params[0] = label, params[1] = destination (mem)
        o << "    call #_" << params[0] << "\n";  // Call function
        // Résultat dans r15, déplacer vers la destination
        move(o, "r15", params[1]);     // Move return value (in r15) to destination
        break;

    case jmp:
        // jmp: params[0] = label
        o << "    jmp " << params[0] << "\n";
        break;

    default:
        o << "    ; Unsupported IR operation for MSP430\n";
        break;
    }
}

//* ---------------------- BasicBlock ---------------------- */

void BasicBlock::gen_asm(std::ostream &o)
{
    // Generate assembly for each instruction in the block
    for (auto instr : instrs)
    {
        instr->gen_asm(o);
    }

    // Handle jumps at the end of the block
    if (!test_var_name.empty() && exit_true != nullptr && exit_false != nullptr)
    {
        // Conditional jump based on test_var_register
        move(o, test_var_register, "r15"); // Load variable into r15
        o << "    cmp #0, r15\n";                     // Compare with zero
        o << "    jeq " << exit_false->label << "\n";  // Branch to false label if zero
        o << "    jmp " << exit_true->label << "\n";   // Otherwise, branch to true label
    }
    else if (exit_true != nullptr)
    {
        // Unconditional jump
        if (exit_true->label != cfg->get_epilogue_label())
        {
            o << "    jmp " << exit_true->label << "\n";
        }
    } 
    else
    {   // si on est à la fin de cfg (fin de function)
        cfg->gen_asm_epilogue(o);
    }
}

//* ---------------------- CFG ---------------------- */

void BasicBlock::add_IRInstr(IRInstr::Operation op, VarType t, std::vector<std::string> params)
{
    bool isCallOrJmp = (op == IRInstr::Operation::call || op == IRInstr::Operation::jmp);
    bool isTblBaseOffset = (op == IRInstr::Operation::copyTblx || op == IRInstr::Operation::addTblx ||
                           op == IRInstr::Operation::subTblx || op == IRInstr::Operation::mulTblx ||
                           op == IRInstr::Operation::divTblx || op == IRInstr::Operation::modTblx );

    std::string p0 = isCallOrJmp ? params[0] : cfg->IR_reg_to_asm(params[0]);
    std::string p1 = params.size() >= 2 ? cfg->IR_reg_to_asm(params[1]) : "";
    std::string p2 = params.size() >= 3 ? cfg->IR_reg_to_asm(params[2]) : "";

    if (isTblBaseOffset) {
        Symbol *p = cfg->currentScope->findVariable(params[0]);
        p0 = std::to_string(p->offset); // Get the offset for the base address
    }

    if (op == IRInstr::Operation::getTblx) {
        Symbol *p = cfg->currentScope->findVariable(params[1]);
        p1 = std::to_string(p->offset); // Get the offset for the index
    }

    std::vector<std::string> paramsRealAddr = {p0, p1, p2};
    
    // Optimization: If it's a copy from a constant, change op to ldconst
    if (op == IRInstr::Operation::copy && CFG::isRegConstant(paramsRealAddr[1])) {
        op = IRInstr::Operation::ldconst;
    }

    IRInstr *instr = new IRInstr(this, op, t, paramsRealAddr);
    instrs.push_back(instr);

    // Update test_var_register if this block sets a condition variable for a later jump
    if (op >= IRInstr::Operation::cmp_eq && op <= IRInstr::Operation::cmp_ge && cfg->current_bb == this) {
        test_var_register = paramsRealAddr[0]; // Store the assembly operand
    }
}

void CFG::gen_asm(std::ostream &o)
{
    o << ".global _" << ast->getName() << "\n"; // Export function symbol

    for (size_t i = 0; i < bbs.size(); i++)
    {
        
        if (i == 0)
        {
            o << "_" << bbs[i]->label << ":\n";
            gen_asm_prologue(o);
        } else {
            o << bbs[i]->label << ":\n";
        }
        bbs[i]->gen_asm(o);
    }
}

// Translate IR Register names to MSP430 assembly operands
std::string CFG::IR_reg_to_asm(std::string &reg, bool ignoreCst)
{
    // Use symbol table to find variable/parameter location
    Symbol *p = currentScope->findVariable(reg);
    if (p != nullptr)
    {
        if (p->isConstant() && !ignoreCst) {
            return "#" + std::to_string(p->getCstValue());
        }

        if (p->scopeType == GLOBAL) {
            return "&_" + reg;  // MSP430 uses & prefix for absolute addressing

        } else {
            // Pour MSP430, l'adresse est calculée relative au SP
            // Pour les variables locales et paramètres:
            return std::to_string(p->offset) + "(r1)";  // offset(SP)
        }
    }

    // Fallback: return the name itself, maybe it's a label or needs later handling.
    return reg;
}

void CFG::gen_asm_prologue(std::ostream &o) {
    o << "    push r4\n";    // Sauvegarder le registre frame pointer
    o << "    mov r1, r4\n";  // Initialiser le frame pointer
    size_t stackSize = getStackSize();
    stackSize = (stackSize + 1) & ~1; // Align to 2 bytes (16 bits)
    if (stackSize > 0) {
        o << "    sub #" << stackSize << ", r1\n";  // Allouer l'espace de pile
    }
}

void CFG::gen_asm_epilogue(std::ostream &o) {
    o << "    mov r4, r1\n";  // Restaurer le stack pointer
    o << "    pop r4\n";      // Restaurer le registre frame pointer
    o << "    ret\n";         // Retour de fonction
}

bool CFG::isRegConstant(std::string& reg)
{
    return reg[0] == '$';
}

//* ---------------------- GlobalVarManager ---------------------- */
void GVM::gen_asm(std::ostream &o)
{
    // Check if there are any global variables
    if (globalScope->getNumberVariable() == 0)
        return;

    o << "\n";
    o << ".data\n"; // Switch to data segment
    o << ".align 2\n"; // Align data (2 bytes for MSP430)

    for (auto const& [name, symbol] : globalScope->getTable())
    {
        // Skip temporary variables if they exist in global scope (unlikely but possible)
        if (SymbolTable::isTempVariable(name))
            continue;

        o << ".global _" << name << "\n"; // Export symbol
        o << "_" << name << ":\n"; // Label for the variable

        if (globalVariableValues.count(name) == 0) {
            // Uninitialized global variable (BSS segment effectively)
            o << "    .space 2\n"; // Reserve 2 bytes, initialized to zero by default
        } else {
            // Initialized global variable
            o << "    .word " << globalVariableValues[name] << "\n"; // .word for 16-bit integer
        }
    }

    o << "\n";
    o << ".text\n"; // Switch back to text segment for code
    o << ".align 2\n";
}

// -------------------------- Code gen -------------------------------
void CodeGenVisitor::gen_asm(ostream &os)
{
    gvm->gen_asm(os);
    os << ".text\n";
    os << "\n;================================================ \n\n";
    for (auto &cfg : cfgs)
    {
        cfg->gen_asm(os);
        os << "\n;================================================= \n\n";
    }
}

#endif // __MSP430__