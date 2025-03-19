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

    //int totalVars = countDeclarations(ctx->block());
    //currentDeclIndex = 0;

    if (!ctx->decl_stmt().empty()) { //si on a des variables globales
        std::cout << "    .data\n";
    }

    currentScope = new SymbolTable(0); //global scope

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
    if ( currentScope->isGlobalScope() ) {      // si ce bloc est une fonction, comme GlobalScope -> FunctionScope -> BlockScope
        // On calculate tous les variables dans la fonction et chercher le premier offset
        int totalVars = countDeclarations(ctx); 
        offset = -totalVars * 4;

        // Allouer l'espace pour les variables (4 octets par variable, aligné à 16 octets)
        int stackSize = totalVars * 4;
        stackSize = (stackSize + 15) & ~15;
        if (stackSize > 0) {
            std::cout << "    subq $" << stackSize << ", %rsp\n";
        }
            
    } else {                                    // sinon, c'est un blockscope
        // On laisse ce scope continue le offset de son parent
        offset = currentScope->getCurrentDeclOffset();
    }

    SymbolTable* parentScope = currentScope;
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
    currentTypeInMultiDeclaration = varType;
    for (auto sdecl : ctx->sub_decl()) {
        this->visit(sdecl);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitSub_decl(ifccParser::Sub_declContext *ctx) {
    std::string varType = currentTypeInMultiDeclaration; 
    std::string varName = ctx->VAR()->getText();

    std::string varAddr;
    if (currentScope->isGlobalScope()) { // si on est dans le scope global
        currentScope->addGlobalVariable(varName, varType);

        // on declare la variable
        std::cout <<"    .globl " << varName << "\n";
        std::cout << varName << ":\n";

        if (ctx->expr()) {
            auto expr = visit(ctx->expr());
            if (!expr.is<Constant>()) { // On vérifie si la variable est initialisée avec une constante
                std::cerr << "error: global variable must be initialized with a constant\n";
                exit(1);
            } 

            int value = expr.as<Constant>().value;
            std::cout << "    .long " << value << "\n";
        } else {
            std::cout << "    .zero 4\n";
        }

    } else { // sinon, on est dans le scope d'une fonction ou d'un block
        int offset = currentScope->addLocalVariable(varName, varType);
        if (!ctx->expr()) return 0;  // si la variable n'est pas initialisée, on ne fait rien

        auto expr = visit(ctx->expr()); 
        if (expr.is<Constant>()) {
            std::cout << "    movl $" << expr.as<Constant>().value << ", " << offset << "(%rbp)" << "\n";
        } else {
            visit(ctx->expr());
            std::cout << "    movl %eax, " << offset << "(%rbp)" << "\n";
        }
    }

    return 0;
}

// Gestion de l'affectation à une variable déjà déclarée
antlrcpp::Any CodeGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx)
{
    // Chercher la variable
    std::string varName = ctx->VAR()->getText();
    Parameters *var = currentScope->findVariable(varName);
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }

    // Adresse de la variable
    std::string varAddr = var->scopeType == ScopeType::GLOBAL 
                                ? varName + "(%rip)" 
                                : std::to_string(var->offset) + "(%rbp)";

    // Évaluer l'expression
    auto expr = visit(ctx->expr());
    if (expr.is<Constant>()) {
        std::cout << "    movl $" << expr.as<Constant>().value << ", " << varAddr << "\n";
    } else {
        std::cout << "    movl %eax, " << varAddr << "\n";
    }
    return 0;
}

// Gestion de l'instruction return
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    auto expr = visit(ctx->expr());
    if (expr.is<Constant>()) {
        std::cout << "    movl $" << expr.as<Constant>().value << ", %eax\n";
    }

    std::cout << "    jmp .Lepilogue\n"; // Aller à l'épilogue
    return 0;
}

// ==============================================================
//                          Expressions
// ==============================================================

antlrcpp::Any CodeGenVisitor::visitUnaryLogicalNotExpression(ifccParser::UnaryLogicalNotExpressionContext *ctx)
{
    auto expr = visit(ctx->expr()); // Évaluer l'expression
    std::string op = ctx->op->getText();
    if (op == "-")
    {
        if (expr.is<Constant>()) {
            return Constant{-expr.as<Constant>().value, expr.as<Constant>().type};
        } else {
            std::cout << "    negl %eax\n"; // Négation arithmétique
        }
    }
    else if (op == "!")
    {
        if (expr.is<Constant>()) {
            return Constant{!expr.as<Constant>().value, expr.as<Constant>().type};
        } else {
            std::cout << "    cmpl $0, %eax\n"; // Comparer avec 0
            std::cout << "    sete %al\n";      // Mettre %al à 1 si %eax est 0
            std::cout << "    movzbl %al, %eax\n";
        }
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    auto left = visit(ctx->expr(0));
    if (!left.is<Constant>()) {  
        std::cout << "    pushq %rax\n";  
    }
    auto right = visit(ctx->expr(1));

    std::string op = ctx->op->getText();

    // Constant folding
    if (left.is<Constant>() && right.is<Constant>()) {
        int result = (op == "+") 
                        ? left.as<Constant>().value + right.as<Constant>().value 
                        : left.as<Constant>().value - right.as<Constant>().value;
        return Constant{result, left.as<Constant>().type};
    }

    // Neutral element: x + 0 = x, x - 0 = x
    if (right.is<Constant>() && right.as<Constant>().value == 0) {
        std::cout << "    popq %rax\n"; // enlever expr-left de la pile vers %eax
        return 0;
    }

    // Neutral element: 0 + x = x
    if (left.is<Constant>() && left.as<Constant>().value == 0) {
        return 0; // fait rien, comme expr-right est déjà dans %eax
    }

    // Special case for "const - x" (need negation)
    if (op == "-" && left.is<Constant>()) {
        std::cout << "    subl $" << left.as<Constant>().value << ", %eax\n";
        std::cout << "    negl %eax\n";
        return 0;
    }

    // Special case for "x + const", "x - const"
    if (right.is<Constant>()) {
        std::cout << "    popq %rax\n";       // Récupère le premier opérande
        std::cout << "    " << (op == "+" ? "addl" : "subl") << " $" << right.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // constant + x
    if (left.is<Constant>() && op=="+") {
        std::cout << "    addl $" << left.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // general case
    if (op == "+") {
        std::cout << "    popq %rcx\n"; // Récupère le premier opérande
        std::cout << "    addl %ecx, %eax\n";
    } else if (op == "-") {
        std::cout << "    popq %rcx\n"; // Récupère le premier opérande
        std::cout << "    subl %eax, %ecx\n";
        std::cout << "    movl %ecx, %eax\n";
    }
    return 0;
}

// Gestion des expressions multiplication
antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    auto left = visit(ctx->expr(0));
    if (!left.is<Constant>()) {  
        std::cout << "    pushq %rax\n";  
    }
    auto right = visit(ctx->expr(1));

    std::string op = ctx->OPM()->getText();

    // Constant folding
    if (left.is<Constant>() && right.is<Constant>()) {
        int result = (op == "*") 
                        ? left.as<Constant>().value * right.as<Constant>().value 
                        :   (op == "/")
                            ? left.as<Constant>().value / right.as<Constant>().value
                            : left.as<Constant>().value % right.as<Constant>().value;
        return Constant{result, left.as<Constant>().type};
    }

    // Neutral element: x * 1 = x, x / 1 = x
    if (right.is<Constant>() && right.as<Constant>().value == 0 && op != "%") {
        std::cout << "    popq %rax\n"; // enlever expr-left de la pile vers %eax
        return 0;
    }

    // Neutral element: 1 * x = x
    if (left.is<Constant>() && left.as<Constant>().value == 0) {
        return 0; // fait rien, comme expr-right est déjà dans %eax
    }

    // Neutral element: x * 0 = 0, x % 0 = 0
    if (right.is<Constant>() && right.as<Constant>().value == 0) {
        std::cout << "    popq %rcx\n"; // Enlever expr-left qu'on a empilé
        return Constant{0, right.as<Constant>().type};
    }

    // Special case: x * constant
    if (right.is<Constant>() && op == "*") {
        std::cout << "    popq %rax\n"; // Enlever expr-left qu'on a empilé
        std::cout << "    imull $" << right.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // Special case: constant * x
    if (left.is<Constant>() && op == "*") {
        std::cout << "    imull $" << left.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // Special case: x / constant, x % constant
    if (right.is<Constant>() && (op == "/" || op == "%")) {
        // Check division by zero
        if (right.as<Constant>().value == 0) {
            std::cerr << "Error: Division/Modulo by zero\n";
            exit(1);
        }

        std::cout << "    popq %rax\n"; // Récupère le premier opérande
        std::cout << "    cdq\n";       // sign extend %eax to %edx:%eax
        std::cout << "    movl $" << right.as<Constant>().value << ", %ecx\n";
        std::cout << "    idivl %ecx\n";
        
        if (op == "%") {
            std::cout << "    movl %edx, %eax\n";
        }
        return 0;
    }

    // Special case: constant / x, constant % x
    if (left.is<Constant>() && (op == "/" || op == "%")) {
       std::cout << "    movl %eax, %ecx\n";
       std::cout << "    movl $" << left.as<Constant>().value << ", %eax\n";
       std::cout << "    cdq\n";
       std::cout << "    idivl %ecx\n";
       
       if (op == "%") {
           std::cout << "    movl %edx, %eax\n";
       }
       return 0;
    }

    // general case
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
    auto left = visit(ctx->expr(0));
    if (!left.is<Constant>()) {
        std::cout << " pushq %rax\n";
    }
    auto right = visit(ctx->expr(1));

    // Constant folding
    if (left.is<Constant>() && right.is<Constant>()) {
        int result = left.as<Constant>().value & right.as<Constant>().value;
        return Constant{result, left.as<Constant>().type};
    }

    // Neutral element: x & -1 = x (toutes les bits à 1)
    if (right.is<Constant>() && right.as<Constant>().value == -1) {
        if (!left.is<Constant>()) {
            std::cout << "    popq %rax\n";  // Récupérer la valeur de left
        }
        return 0;
    }

    // Neutral element: -1 & x = x (toutes les bits à 1)
    if (left.is<Constant>() && left.as<Constant>().value == -1) {
        return 0;  // right est déjà dans %eax
    }

    // Zero element: x & 0 = 0, 0 & x = 0
    if ((right.is<Constant>() && right.as<Constant>().value == 0) ||
        (left.is<Constant>() && left.as<Constant>().value == 0)) {
        if (!left.is<Constant>()) {
            std::cout << "    popq %rcx\n";  // Dépiler left (non utilisé)
        }
        return Constant{0, left.as<Constant>().type};
    }

    // Special case: x & constant
    if (right.is<Constant>()) {
        std::cout << "    popq %rax\n";  // Récupérer la valeur de left
        std::cout << "    andl $" << right.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // Special case: constant & x
    if (left.is<Constant>()) {
        std::cout << "    andl $" << left.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // General case
    std::cout << "    popq %rcx\n";
    std::cout << "    andl %ecx, %eax\n";
    return 0;
}

// Gestion des opérations OR bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseOrExpression(ifccParser::BitwiseOrExpressionContext *ctx)
{
    auto left = visit(ctx->expr(0));
    if (!left.is<Constant>()) {
        std::cout << "    pushq %rax\n";
    }
    auto right = visit(ctx->expr(1));

    // Constant folding
    if (left.is<Constant>() && right.is<Constant>()) {
        int result = left.as<Constant>().value | right.as<Constant>().value;
        return Constant{result, left.as<Constant>().type};
    }

    // Neutral element: x | 0 = x
    if (right.is<Constant>() && right.as<Constant>().value == 0) {
        if (!left.is<Constant>()) {
            std::cout << "    popq %rax\n";  // Récupérer la valeur de left
        }
        return 0;
    }

    // Neutral element: 0 | x = x
    if (left.is<Constant>() && left.as<Constant>().value == 0) {
        return 0;  // right est déjà dans %eax
    }

    // Identity element: x | -1 = -1, -1 | x = -1 (toutes les bits à 1)
    if ((right.is<Constant>() && right.as<Constant>().value == -1) ||
        (left.is<Constant>() && left.as<Constant>().value == -1)) {
        if (!left.is<Constant>()) {
            std::cout << "    popq %rcx\n";  // Dépiler left (non utilisé)
        }
        return Constant{-1, VarType::INT};
    }

    // Special case: x | constant
    if (right.is<Constant>()) {
        std::cout << "    popq %rax\n";  // Récupérer la valeur de left
        std::cout << "    orl $" << right.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // Special case: constant | x
    if (left.is<Constant>()) {
        std::cout << "    orl $" << left.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // General case
    std::cout << "    popq %rcx\n";
    std::cout << "    orl %ecx, %eax\n";
    return 0;
}

// Gestion des opérations XOR bit à bit
antlrcpp::Any CodeGenVisitor::visitBitwiseXorExpression(ifccParser::BitwiseXorExpressionContext *ctx)
{
    auto left = visit(ctx->expr(0));
    if (!left.is<Constant>()) {
        std::cout << "    pushq %rax\n";
    }
    auto right = visit(ctx->expr(1));

    // Constant folding
    if (left.is<Constant>() && right.is<Constant>()) {
        int result = left.as<Constant>().value ^ right.as<Constant>().value;
        return Constant{result, left.as<Constant>().type};
    }

    // Neutral element: x ^ 0 = x
    if (right.is<Constant>() && right.as<Constant>().value == 0) {
        if (!left.is<Constant>()) {
            std::cout << "    popq %rax\n";  // Récupérer la valeur de left
        }
        return 0;
    }

    // Neutral element: 0 ^ x = x
    if (left.is<Constant>() && left.as<Constant>().value == 0) {
        return 0;  // right est déjà dans %eax
    }

    // Identity element: x ^ -1 = ~x (inverse tous les bits)
    if (right.is<Constant>() && right.as<Constant>().value == -1) {
        std::cout << "    popq %rax\n";  // Récupérer la valeur de left
        std::cout << "    notl %eax\n";
        return 0;
    }

    // Identity element: -1 ^ x = ~x (inverse tous les bits)
    if (left.is<Constant>() && left.as<Constant>().value == -1) {
        std::cout << "    notl %eax\n";
        return 0;
    }

    // Special case: x ^ x = 0
    // Cette optimisation nécessiterait une analyse du code pour détecter si les deux opérandes sont identiques

    // Special case: x ^ constant
    if (right.is<Constant>()) {
        std::cout << "    popq %rax\n";  // Récupérer la valeur de left
        std::cout << "    xorl $" << right.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // Special case: constant ^ x
    if (left.is<Constant>()) {
        std::cout << "    xorl $" << left.as<Constant>().value << ", %eax\n";
        return 0;
    }

    // General case
    std::cout << "    popq %rcx\n";
    std::cout << "    xorl %ecx, %eax\n";
    return 0;
}


antlrcpp::Any CodeGenVisitor::visitLogiqueParesseuxExpression(ifccParser::LogiqueParesseuxExpressionContext *ctx)
{
    // TODO: Implementer les optimisations pour les constantes
    std::string op = ctx->op->getText();
    std::string labelTrue = generateLabel(); 
    std::string labelEnd = generateLabel();  

    if (op == "&&")
    {
        auto left = visit(ctx->expr(0)); // Évaluer le premier opérande
        if (left.is<Constant>()) { std::cout << "    movl $" << left.as<Constant>().value << ", %eax\n"; }
        std::cout << "    cmpl $0, %eax\n"; 
        std::cout << "    je " << labelEnd << "\n"; // Sauter à la fin si faux

        auto right = visit(ctx->expr(1)); // Évaluer le deuxième opérande
        if (right.is<Constant>()) { std::cout << "    movl $" << left.as<Constant>().value << ", %eax\n"; }
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
        auto left = visit(ctx->expr(0)); // Évaluer le premier opérande
        if (left.is<Constant>()) { std::cout << "    movl $" << left.as<Constant>().value << ", %eax\n"; }
        std::cout << "    cmpl $0, %eax\n"; 
        std::cout << "    jne " << labelTrue << "\n"; // Sauter à vrai si non zéro

        auto right = visit(ctx->expr(1)); // Évaluer le deuxième opérande
        if (right.is<Constant>()) { std::cout << "    movl $" << left.as<Constant>().value << ", %eax\n"; }
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

// Gestion des expressions de parenthèses
antlrcpp::Any CodeGenVisitor::visitParenthesisExpression(ifccParser::ParenthesisExpressionContext *ctx)
{
    return visit(ctx->expr());
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
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared\n";
        exit(1);
    }

    if (var->scopeType == ScopeType::GLOBAL) {
        std::cout << "    movl " << varName << "(%rip), %eax" << "\n";
    } else {
        std::cout << "    movl " << var->offset << "(%rbp), %eax" << "\n";
    }

    return 0;
}

// Gestion des constantes
antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    int value = std::stoi(ctx->CONST()->getText());
    //std::cout << "    movl $" << value << ", %eax" << "\n";
    return Constant{value, VarType::INT};
}

antlrcpp::Any CodeGenVisitor::visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx)
{
    std::string value = ctx->CONST_CHAR()->getText().substr(1, 1);
    int convertValue = value[0];
    //std::cout << "    movl $" << convertValue << ", %eax" << "\n";
    return Constant{convertValue, VarType::CHAR};
}

// Gestion des expressions de comparaison
antlrcpp::Any CodeGenVisitor::visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx)
{
    // TODO: Implementer les optimisations pour les constantes
    auto left = visit(ctx->expr(0)); // Évalue l'opérande gauche
    if (left.is<Constant>()) { std::cout << "    movl $" << left.as<Constant>().value << ", %eax\n"; }
    std::cout << "    pushq %rax\n";
    
    auto right = visit(ctx->expr(1)); // Évalue l'opérande droite
    if (right.is<Constant>()) { std::cout << "    movl $" << right.as<Constant>().value << ", %eax\n"; }
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

    // TODO: Implementer les optimisations pour les constantes

    // Evaluate each argument and move the result into the appropriate register.
    // According to the Linux System V AMD64 ABI, the first six integer arguments go in:
    // 1st: %rdi, 2nd: %rsi, 3rd: %rdx, 4th: %rcx, 5th: %r8, 6th: %r9.
    // Here we assume that the result of an expression is left in %eax, so we move
    // that 32-bit value into the corresponding register (using the "d" suffix for the lower 32 bits).
    for (size_t i = 0; i < args.size(); i++)
    {
        auto expr = visit(args[i]);
        if (expr.is<Constant>()) { std::cout << "    movl $" << expr.as<Constant>().value << ", %eax\n"; }
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