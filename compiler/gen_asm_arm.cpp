#include <sstream>
#include <vector>
#include "CodeGenVisitor.h"
#include "IR.h"
using namespace std;

// Génération de code assembleur pour l'instruction pour les machines ARM

#ifdef __arm__

// For ARM32, up to four arguments to a function are passed in registers. 
// r0 through r3 are used to pass the arguments and the call return address is stored in the link register.
vector<string> argRegs = {"r0", "r1", "r2", "r3"};
vector<string> tempRegs = {"r0", "r1", "r2", "r3", "r12"};
string returnReg = "r0"; // Register used for return value

bool is_cst(const std::string& s) {
    return !s.empty() && s[0] == '#';
}

bool is_memory(const std::string& s) {
    return !s.empty() && s[0] == '[' && s.back() == ']';
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
            o << "    mov " << dest << ", " << src << "\n";
        } else if (is_memory(dest)) {
            o << "    str " << src << ", " << dest << "\n";
        } else if (is_global(dest)) {
            o << "    ldr r3, =" << dest << "\n";
            o << "    str " << src << ", [r3]\n";
        }
    } else if (is_memory(src)) {
        if (is_register(dest)) {
            o << "    ldr " << dest << ", " << src << "\n";
        } else if (is_memory(dest)) {
            o << "    ldr r3, " << src << "\n";
            o << "    str r3, " << dest << "\n";
        }
    } else if (is_cst(src)) {
        if (is_register(dest)) {
            o << "    mov " << dest << ", " << src << "\n";
        } else if (is_memory(dest)) {
            o << "    mov r3, " << src << "\n";
            o << "    str r3, " << dest << "\n";
        }
    } else if (is_global(src)) {
        if (is_register(dest)) {
            o << "    ldr " << dest << ", =" << src << "\n";
            o << "    ldr " << dest << ", [" << dest << "]\n";
        } else {
            o << "    ; Unsupported move: " << src << " to " << dest << "\n";
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
        move(o, params[1], "r3"); // Move immediate constant to r3
        move(o, "r3", params[0]); // Move r3 to destination
        break;
    case copy:
        // copy: params[0] = destination (memory), params[1] = source (memory or immediate)
        move(o, params[1], "r3"); // Move source to r3
        move(o, "r3", params[0]); // Move r3 to destination
        break;
    case add:
        // add: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    add r0, r0, r1\n";
        move(o, "r0", params[0]); // Move r0 to destination
        break;
    case sub:
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    sub r0, r0, r1\n";
        move(o, "r0", params[0]); // Move r0 to destination
        break;
    case mul:
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    mul r0, r0, r1\n";
        move(o, "r0", params[0]); // Move r0 to destination
        break;
    case div:
        // ARM32 doesn't have direct division instructions in base instruction set
        // Using bl __aeabi_idiv for signed division
        move(o, params[1], "r0");  // dividend in r0
        move(o, params[2], "r1");  // divisor in r1
        o << "    bl __aeabi_idiv\n";  // result in r0
        move(o, "r0", params[0]); // Move r0 to destination
        break;
    case mod:
        // For modulo, we need to compute dividend - (quotient * divisor)
        move(o, params[1], "r0");  // dividend in r0
        move(o, params[2], "r1");  // divisor in r1
        o << "    bl __aeabi_idiv\n";  // quotient in r0 = dividend / divisor
        o << "    mov r2, r0\n";       // save quotient in r2
        move(o, params[2], "r1");      // reload divisor in r1
        o << "    mul r2, r2, r1\n";   // r2 = quotient * divisor
        move(o, params[1], "r0");      // reload dividend in r0
        o << "    sub r0, r0, r2\n";   // r0 = dividend - (quotient * divisor)
        move(o, "r0", params[0]);      // Move result to destination
        break;

    case copyTblx: {
        // copyTblx: params[0] = base_offset (string literal number), params[1] = value (mem/imm), params[2] = index (mem/imm)
        // Address = fp - base_offset + index * 4
        move(o, params[2], "r1");      // Load index into r1
        o << "    lsl r2, r1, #2\n";         // r2 = index * 4
        o << "    sub r3, fp, #" << params[0] << "\n"; // r3 = fp - base_offset
        o << "    add r3, r3, r2\n";        // final addr = base + offset
        move(o, params[1], "r0");      // Load value to store into r0
        o << "    str r0, [r3]\n";           // Store value at the calculated address
        break;
    }
    case addTblx: {
        // addTblx: params[0]=base_offset, params[1]=value_to_add (mem/imm), params[2]=index (mem/imm)
        move(o, params[2], "r1");      // index
        o << "    lsl r2, r1, #2\n";         // offset = index * 4
        o << "    sub r3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add r3, r3, r2\n";         // final addr = base + offset
        o << "    ldr r2, [r3]\n";           // Load current array value into r2
        move(o, params[1], "r0");      // value_to_add
        o << "    add r2, r2, r0\n";         // Add the value
        o << "    str r2, [r3]\n";           // Store back
        break;
    }
    case subTblx: {
        move(o, params[2], "r1");      // index
        o << "    lsl r2, r1, #2\n";         // offset = index * 4
        o << "    sub r3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add r3, r3, r2\n";         // final addr = base + offset
        o << "    ldr r2, [r3]\n";           // Load current array value into r2
        move(o, params[1], "r0");      // value_to_sub
        o << "    sub r2, r2, r0\n";         // Subtract the value
        o << "    str r2, [r3]\n";           // Store back
        break;
    }
    case mulTblx: {
        move(o, params[2], "r1");      // index
        o << "    lsl r2, r1, #2\n";         // offset = index * 4
        o << "    sub r3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add r3, r3, r2\n";         // final addr = base + offset
        o << "    ldr r2, [r3]\n";           // Load current array value into r2
        move(o, params[1], "r0");      // value_to_mul
        o << "    mul r2, r2, r0\n";         // Multiply the value
        o << "    str r2, [r3]\n";           // Store back
        break;
    }
    case divTblx: {
        move(o, params[2], "r1");      // index
        o << "    lsl r2, r1, #2\n";         // offset = index * 4
        o << "    sub r3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add r3, r3, r2\n";         // final addr = base + offset
        o << "    ldr r0, [r3]\n";           // Load current array value (dividend) into r0
        move(o, params[1], "r1");      // divisor
        o << "    bl __aeabi_idiv\n";        // Divide r0/r1, result in r0
        o << "    str r0, [r3]\n";           // Store back
        break;
    }
    case modTblx: {
        move(o, params[2], "r1");      // index
        o << "    lsl r2, r1, #2\n";         // offset = index * 4
        o << "    sub r3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add r3, r3, r2\n";         // final addr = base + offset
        o << "    ldr r0, [r3]\n";           // Load current array value (dividend) into r0
        move(o, params[1], "r1");      // divisor
        o << "    push {r1, r3}\n";          // Save r1 (divisor) and r3 (address)
        o << "    bl __aeabi_idiv\n";        // quotient in r0 = dividend / divisor
        o << "    mov r2, r0\n";             // save quotient in r2
        o << "    pop {r1, r3}\n";           // Restore r1 (divisor) and r3 (address)
        o << "    ldr r0, [r3]\n";           // reload dividend
        o << "    mul r2, r2, r1\n";         // r2 = quotient * divisor
        o << "    sub r0, r0, r2\n";         // r0 = dividend - (quotient * divisor)
        o << "    str r0, [r3]\n";           // Store result back
        break;
    }
    case getTblx: {
        // getTblx: params[0] = destination (mem), params[1] = base_offset (string literal number), params[2] = index (mem/imm)
        move(o, params[2], "r1");      // index
        o << "    lsl r2, r1, #2\n";         // offset = index * 4
        o << "    sub r3, fp, #" << params[1] << "\n"; // base addr = fp - base_offset
        o << "    add r3, r3, r2\n";         // final addr = base + offset
        o << "    ldr r0, [r3]\n";           // Load value from array element into r0
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case incr:
        // incr: params[0] = var (memory)
        move(o, params[0], "r0"); // Load variable into r0
        o << "    add r0, r0, #1\n";
        move(o, "r0", params[0]); // Move r0 to destination
        break;

    case decr:
        // decr: params[0] = var (memory)
        move(o, params[0], "r0"); // Load variable into r0
        o << "    sub r0, r0, #1\n";
        move(o, "r0", params[0]); // Move r0 to destination
        break;

    case cmp_eq: { // ==
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    cmp r0, r1\n";
        o << "    moveq r0, #1\n";    // Move 1 to r0 if equal
        o << "    movne r0, #0\n";    // Move 0 to r0 if not equal
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case cmp_lt: { // <
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    cmp r0, r1\n";
        o << "    movlt r0, #1\n";    // Move 1 to r0 if less than
        o << "    movge r0, #0\n";    // Move 0 to r0 if greater or equal
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case cmp_le: { // <=
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    cmp r0, r1\n";
        o << "    movle r0, #1\n";    // Move 1 to r0 if less or equal
        o << "    movgt r0, #0\n";    // Move 0 to r0 if greater than
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case cmp_ne: { // !=
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    cmp r0, r1\n";
        o << "    movne r0, #1\n";    // Move 1 to r0 if not equal
        o << "    moveq r0, #0\n";    // Move 0 to r0 if equal
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case cmp_gt: { // >
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    cmp r0, r1\n";
        o << "    movgt r0, #1\n";    // Move 1 to r0 if greater than
        o << "    movle r0, #0\n";    // Move 0 to r0 if less or equal
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case cmp_ge: { // >=
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    cmp r0, r1\n";
        o << "    movge r0, #1\n";    // Move 1 to r0 if greater or equal
        o << "    movlt r0, #0\n";    // Move 0 to r0 if less than
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case bit_and:
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    and r0, r0, r1\n";
        move(o, "r0", params[0]);     // Move r0 to destination
        break;

    case bit_or:
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    orr r0, r0, r1\n";
        move(o, "r0", params[0]);     // Move r0 to destination
        break;

    case bit_xor:
        move(o, params[1], "r0");
        move(o, params[2], "r1");
        o << "    eor r0, r0, r1\n";
        move(o, "r0", params[0]);     // Move r0 to destination
        break;

    case unary_minus:
        move(o, params[1], "r0");
        o << "    rsb r0, r0, #0\n";  // r0 = 0 - r0
        move(o, "r0", params[0]);     // Move r0 to destination
        break;

    case not_op: { // logical not !
        move(o, params[1], "r0");
        o << "    cmp r0, #0\n";
        o << "    moveq r0, #1\n";    // Move 1 to r0 if r0 == 0
        o << "    movne r0, #0\n";    // Move 0 to r0 if r0 != 0
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case log_and: { // &&
        int currentLabel = labelCounter++;
        std::string labelFalse = ".Lfalse" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        move(o, params[1], "r0");
        o << "    cmp r0, #0\n";
        o << "    beq " << labelFalse << "\n"; // if first is false, result is false

        move(o, params[2], "r0");
        o << "    cmp r0, #0\n";
        o << "    beq " << labelFalse << "\n"; // if second is false, result is false

        // Both are true
        o << "    mov r0, #1\n";
        o << "    b " << labelEnd << "\n";

        o << labelFalse << ":\n";
        o << "    mov r0, #0\n";

        o << labelEnd << ":\n";
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case log_or: { // ||
        int currentLabel = labelCounter++;
        std::string labelTrue = ".Ltrue" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        move(o, params[1], "r0");
        o << "    cmp r0, #0\n";
        o << "    bne " << labelTrue << "\n"; // if first is true, result is true

        move(o, params[2], "r0");
        o << "    cmp r0, #0\n";
        o << "    bne " << labelTrue << "\n"; // if second is true, result is true

        // Both are false
        o << "    mov r0, #0\n";
        o << "    b " << labelEnd << "\n";

        o << labelTrue << ":\n";
        o << "    mov r0, #1\n";

        o << labelEnd << ":\n";
        move(o, "r0", params[0]);     // Move r0 to destination
        break;
    }
    case rmem:
        // rmem: params[0] = destination (mem), params[1] = address (mem/imm)
        move(o, params[1], "r1");     // Load address into r1
        o << "    ldr r0, [r1]\n";    // Load value from address in r1 into r0
        move(o, "r0", params[0]);     // Move r0 to destination
        break;

    case wmem:
        // wmem: params[0] = address (mem/imm), params[1] = value (mem/imm)
        move(o, params[0], "r1");     // Load address into r1
        move(o, params[1], "r0");     // Load value into r0
        o << "    str r0, [r1]\n";    // Store value r0 to address in r1
        break;

    case call:
        // call: params[0] = label, params[1] = destination (mem)
        // ARM32: Parameters r0-r3 should be set up before the call
        o << "    bl _" << params[0] << "\n";  // Branch with link (call)
        move(o, "r0", params[1]);     // Move return value (in r0) to destination
        break;

    case jmp:
        // jmp: params[0] = label
        o << "    b " << params[0] << "\n";
        break;

    default:
        o << "    @ Unsupported IR operation for ARM32\n";
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
        // Conditional jump based on test_var_register (already in assembly format, e.g., [fp, #-8])
        move(o, test_var_register, "w0"); // Load variable into w0
        o << "    cmp w0, #0\n";                        // Compare with zero
        o << "    b.eq " << exit_false->label << "\n";    // Branch to false label if zero
        o << "    b " << exit_true->label << "\n";       // Otherwise, branch to true label
    }
    else if (exit_true != nullptr)
    {
        // Unconditional jump
        if (exit_true->label != cfg->get_epilogue_label())
        {
            o << "    b " << exit_true->label << "\n";
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
        test_var_register = paramsRealAddr[0]; // Store the assembly operand (e.g., "[fp, #-12]")
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

// Translate IR Register names to ARM64 assembly operands
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
            return "_" + reg;

        } else {
            // Return frame pointer relative address [fp, #-offset]
            return "[fp, #-" + std::to_string(p->offset) + "]";
        }
    }

    // Fallback: return the name itself, maybe it's a label or needs later handling.
    return reg;
}

void CFG::gen_asm_prologue(std::ostream &o) {
    o << "    push {fp, lr}\n";
    o << "    mov fp, sp\n";
    size_t stackSize = getStackSize();
    stackSize = (stackSize + 7) & ~7; // Align to 8 bytes
    if (stackSize > 0) {
        o << "    sub sp, sp, #" << stackSize << "\n";
    }
}

void CFG::gen_asm_epilogue(std::ostream &o) {
    o << "    mov sp, fp\n";
    o << "    pop {fp, pc}\n";
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
    o << ".align 2\n"; // Align data (4 bytes for integers)

    for (auto const& [name, symbol] : globalScope->getTable())
    {
        // Skip temporary variables if they exist in global scope (unlikely but possible)
        if (SymbolTable::isTempVariable(name))
            continue;

        o << ".set fp, x29\n";
        o << ".global _" << name << "\n"; // Export symbol
        o << "_" << name << ":\n"; // Label for the variable

        if (globalVariableValues.count(name) == 0) {
            // Uninitialized global variable (BSS segment effectively)
            o << "    .space 4\n"; // Reserve 4 bytes, initialized to zero by default
        } else {
            // Initialized global variable
            o << "    .word " << globalVariableValues[name] << "\n"; // .word for 32-bit integer
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

#endif // __arm__