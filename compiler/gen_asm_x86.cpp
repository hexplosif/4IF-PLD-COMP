#include <sstream>
#include <vector>
#include "IR.h"
#include "CodeGenVisitor.h"
using namespace std;

// Génération de code assembleur pour l'instruction for x86 machine

#if defined(__x86_64__) && !defined(__BLOCKED__)

// According to the Linux System V AMD64 ABI, the first six integer arguments go in:
// 1st: %rdi, 2nd: %rsi, 3rd: %rdx, 4th: %rcx, 5th: %r8, 6th: %r9.
// Here we assume that the result of an expression is left in %eax, so we move
// that 32-bit value into the corresponding register (using the "d" suffix for the lower 32 bits).
vector<string> argRegs = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
vector<string> tempRegs = {"%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi", "%e8d", "%e9d"}; // Temporary registers
vector<string> floatRegs = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"}; // Floating point registers
string returnReg = "%eax"; // Register used for return value
string floatReturnReg = "%xmm0"; // Register used for return value (float)


bool isRegister(std::string &reg)
{
    return (!reg.empty() && reg[0] == '%' );
}

void move(std::ostream &o, VarType t, std::string src, std::string dest)
{
    if (t == VarType::FLOAT || t == VarType::FLOAT_PTR)
    {
        // if (isRegister(src) && isRegister(dest))
        //     o << "    movd " << src << ", " << dest << "\n";
        // else
        o << "    movss " << src << ", " << dest << "\n";
        return;
    }
    o << "    movl " << src << ", " << dest << "\n";
}

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
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm5");
            move(o, t, "%xmm5", params[0]);
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        if (params[0] != "%eax")
            o << "    movl %eax, " << params[0] << "\n"; // Stocke le résultat
        break;
    case add:
        // add: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    addss " << params[2] << ", %xmm0\n";
            move(o, t, "%xmm0", params[0]);
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    addl " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case sub:
        // sub: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    subss " << params[2] << ", %xmm0\n";
            move(o, t, "%xmm0", params[0]);
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    subl " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case mul:
        // mul: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    mulss " << params[2] << ", %xmm0\n";
            move(o, t, "%xmm0", params[0]);
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    imull " << params[2] << ", %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case div: // params : dest, source1, source2
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    divss " << params[2] << ", %xmm0\n";
            move(o, t, "%xmm0", params[0]);
            break;
        }

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
        if (t == VarType::FLOAT_PTR) {
            move(o, t, params[1], "%xmm0");
            move (o, VarType::INT, params[2], "%eax");                     // Load index into %eax
            o << "    movslq %eax, %rbx\n";                                // Sign extend to 64-bit
            o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n"; // Correct displacement and scaling
            move (o, t, "%xmm0", "(%rax)"); // Store back
            break;
        }
        
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n";                                // Sign extend to 64-bit
        o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n"; // Correct displacement and scaling
        o << "    movl " << params[1] << ", %edx\n";
        o << "    movl %edx, (%rax)\n";
        break;
    }
    case addTblx: {
        // add: params[0] = destination, params[1] = expr, params[2] = position
        if (t == VarType::FLOAT_PTR) {
            move(o, t, params[1], "%xmm0");
            move (o, VarType::INT, params[2], "%eax");                      // Load index into %eax
            o << "    movslq %eax, %rbx\n";                                 // Sign extend to 64-bit
            o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n";
            move (o, t, "(%rax)", "%xmm1");                                 // Load current array value into %xmm1
            o << "    addss %xmm1, %xmm0\n";
            move (o, t, "%xmm0", "(%rax)");                                 // Store back
            break;
        }
        
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
        if (t == VarType::FLOAT_PTR) {
            move(o, t, params[1], "%xmm0");
            move (o, VarType::INT, params[2], "%eax");                      // Load index into %eax
            o << "    movslq %eax, %rbx\n";                                 // Sign extend to 64-bit
            o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n";
            move (o, t, "(%rax)", "%xmm1");                                 // Load current array value into %xmm1
            o << "    subss %xmm0, %xmm1\n";
            move (o, t, "%xmm1", "(%rax)");                                 // Store back
            break;
        }
        
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
        if (t == VarType::FLOAT_PTR) {
            move(o, t, params[1], "%xmm0");
            move (o, VarType::INT, params[2], "%eax");                      // Load index into %eax
            o << "    movslq %eax, %rbx\n";                                 // Sign extend to 64-bit
            o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n";
            move (o, t, "(%rax)", "%xmm1");                                 // Load current array value into %xmm1
            o << "    mulss %xmm1, %xmm0\n";
            move (o, t, "%xmm0", "(%rax)");                                 // Store back
            break;
        }
        
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
        if (t == VarType::FLOAT_PTR) {
            move(o, t, params[1], "%xmm0");
            move (o, VarType::INT, params[2], "%eax");                      // Load index into %eax
            o << "    movslq %eax, %rbx\n";                                 // Sign extend to 64-bit
            o << "    leaq -" << (params[0]) << "(%rbp, %rbx, 4), %rax\n";
            move (o, t, "(%rax)", "%xmm1");                                 // Load current array value into %xmm1
            o << "    divss %xmm0, %xmm1\n";
            move (o, t, "%xmm1", "(%rax)");                                 // Store back
            break;
        }
        
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
        if (t == VarType::FLOAT_PTR) {
            move (o, VarType::INT, params[2], "%eax");                      // Load index into %eax
            o << "    movslq %eax, %rbx\n"; // Sign extend to 64-bit
            o << "    leaq -" << (params[1]) << "(%rbp, %rbx, 4), %rax\n";  // Correct displacement and scaling
            move (o, t, "(%rax)", "%xmm1"); // Store back
            move (o, t, "%xmm1", params[0]);
            break;
        }
        
        o << "    movl " << params[2] << ", %eax\n";
        o << "    movslq %eax, %rbx\n";                                // Sign extend to 64-bit
        o << "    leaq -" << (params[1]) << "(%rbp, %rbx, 4), %rax\n"; // Correct displacement and scaling
        o << "    movl (%rax), %edx\n";
        o << "    movl %edx, " << params[0] << "\n";
        break;
    }
    case incr:
        // incr: params[0] = var
        if (t == VarType::FLOAT) {
            move(o, t, params[0], "%xmm1");
            move(o, t, params[1], "%xmm0"); // params[1] = label for float 1.0
            o << "    addss	%xmm1, %xmm0\n";
            o << "    movss %xmm0, " << params[0] << "\n";
            break;
        }

        o << "    movl " << params[0] << ", %eax\n";
        o << "    addl $1, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case decr:
        // decr: params[0] = var
        if (t == VarType::FLOAT) {
            move(o, t, params[0], "%xmm0");
            move(o, t, params[1], "%xmm1"); // params[1] = label for float 1.0
            o << "    subss	%xmm1, %xmm0\n";
            o << "    movss %xmm0, " << params[0] << "\n";
            break;
        }

        o << "    movl " << params[0] << ", %eax\n";
        o << "    subl $1, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_eq:
        // cmp_eq: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    ucomiss " << params[2] << ", %xmm0\n";
            o << "    setnp %al\n";
            o << "    movl	$0, %edx\n";
            move(o, t, params[1], "%xmm0");
            o << "    ucomiss " << params[2] << ", %xmm0\n";
            o << "    cmovne %edx, %eax\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << params[0] << "\n";
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case cmp_lt:
        // cmp_lt: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[2], "%xmm0");
            o << "    comiss " << params[1] << ", %xmm0\n";
            o << "    seta %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << params[0] << "\n";
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setl %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;
    case cmp_le:
        // cmp_le: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[2], "%xmm0");
            o << "    comiss " << params[1] << ", %xmm0\n";
            o << "    setnb %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << params[0] << "\n";
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setle %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_ne:
        // cmp_ne: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    ucomiss " << params[2] << ", %xmm0\n";
            o << "    setp %al\n";
            o << "    movl	$1, %edx\n";
            move(o, t, params[1], "%xmm0");
            o << "    ucomiss " << params[2] << ", %xmm0\n";
            o << "    cmovne %edx, %eax\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << params[0] << "\n";
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setne %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_gt:
        // cmp_gt: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    comiss " << params[2] << ", %xmm0\n";
            o << "    seta %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << params[0] << "\n";
            break;
        }

        o << "    movl " << params[1] << ", %eax\n";
        o << "    cmpl " << params[2] << ", %eax\n";
        o << "    setg %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << params[0] << "\n";
        break;

    case cmp_ge:
        // cmp_ge: params[0] = dest, params[1] = gauche, params[2] = droite
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            o << "    comiss " << params[2] << ", %xmm0\n";
            o << "    setnb %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << params[0] << "\n";
            break;
        }

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
        if (t == VarType::FLOAT) {
            move(o, t, params[1], "%xmm0");
            move(o, t, params[2], "%xmm1"); // params[2] = float data for unary
            o << "    xorps %xmm1, %xmm0\n"; 
            move(o, t, "%xmm0", params[0]);
            break;
        }

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

    case intToFloat:
        // intToFloat: params[0] = destination, params[1] = source
        o << "    pxor %xmm0, %xmm0\n"; // Clear xmm0
        o << "    cvtsi2ssl " << params[1] << ", %xmm0\n"; // Convert int to double
        move(o, t, "%xmm0", params[0]);
        break;

    case floatToInt:
        // floatToInt: params[0] = destination, params[1] = source
        o << "    cvttss2sil " << params[1] << ", %eax\n"; // Convert double to int
        o << "    movl %eax, " << params[0] << "\n";
        break;

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
        // call: params[0] = label
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
            if (p->getType() != VarType::FLOAT) {
                return "$" + p->getCstValue();
            }

            std::string label = rodm->putFloatIfNotExists(std::stof(p->getCstValue()));
            return label + "(%rip)";
        }

        if (p->scopeType == GLOBAL)
        {
            return reg + "(%rip)";
        }
        return "-" + to_string(p->offset) + "(%rbp)";
    }

    if (!reg.empty() && reg[0] == '.' && reg[1] == 'L') // Label
    {
        return reg + "(%rip)";
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
    for (auto const& [name, symbol] : globalScope->getTable())
    {
        // Skip temporary variables if they exist in global scope, they are only used to store constants value
        if (SymbolTable::isTempVariable(name))
            continue;

        o << "    .globl " << name << "\n";
        o << name << ":\n";
        if (globalVariableValues.count(name) == 0) {
            o << "    .zero 4\n";
        } else {
            // Initialized global variable
            VarType t = symbol.type;
            std::string value = globalVariableValues[name];

            if (t == VarType::FLOAT) {
                std::string hexIEEEValue = floatToLong_Ieee754(std::stof(value));
                std::string lower = hexIEEEValue.substr(0, 8);
                unsigned long long lowerLong = std::stoull(lower, nullptr, 16);
                o << "    .long " << lowerLong << "\n";
            } else if (t == VarType::INT || t == VarType::CHAR) {
                o << "    .long " << value << "\n";
            }
        }
    }
}

// ---------------------- Read only data Manager ---------------------- */
void RoDM::gen_asm(std::ostream &o)
{
    o << ".section .rodata" << std::endl;
    for (const auto &pair : floatData)
    {
        o << "    .align 4" << std::endl;
        o << pair.first << ":" << "         // = " << pair.second << std::endl;
        std::string hexIEEEValue = floatToLong_Ieee754(pair.second);

        std::string lower = hexIEEEValue.substr(0, 8);
        // std::string upper = hexIEEEValue.substr(8, 8);
        
        // Convert to long long representation
        unsigned long long lowerLong = std::stoull(lower, nullptr, 16);
        // unsigned long long upperLong = std::stoull(upper, nullptr, 16);
        // Lower 32 bits
        // o << "    .long " << upperLong << std::endl;
        // Upper 32 bits
        o << "    .long " << lowerLong << std::endl;
    }

    if (needDataForUnaryOp) {
        o << labelDataForUnaryOp << ":" << "         // = " << 0.0f << std::endl;
        o << "    .long -2147483648" << std::endl;
        o << "    .long 0" << std::endl;
        o << "    .long 0" << std::endl;
        o << "    .long 0" << std::endl;
    }

    o << "\t.text" << std::endl;
}

// -------------------------- Code gen -------------------
void CodeGenVisitor::gen_asm(ostream &os)
{
    os << "// Global variables\n\n";
    gvm->gen_asm(os);
    os << ".text\n";
    os << "\n//================================================ \n\n";
    for (auto &cfg : cfgs)
    {
        cfg->gen_asm(os);
        os << "\n//================================================= \n\n";
    }
    os << "// Read only data\n\n";
    rodm->gen_asm(os);
}

#endif // __x86_64__