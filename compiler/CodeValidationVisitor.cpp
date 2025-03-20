#include "CodeValidationVisitor.h"
#include "SymbolTable.h"

#define RESET "\033[0m"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"

antlrcpp::Any CodeValidationVisitor::visitProg(ifccParser::ProgContext* ctx) {
    for (auto stmt : ctx->decl_stmt()) {
        visit(stmt);
    }

    visit(ctx->block());
    
    // Check for unused variables
    for (auto& entry : usageMap) {
        std::string var = entry.first;
        bool used = entry.second;

        if (!used) {
            genNotUsedVariableWarningMessage(var);
        }
    }
    return 0;
}

antlrcpp::Any CodeValidationVisitor::visitBlock(ifccParser::BlockContext* ctx) {
    SymbolTable *parentScope = currentScope;
    currentScope = new SymbolTable(0);
    currentScope->setParent(parentScope);
    
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    
    // Return to parent scope and clean up memory after block ends
    SymbolTable* oldScope = currentScope;
    currentScope = currentScope->getParent();
    delete oldScope;

    return 0;
}

antlrcpp::Any CodeValidationVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) {
    std::string varName = ctx->VAR()->getText();
    int line = ctx->getStart()->getLine(); // Get line number in case error
    int column = ctx->getStart()->getCharPositionInLine(); // Get column number in case error

    // Check if the variable is already declared
    if (!currentScope->findVariable(varName)) {
        genUsedBeforeDeclarationErrorMessage(line, column, varName);
        exit(1);
    }
    usageMap[varName] = true;

    // Check if right-hand side is a variable and validate its declaration
    if (ctx->expr())
    {
        visit(ctx->expr());
    }
    return 0;
}

antlrcpp::Any CodeValidationVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext* ctx) {
    std::string varType = ctx->type()->getText();
    for (auto subDecl : ctx->sub_decl()) {
        visitSub_declWithType(subDecl, varType);
    }
    return 0;
}

antlrcpp::Any CodeValidationVisitor::visitSub_declWithType(ifccParser::Sub_declContext *ctx, std::string varType)
{
    std::string varName = ctx->VAR()->getText();
    int line = ctx->getStart()->getLine(); // Get line number in case error
    int column = ctx->getStart()->getCharPositionInLine(); // Get column number in case error

    if (currentScope->findVariableThisScope(varName)) {
        genRedeclarationErrorMessage(line, column, varName);
        exit(1);
    }

    if (currentScope->isGlobalScope())
    { // si on est dans le scope global
        currentScope->addGlobalVariable(varName, varType);
        usageMap[varName] = false; // declared but not used yet

        // On vérifie si la variable est initialisée avec une constante
        if (ctx->expr() && !isExprIsConstant(ctx->expr()))
        {
            genInvalidInitialiseGlobalVariableErrorMessage(line, column);
            exit(1);
        }
    }
    else
    { // sinon, on est dans le scope d'une fonction ou d'un block
        currentScope->addLocalVariable(varName, varType);
        usageMap[varName] = false; // declared but not used yet

        if (ctx->expr())
        {
            visit(ctx->expr());
        }
    }

    return 0;
}

antlrcpp::Any CodeValidationVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    int line = ctx->getStart()->getLine(); // Get line number in case error
    int column = ctx->getStart()->getCharPositionInLine(); // Get column number in case error

    SymbolParameters *var = currentScope->findVariable(varName);

    if (var == nullptr) 
    {
        genUsedBeforeDeclarationErrorMessage(line, column, varName);
        exit(1);
    }
    else
    {
        usageMap[varName] = true;
    }
    return 0;
}

void CodeValidationVisitor::genUsedBeforeDeclarationErrorMessage(int line, int column, std::string &var) const {
    std::cerr << RED << "error: " << RESET 
              << "on line " << line << " at column " << column << ": " 
              << "variable ‘" << var << "’ used before declaration"
              << std::endl;
}
void CodeValidationVisitor::genRedeclarationErrorMessage(int line, int column, std::string &var) const {
    std::cerr << RED << "error: " << RESET 
              << "on line " << line << " at column " << column << ": " 
              << "redeclaration of ‘" << var << "’"
              << std::endl;
}
void CodeValidationVisitor::genNotUsedVariableWarningMessage(std::string &var) const {
    std::cerr << YELLOW << "warning: " << RESET
              << "variable ‘" << var << "’ declared but not used.\n";
}

void CodeValidationVisitor::genInvalidInitialiseGlobalVariableErrorMessage(int line, int column) const {
    std::cerr << RED << "error: " << RESET 
              << "on line " << line << " at column " << column << ": " 
              << "global variable must be initialized with a constant"
              << std::endl;
}

bool CodeValidationVisitor::isExprIsConstant(ifccParser::ExprContext *ctx)
{
    if (dynamic_cast<ifccParser::ConstantExpressionContext *>(ctx) != nullptr ||
        dynamic_cast<ifccParser::ConstantCharExpressionContext *>(ctx) != nullptr)
    {
        return true;
    }
    return false;
}

