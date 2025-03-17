#include "CodeGenVisitor.h"

// Pour rappel : pour N variables, la 1ère déclarée aura offset = -4 * N, la 2ème = -4 * (N-1), ..., la dernière = -4.
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    // Première passe : compter le nombre de variables déclarées
    totalVars = 0;
    for (auto stmt : ctx->stmt()) {
        if (auto decl = dynamic_cast<ifccParser::Decl_stmtContext *>(stmt)) {
            totalVars++;
        }
    }
    currentDeclIndex = 0; // Réinitialiser le compteur

    // Prologue
    #ifdef __APPLE__
    std::cout << ".globl _main\n";
    std::cout << "_main:\n";
    #else
    std::cout << ".globl main\n";
    std::cout << "main:\n";
    #endif

    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";

    // Allouer l'espace pour les variables (totalVars * 4, aligné à 16 octets)
    int stackSize = totalVars * 4;
    stackSize = (stackSize + 15) & ~15; // alignement à 16
    if (stackSize > 0) {
        std::cout << "    subq $" << stackSize << ", %rsp\n";
    }

    // Visiter toutes les instructions
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }

    visit(ctx->return_stmt());

    // Épilogue
    std::cout << "    movq %rbp, %rsp\n"; // Restaurer rsp
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    // Incrémenter le compteur de déclaration
    currentDeclIndex++;
    // Calculer l'offset : première déclaration -> -4*totalVars, dernière -> -4*1
    int offset = -4 * (totalVars - currentDeclIndex + 1);
    variables[varName] = offset;

    if (ctx->expr()) {
        visit(ctx->expr()); // Évaluer l'expression
        std::cout << "    movl %eax, " << offset << "(%rbp)" << "\n";
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    if (variables.find(varName) == variables.end()) {
        std::cerr << "error: variable " << varName << " not declared\n";
        exit(1);
    }
    visit(ctx->expr());
    std::cout << "    movl %eax, " << variables[varName] << "(%rbp)" << "\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitwiseAndExpression(ifccParser::BitwiseAndExpressionContext *ctx) {
    visit(ctx->expr(0));  
    std::cout << "    pushq %rax\n";  // Sauvegarder le résultat
    visit(ctx->expr(1));  
    std::cout << "    popq %rcx\n";   // Récupérer le premier opérande dans %rcx
    std::cout << "    andl %ecx, %eax\n"; // Faire le AND
    return 0;
    }

antlrcpp::Any CodeGenVisitor::visitBitwiseOrExpression(ifccParser::BitwiseOrExpressionContext *ctx) {
    visit(ctx->expr(0));  
    std::cout << "    pushq %rax\n";  // Sauvegarder le résultat
    visit(ctx->expr(1));  
    std::cout << "    popq %rcx\n";   // Récupérer le premier opérande dans %rcx
    std::cout << "    orl %ecx, %eax\n"; // Faire le OR
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitwiseXorExpression(ifccParser::BitwiseXorExpressionContext *ctx) {
    visit(ctx->expr(0));  
    std::cout << "    pushq %rax\n";  // Sauvegarder le résultat
    visit(ctx->expr(1));  
    std::cout << "    popq %rcx\n";   // Récupérer le premier opérande dans %rcx
    std::cout << "    xorl %ecx, %eax\n"; // Faire le XOR
    return 0;
}


antlrcpp::Any CodeGenVisitor::visitUnaryLogicalNotExpression(ifccParser::UnaryLogicalNotExpressionContext *ctx) {
    visit(ctx->expr()); // Évaluer l'expression
    std::string op = ctx->op->getText();
    if (op == "-") {
        std::cout << "    negl %eax\n"; // Négation arithmétique
    } else if (op == "!") {
        std::cout << "    cmpl $0, %eax\n"; // Comparer avec 0
        std::cout << "    sete %al\n"; // Mettre %al à 1 si %eax est 0
        std::cout << "    movzbl %al, %eax\n"; 
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    if (ctx->op->getText() == "+") {
        
        // a + b : évaluer a puis b
        visit(ctx->expr(0)); // évalue a, résultat dans %eax
        std::cout << "    pushq %rax\n";  // sauvegarde a
        visit(ctx->expr(1)); // évalue b, résultat dans %eax
        std::cout << "    popq %rcx\n";   // récupère a dans %rcx
        std::cout << "    addl %ecx, %eax\n"; // %eax = a + b
    }
    else if (ctx->op->getText() == "-") {
        // a - b : évaluer a puis b
        visit(ctx->expr(0));         // évalue a, résultat dans %eax
        std::cout << "    pushq %rax\n";  // sauvegarde a
        visit(ctx->expr(1));         // évalue b, résultat dans %eax
        std::cout << "    popq %rcx\n";   // récupère a dans %rcx
        std::cout << "    subl %eax, %ecx\n"; // %ecx = a - b
        std::cout << "    movl %ecx, %eax\n";  // mettre le résultat dans %eax
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitMulExpression(ifccParser::MulExpressionContext *ctx) {
    visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1));
    std::cout << "    popq %rcx\n";
    std::cout << "    imull %ecx, %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    if (variables.find(varName) == variables.end()) {
        std::cerr << "error: variable " << varName << " not declared\n";
        exit(1);
    }
    std::cout << "    movl " << variables[varName] << "(%rbp), %eax" << "\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx) {
    int value = stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << value << ", %eax" << "\n";
    return 0;
}


antlrcpp::Any CodeGenVisitor::visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx) {
    visit(ctx->expr(0)); // Évalue l'opérande gauche
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1)); // Évalue l'opérande droite
    std::cout << "    popq %rcx\n";
    
    std::cout << "    cmpl %eax, %ecx\n"; // Compare ecx (gauche) et eax (droite)
    
    std::string op = ctx->op->getText();
    
    if (op == "==") {
        std::cout << "    sete %al\n";
    } else if (op == "!=") {
        std::cout << "    setne %al\n";
    } else if (op == "<") {
        std::cout << "    setl %al\n";
    } else if (op == ">") {
        std::cout << "    setg %al\n";
    } else if (op == "<=") {                // Bonus
        std::cout << "    setle %al\n";
    } else if (op == ">=") {                // Bonus
        std::cout << "    setge %al\n";
    }
    
    std::cout << "    movzbl %al, %eax\n"; // Convertit le résultat booléen en 32 bits
    
    return 0;
}