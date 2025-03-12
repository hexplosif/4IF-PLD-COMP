#include "CodeGenVisitor.h"
#include <map>
#include <iostream>

std::map<std::string, int> CodeGenVisitor::symbolTable;
int CodeGenVisitor::stackOffset = 0;

// Fonction pour ajouter une nouvelle variable à la table des symboles
antlrcpp::Any CodeGenVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx) {
    for (auto varDecl : ctx->var_decl_list()->var_decl()) {
        std::string varName = varDecl->VAR()->getText();

<<<<<<< HEAD
        // Allouer de l'espace sur la pile
        stackOffset -= 4;
        symbolTable[varName] = stackOffset;

        // Vérifier s'il y a une initialisation
        if (varDecl->expr()) {
            if (varDecl->expr()->CONST()) {
                int value = std::stoi(varDecl->expr()->CONST()->getText());
                std::cout << "    movl $" << value << ", " << stackOffset << "(%rbp)" << std::endl;
            } else if (varDecl->expr()->VAR()) {
                std::string sourceVar = varDecl->expr()->VAR()->getText();
                std::cout << "    movl " << symbolTable[sourceVar] << "(%rbp), %eax" << std::endl;
                std::cout << "    movl %eax, " << stackOffset << "(%rbp)" << std::endl;
            }
=======
    // Vérifier si une initialisation est présente
    if (ctx->expr()) {
        if (ctx->expr()->CONST()) {
            int value = std::stoi(ctx->expr()->CONST()->getText());
            std::cout << "    movl $" << value << ", " << stackOffset << "(%rbp)" << std::endl;
        } else if (ctx->expr()->VAR()) {
            std::string sourceVar = ctx->expr()->VAR()->getText();
            std::cout << "    movl " << symbolTable[sourceVar] << "(%rbp), %eax" << std::endl;
            std::cout << "    movl %eax, " << stackOffset << "(%rbp)" << std::endl;
>>>>>>> 51710e0c0cb45c698b4aa70986ce156239b6bc81
        }
    }
    return 0;
}


<<<<<<< HEAD

antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
    int count = ctx->assign_expr().size(); // Nombre d'affectations
    for (int i = 0; i < count; i++) {
        std::string varName = ctx->assign_expr(i)->VAR()->getText();
        
        // Si l'expression est une constante
        if (ctx->assign_expr(i)->expr()->CONST()) {
            int value = std::stoi(ctx->assign_expr(i)->expr()->CONST()->getText());
            std::cout << "    movl $" << value << ", " << symbolTable[varName] << "(%rbp)" << std::endl;
        } 
        // Si l'expression est une variable
        else if (ctx->assign_expr(i)->expr()->VAR()) {
            std::string sourceVar = ctx->assign_expr(i)->expr()->VAR()->getText();
            std::cout << "    movl " << symbolTable[sourceVar] << "(%rbp), %eax" << std::endl;
            std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)" << std::endl;
        }
    }
=======
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

>>>>>>> 51710e0c0cb45c698b4aa70986ce156239b6bc81
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
