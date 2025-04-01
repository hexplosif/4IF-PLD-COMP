#ifndef IR_H
#define IR_H

#include <vector>
#include <string>
#include <ostream>
#include <iostream>
#include <initializer_list>
#include <map>       // Ajout pour std::map
#include "symbole.h"
#include "SymbolTable.h"  // Inclure notre table des symboles

class BasicBlock;
class CFG;
class DefFonction;


//! The class for one 3-address instruction
class IRInstr {
public:
    /** The instructions themselves -- feel free to subclass instead */
    typedef enum {
        ldconst,
        copy,
        add,
        sub,
        mul,
        div,
        mod,
        rmem,
        wmem, 
        cmp_eq,
        cmp_ne,
        cmp_lt,
        cmp_le,
        cmp_gt,
        cmp_ge,
        bit_and,
        bit_or,
        bit_xor,
        unary_minus,
        not_op,
        log_and,
        log_or,
        call,
        jmp
    } Operation;

    /**  constructor */
    IRInstr(BasicBlock* bb_, Operation op, VarType t, std::vector<std::string> params);

    /** Actual code generation */
    void gen_asm(std::ostream &o); /**< x86 assembly code generation for this IR instruction */

private:
    BasicBlock* bb; /**< The BB this instruction belongs to, which provides a pointer to the CFG this instruction belongs to */
    Operation op;
    VarType t;
    std::vector<std::string> params; /**< For 3-op instrs: d, x, y; for ldconst: d, c;  For call: label, d, params;  for wmem and rmem: choose yourself */
};




/**  The class for a basic block */
class BasicBlock {
public:
    BasicBlock(CFG* cfg, std::string entry_label);
    void gen_asm(std::ostream &o); /**< x86 assembly code generation for this basic block (very simple) */

    void add_IRInstr(IRInstr::Operation op, VarType t, std::vector<std::string> params);

    BasicBlock* exit_true;  /**< pointer to the next basic block, true branch. If nullptr, return from procedure */ 
    BasicBlock* exit_false; /**< pointer to the next basic block, false branch. If nullptr, the basic block ends with an unconditional jump */
    std::string label; /**< label of the BB, also will be the label in the generated code */
    CFG* cfg; /** < the CFG where this block belongs */
    std::vector<IRInstr*> instrs; /** < the instructions themselves. */
    std::string test_var_name;  /**< when generating IR code for an if(expr) or while(expr) etc,
                                     store here the name of the variable that holds the value of expr */
};




/** The class for the control flow graph, also includes the symbol table */
class CFG {
public:
    CFG(DefFonction* ast);

    DefFonction* ast; /**< The AST this CFG comes from */

    void add_bb(BasicBlock* bb);

    // x86 code generation: could be encapsulated in a processor class in a retargetable compiler
    void gen_asm(std::ostream& o);
    std::string IR_reg_to_asm(std::string& reg); /**< helper method: inputs a IR reg or input variable, returns e.g. "-24(%rbp)" for the proper value of 24 */
    void gen_asm_prologue(std::ostream& o);
    void gen_asm_epilogue(std::ostream& o);

    // basic block management
    std::string new_BB_name();
    BasicBlock* current_bb;

    // On remplace ces membres par notre instance de SymbolTable
    SymbolTable* currentScope = nullptr; /**< the symbol table of the current scope */

protected:

    int nextFreeSymbolIndex; /**< to allocate new symbols in the symbol table */
    int nextBBnumber; /**< just for naming */

    std::vector<BasicBlock*> bbs; /**< all the basic blocks of this CFG*/
};


/** The class to manage global variables */
class GVM { // Global Variable Manager
    public:
        GVM();
        void gen_asm(std::ostream& o);

        void addGlobalVariable(std::string name, std::string type);
        void setGlobalVariableValue(std::string name, int value);
        std::string addTempConstVariable(std::string type, int value);

        SymbolTable* getGlobalScope() { return globalScope; }

    protected:
        SymbolTable* globalScope; /**< the symbol table of the global scope */
        std::map<std::string, int> globalVariableValues; /**< the values of the global variables */
};

#endif
