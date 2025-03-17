#include "CodeGenVisitor.h"

int CodeGenVisitor::countDeclarations(antlr4::tree::ParseTree *tree)
{
    int count = 0;
    if (dynamic_cast<ifccParser::Decl_stmtContext *>(tree) != nullptr)
    {
        count++;
    }
    // Utiliser la propriété children pour itérer sur les sous-arbres
    for (auto child : tree->children)
    {
        count += countDeclarations(child);
    }
    return count;
}

// La fonction visitProg gère le prologue, la réservation de la pile et l'appel au bloc principal.
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    // Compter toutes les déclarations dans tout le programme (même dans les blocs imbriqués)
    totalVars = countDeclarations(ctx->block());
    currentDeclIndex = 0;

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

    // Allouer l'espace pour les variables (4 octets par variable, aligné à 16 octets)
    int stackSize = totalVars * 4;
    stackSize = (stackSize + 15) & ~15;
    if (stackSize > 0)
        std::cout << "    subq $" << stackSize << ", %rsp\n";

    // Visiter le bloc principal
    visit(ctx->block());

    // Épilogue
    std::cout << ".Lepilogue:\n";         // epilogue label
    std::cout << "    movq %rbp, %rsp\n"; // Restaurer rsp
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}

// Parcourt un bloc délimité par '{' et '}'
antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }
    return 0;
}

// ==============================================================
//                          Statements
// ==============================================================

// Gestion de la déclaration d'une variable
antlrcpp::Any CodeGenVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx)
{
    std::string varType = ctx->type()->getText();
    std::string varName = ctx->VAR()->getText();
    currentDeclIndex++;
    // Calcul de l'offset : première déclaration -> -4*totalVars, dernière -> -4
    int offset = -4 * (totalVars - currentDeclIndex + 1);
    variables[varName] = Parameters{varType, offset};

    if (ctx->expr())
    {
        visit(ctx->expr());
        std::cout << "    movl %eax, " << offset << "(%rbp)" << "\n";
    }
    return 0;
}

// Gestion de l'affectation à une variable déjà déclarée
antlrcpp::Any CodeGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    if (variables.find(varName) == variables.end())
    {
        std::cerr << "error: variable " << varName << " not declared\n";
        exit(1);
    }
    visit(ctx->expr());
    std::cout << "    movl %eax, " << variables[varName].offset << "(%rbp)" << "\n";
    return 0;
}

// Gestion de l'instruction return
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    visit(ctx->expr());
    std::cout << "    jmp .Lepilogue\n"; // Aller à l'épilogue
    return 0;
}

// ==============================================================
//                          Expressions
// ==============================================================

antlrcpp::Any CodeGenVisitor::visitUnaryLogicalNotExpression(ifccParser::UnaryLogicalNotExpressionContext *ctx)
{
    visit(ctx->expr()); // Évaluer l'expression
    std::string op = ctx->op->getText();
    if (op == "-")
    {
        std::cout << "    negl %eax\n"; // Négation arithmétique
    }
    else if (op == "!")
    {
        std::cout << "    cmpl $0, %eax\n"; // Comparer avec 0
        std::cout << "    sete %al\n";      // Mettre %al à 1 si %eax est 0
        std::cout << "    movzbl %al, %eax\n";
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    if (ctx->op->getText() == "+")
    {
        visit(ctx->expr(0));                  // Évalue le premier opérande
        std::cout << "    pushq %rax\n";      // Sauvegarde du résultat
        visit(ctx->expr(1));                  // Évalue le second opérande
        std::cout << "    popq %rcx\n";       // Récupère le premier opérande
        std::cout << "    addl %ecx, %eax\n"; // Additionne
    }
    else if (ctx->op->getText() == "-")
    {
        visit(ctx->expr(0));
        std::cout << "    pushq %rax\n";
        visit(ctx->expr(1));
        std::cout << "    popq %rcx\n";
        std::cout << "    subl %eax, %ecx\n";
        std::cout << "    movl %ecx, %eax\n";
    }
    return 0;
}

// Gestion des expressions multiplication
antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    std::string op = ctx->OPM()->getText();

    // Evaluer a (opérande gauche)
    visit(ctx->expr(0));             // a dans %eax
    std::cout << "    pushq %rax\n"; // Empiler a

    // Evaluer b (opérande droite)
    visit(ctx->expr(1)); // b dans %eax

    if (op == "*")
    {
        // Dépiler a dans %rcx
        std::cout << "    popq %rcx\n";
        std::cout << "    imull %ecx, %eax\n";
    }
    else if (op == "/" || op == "%")
    {
        std::cout << "    pushq %rax\n"; // Empiler b
        std::cout << "    popq %rcx\n";  // Dépiler b dans %rcx
        std::cout << "    popq %rax\n";  // Dépiler a dans %rax

        std::cout << "    cltd\n";       // sign extend %eax to %edx:%eax
        std::cout << "    idivl %ecx\n"; // %eax = a /b , %edx = a % b

        if (op == "%")
        {
            std::cout << "    movl %edx, %eax\n"; // %eax = a % b
        }
    }

    return 0;
}

// Gestion des opérations AND bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseAndExpression(ifccParser::BitwiseAndExpressionContext *ctx)
{
    visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1));
    std::cout << "    popq %rcx\n";
    std::cout << "    andl %ecx, %eax\n";
    return 0;
}

// Gestion des opérations OR bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseOrExpression(ifccParser::BitwiseOrExpressionContext *ctx)
{
    visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1));
    std::cout << "    popq %rcx\n";
    std::cout << "    orl %ecx, %eax\n";
    return 0;
}

// Gestion des opérations XOR bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseXorExpression(ifccParser::BitwiseXorExpressionContext *ctx)
{
    visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1));
    std::cout << "    popq %rcx\n";
    std::cout << "    xorl %ecx, %eax\n";
    return 0;
}

// Gestion de l'accès à une variable
antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    if (variables.find(varName) == variables.end())
    {
        std::cerr << "error: variable " << varName << " not declared\n";
        exit(1);
    }
    std::cout << "    movl " << variables[varName].offset << "(%rbp), %eax" << "\n";

    return 0;
}

// Gestion des constantes
antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    int value = std::stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << value << ", %eax" << "\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx)
{
    std::string value = ctx->CONST_CHAR()->getText().substr(1, 1);
    int convertValue = value[0];
    std::cout << "    movl $" << convertValue << ", %eax" << "\n";
    return 0;
}

// Gestion des expressions de comparaison

antlrcpp::Any CodeGenVisitor::visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx)
{
    visit(ctx->expr(0)); // Évalue l'opérande gauche
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1)); // Évalue l'opérande droite
    std::cout << "    popq %rcx\n";
    std::cout << "    cmpl %eax, %ecx\n";

    std::string op = ctx->op->getText();
    if (op == "==")
    {
        std::cout << "    sete %al\n";
    }
    else if (op == "!=")
    {
        std::cout << "    setne %al\n";
    }
    else if (op == "<")
    {
        std::cout << "    setl %al\n";
    }
    else if (op == ">")
    {
        std::cout << "    setg %al\n";
    }
    else if (op == "<=")
    {
        std::cout << "    setle %al\n";
    }
    else if (op == ">=")
    {
        std::cout << "    setge %al\n";
    }
    std::cout << "    movzbl %al, %eax\n"; // Conversion en entier 32 bits
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallExpression(ifccParser::FunctionCallExpressionContext *ctx)
{
    // Get the function name from the VAR token
    std::string funcName = ctx->VAR()->getText();

    // Get the list of argument expressions
    auto args = ctx->expr();

    // Check if the function call has more than 6 arguments.
    if (args.size() > 6)
    {
        std::cerr << "Error: function call with more than 6 arguments not supported." << std::endl;
        exit(1);
    }

    // Evaluate each argument and move the result into the appropriate register.
    // According to the Linux System V AMD64 ABI, the first six integer arguments go in:
    // 1st: %rdi, 2nd: %rsi, 3rd: %rdx, 4th: %rcx, 5th: %r8, 6th: %r9.
    // Here we assume that the result of an expression is left in %eax, so we move
    // that 32-bit value into the corresponding register (using the "d" suffix for the lower 32 bits).
    for (size_t i = 0; i < args.size(); i++)
    {
        visit(args[i]);
        switch (i)
        {
        case 0:
            std::cout << "    movl %eax, %edi\n";
            break;
        case 1:
            std::cout << "    movl %eax, %esi\n";
            break;
        case 2:
            std::cout << "    movl %eax, %edx\n";
            break;
        case 3:
            std::cout << "    movl %eax, %ecx\n";
            break;
        case 4:
            std::cout << "    movl %eax, %r8d\n";
            break;
        case 5:
            std::cout << "    movl %eax, %r9d\n";
            break;
        }
    }

    // Finally, generate the call instruction
    std::cout << "    call " << funcName << "\n";

    // The function's return value (if any) will be in %eax.
    return 0;
}
