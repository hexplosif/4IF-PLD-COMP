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
        std::cout << "    .data" << std::endl;
    }

    // Visiter tous decl_stmt
    for (auto decl : ctx->decl_stmt())
    {
        visit(decl);
    }

    // Prologue
#ifdef __APPLE__
    std::cout << ".globl _main" << std::endl;
    std::cout << "_main:" << std::endl;
#else
    std::cout << "    .text" << std::endl;
    std::cout << "    .globl main" << std::endl;
    std::cout << "main:" << std::endl;
#endif
    std::cout << "    pushq %rbp" << std::endl;
    std::cout << "    movq %rsp, %rbp" << std::endl;

    visit(ctx->block());

    // Épilogue
    std::cout << ".Lepilogue:" << std::endl;         // epilogue label
    std::cout << "    movq %rbp, %rsp" << std::endl; // Restaurer rsp
    std::cout << "    popq %rbp" << std::endl;
    std::cout << "    ret" << std::endl;

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
            std::cout << "    subq $" << stackSize << ", %rsp" << std::endl;
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
    if (ctx->CONST() != nullptr) { // C'est un tableaux
        int arraySize = std::stoi(ctx->CONST()->getText()); // Taille du tableau

        if (currentScope->isGlobalScope()) {
            currentScope->addGlobalVariable(varName, varType);
            std::cout << "    .globl " << varName << std::endl;
            std::cout << varName << ":" << std::endl;
            std::cout << "    .zero " << (arraySize * 4) << std::endl; // Réserver de l'espace pour le tableau
        } else {
            int arraySize = std::stoi(ctx->CONST()->getText());
            int arrayBytes = arraySize * 4;
            int offset = currentScope->addLocalVariable(varName, varType, arrayBytes, true); 
            std::cout << "    subq $" << arrayBytes << ", %rsp" << std::endl;
        }
    }
    else {
        std::string varAddr;
        if (currentScope->isGlobalScope())
        { // si on est dans le scope global
            currentScope->addGlobalVariable(varName, varType);

            // On vérifie si la variable est initialisée avec une constante
            if (ctx->expr() && !isExprIsConstant(ctx->expr()))
            {
                std::cerr << "error: global variable must be initialized with a constant" << std::endl;
                exit(1);
            }

            // on declare la variable
            std::cout << "    .globl " << varName << std::endl;
            std::cout << varName << ":" << std::endl;

            if (ctx->expr())
            {
                int value = getConstantValueFromExpr(ctx->expr());
                std::cout << "    .long " << value << std::endl;
            }
            else
            {
                std::cout << "    .zero 4" << std::endl;
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
                std::cout << "    movl %eax, " << offset << "(%rbp)" << std::endl;
            }
        }
    }
    return 0;
}

// Gestion de l'affectation à une variable déjà déclarée
antlrcpp::Any CodeGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx)
{
    if (ctx->VAR() != nullptr && ctx->expr(0) != nullptr && ctx->getText().find("[") != std::string::npos) {
        std::string arrayName = ctx->VAR()->getText(); // Nom du tableau
        SymbolParameters *array = currentScope->findVariable(arrayName);
        if (array == nullptr) {
            std::cerr << "error: array " << arrayName << " not declared." << std::endl;
            exit(1);
        }

        // Évaluer l'indice du tableau
        visit(ctx->expr(0)); // L'indice est évalué dans %eax
        std::cout << "    pushq %rax" << std::endl;
        // Évaluer la valeur à affecter
        visit(ctx->expr(1)); // La valeur est évaluée dans %eax

        std::string op = ctx->op_assign()->getText();
        std::cout << "    popq %rcx" << std::endl;

        if (op == "=") {
            // Affectation simple
            if (array->scopeType == SymbolScopeType::GLOBAL) {
                std::cout << "    movl %eax, " << arrayName << "(,%ecx,4)" << std::endl;
            } else {
                std::cout << "    movl %eax, " << array->offset << "(%rbp,%rcx,4)" << std::endl;
            }
        } else if (op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=") {
            // Opérateurs composés
            if (array->scopeType == SymbolScopeType::GLOBAL) {
                std::cout << "    movl " << arrayName << "(,%ecx,4), %eax" << std::endl;
            } else {
                std::cout << "    movl " << array->offset << "(%rbp,%ecx,4), %eax" << std::endl;
            }
            std::cout << "    pushq %rax" << std::endl; // Sauvegarder la valeur actuelle dans la pile
            visit(ctx->expr(1)); // Évaluer la nouvelle expression
            std::cout << "    popq %rcx" << std::endl; // Récupérer la valeur actuelle dans %rcx

            if (op == "+=") {
                std::cout << "    addl %ecx, %eax" << std::endl;
            } else if (op == "-=") {
                std::cout << "    subl %ecx, %eax" << std::endl;
            } else if (op == "*=") {
                std::cout << "    imull %ecx, %eax" << std::endl;
            } else if (op == "/=" || op == "%=") {
                std::cout << "    cltd" << std::endl;
                std::cout << "    idivl %ecx" << std::endl;
                if (op == "%=") {
                    std::cout << "    movl %edx, %eax" << std::endl;
                }
            }

            // Sauvegarder le résultat dans le tableau
            if (array->scopeType == SymbolScopeType::GLOBAL) {
                std::cout << "    movl %eax, " << arrayName << "(,%ecx,4)" << std::endl;
            } else {
                std::cout << "    movl %eax, " << array->offset << "(%rbp,%ecx,4)" << std::endl;
            }
        }
    }
    // Affectation à une variable simple
    else {
        std::string varName = ctx->VAR()->getText();
        SymbolParameters *var = currentScope->findVariable(varName);
        if (var == nullptr)
        {
            std::cerr << "error: variable " << varName << " not declared." << std::endl;
            exit(1);
        }

        std::string op = ctx->op_assign()->getText();

        if (op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=")
        {
            if (var->scopeType == SymbolScopeType::GLOBAL)
            {
                std::cout << "    movl " << varName << "(%rip), %eax" << std::endl;
            }
            else
            {
                std::cout << "    movl " << var->offset << "(%rbp), %eax" << std::endl;
            }
            std::cout << "    pushq %rax" << std::endl;
            visit(ctx->expr(0));
            std::cout << "    popq %rcx" << std::endl;

            std::cout << "    movl %eax, %ebx" << std::endl;
            std::cout << "    movl %ecx, %eax" << std::endl;
            std::cout << "    movl %ebx, %ecx" << std::endl;
            if (op == "+=")
            {
                std::cout << "    addl %ecx, %eax" << std::endl;
            }
            else if (op == "-=")
            {
                std::cout << "    subl %ecx, %eax" << std::endl;
            }
            else if (op == "*=")
            {
                std::cout << "    imull %ecx, %eax" << std::endl;
            }
            else if (op == "/=" || op == "%=")
            {
                std::cout << "    cltd" << std::endl;
                std::cout << "    idivl %ecx" << std::endl;
                if (op == "%=")
                {
                    std::cout << "    movl %edx, %eax" << std::endl;
                }
            }

            if (var->scopeType == SymbolScopeType::GLOBAL)
            {
                std::cout << "    movl %eax, " << varName << "(%rip)" << std::endl;
            }
            else
            {
                std::cout << "    movl %eax, " << var->offset << "(%rbp)" << std::endl;
            }
        }
        else if (op == "=")
        {
            visit(ctx->expr(0));
            if (var->scopeType == SymbolScopeType::GLOBAL)
            {
                std::cout << "    movl %eax, " << varName << "(%rip)" << std::endl;
            }
            else
            {
                std::cout << "    movl %eax, " << var->offset << "(%rbp)" << std::endl;
            }
        }
    }
    return 0;
}
// Gestion de l'instruction return
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    visit(ctx->expr());
    std::cout << "    jmp .Lepilogue" << std::endl; // Aller à l'épilogue
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
        std::cout << "    negl %eax" << std::endl; // Négation arithmétique
    }
    else if (op == "!")
    {
        std::cout << "    cmpl $0, %eax" << std::endl; // Comparer avec 0
        std::cout << "    sete %al" << std::endl;      // Mettre %al à 1 si %eax est 0
        std::cout << "    movzbl %al, %eax" << std::endl;
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    if (ctx->op->getText() == "+")
    {
        visit(ctx->expr(0));                             // Évalue le premier opérande
        std::cout << "    pushq %rax" << std::endl;      // Sauvegarde du résultat
        visit(ctx->expr(1));                             // Évalue le second opérande
        std::cout << "    popq %rcx" << std::endl;       // Récupère le premier opérande
        std::cout << "    addl %ecx, %eax" << std::endl; // Additionne
    }
    else if (ctx->op->getText() == "-")
    {
        visit(ctx->expr(0));
        std::cout << "    pushq %rax" << std::endl;
        visit(ctx->expr(1));
        std::cout << "    popq %rcx" << std::endl;
        std::cout << "    subl %eax, %ecx" << std::endl;
        std::cout << "    movl %ecx, %eax" << std::endl;
    }
    return 0;
}

// Gestion des expressions multiplication
antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    std::string op = ctx->OPM()->getText();

    // Evaluer a (opérande gauche)
    visit(ctx->expr(0));                        // a dans %eax
    std::cout << "    pushq %rax" << std::endl; // Empiler a

    // Evaluer b (opérande droite)
    visit(ctx->expr(1)); // b dans %eax

    if (op == "*")
    {
        // Dépiler a dans %rcx
        std::cout << "    popq %rcx" << std::endl;
        std::cout << "    imull %ecx, %eax" << std::endl;
    }
    else if (op == "/" || op == "%")
    {
        std::cout << "    pushq %rax" << std::endl; // Empiler b
        std::cout << "    popq %rcx" << std::endl;  // Dépiler b dans %rcx
        std::cout << "    popq %rax" << std::endl;  // Dépiler a dans %rax

        std::cout << "    cltd" << std::endl;       // sign extend %eax to %edx:%eax
        std::cout << "    idivl %ecx" << std::endl; // %eax = a /b , %edx = a % b

        if (op == "%")
        {
            std::cout << "    movl %edx, %eax" << std::endl; // %eax = a % b
        }
    }

    return 0;
}

// Gestion des opérations AND bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseAndExpression(ifccParser::BitwiseAndExpressionContext *ctx)
{
    visit(ctx->expr(0));
    std::cout << "    pushq %rax" << std::endl;
    visit(ctx->expr(1));
    std::cout << "    popq %rcx" << std::endl;
    std::cout << "    andl %ecx, %eax" << std::endl;
    return 0;
}

// Gestion des opérations OR bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseOrExpression(ifccParser::BitwiseOrExpressionContext *ctx)
{
    visit(ctx->expr(0));
    std::cout << "    pushq %rax" << std::endl;
    visit(ctx->expr(1));
    std::cout << "    popq %rcx" << std::endl;
    std::cout << "    orl %ecx, %eax" << std::endl;
    return 0;
}

// Gestion des opérations XOR bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseXorExpression(ifccParser::BitwiseXorExpressionContext *ctx)
{
    visit(ctx->expr(0));
    std::cout << "    pushq %rax" << std::endl;
    visit(ctx->expr(1));
    std::cout << "    popq %rcx" << std::endl;
    std::cout << "    xorl %ecx, %eax" << std::endl;
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
        std::cout << "    cmpl $0, %eax" << std::endl;
        std::cout << "    je " << labelEnd << std::endl; // Sauter à la fin si faux

        visit(ctx->expr(1)); // Évaluer le deuxième opérande
        std::cout << "    cmpl $0, %eax" << std::endl;
        std::cout << "    je " << labelEnd << std::endl; // Sauter à la fin si faux

        std::cout << labelTrue << ":" << std::endl;
        std::cout << "    movl $1, %eax" << std::endl; // Les deux opérandes sont vrais
        std::cout << "    jmp " << labelEnd << std::endl;

        std::cout << labelEnd << ":" << std::endl;
        std::cout << "    movl $0, %eax" << std::endl; // Au moins un opérande est faux
    }
    else if (op == "||")
    {
        visit(ctx->expr(0)); // Évaluer le premier opérande
        std::cout << "    cmpl $0, %eax" << std::endl;
        std::cout << "    jne " << labelTrue << std::endl; // Sauter à vrai si non zéro

        visit(ctx->expr(1)); // Évaluer le deuxième opérande
        std::cout << "    cmpl $0, %eax" << std::endl;
        std::cout << "    jne " << labelTrue << std::endl; // Sauter à vrai si non zéro

        std::cout << "    movl $0, %eax" << std::endl; // Les deux opérandes sont faux
        std::cout << "    jmp " << labelEnd << std::endl;

        std::cout << labelTrue << ":" << std::endl;
        std::cout << "    movl $1, %eax" << std::endl; // Au moins un opérande est vrai

        std::cout << labelEnd << ":" << std::endl;
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
    SymbolParameters *var = currentScope->findVariable(varName);
    if (var == nullptr)
    {
        std::cerr << "error: variable " << varName << " not declared" << std::endl;
        exit(1);
    }

    if (var->scopeType == SymbolScopeType::GLOBAL)
    {
        std::cout << "    movl " << varName << "(%rip), %eax" << std::endl;
    }
    else
    {
        std::cout << "    movl " << var->offset << "(%rbp), %eax" << std::endl;
    }

    return 0;
}


antlrcpp::Any CodeGenVisitor::visitArrayAccessExpression(ifccParser::ArrayAccessExpressionContext *ctx)
{
    std::string arrayName = ctx->VAR()->getText(); // Nom du tableau
    SymbolParameters *array = currentScope->findVariable(arrayName);
    if (array == nullptr) {
        std::cerr << "error: array " << arrayName << " not declared." << std::endl;
        exit(1);
    }

    // Évaluer l'indice du tableau
    visit(ctx->expr()); // L'indice est évalué dans %eax

    if (array->scopeType == SymbolScopeType::GLOBAL) {
        // Accès à un tableau global
        std::cout << "    movl " << arrayName << "(,%rax,4), %eax" << std::endl;
    } else {
        // Accès à un tableau local
        std::cout << "    movl " << array->offset << "(%rbp,%rax,4), %eax" << std::endl;
        
    }

    return 0;
}

// Gestion des constantes
antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    int value = std::stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << value << ", %eax" << std::endl;
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx)
{
    std::string value = ctx->CONST_CHAR()->getText().substr(1, 1);
    int convertValue = value[0];
    std::cout << "    movl $" << convertValue << ", %eax" << std::endl;
    return 0;
}

// Gestion des expressions de comparaison

antlrcpp::Any CodeGenVisitor::visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx)
{
    visit(ctx->expr(0)); // Évalue l'opérande gauche
    std::cout << "    pushq %rax" << std::endl;
    visit(ctx->expr(1)); // Évalue l'opérande droite
    std::cout << "    popq %rcx" << std::endl;
    std::cout << "    cmpl %eax, %ecx" << std::endl;

    std::string op = ctx->op->getText();
    if (op == "==")
    {
        std::cout << "    sete %al" << std::endl;
    }
    else if (op == "!=")
    {
        std::cout << "    setne %al" << std::endl;
    }
    else if (op == "<")
    {
        std::cout << "    setl %al" << std::endl;
    }
    else if (op == ">")
    {
        std::cout << "    setg %al" << std::endl;
    }
    else if (op == "<=")
    {
        std::cout << "    setle %al" << std::endl;
    }
    else if (op == ">=")
    {
        std::cout << "    setge %al" << std::endl;
    }
    std::cout << "    movzbl %al, %eax" << std::endl; // Conversion en entier 32 bits
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
            std::cout << "    movl %eax, %edi" << std::endl;
            break;
        case 1:
            std::cout << "    movl %eax, %esi" << std::endl;
            break;
        case 2:
            std::cout << "    movl %eax, %edx" << std::endl;
            break;
        case 3:
            std::cout << "    movl %eax, %ecx" << std::endl;
            break;
        case 4:
            std::cout << "    movl %eax, %r8d" << std::endl;
            break;
        case 5:
            std::cout << "    movl %eax, %r9d" << std::endl;
            break;
        }
    }

    // Finally, generate the call instruction
    std::cout << "    call " << funcName << std::endl;

    // The function's return value (if any) will be in %eax.
    return 0;
}

// Increment and Decrement

antlrcpp::Any CodeGenVisitor::visitPostIncrementExpression(ifccParser::PostIncrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    SymbolParameters *var = currentScope->findVariable(varName);
    if (var == nullptr)
    {
        std::cerr << "error: variable " << varName << " not declared." << std::endl;
        exit(1);
    }

    if (var->scopeType == SymbolScopeType::GLOBAL)
    {
        std::cout << "    movl " << varName << "(%rip), %eax" << std::endl;
        std::cout << "    addl $1, " << varName << "(%rip)" << std::endl;
    }
    else
    {
        std::cout << "    movl " << var->offset << "(%rbp), %eax" << std::endl;
        std::cout << "    addl $1, " << var->offset << "(%rbp)" << std::endl;
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitPostDecrementExpression(ifccParser::PostDecrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    SymbolParameters *var = currentScope->findVariable(varName);
    if (var == nullptr)
    {
        std::cerr << "error: variable " << varName << " not declared." << std::endl;
        exit(1);
    }

    if (var->scopeType == SymbolScopeType::GLOBAL)
    {
        std::cout << "    movl " << varName << "(%rip), %eax" << std::endl;
        std::cout << "    subl $1, " << varName << "(%rip)" << std::endl;
    }
    else
    {
        std::cout << "    movl " << var->offset << "(%rbp), %eax" << std::endl;
        std::cout << "    subl $1, " << var->offset << "(%rbp)" << std::endl;
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