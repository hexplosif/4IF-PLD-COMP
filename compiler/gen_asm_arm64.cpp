#include <sstream>
#include <vector>
#include <string>
#include <algorithm> // Required for std::all_of
#include "IR.h"
#include "CodeGenVisitor.h"

using namespace std;

// ARM64 Assembly Code Generation

#if defined(__aarch64__) && !defined(__BLOCKED__)

// AAPCS64: First 8 integer arguments go in w0-w7.
vector<string> argRegs = {"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"};
// General purpose temporary registers (caller-saved)
vector<string> tempRegs = {"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8", "w9", "w10", "w11"};
string returnReg = "w0"; // Register used for return value

bool is_cst(const std::string& s) {
    return !s.empty() && s[0] == '#';
}

bool is_memory(const std::string& s) {
    return !s.empty() && s[0] == '[' && s.back() == ']';
}

bool is_register(const std::string& s) {
    return !s.empty() && s[0] == 'w' && std::all_of(s.begin() + 1, s.end(), ::isdigit);
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
        } else if (is_cst(dest)) {
            o << "    mov " << dest << ", " << src << "\n";
        } else if (is_global(dest)) {
            o << "    adrp	x8, " << dest << "@PAGE\n";
            o << "    str " << src << ", [x8, " << dest << "@PAGEOFF]" << "\n";
        }
    } else if (is_memory(src)) {
        if (is_register(dest)) {
            o << "    ldr " << dest << ", " << src << "\n";
        } else if (is_memory(dest)) {
            o << "    ldr w9, " << src << "\n"; // Load into a temporary register
            o << "    str w9, " << dest << "\n"; // Store into destination memory
        } else if (is_cst(dest)) {
            o << "    ldr w9, " << src << "\n"; // Load into a temporary register
            o << "    mov " << dest << ", w9\n"; // Move to constant
        }
    } else if (is_cst(src)) {
        if (is_register(dest)) {
            o << "    mov " << dest << ", " << src << "\n";
        } else if (is_memory(dest)) {
            o << "    mov " << src << ", w9\n"; // Move to a temporary register
            o << "    str w9, " << dest << "\n"; // Store into destination memory
        }
    } else if (is_global(src)) {
        if (is_register(dest)) {
            o << "    adrp	x8, " << src << "@PAGE\n";
            o << "    ldr " << dest << ", [x8, " << src << "@PAGEOFF]" << "\n";
        } else {
            o << "    ; Unsupported move operation: " << src << " to " << dest << "\n";
        }
    } else {
        o << "    ; Unsupported move operation: " << src << " to " << dest << "\n";
    }
}

void IRInstr::gen_asm(std::ostream &o)
{
    static int labelCounter = 0;

    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = immediate constant (#val)
        move(o, params[1], "w9"); // Move immediate constant to destination
        move(o, "w9", params[0]); // Move w0 to destination
        break;
    case copy:
        // copy: params[0] = destination (memory), params[1] = source (memory or immediate)
        move(o, params[1], "w9"); // Move source to w0
        move(o, "w9", params[0]); // Move w0 to destination
        break;
    case add:
        // add: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    add w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case sub:
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    sub w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case mul:
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    mul w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case div:
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    sdiv w0, w0, w1\n"; // Signed division
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case mod:
        move(o, params[1], "w0"); // dividend
        move(o, params[2], "w1"); // divisor
        o << "    sdiv w2, w0, w1\n";   // quotient in w2
        o << "    msub w0, w2, w1, w0\n"; // remainder = dividend - (quotient * divisor)
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case copyTblx: {
        // copyTblx: params[0] = base_offset (string literal number), params[1] = value (mem/imm), params[2] = index (mem/imm)
        // Address = fp - base_offset + index * 4
        move(o, params[2], "w1");      // Load index into w1
        move(o, params[1], "w0");      // Load value to store into w0
        o << "    lsl w2, w1, #2\n";         // w2 = index * 4
        o << "    sub x3, fp, #" << params[0] << "\n"; // w3 = fp - base_offset
        o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
        o << "    str w0, [x2]\n";           // Store value at the calculated address
        break;
    }
    case addTblx: {
        // addTblx: params[0]=base_offset, params[1]=value_to_add (mem/imm), params[2]=index (mem/imm)
        move(o, params[2], "w1");      // index
        move(o, params[1], "w0");      // value_to_add
        o << "    lsl w2, w1, #2\n";         // offset = index * 4
        o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
        o << "    ldr w3, [x2]\n";           // Load current array value into w3
        o << "    add w3, w3, w0\n";         // Add the value
        o << "    str w3, [x2]\n";           // Store back
        break;
    }
     case subTblx: {
        move(o, params[2], "w1");      // index
        move(o, params[1], "w0");      // value_to_sub
        o << "    lsl w2, w1, #2\n";         // offset = index * 4
        o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
        o << "    ldr w3, [x2]\n";           // Load current array value into w3
        o << "    sub w3, w3, w0\n";         // Subtract the value
        o << "    str w3, [x2]\n";           // Store back
        break;
    }
    case mulTblx: {
        move(o, params[2], "w1");      // index
        move(o, params[1], "w0");      // value_to_mul
        o << "    lsl w2, w1, #2\n";         // offset = index * 4
        o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
        o << "    ldr w3, [x2]\n";           // Load current array value into w3
        o << "    mul w3, w3, w0\n";         // Multiply the value
        o << "    str w3, [x2]\n";           // Store back
        break;
    }
    case divTblx: {
        move(o, params[2], "w1");      // index
        move(o, params[1], "w0");      // divisor
        o << "    lsl w2, w1, #2\n";         // offset = index * 4
        o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
        o << "    ldr w3, [x2]\n";           // Load current array value (dividend) into w3
        o << "    sdiv w3, w3, w0\n";        // Divide the value
        o << "    str w3, [x2]\n";           // Store back
        break;
    }
     case modTblx: {
        move(o, params[2], "w1");      // index
        move(o, params[1], "w0");      // divisor
        o << "    lsl w2, w1, #2\n";         // offset = index * 4
        o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
        o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
        o << "    ldr w3, [x2]\n";           // Load current array value (dividend) into w3
        o << "    sdiv w4, w3, w0\n";        // quotient in w4
        o << "    msub w3, w4, w0, w3\n";    // remainder = dividend - (quotient * divisor)
        o << "    str w3, [x2]\n";           // Store back
        break;
    }
    case getTblx: {
        // getTblx: params[0] = destination (mem), params[1] = base_offset (string literal number), params[2] = index (mem/imm)
        move(o, params[2], "w1");      // index
        o << "    lsl w2, w1, #2\n";         // offset = index * 4
        o << "    sub x3, fp, #" << params[1] << "\n"; // base addr = fp - base_offset
        o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
        o << "    ldr w0, [x2]\n";           // Load value from array element into w0
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    }
    case incr:
        // incr: params[0] = var (memory)
        move(o, params[0], "w0"); // Load variable into w0
        o << "    add w0, w0, #1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case decr:
        // decr: params[0] = var (memory)
        move(o, params[0], "w0"); // Load variable into w0
        o << "    sub w0, w0, #1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_eq: // ==
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, eq\n"; // Set w0 to 1 if eq, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case cmp_lt: // <
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, lt\n"; // Set w0 to 1 if lt, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case cmp_le: // <=
         move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, le\n"; // Set w0 to 1 if le, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_ne: // !=
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, ne\n"; // Set w0 to 1 if ne, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_gt: // >
         move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, gt\n"; // Set w0 to 1 if gt, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_ge: // >=
         move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, ge\n"; // Set w0 to 1 if ge, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case bit_and:
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    and w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case bit_or:
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    orr w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case bit_xor:
        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    eor w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case unary_minus:
        move(o, params[1], "w0");
        o << "    neg w0, w0\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case not_op: // logical not !
        move(o, params[1], "w0");
        o << "    cmp w0, #0\n";
        o << "    cset w0, eq\n"; // Set w0 to 1 if w0 == 0, else 0
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case log_and: { // &&
        int currentLabel = labelCounter++;
        std::string labelFalse = ".Lfalse" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        move(o, params[1], "w0");
        o << "    cmp w0, #0\n";
        o << "    b.eq " << labelFalse << "\n"; // if first is false, result is false

        move(o, params[2], "w0");
        o << "    cmp w0, #0\n";
        o << "    b.eq " << labelFalse << "\n"; // if second is false, result is false

        // Both are true
        o << "    mov w0, #1\n";
        o << "    b " << labelEnd << "\n";

        o << labelFalse << ":\n";
        o << "    mov w0, #0\n";

        o << labelEnd << ":\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    }
    case log_or: { // ||
        int currentLabel = labelCounter++;
        std::string labelTrue = ".Ltrue" + std::to_string(currentLabel);
        std::string labelEnd = ".Lend" + std::to_string(currentLabel);

        move(o, params[1], "w0");
        o << "    cmp w0, #0\n";
        o << "    b.ne " << labelTrue << "\n"; // if first is true, result is true

        move(o, params[2], "w0");
        o << "    cmp w0, #0\n";
        o << "    b.ne " << labelTrue << "\n"; // if second is true, result is true

        // Both are false
        o << "    mov w0, #0\n";
        o << "    b " << labelEnd << "\n";

        o << labelTrue << ":\n";
        o << "    mov w0, #1\n";

        o << labelEnd << ":\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    }

    case rmem:
        // rmem: params[0] = destination (mem), params[1] = address (mem/imm)
        move(o, params[1], "w1"); // Load address into w1
        o << "    ldr w0, [x1]\n";        // Load value from address in w1 into w0
        move(o, "w0", params[0]); // Move w0 to destination // Store value to destination
        break;

    case wmem:
        // wmem: params[0] = address (mem/imm), params[1] = value (mem/imm)
        move(o, params[0], "w1"); // Load address into w1
        move(o, params[1], "w0"); // Load value into w0
        o << "    str w0, [x1]\n";        // Store value w0 to address in w1
        break;

    case call:
        // call: params[0] = label, params[1] = destination (mem), params[2]... = parameters (assume setup before call)
        // Parameters should be loaded into w0-w7 by preceding instructions (e.g., copy)
        o << "    bl _" << params[0] << "\n"; // Branch with link (call)
        move(o, "w0", params[1]); // Move return value to w0
        break;

    case jmp:
        // jmp: params[0] = label
        o << "    b " << params[0] << "\n";
        break;

    default:
        o << "    // Unsupported IR operation for ARM64\n";
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


void CFG::gen_asm_prologue(std::ostream &o)
{
    // Standard ARM64 prologue
    // stp: store pair of registers
    // Stores frame pointer (x29) and link register (x30) to stack
    // Pre-indexed addressing: Decrements SP by 16 *before* storing. ! means update SP.
    o << "    stp fp, x30, [sp, #-16]!\n";
    // Set new frame pointer to current stack pointer
    o << "    mov fp, sp\n";

    // Allocate stack space for local variables. Must be multiple of 16.
    size_t stackSize = getStackSize();
    // Align stack size to 16 bytes
    stackSize = (stackSize + 15) & ~15;
    if (stackSize > 0) {
        o << "    sub sp, sp, #" << stackSize << "\n";
    }
}

void CFG::gen_asm_epilogue(std::ostream &o)
{
    // Standard ARM64 epilogue
    size_t stackSize = getStackSize();
    stackSize = (stackSize + 15) & ~15; // Align to 16 bytes, matching prologue

    // Deallocate stack space (alternative: mov sp, x29 if stack size fixed)
    if (stackSize > 0) {
        o << "    add sp, sp, #" << stackSize << "\n";
    }
    // Restore frame pointer and link register
    o << "    ldp x29, x30, [sp], #16\n";
    o << "    ret\n";
}

// Helper to check if a string represents an immediate constant ('#...')
bool CFG::isRegConstant(std::string& reg)
{
    return !reg.empty() && reg[0] == '#';
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

#endif // __aarch64__