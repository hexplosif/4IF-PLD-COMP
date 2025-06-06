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
// Floating point registers (caller-saved)
vector<string> floatRegs = {"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7"};
string returnReg = "w0"; // Register used for return value
string floatReturnReg = "s0"; // Register used for return value (float)

bool is_cst(const std::string& s) {
    return !s.empty() && s[0] == '#';
}

bool is_memory(const std::string& s) {
    return !s.empty() && s[0] == '[' && s.back() == ']';
}

bool is_register(const std::string& s) {
    return !s.empty() && (s[0] == 'w' || s[0] == 's') && std::all_of(s.begin() + 1, s.end(), ::isdigit);
}

bool is_global(const std::string& s) {
    return !s.empty() && s[0] == '_';
}

bool is_memory_big_offset(const std::string& s) {
    return (!s.empty()) && (s.size() > 3) && s[0] == ';' && s[1] == ';' && s[2] == ';' && s[3] == '[' && s.back() == ']';
}

std::string get_memory_offset(const std::string& s) {
    // s = ;;;[<reg>,#-<offset>]
    // Extract the offset from the memory address
    size_t start = s.find(',') + 4;
    size_t end = s.find(']');
    return s.substr(start, end - start);
}

std::string get_memory_register(const std::string& s) {
    // s = ;;;[<reg>,#<offset>]
    // Extract the register from the memory address
    size_t start = s.find('[') + 1; // Skip ;;;[
    size_t end = s.find(',');
    return s.substr(start, end - start);
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
        }
    } else {
        o << "    ; Unsupported move operation: " << src << " to " << dest << "\n";
    }
}

void fmove(std::ostream &o, const std::string& src, const std::string& dest) {
    if (is_register(src)) {
        if (is_register(dest)) {
            o << "    fmov " << dest << ", " << src << "\n";
        } else if (is_memory(dest)) {
            o << "    str " << src << ", " << dest << "\n";
        } else if (is_cst(dest)) {
            o << "    fmov " << dest << ", " << src << "\n";
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
            o << "    fmov " << dest << ", w9\n"; // Move to constant
        }
    } else if (is_cst(src)) {
        if (is_register(dest)) {
            o << "    fmov " << dest << ", " << src << "\n";
        } else if (is_memory(dest)) {
            o << "    fmov " << src << ", w9\n"; // Move to a temporary register
            o << "    str w9, " << dest << "\n"; // Store into destination memory
        }
    } else if (is_global(src)) {
        if (is_register(dest)) {
            o << "    adrp	x8, " << src << "@PAGE\n";
            o << "    ldr w1, [x8, " << src << "@PAGEOFF]" << "\n";
            o << "    fmov " << dest << ", w1\n";
        }
    } else {
        o << "    ; Unsupported move operation: " << src << " to " << dest << "\n";
    }
}

void IRInstr::gen_asm(std::ostream &o)
{
    static int labelCounter = 0;

    for (int i = 0; i < params.size(); i++) {
        if (is_memory_big_offset(params[i])) {
            // cout << "; Memory address with big offset: " << params[i] << "-> [x" << i + 2 << "]" << endl;
            std::string offset = get_memory_offset(params[i]);
            std::string reg = get_memory_register(params[i]);

            o << "    sub x" << i + 6 << ", " << reg << ", #" << offset << "\n"; // Calculate the address
            params[i] = std::string("[x") + std::to_string(i + 6) + "]"; // Update the parameter to use the calculated address
        }
    }

    switch (op)
    {
    case ldconst:
        // ldconst: params[0] = destination, params[1] = immediate constant (#val)
        move(o, params[1], "w9"); // Move immediate constant to destination
        move(o, "w9", params[0]); // Move w0 to destination
        break;
    case copy:
        // copy: params[0] = destination (memory), params[1] = source (memory or immediate)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, "s0", params[0]);
            break;
        }

        move(o, params[1], "w9"); // Move source to w0
        move(o, "w9", params[0]); // Move w0 to destination
        break;
    case add:
        // add: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fadd s0, s0, s1\n";
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    add w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case sub:
        // sub: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fsub s0, s0, s1\n";
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    sub w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case mul:
        // mul: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fmul s0, s0, s1\n";
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    mul w0, w0, w1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case div:
        // div: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fdiv s0, s0, s1\n";
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    sdiv w0, w0, w1\n"; // Signed division
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case mod:
        // mod: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)

        move(o, params[1], "w0"); // dividend
        move(o, params[2], "w1"); // divisor
        o << "    sdiv w2, w0, w1\n";   // quotient in w2
        o << "    msub w0, w2, w1, w0\n"; // remainder = dividend - (quotient * divisor)
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case copyTblx: {
        // copyTblx: params[0] = base_offset (string literal number), params[1] = value (mem/imm), params[2] = index (mem/imm)
        // Address = fp - base_offset + index * 4
        if (t == VarType::FLOAT_PTR) {
            move(o, params[2], "w5");      // Load index into w5
            fmove(o, params[1], "s0");      // Load value to store into s0
            o << "    lsl w2, w5, #2\n";         // w2 = index * 4
            o << "    sub x3, fp, #" << params[0] << "\n"; // w3 = fp - base_offset
            o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
            fmove(o, "s0", "[x2]");           // Move s0 to a temporary register
            break;
        }

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
        // Address = fp - base_offset + index * 4
        if (t == VarType::FLOAT_PTR) {
            move(o, params[2], "w5");      // index
            fmove(o, params[1], "s0");      // value_to_add
            o << "    lsl w2, w5, #2\n";         // offset = index * 4
            o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
            o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
            o << "    ldr s1, [x2]\n";           // Load current array value into s1
            o << "    fadd s1, s1, s0\n";         // Add the value
            o << "    str s1, [x2]\n";           // Store back
            break;
        }

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
        // subTblx: params[0]=base_offset, params[1]=value_to_sub (mem/imm), params[2]=index (mem/imm)
        // Address = fp - base_offset + index * 4
        if (t == VarType::FLOAT_PTR) {
            move(o, params[2], "w5");      // index
            fmove(o, params[1], "s0");      // value_to_sub
            o << "    lsl w2, w5, #2\n";         // offset = index * 4
            o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
            o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
            o << "    ldr s1, [x2]\n";           // Load current array value into s1
            o << "    fsub s1, s1, s0\n";         // Subtract the value
            o << "    str s1, [x2]\n";           // Store back
            break;
        }

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
        // mulTblx: params[0]=base_offset, params[1]=value_to_mul (mem/imm), params[2]=index (mem/imm)
        // Address = fp - base_offset + index * 4
        if (t == VarType::FLOAT_PTR) {
            move(o, params[2], "w5");      // index
            fmove(o, params[1], "s0");      // value_to_mul
            o << "    lsl w2, w5, #2\n";         // offset = index * 4
            o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
            o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
            o << "    ldr s1, [x2]\n";           // Load current array value into s1
            o << "    fmul s1, s1, s0\n";         // Multiply the value
            o << "    str s1, [x2]\n";           // Store back
            break;
        }

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
        // divTblx: params[0]=base_offset, params[1]=divisor (mem/imm), params[2]=index (mem/imm)
        // Address = fp - base_offset + index * 4
        if (t == VarType::FLOAT_PTR) {
            move(o, params[2], "w5");      // index
            fmove(o, params[1], "s0");      // divisor
            o << "    lsl w2, w5, #2\n";         // offset = index * 4
            o << "    sub x3, fp, #" << params[0] << "\n"; // base addr = fp - base_offset
            o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
            o << "    ldr s1, [x2]\n";           // Load current array value (dividend) into s1
            o << "    fdiv s1, s1, s0\n";         // Divide the value
            o << "    str s1, [x2]\n";           // Store back
            break;
        }

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
        // Address = fp - base_offset + index * 4
        if (t == VarType::FLOAT_PTR) {
            move(o, params[2], "w5");      // index
            o << "    lsl w2, w5, #2\n";         // offset = index * 4
            o << "    sub x3, fp, #" << params[1] << "\n"; // base addr = fp - base_offset
            o << "    add x2, x3, w2, uxtw\n";         // final addr = base + offset
            o << "    ldr s0, [x2]\n";           // Load value from array element into s0
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

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
        if (t == VarType::FLOAT) {
            fmove(o, params[0], "s0");
            fmove(o, "#1.00000000", "s1");
            o << "    fadd s0, s0, s1\n";
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

        move(o, params[0], "w0"); // Load variable into w0
        o << "    add w0, w0, #1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case decr:
        // decr: params[0] = var (memory)
        if (t == VarType::FLOAT) {
            fmove(o, params[0], "s0");
            fmove(o, "#1.00000000", "s1");
            o << "    fsub s0, s0, s1\n";
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

        move(o, params[0], "w0"); // Load variable into w0
        o << "    sub w0, w0, #1\n";
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_eq: // ==
        // cmp_eq: params[0] = destination, params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fcmp s0, s1\n";
            o << "    cset w8, eq\n"; // Set w0 to 1 if eq, 0 otherwise
            o << "    and w8, w8, #0x1\n"; // Mask to get the result
            move(o, "w8", params[0]); // Move w0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, eq\n"; // Set w0 to 1 if eq, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;
    case cmp_lt: // <
        // cmp_lt: params[0] = destination, params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fcmp s0, s1\n";
            o << "    cset w8, mi\n"; // Set w0 to 1 if lt, 0 otherwise
            o << "    and w8, w8, #0x1\n"; // Mask to get the result
            move(o, "w8", params[0]); // Move w0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, lt\n"; // Set w0 to 1 if lt, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_le: // <=
        // cmp_le: params[0] = destination, params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fcmp s0, s1\n";
            o << "    cset w8, ls\n"; // Set w0 to 1 if le, 0 otherwise
            o << "    and w8, w8, #0x1\n"; // Mask to get the result
            move(o, "w8", params[0]); // Move w0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, le\n"; // Set w0 to 1 if le, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_ne: // !=
        // cmp_ne: params[0] = destination, params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fcmp s0, s1\n";
            o << "    cset w8, ne\n"; // Set w0 to 1 if ne, 0 otherwise
            o << "    and w8, w8, #0x1\n"; // Mask to get the result
            move(o, "w8", params[0]); // Move w0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, ne\n"; // Set w0 to 1 if ne, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_gt: // >
        // cmp_gt: params[0] = destination, params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fcmp s0, s1\n";
            o << "    cset w8, gt\n"; // Set w0 to 1 if gt, 0 otherwise
            o << "    and w8, w8, #0x1\n"; // Mask to get the result
            move(o, "w8", params[0]); // Move w0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, gt\n"; // Set w0 to 1 if gt, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case cmp_ge: // >=
        // cmp_ge: params[0] = destination, params[1] = left (mem/imm), params[2] = right (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            fmove(o, params[2], "s1");
            o << "    fcmp s0, s1\n";
            o << "    cset w8, ge\n"; // Set w0 to 1 if ge, 0 otherwise
            o << "    and w8, w8, #0x1\n"; // Mask to get the result
            move(o, "w8", params[0]); // Move w0 to destination
            break;
        }

        move(o, params[1], "w0");
        move(o, params[2], "w1");
        o << "    cmp w0, w1\n";
        o << "    cset w0, ge\n"; // Set w0 to 1 if ge, 0 otherwise
        move(o, "w0", params[0]); // Move w0 to destination
        break;

    case bit_and:
        // bit_and: params[0] = dest (mem), params[1] = left (mem/imm), params[2] = right (mem/imm)
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
        // unary_minus: params[0] = destination, params[1] = source (mem/imm)
        if (t == VarType::FLOAT) {
            fmove(o, params[1], "s0");
            o << "    fneg s0, s0\n"; // Negate the float
            fmove(o, "s0", params[0]); // Move s0 to destination
            break;
        }

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

    case intToFloat:
        // intToFloat: params[0] = destination, params[1] = source
        move(o, params[1], "w0");
        o << "    scvtf s0, w0\n"; // Convert int to float
        fmove(o, "s0", params[0]); // Move s0 to destination
        break;

    case floatToInt:
        // floatToInt: params[0] = destination, params[1] = source
        fmove(o, params[1], "s0");
        o << "    fcvtzs w0, s0\n"; // Convert float to int
        move(o, "w0", params[0]); // Move w0 to destination
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
            if (p->getType() != VarType::FLOAT) {
                return "#" + p->getCstValue();
            }

            std::string label = rodm->putFloatIfNotExists(std::stof(p->getCstValue()));
            return "_" + label;
        }

        if (p->scopeType == GLOBAL) {
            return "_" + reg;

        } else {

            if (p->offset < 254) {
                // Return frame pointer relative address [fp, #offset]
                return "[fp, #-" + std::to_string(p->offset) + "]";
            }

            // With offset > 254, we can not use [fp, #offset] directly, so add ;;; to indicate
            // that this is a special case and needs to be handled differently
            return ";;;[fp, #-" + std::to_string(p->offset) + "]";
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

    o << "    .section   __DATA, __data\n";

    for (auto const& [name, symbol] : globalScope->getTable())
    {
        // Skip temporary variables if they exist in global scope (unlikely but possible)
        if (SymbolTable::isTempVariable(name))
            continue;

        o << "    .global _" << name << "\n"; // Export symbol
        o << "_" << name << ":\n"; // Label for the variable

        if (globalVariableValues.count(name) == 0) {
            // Uninitialized global variable (BSS segment effectively)
            o << "    .space 4\n"; // Reserve 4 bytes, initialized to zero by default
        } else {
            // Initialized global variable
            VarType t = symbol.type;
            std::string value = globalVariableValues[name];
            if (t == VarType::INT || t == VarType::CHAR) {
                o << "    .long " << value << "\n"; // Store integer value
            } else if (t == VarType::FLOAT) {
                float fValue = std::stof(value);
                std::string hexIEEEValue = floatToLong_Ieee754(fValue);
                o << "    .long 0x" << hexIEEEValue << "\t\t ; float " << fValue << "\n"; // Store float value
            } else {
                o << "    .long 0\n"; // Default to zero for unsupported types
            }
        }
    }
}

//* ---------------------- Read only data manager ---------------------- */
void RoDM::gen_asm(std::ostream &o)
{
    o << "    .section __TEXT,__const" << std::endl;
    for (const auto &pair : floatData)
    {
        o << "_" << pair.first << ":" << "         ; = " << pair.second << std::endl;
        std::string hexIEEEValue = floatToLong_Ieee754(pair.second);
        o << "    .word " << "0x" << hexIEEEValue << "\t\t ; float " << pair.second <<  std::endl;
    }

    // if (needDataForUnaryOp) {
    //     o << labelDataForUnaryOp << ":" << "         // = " << 0.0f << std::endl;
    //     o << "    .long -2147483648" << std::endl;
    //     o << "    .long 0" << std::endl;
    //     o << "    .long 0" << std::endl;
    //     o << "    .long 0" << std::endl;
    // }

    // o << "\t.text" << std::endl;
}

//* -------------------------- Code gen -------------------------------
void CodeGenVisitor::gen_asm(ostream &os)
{
    os << "    .section __TEXT, __text\n";
    os << "    .p2align 2\n";

    os << "\n;================================================ \n\n";
    for (auto &cfg : cfgs)
    {
        cfg->gen_asm(os);
        os << "\n;================================================= \n\n";
    }
    os << "; Global Variables \n";
    gvm->gen_asm(os);
    os << "\n;================================================= \n\n";
    os << "; Read only data \n";
    rodm->gen_asm(os);
}

#endif // __aarch64__