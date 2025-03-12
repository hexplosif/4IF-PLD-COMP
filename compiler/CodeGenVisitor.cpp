#include "CodeGenVisitor.h"
#include <map>
#include <iostream>

std::map<std::string, int> CodeGenVisitor::symbolTable;
int CodeGenVisitor::stackOffset = 0;

// Fonction pour ajouter une nouvelle variable à la table des symboles
antlrcpp::Any CodeGenVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    
    // Décrémenter l'offset de 4 pour allouer de la place sur la pile pour chaque variable
    stackOffset -= 4;
    symbolTable[varName] = stackOffset;

    // Vérifier si une initialisation est présente
    if (ctx->expr()) {
        if (ctx->expr()->CONST()) {
            int value = std::stoi(ctx->expr()->CONST()->getText());
            std::cout << "    movl $" << value << ", " << stackOffset << "(%rbp)" << std::endl;
        } else if (ctx->expr()->VAR()) {
            std::string sourceVar = ctx->expr()->VAR()->getText();
            std::cout << "    movl " << symbolTable[sourceVar] << "(%rbp), %eax" << std::endl;
            std::cout << "    movl %eax, " << stackOffset << "(%rbp)" << std::endl;
        }
    }
    
    return 0;
}


antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    
    if (ctx->expr()->CONST()) {
        // Cas où l'affectation est une constante (ex: a = 3)
        int value = std::stoi(ctx->expr()->CONST()->getText());
        std::cout << "    movl $" << value << ", " << symbolTable[varName] << "(%rbp)" << std::endl;
    } else if (ctx->expr()->VAR()) {
        // Cas où l'affectation est une variable (ex: b = a)
        std::string sourceVar = ctx->expr()->VAR()->getText();
        std::cout << "    movl " << symbolTable[sourceVar] << "(%rbp), %eax" << std::endl;
        std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)" << std::endl;
    }

    return 0;
}

// Fonction pour générer le code du return (ex : return a;)
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    // Vérifier si l'expression est une constante ou une variable
    if (ctx->expr()->CONST()) {
        // Si c'est une constante, utiliser `stoi` pour récupérer sa valeur
        int retval = std::stoi(ctx->expr()->CONST()->getText());
        std::cout << "    movl $" << retval << ", %eax\n";
    } else if (ctx->expr()->VAR()) {
        // Si c'est une variable, récupérer l'offset dans la table des symboles
        std::string varName = ctx->expr()->VAR()->getText();
        int offset = symbolTable[varName];
        std::cout << "    movl " << offset << "(%rbp), %eax\n";
    }

    return 0;
}

// Fonction pour gérer les expressions (constantes et variables)
antlrcpp::Any CodeGenVisitor::visitExpr(ifccParser::ExprContext *ctx) {
    if (ctx->CONST()) {
        // Si c'est une constante, on la charge directement dans %eax
        int value = std::stoi(ctx->CONST()->getText());
        std::cout << "    movl $" << value << ", %eax" << std::endl;
    } else if (ctx->VAR()) {
        // Si c'est une variable, on charge sa valeur depuis la pile
        std::string varName = ctx->VAR()->getText();
        std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax" << std::endl;
    }
    return 0;
}

// Générer le code pour le programme entier
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    std::cout << ".globl main\n";
    std::cout << " main: \n";

    // Prologue
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";

    // Visite les déclarations et affectations
    for (auto stmt : ctx->stmt()) {
        this->visit(stmt);
    }

    // Retour
    this->visit(ctx->return_stmt());

    // Epilogue
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}
