#include "CodeGenVisitor.h"

int CodeGenVisitor::countDeclarations(antlr4::tree::ParseTree *tree)
{
    int count = 0;
    if (dynamic_cast<ifccParser::Sub_declContext *>(tree) != nullptr)
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
    // Compter toutes les déclarations dans tout le programme (même dans les blocs imbriqués).
    // Attention, les offsets ne sont pas global, mais pour chaque functions.
    // Ca veut dire chaque fonction a son propre offset pour les variables.
    // Donc il faut recalculer les offsets pour chaque fonction, pas dans le visitProg.

    // int totalVars = countDeclarations(ctx->block());
    // currentDeclIndex = 0;

    if (!ctx->decl_stmt().empty())
    { // si on a des variables globales
        std::cout << "    .data\n";
    }

    // Visiter tous decl_stmt
    for (auto decl : ctx->decl_stmt())
    {
        visit(decl);
    }

    // Prologue
#ifdef __APPLE__
    std::cout << ".globl _main\n";
    std::cout << "_main:\n";
#else
    std::cout << "    .text\n";
    std::cout << "    .globl main\n";
    std::cout << "main:\n";
#endif
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";

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
    // Creer un nouveau scope quand on entre un nouveau block
    int offset;
    if (currentScope->isGlobalScope())
    { // si ce bloc est une fonction, comme GlobalScope -> FunctionScope -> BlockScope
        // On calculate tous les variables dans la fonction et chercher le premier offset
        int totalVars = countDeclarations(ctx);
        offset = -totalVars * 4;

        // Allouer l'espace pour les variables (4 octets par variable, aligné à 16 octets)
        int stackSize = totalVars * 4;
        stackSize = (stackSize + 15) & ~15;
        if (stackSize > 0)
        {
            std::cout << "    subq $" << stackSize << ", %rsp\n";
        }
    }
    else
    { // sinon, c'est un blockscope
        // On laisse ce scope continue le offset de son parent
        offset = currentScope->getCurrentDeclOffset();
    }

    SymbolTable *parentScope = currentScope;
    currentScope = new SymbolTable(offset);
    currentScope->setParent(parentScope);

    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }

    // le block est finis, on synchronize le parent et ce scope; on revient vers scope de parent
    currentScope->getParent()->synchronize(currentScope);
    currentScope = currentScope->getParent();

    return 0;
}

// ==============================================================
//                          Statements
// ==============================================================

// Gestion de la déclaration d'une variable
antlrcpp::Any CodeGenVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx)
{
    std::string varType = ctx->type()->getText();
    for (auto sdecl : ctx->sub_decl())
    {
        visitSub_declWithType(sdecl, varType);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitSub_declWithType(ifccParser::Sub_declContext *ctx, std::string varType)
{
    std::string varName = ctx->VAR()->getText();

    std::string varAddr;
    if (currentScope->isGlobalScope())
    { // si on est dans le scope global
        currentScope->addGlobalVariable(varName, varType);

        // On vérifie si la variable est initialisée avec une constante
        if (ctx->expr() && !isExprIsConstant(ctx->expr()))
        {
            std::cerr << "error: global variable must be initialized with a constant\n";
            exit(1);
        }

        // on declare la variable
        std::cout << "    .globl " << varName << "\n";
        std::cout << varName << ":\n";

        if (ctx->expr())
        {
            int value = getConstantValueFromExpr(ctx->expr());
            std::cout << "    .long " << value << "\n";
        }
        else
        {
            std::cout << "    .zero 4\n";
        }
    }
    else
    { // sinon, on est dans le scope d'une fonction ou d'un block
        std::cout << "    #    Local Sub_decl: " << varName << std::endl;
        int offset = currentScope->addLocalVariable(varName, varType);
        varAddr = std::to_string(offset) + "(%rbp)";

        if (ctx->expr())
        {
            visit(ctx->expr());
            std::cout << "    movl %eax, " << offset << "(%rbp)" << "\n";
        }
    }

    return 0;
}

// Gestion de l'affectation à une variable déjà déclarée
antlrcpp::Any CodeGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    Parameters *var = currentScope->findVariable(varName);
    if (var == nullptr)
    {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }

    std::string op = ctx->op_assign()->getText();

    if (op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=") {
        if (var->scopeType == ScopeType::GLOBAL) {
            std::cout << "    movl " << varName << "(%rip), %eax\n";
        } else {
            std::cout << "    movl " << var->offset << "(%rbp), %eax\n";
        }
        std::cout << "    pushq %rax\n";
        visit(ctx->expr());
        std::cout << "    popq %rcx\n";

        std::cout << "    movl %eax, %ebx\n"; 
        std::cout << "    movl %ecx, %eax\n"; 
        std::cout << "    movl %ebx, %ecx\n"; 
        if (op == "+=") {
            std::cout << "    addl %ecx, %eax\n";
        } else if (op == "-=") {
            std::cout << "    subl %ecx, %eax\n";
        } else if (op == "*=") {
            std::cout << "    imull %ecx, %eax\n";
        } else if (op == "/=" || op == "%=") { 
            std::cout << "    cltd\n";            
            std::cout << "    idivl %ecx\n";      
            if (op == "%=") {
                std::cout << "    movl %edx, %eax\n"; 
            }
        }

        if (var->scopeType == ScopeType::GLOBAL) {
            std::cout << "    movl %eax, " << varName << "(%rip)\n";
        } else {
            std::cout << "    movl %eax, " << var->offset << "(%rbp)\n";
        }
    } else if (op == "=") {
        visit(ctx->expr());
        if (var->scopeType == ScopeType::GLOBAL) {
            std::cout << "    movl %eax, " << varName << "(%rip)\n";
        } else {
            std::cout << "    movl %eax, " << var->offset << "(%rbp)\n";
        }
    }

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

antlrcpp::Any CodeGenVisitor::visitLogiqueParesseuxExpression(ifccParser::LogiqueParesseuxExpressionContext *ctx)
{
    std::string op = ctx->op->getText();
    std::string labelTrue = generateLabel();
    std::string labelEnd = generateLabel();

    if (op == "&&")
    {
        visit(ctx->expr(0)); // Évaluer le premier opérande
        std::cout << "    cmpl $0, %eax\n";
        std::cout << "    je " << labelEnd << "\n"; // Sauter à la fin si faux

        visit(ctx->expr(1)); // Évaluer le deuxième opérande
        std::cout << "    cmpl $0, %eax\n";
        std::cout << "    je " << labelEnd << "\n"; // Sauter à la fin si faux

        std::cout << labelTrue << ":\n";
        std::cout << "    movl $1, %eax\n"; // Les deux opérandes sont vrais
        std::cout << "    jmp " << labelEnd << "\n";

        std::cout << labelEnd << ":\n";
        std::cout << "    movl $0, %eax\n"; // Au moins un opérande est faux
    }
    else if (op == "||")
    {
        visit(ctx->expr(0)); // Évaluer le premier opérande
        std::cout << "    cmpl $0, %eax\n";
        std::cout << "    jne " << labelTrue << "\n"; // Sauter à vrai si non zéro

        visit(ctx->expr(1)); // Évaluer le deuxième opérande
        std::cout << "    cmpl $0, %eax\n";
        std::cout << "    jne " << labelTrue << "\n"; // Sauter à vrai si non zéro

        std::cout << "    movl $0, %eax\n"; // Les deux opérandes sont faux
        std::cout << "    jmp " << labelEnd << "\n";

        std::cout << labelTrue << ":\n";
        std::cout << "    movl $1, %eax\n"; // Au moins un opérande est vrai

        std::cout << labelEnd << ":\n";
    }

    return 0;
}

std::string CodeGenVisitor::generateLabel()
{
    static int labelCounter = 0;
    return ".L" + std::to_string(labelCounter++);
}

// Gestion de l'accès à une variable
antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    Parameters *var = currentScope->findVariable(varName);
    if (var == nullptr)
    {
        std::cerr << "error: variable " << varName << " not declared\n";
        exit(1);
    }

    if (var->scopeType == ScopeType::GLOBAL)
    {
        std::cout << "    movl " << varName << "(%rip), %eax" << "\n";
    }
    else
    {
        std::cout << "    movl " << var->offset << "(%rbp), %eax" << "\n";
    }

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

//Increment and Decrement

antlrcpp::Any CodeGenVisitor::visitPostIncrementExpression(ifccParser::PostIncrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    Parameters *var = currentScope->findVariable(varName);
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }

    if (var->scopeType == ScopeType::GLOBAL) {
        std::cout << "    movl " << varName << "(%rip), %eax\n"; 
        std::cout << "    addl $1, " << varName << "(%rip)\n"; 
    } else {
        std::cout << "    movl " << var->offset << "(%rbp), %eax\n"; 
        std::cout << "    addl $1, " << var->offset << "(%rbp)\n"; 
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitPostDecrementExpression(ifccParser::PostDecrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    Parameters *var = currentScope->findVariable(varName);
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }

    if (var->scopeType == ScopeType::GLOBAL) {
        std::cout << "    movl " << varName << "(%rip), %eax\n"; 
        std::cout << "    subl $1, " << varName << "(%rip)\n";  
    } else {
        std::cout << "    movl " << var->offset << "(%rbp), %eax\n";
        std::cout << "    subl $1, " << var->offset << "(%rbp)\n";  
    }

    return 0;
}

// ==============================================================
//                          Others
// ==============================================================
bool CodeGenVisitor::isExprIsConstant(ifccParser::ExprContext *ctx)
{
    if (dynamic_cast<ifccParser::ConstantExpressionContext *>(ctx) != nullptr ||
        dynamic_cast<ifccParser::ConstantCharExpressionContext *>(ctx) != nullptr)
    {
        return true;
    }
    return false;
}

int CodeGenVisitor::getConstantValueFromExpr(ifccParser::ExprContext *ctx)
{
    if (auto constExpr = dynamic_cast<ifccParser::ConstantExpressionContext *>(ctx))
    {
        std::string constText = constExpr->CONST()->getText();
        return std::stoi(constText);
    }
    // TODO: get case of char

    return 0;
}